/*
 * io/io_posix.cc
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#include "../first.hh"

#include <cerrno>         // for errno
#include <sys/types.h>    // for open()
#include <sys/stat.h>     //  "    "
#include <fcntl.h>        //  "    "
#include <unistd.h>       // for close()

#include "../log.hh"      // for ff_log()

#include "extent_posix.hh" // for ft_extent_posixs()
#include "util_posix.hh"   // for ft_posix_*() misc functions
#include "io_posix.hh"     // for ft_io_posix


FT_IO_NAMESPACE_BEGIN

char const * const ft_io::label[ft_io_posix::FC_FILE_COUNT] = { "DEVICE", "LOOP-FILE", "ZERO-FILE" };

/** default constructor */
ft_io_posix::ft_io_posix(ft_job & job)
: super_type(job)
{
    /* mark fd[] as invalid: they are not open yet */
    for (ft_size i = 0; i < FC_FILE_COUNT; i++)
        fd[i] = -1;
}

/** destructor. calls close() */
ft_io_posix::~ft_io_posix()
{
    close();
}

/** return true if this ft_io_posix is currently (and correctly) open */
bool ft_io_posix::is_open() const
{
    bool flag = false;
    if (dev_length() != 0) {
        ft_size i;
        for (i = 0; i < FC_FILE_COUNT; i++)
            if (fd[i] < 0)
                break;
        flag = i == FC_FILE_COUNT;
    }
    return flag;
}

/** check for consistency and open DEVICE, LOOP-FILE and ZERO-FILE */
int ft_io_posix::open(char const* const path[FC_FILE_COUNT])
{
    if (is_open())
        return EISCONN; // already open!

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
 * return 0 for success, 1 for error (prints by itself error message to stderr)
 */
void ft_io_posix::close()
{
    for (ft_size i = 0; i < FC_FILE_COUNT; i++) {
        if (fd[i] >= 0) {
            if (::close(fd[i]) != 0)
                ff_log(FC_WARN, errno, "warning: closing %s file descriptor [%d] failed", label[i], fd[i]);
            fd[i] = -1;
        }
    }
    super_type::close();
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
        if (!is_open()) {
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

FT_IO_NAMESPACE_END
