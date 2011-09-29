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
    : this_path(), this_dir(NULL)
{ }

/** destructor. calls close() */
fm_io_posix_dir::~fm_io_posix_dir()
{
    close();
}

/** open a directory */
int fm_io_posix_dir::open(const ft_string & path)
{
    int err = 0;
    if (this_dir != NULL)
        err = EISCONN;
    else if ((this_dir = opendir(path.c_str())) == NULL)
        err = errno;
    else {
        this_path = path;
        return err;
    }
    err = ff_log(FC_ERROR, err, "failed to open directory `%s'", path.c_str());
    return err;
}

/** close the currently open directory */
int fm_io_posix_dir::close()
{
    int err = 0;
    if (this_dir == NULL)
        err = ENOTCONN;
    else if (closedir(this_dir) != 0)
        err = errno;
    else {
        this_path.clear();
        this_dir = NULL;
        return err;
    }
    err = ff_log(FC_ERROR, err, "failed to close directory `%s'", this_path.c_str());
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
        if ((err = errno) == 0) // 0 for success or end-of-dir
            return err;
    }
    err = ff_log(FC_ERROR, err, "failed to read directory `%s'", this_path.c_str());
    return err;
}

FT_IO_NAMESPACE_END
