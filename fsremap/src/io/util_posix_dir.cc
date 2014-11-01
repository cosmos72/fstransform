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
 * io/util.cc
 *
 *  Created on: Mar 6, 2011
 *      Author: max
 */

#include "../first.hh"

#include "../types.hh"   // for ft_mode

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


/** create a directory */
int ff_posix_mkdir(const char * path, ft_mode mode)
{
    int err = mkdir(path, mode);
    return err == 0 ? err : errno;
}

int ff_posix_mkdir_recursive(const ft_string & path)
{
	return ENOSYS;
}

int ff_posix_remove_recursive(const ft_string & path)
{
	return ENOSYS;
}


FT_IO_NAMESPACE_END
