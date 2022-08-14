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
 * io/io_posix_xattr.hh
 *
 *  Created on: Apr 17, 2021
 *      Author: max
 */

#ifndef FSMOVE_IO_IO_POSIX_XATTR_HH
#define FSMOVE_IO_IO_POSIX_XATTR_HH

#include "../types.hh" // for ft_u64, ft_string */

FT_IO_NAMESPACE_BEGIN

/**
 * class dealing with extended attributes on POSIX systems
 */
class fm_io_posix_xattr {
  public:
    fm_io_posix_xattr();
    ~fm_io_posix_xattr(); // calls close()

    int openfile(const ft_string &path); // return file descriptor, or < 0
    int opendir(const ft_string &path);  // return file descriptor, or < 0
    int close();

    // copy current flags and projid to specified file descriptor,
    // assuming it *will* be modified afterwards
    int copy_xattr_before(const ft_string &path, int fd) const;

    // copy current flags and projid to specified file descriptor,
    // assuming it will *not* be modified afterwards
    int copy_xattr_after(const ft_string &path, int fd) const;

  private:
    int open(const ft_string &path, bool isdir);

    int copy_xattr_to(const ft_string &path, int fd, ft_u64 clearmask, ft_u64 copymask) const;

    int this_fd; // file descriptor
    ft_u32 this_flags;
    ft_u32 this_projid;
};

FT_IO_NAMESPACE_END

#endif /* FSMOVE_IO_IO_POSIX_XATTR_HH */
