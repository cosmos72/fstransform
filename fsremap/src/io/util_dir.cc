/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 2 of the License, or
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
 * io/util.cc
 *
 *  Created on: Mar 6, 2011
 *      Author: max
 */

#include "../first.hh"

#include "../types.hh"   // for ft_mode
#include "../log.hh"     // for ff_log

#if defined(FT_HAVE_STRING_H)
# include <string.h>        // for memchr()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>         // for memchr()
#endif

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>        // for errno
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>         // for errno
#endif

#ifdef FT_HAVE_SYS_STAT_H
# include <sys/stat.h>     // for mkdir()
#endif
#ifdef FT_HAVE_SYS_TYPES_H
# include <sys/types.h>    // for mkdir()
#endif




FT_IO_NAMESPACE_BEGIN


/**
 * create a directory, return 0 (success) or error.
 * note: path MUST NOT end with '/'
 */
int ff_mkdir(const char * path, ft_mode mode)
{
#ifdef __USE_POSIX
    int err = mkdir(path, mode);
    return err != 0 ? errno : 0;
#else
    return ENOSYS;
#endif
}

int ff_mkdir_or_warn(const char * path, ft_mode mode)
{
    int err = ff_mkdir(path, mode);
    if (err != 0 && err != EEXIST)
        err = ff_log(FC_WARN, err, "failed to create directory `%s'", path);
    return err;
}

int ff_mkdir_recursive(const ft_string & path)
{
    ft_string partial;
    size_t len = path.length();
    const char * start = path.c_str(), * slash, * prev = start, * end = start + len;
    int err = 0;
    partial.reserve(len);
    while ((slash = (const char *)memchr(prev, '/', end - prev)) != NULL)
    {
        // if path starts with "/", try to create "/", NOT the unnamed directory ""
        partial.assign(start, slash == start ? 1 : slash - start);
        err = ff_mkdir_or_warn(partial.c_str(), 0700);
        if (err != 0 && err != EEXIST)
            return err;

        prev = slash + 1;
    }
    // if path does not end with "/", create the last segment
    if (len != 0 && prev != end)
        err = ff_mkdir_or_warn(path.c_str(), 0700);
    return err;
}

int ff_remove_recursive(const ft_string & FT_ARG_UNUSED(path))
{
    return ENOSYS;
}


FT_IO_NAMESPACE_END
