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
 * io/io_prealloc.hh
 *
 *  Created on: Apr 15, 2012
 *      Author: max
 */

#ifndef FSMOVE_IO_IO_PREALLOC_HH
#define FSMOVE_IO_IO_PREALLOC_HH

#include "../types.hh"    // for ft_string, FT_HAVE_FALLOCATE
#include "io_posix.hh"    // for fm_io_posix

// io_prealloc requires fallocate()
#ifdef FT_HAVE_FALLOCATE
# define FT_HAVE_FM_IO_IO_PREALLOC

FT_IO_NAMESPACE_BEGIN

/**
 * class performing I/O on POSIX systems with preallocation.
 * performs target files preallocation WITHOUT modifying the source file directory
 */
class fm_io_prealloc: public fm_io_posix
{
private:
    typedef fm_io_posix super_type;

protected:
    /**
     * does nothing: fr_io_prealloc never needs to ::sync().
     */
    virtual void sync();

    /**
     * copy the contents of single regular file 'source_path' to 'target_path'.
     * Since we are preallocating, we just preallocate enough blocks inside 'target_path'
     */
    virtual int copy_file_contents(const ft_string & source_path, const ft_stat & source_stat, const ft_string & target_path);

    /**
     * remove a regular file inside source directory
     * Since we are preallocating, we can (and will) avoid any modification
     * to the source file system. Thus this method does nothing.
     */
    virtual int remove_file(const char * source_path);

    /**
     * remove a special file inside source directory
     * Since we are preallocating, we can (and will) avoid any modification
     * to the source file system. Thus this method does nothing.
     */
    virtual int remove_special(const char * source_path);

    /**
     * remove a source directory, which must be empty
     * exception: will not remove '/lost+found' directory inside source_root()
     * Since we are preallocating, we can (and will) avoid any modification
     * to the source file system. Thus this method does nothing.
     */
    virtual int remove_dir(const ft_string & path);

public:
    /** default constructor. */
    fm_io_prealloc();

    /** check for consistency and open SOURCE_ROOT, TARGET_ROOT */
    virtual int open(const fm_args & args);

    /** destructor. */
    virtual ~fm_io_prealloc();
};

FT_IO_NAMESPACE_END

#endif /* FT_HAVE_FM_IO_IO_PREALLOC */

#endif /* FSMOVE_IO_IO_PREALLOC_HH */
