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

#include "../fail.hh"     // for ff_fail()

#include "posix_extent.hh" // for ft_posix_extents()
#include "posix_util.hh"   // for ft_posix_*() misc functions
#include "io_posix.hh"     // for ft_io_posix


FT_IO_NAMESPACE_BEGIN

char const * const ft_io::label[ft_io_posix::FC_FILE_COUNT] = { "DEVICE", "LOOP-FILE", "ZERO-FILE" };

/** default constructor */
ft_io_posix::ft_io_posix()
: super_type()
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
                err = ff_fail(errno, "error opening %s '%s'", label[i], path[i]);
                break;
            }

            if (i == 0)
                /* for DEVICE, we want to know its dev_t */
                err = ff_posix_blkdev_dev(fd[i], & dev[i]);
            else
                /* for LOOP-FILE and ZERO-FILE, we want to know the dev_t of the device they are stored into */
                err = ff_posix_dev(fd[i], & dev[i]);

            if (err != 0) {
                err = ff_fail(err, "error in %s fstat('%s')", label[i], path[i]);
                break;
            }

            if (i == 0) {
                /* for DEVICE, we also want to know its length */
                if ((err = ff_posix_blkdev_size(fd[0], & dev_len)) != 0) {
                    err = ff_fail(err, "error in %s ioctl('%s', BLKGETSIZE64)", label[0], path[0]);
                    break;
                }
                /* device length is retrieved ONLY here. we must remember it */
                dev_length(dev_len);
            } else {
                /* for LOOP-FILE and ZERO-FILE, we check they are actually contained in DEVICE */
                if (dev[0] != dev[i]) {
                    err = ff_fail(EINVAL, "invalid arguments: '%s' is device 0x%04x, but %s '%s' is contained in device 0x%04x\n",
                                  path[0], (unsigned)dev[0], path[i], (unsigned)dev[i]);
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
                ff_fail(errno, "warning: closing %s file descriptor [%d] failed", label[i], fd[i]);
            fd[i] = -1;
        }
    }
    super_type::close();
}


/**
 * retrieve LOOP-FILE extents and insert them into ret_vector.
 * return 0 for success, else error (and ret_vector contents will be unchanged).
 */
int ft_io_posix::loop_file_extents_list(ft_extent_list & ret_list)
{
    if (!is_open())
        return ENOTCONN; // not open!

    /* ff_posix_extents() appends into ret_list, does NOT overwrite it */
    return ff_posix_extents(fd[FC_LOOP_FILE], ret_list);
}

/**
 * retrieve FREE SPACE extents and insert them into ret_list.
 * trick: we filled the device free space with ZERO-FILE,
 * so now we actually retrieve the extents used by ZERO-FILE.
 *
 * return 0 for success, else error (and ret_list contents will be UNDEFINED).
 */
int ft_io_posix::free_space_extents_list(ft_extent_list & ret_list)
{
    if (!is_open())
        return ENOTCONN; // not open!

    /* ff_posix_extents() appends into ret_list, does NOT overwrite it */
    return ff_posix_extents(fd[FC_ZERO_FILE], ret_list);
}


FT_IO_NAMESPACE_END
