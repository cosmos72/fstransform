/*
 * io/io_posix.cc
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#include "../first.hh"

#include <cerrno>         // for errno
#include <cstdlib>        // for malloc(), free()
#include <cstring>        // for memset()
#include <sys/types.h>    // for open()
#include <sys/stat.h>     //  "    "
#include <fcntl.h>        //  "    "
#include <unistd.h>       // for close(), unlink()
#include <sys/mman.h>     // for mmap(), munmap()

#include "../log.hh"      // for ff_log()
#include "../util.hh"     // for ff_max2(), ff_min2()

#include "extent_posix.hh" // for ft_extent_posixs()
#include "util_posix.hh"   // for ft_posix_*() misc functions
#include "io_posix.hh"     // for ft_io_posix


FT_IO_NAMESPACE_BEGIN

char const * const ft_io::label[ft_io_posix::FC_ALL_FILE_COUNT] = { "DEVICE", "LOOP-FILE", "ZERO-FILE", "STORAGE-FILE" };

/** default constructor */
ft_io_posix::ft_io_posix(ft_job & job)
: super_type(job), storage_mmap(MAP_FAILED), storage_mmap_size(0)
{
    /* mark fd[] as invalid: they are not open yet */
    for (ft_size i = 0; i < FC_ALL_FILE_COUNT; i++)
        fd[i] = -1;
}

/** destructor. calls close() */
ft_io_posix::~ft_io_posix()
{
    close();
}

/** return true if a single descriptor/stream is open */
bool ft_io_posix::is_open0(ft_size i) const
{
    return fd[i] >= 0;
}

/** close a single descriptor/stream */
void ft_io_posix::close0(ft_size i)
{
    if (fd[i] >= 0) {
        if (::close(fd[i]) != 0)
            ff_log(FC_WARN, errno, "warning: closing %s file descriptor [%d] failed", label[i], fd[i]);
        fd[i] = -1;
    }
}

/** return true if this ft_io_posix is currently (and correctly) open */
bool ft_io_posix::is_open() const
{
    return dev_length() != 0 && is_open0(FC_DEVICE);
}

/** check for consistency and open DEVICE, LOOP-FILE and ZERO-FILE */
int ft_io_posix::open(char const* const path[FC_FILE_COUNT])
{
    if (is_open()) {
        // already open!
        ff_log(FC_ERROR, 0, "unexpected call, I/O is already open");
        return EISCONN;
    }

    ft_uoff dev_len;
    ft_size i;
    int err = 0;
    ft_dev dev[FC_FILE_COUNT];

    for (i = 0; i < FC_FILE_COUNT; i++)
        dev[i] = 0;

    do {
        for (i = 0; i < FC_FILE_COUNT; i++) {
            if ((fd[i] = ::open(path[i], O_RDONLY)) < 0) {
                err = ff_log(FC_ERROR, errno, "error opening %s '%s'", label[i], path[i]);
                break;
            }

            if (i == FC_DEVICE)
                /* for DEVICE, we want to know its dev_t */
                err = ff_posix_blkdev_dev(fd[i], & dev[i]);
            else
                /* for LOOP-FILE and ZERO-FILE, we want to know the dev_t of the device they are stored into */
                err = ff_posix_dev(fd[i], & dev[i]);

            if (err != 0) {
                err = ff_log(FC_ERROR, err, "error in %s fstat('%s')", label[i], path[i]);
                break;
            }

            if (i == FC_DEVICE) {
                /* for DEVICE, we also want to know its length */
                if ((err = ff_posix_blkdev_size(fd[FC_DEVICE], & dev_len)) != 0) {
                    err = ff_log(FC_ERROR, err, "error in %s ioctl('%s', BLKGETSIZE64)", label[0], path[0]);
                    break;
                }
                /* device length is retrieved ONLY here. we must remember it */
                dev_length(dev_len);
            } else {
                /* for LOOP-FILE and ZERO-FILE, we check they are actually contained in DEVICE */
                if (dev[FC_DEVICE] != dev[i]) {
                    ff_log(FC_ERROR, 0, "invalid arguments: '%s' is device 0x%04x, but %s '%s' is contained in device 0x%04x\n",
                           path[FC_DEVICE], (unsigned)dev[FC_DEVICE], label[i], path[i], (unsigned)dev[i]);
                    err = EINVAL;
                    break;
                }
            }
        }
    } while (0);

    if (err != 0)
        close();

    return err;
}



/**
 * close file descriptors.
 * return 0 for success, 1 for error (logs by itself error message)
 */
void ft_io_posix::close()
{
    close_storage();
    for (ft_size i = 0; i < FC_FILE_COUNT; i++)
        close0(i);
    super_type::close();
}



/** return true if this I/O has open descriptors/streams to LOOP-FILE and FREE-SPACE */
bool ft_io_posix::is_open_extents() const
{
    bool flag = false;
    if (dev_length() != 0) {
        ft_size which[] = { FC_LOOP_FILE, FC_ZERO_FILE };
        ft_size i, n = sizeof(which)/sizeof(which[0]);
        for (i = 0; i < n; i++)
            if (!is_open0(which[i]) < 0)
                break;
        flag = i == n;
    }
    return flag;
}

/**
 * retrieve LOOP-FILE extents and FREE-SPACE extents and insert them into
 * the vectors loop_file_extents and free_space_extents.
 * the vectors will be ordered by extent ->logical.
 *
 * return 0 for success, else error (and vectors contents will be UNDEFINED).
 *
 * if success, also returns in ret_effective_block_size_log2 the log2()
 * of device effective block size.
 * the device effective block size is defined as follows:
 * it is the largest power of 2 that exactly divides all physical,
 * logical and lengths in all returned extents (both for LOOP-FILE
 * and for FREE-SPACE) and that also exactly exactly divides device length.
 *
 * must be overridden by sub-classes.
 *
 * a common trick subclasses may use to implement this method
 * is to fill the device's free space with a ZERO-FILE,
 * and actually retrieve the extents used by ZERO-FILE.
 */
int ft_io_posix::read_extents(ft_vector<ft_uoff> & loop_file_extents,
                              ft_vector<ft_uoff> & free_space_extents,
                              ft_uoff & ret_block_size_bitmask)
{
    ft_uoff block_size_bitmask = ret_block_size_bitmask;
    int err = 0;
    do {
        if (!is_open_extents()) {
            err = ENOTCONN; // not open!
            break;
        }

        /* ff_read_extents_posix() appends into ft_vector<T>, does NOT overwrite it */
        if ((err = ff_read_extents_posix(fd[FC_LOOP_FILE], dev_length(), loop_file_extents, block_size_bitmask)) != 0)
            break;
        if ((err = ff_read_extents_posix(fd[FC_ZERO_FILE], dev_length(), free_space_extents, block_size_bitmask)) != 0)
            break;

    } while (0);

    if (err == 0)
        ret_block_size_bitmask = block_size_bitmask;

    return err;
}


/**
 * close the file descriptors for LOOP-FILE and ZERO-FILE
 */
void ft_io_posix::close_extents()
{
    ft_size which[] = { FC_LOOP_FILE, FC_ZERO_FILE };
    for (ft_size i = 0; i < sizeof(which)/sizeof(which[0]); i++)
        close0(which[i]);
}


/**
 * create file job.job_dir() + '/storage.bin', fill it with job.job_storage_size() bytes of zeros,
 * and mmap() it. return 0 if success, else error
 */
int ft_io_posix::create_storage()
{
    const ft_size i = FC_STORAGE_FILE;

    if (is_open0(i) || storage_mmap != MAP_FAILED) {
        // already open!
        ff_log(FC_ERROR, 0, "unexpected call to create_storage(), %s is already open", label[i]);
        return EISCONN;
    }
    std::string filepath = job_dir();
    filepath += "storage.bin";
    const char * path = filepath.c_str();
    int err = 0;

    do {
        ft_uoff length = job_storage_size();
        double pretty_len = 0.0;
        const char * pretty_label = ff_pretty_size(length, & pretty_len);

        ff_log(FC_INFO, 0, "creating %s '%s'...", label[i], path);
        ff_log(FC_INFO, 0, "%s will be %.2f %sbytes long", label[i], pretty_len, pretty_label);

        /* check that length is a multiple of 16k: mmap() requires length to be a multiple of PAGE_SIZE */
        enum { _PAGE_SIZE_minus_1 =
#ifdef PAGE_SIZE
                PAGE_SIZE - 1
#else
                16*1024 - 1
#endif
                };
        if ((length & _PAGE_SIZE_minus_1) != 0) {
            ff_log(FC_ERROR, 0, "internal error, %s length = %"FS_ULL" is not a multiple of PAGE_SIZE ", label[i], (FT_ULL) length);
            err = EINVAL;
            break;
        }
        storage_mmap_size = (ft_size) length;
        if (storage_mmap_size < 0 || length != (ft_uoff) storage_mmap_size) {
            ff_log(FC_ERROR, 0, "internal error, %s length = %"FS_ULL" is larger than addressable memory", label[i], (FT_ULL) length);
            err = EINVAL;
            break;
        }

        char zero = '\0';

        if ((fd[i] = ::open(path, O_RDWR|O_CREAT|O_TRUNC, 0600)) < 0) {
            err = ff_log(FC_ERROR, errno, "error opening %s", label[i], path);
            break;
        }
        if (lseek(fd[i], (ft_off)length - 1, SEEK_SET) != (ft_off)length - 1) {
            err = ff_log(FC_ERROR, errno, "error in %s lseek()", label[i]);
            break;
        }
        if (::write(fd[i], & zero, 1) != 1) {
            err = ff_log(FC_ERROR, errno, "error in %s write()", label[i]);
            break;
        }

        if ((storage_mmap = mmap(NULL, storage_mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd[i], 0)) == MAP_FAILED) {
            err = ff_log(FC_ERROR, errno, "error in %s mmap()", label[i]);
            break;
        }

        /** all done. we do not need fd[i] anymore */
        close0(i);

        ff_log(FC_INFO, 0, "%s created and mmapped()", label[i]);

    } while (0);

    if (err != 0) {
        bool need_unlink = is_open0(i);
        close_storage();
        if (need_unlink && unlink(path) != 0)
            ff_log(FC_WARN, errno, "warning: removing %s file '%s' failed", label[i], path);
    }

    return err;
}

/** close and munmap() STORAGE-FILE. called by close() */
void ft_io_posix::close_storage()
{
    const ft_size i = FC_STORAGE_FILE;
    if (storage_mmap != MAP_FAILED) {
        if (munmap(storage_mmap, storage_mmap_size) != 0)
            ff_log(FC_WARN, errno, "warning: %s munmap() failed", label[i]);
        storage_mmap = MAP_FAILED;
    }
    storage_mmap_size = 0;
    close0(i);
}


FT_IO_NAMESPACE_END
