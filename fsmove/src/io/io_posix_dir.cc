/*
 * io/io_posix_dir.cc
 *
 *  Created on: Sep 22, 2011
 *      Author: max
 */

#include "../first.hh"

#include <cerrno>           // for errno and error codes
#include <sys/types.h>      // for DIR, opendir()
#include <dirent.h>         //  "   "     "      , readdir(), closedir()

#include "../log.hh"        // for ff_log()
#include "io_posix_dir.hh"  // for fm_io_posix_dir


FT_IO_NAMESPACE_BEGIN

/** default constructor */
fm_io_posix_dir::fm_io_posix_dir()
    : this_dir(NULL)
{ }

/** destructor. calls close() */
fm_io_posix_dir::~fm_io_posix_dir()
{
    close();
}

/** open a directory */
int fm_io_posix_dir::open(const ft_string & path)
{
    int err;
    if (this_dir != NULL)
        err = EISCONN;
    else if ((this_dir = opendir(path.c_str())) != NULL)
        err = 0;
    else
        err = errno;
    // TODO log if err != 0
    return err;
}

/** close the currently open directory */
int fm_io_posix_dir::close()
{
    int err;
    if (this_dir == NULL)
        err = ENOTCONN;
    else if ((err = closedir(this_dir)) == 0)
        this_dir = NULL;
    else
        err = errno;
    // TODO log if err != 0
    return err;
}


/**
 *  get next directory entry.
 *  returns 0 if success (NULL result indicates EOF),
 *  else returns error code
 */
int fm_io_posix_dir::next(fm_io_posix_dirent * & result)
{
    int err;
    if (this_dir == NULL)
        err = ENOTCONN;
    else {
        errno = 0;
        result = readdir(this_dir);
        err = errno; // 0 for success or end-of-dir
        // TODO log if err != 0
    }
    return err;
}

FT_IO_NAMESPACE_END
