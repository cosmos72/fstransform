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
 * io/io_prealloc.cc
 *
 *  Created on: Apr 15, 2012
 *      Author: max
 */

#include "../first.hh"


#if defined(FT_HAVE_CERRNO)
# include <cerrno>         // for errno and error codes
#elif defined(FT_HAVE_ERRNO_H)
# include <errno.h>
#endif

#ifdef FT_HAVE_FCNTL_H
#include <fcntl.h>         // for open(), fallocate()
#endif
#ifdef FT_HAVE_SYS_STAT_H
# include <sys/stat.h>     // for   "
#endif
#ifdef FT_HAVE_SYS_TYPES_H
# include <sys/types.h>    // for   "
#endif
#ifdef FT_HAVE_UNISTD_H
# include <unistd.h>       // for close()
#endif

#include "../log.hh"       // for ff_log()
#include "io_prealloc.hh"  // for fm_io_prealloc, FT_HAVE_FM_IO_IO_PREALLOC


#ifdef FT_HAVE_FM_IO_IO_PREALLOC

FT_IO_NAMESPACE_BEGIN


/** default constructor. */
fm_io_prealloc::fm_io_prealloc()
: super_type()
{ }

/** destructor. */
fm_io_prealloc::~fm_io_prealloc()
{ }

/** check for consistency and open SOURCE_ROOT, TARGET_ROOT */
int fm_io_prealloc::open(const fm_args & args)
{
	int err = super_type::open(args);
	if (err == 0)
		progress_msg(" still to preallocate");
	return err;
}

/**
 * does nothing: fr_io_prealloc never needs to ::sync().
 */
void fm_io_prealloc::sync()
{ }

/**
 * remove the regular file 'source_path'
 * Since we are preallocating, we can (and will) avoid any modification
 * to the source file system. Thus this method does nothing.
 */
int fm_io_prealloc::remove_file(const char * FT_ARG_UNUSED(source_path))
{
    return 0;
}

/**
 * remove the special file 'source_path'
 * Since we are preallocating, we can (and will) avoid any modification
 * to the source file system. Thus this method does nothing.
 */
int fm_io_prealloc::remove_special(const char * FT_ARG_UNUSED(source_path))
{
    return 0;
}

/**
 * remove a source directory.
 * exception: we are not supposed to delete 'lost+found' directory inside source_root()
 * Since we are preallocating, we can (and will) avoid any modification
 * to the source file system. Thus this method does nothing.
 */
int fm_io_prealloc::remove_dir(const ft_string & path)
{
    ff_log(FC_TRACE, 0, "remove_dir()   `%s'", path.c_str());
    return 0;
}

/**
 * copy the contents of regular file 'source_path' to 'target_path'.
 * Since we are preallocating, we just preallocate enough blocks inside 'target_path'
 */
int fm_io_prealloc::copy_file_contents(const ft_string & FT_ARG_UNUSED(source_path), const ft_stat & source_stat, const ft_string & target_path)
{
    const char * target = target_path.c_str();
    int err = 0;

#ifndef O_EXCL
# define O_EXCL 0
#endif
    int out_fd = ::open(target, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL, 0600);
    if (out_fd < 0)
        err = ff_log(FC_ERROR, errno, "failed to create target file `%s'", target);

    if (err == 0) {
        err = this->periodic_check_free_space();
        if (err == 0) {
            ft_off len = source_stat.st_size;
            // we ONLY want REAL preallocation: avoid posix_fallocate(),
            // as it will happily write a bunch of zeroes in target file if REAL real preallocation is not supported,
            // spoiling our purpose of NOT increasing disk usage of the loop file we are writing into
            if (len == 0 || fallocate(out_fd, 0, 0, len) == 0)
                err = this->periodic_check_free_space(len);
            else
                err = ff_log(FC_ERROR, errno, "failed to preallocate %" FT_ULL " bytes for target file '%s': error in fallocate()",
                             (ft_ull) len, target);
        }
    }
    if (out_fd >= 0)
        (void) ::close(out_fd);

    if (err == 0)
        err = this->copy_stat(target, source_stat);

    return err;
}

FT_IO_NAMESPACE_END

#endif /* FT_HAVE_FM_IO_IO_PREALLOC */
