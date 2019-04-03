/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * io/io_posix_dir.cc
 *
 *  Created on: Sep 22, 2011
 *      Author: max
 */

#include "../first.hh"

#if defined(FT_HAVE_CERRNO)
# include <cerrno>           // for errno and error codes
#elif defined(FT_HAVE_ERRNO_H)
# include <errno.h>
#endif

#ifdef FT_HAVE_SYS_TYPES_H
# include <sys/types.h>      // for DIR, opendir()
#endif
#ifdef FT_HAVE_DIRENT_H
# include <dirent.h>         //  "   "     "      , readdir(), closedir()
#endif

#include "../log.hh"        // for ff_log()
#include "io_posix_dir.hh"  // for ft_io_posix_dir


FT_IO_NAMESPACE_BEGIN

/** default constructor */
ft_io_posix_dir::ft_io_posix_dir()
    : this_path(), this_dir(NULL)
{ }

/** destructor. calls close() */
ft_io_posix_dir::~ft_io_posix_dir()
{
    close();
}

/** open a directory */
int ft_io_posix_dir::open(const ft_string & path)
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
    return ff_log(FC_ERROR, err, "failed to open directory `%s'", path.c_str());
}

/** close the currently open directory */
int ft_io_posix_dir::close()
{
    if (this_dir != NULL) {
    	if (closedir(this_dir) != 0)
    		return ff_log(FC_ERROR, errno, "failed to close directory `%s'", this_path.c_str());
    	this_dir = NULL;
    }
	this_path.clear();
    return 0;
}


/**
 *  get next directory entry.
 *  returns 0 if success (NULL result indicates EOF),
 *  else returns error code
 */
int ft_io_posix_dir::next(ft_io_posix_dirent * & result)
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
    return ff_log(FC_ERROR, err, "failed to read directory `%s'", this_path.c_str());
}

FT_IO_NAMESPACE_END
