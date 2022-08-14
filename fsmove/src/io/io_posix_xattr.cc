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
 * io/io_posix.cc
 *
 *  Created on: Sep 22, 2011
 *      Author: max
 */

#include "../first.hh"

#include "io_posix_xattr.hh"
#include "../log.hh" // for ff_log()

#ifdef FT_HAVE_FCNTL_H
#include <fcntl.h> // for open()
#endif
#ifdef FT_HAVE_ERRNO_H
#include <errno.h> // for errno
#endif
#ifdef FT_HAVE_LINUX_FS_H
#include <linux/fs.h> // for struct fsxattr
#endif
#ifdef FT_HAVE_SYS_IOCTL_H
#include <sys/ioctl.h> // for ioctl()
#endif
#ifdef FT_HAVE_SYS_STAT_H
#include <sys/stat.h> // for open()
#endif
#ifdef FT_HAVE_SYS_TYPES_H
#include <sys/types.h> // for open()
#endif
#ifdef FT_HAVE_UNISTD_H
#include <unistd.h> // for close()
#endif

FT_IO_NAMESPACE_BEGIN

/** default constructor */
fm_io_posix_xattr::fm_io_posix_xattr() : this_fd(-1), this_flags(0), this_projid(0) {
}

/** destructor. calls close() */
fm_io_posix_xattr::~fm_io_posix_xattr() {
    (void)close();
}

/** open file read/write, get its extended attributes, remove 'immutable' attribute */
int fm_io_posix_xattr::openfile(const ft_string &path) {
    return open(path, false);
}

int fm_io_posix_xattr::opendir(const ft_string &path) {
    return open(path, true);
}

int fm_io_posix_xattr::open(const ft_string &path, bool isdir) {
    const char *cpath = path.c_str();
    int err = close();
    if (err != 0) {
        return err;
    }

#ifndef O_DIRECTORY
#define O_DIRECTORY 0
#endif

#ifdef FS_IOC_FSGETXATTR
    int err_open_rdwr = 0;
    bool readonly = false;
    if (isdir) {
        err = ::open(cpath, O_RDONLY | O_DIRECTORY);
    } else if ((err = err_open_rdwr = ::open(cpath, O_RDWR)) < 0) {
        err = ::open(cpath, O_RDONLY);
        readonly = true;
    }
    if (err < 0) {
        return err;
    }
    this_fd = err;
    this_projid = this_flags = 0;

    struct fsxattr xattr = {};
    if (ioctl(this_fd, FS_IOC_FSGETXATTR, &xattr) < 0) {
        ff_log(readonly ? FC_ERROR : FC_WARN, errno, //
               "failed to ioctl(FS_IOC_FSGETXATTR) `%s'", cpath);
        if (readonly) {
            (void)close();
            return err_open_rdwr;
        }
    }
    this_flags = xattr.fsx_xflags;
    this_projid = xattr.fsx_projid;

    if (readonly || (xattr.fsx_xflags & (FS_XFLAG_IMMUTABLE | FS_XFLAG_APPEND)) != 0) {
        xattr.fsx_xflags &= ~(FS_XFLAG_IMMUTABLE | FS_XFLAG_APPEND);
        if (ioctl(this_fd, FS_IOC_FSSETXATTR, &xattr) < 0) {
            ff_log(FC_WARN, errno, "failed ioctl(FS_IOC_FSSETXATTR) `%s'", cpath);
        }
        if (readonly) {
            // retry opening read/write after removing immutable and append flags
            (void)close();
            if ((err = ::open(cpath, O_RDWR)) < 0) {
                return err;
            }
            this_fd = err;
        }
    }
#else
    err = ::open(cpath, isdir ? O_RDONLY | O_DIRECTORY : O_RDWR);
    if (err >= 0) {
        this_fd = err;
    }
#endif
    return err;
}

/** close file descriptor */
int fm_io_posix_xattr::close() {
    int err = 0;
    if (this_fd >= 0 && (err = ::close(this_fd)) == 0) {
        this_fd = -1;
    }
    return err;
}

// copy current flags and projid to specified file descriptor,
// assuming it *will* be modified afterwards
int fm_io_posix_xattr::copy_xattr_before(const ft_string &path, int fd) const {
    const ft_u64 clearmask = FS_XFLAG_NOSYMLINKS;
    const ft_u64 copymask = FS_XFLAG_REALTIME | FS_XFLAG_NOATIME | FS_XFLAG_NODUMP |
                            FS_XFLAG_RTINHERIT | FS_XFLAG_PROJINHERIT | FS_XFLAG_NODEFRAG |
                            FS_XFLAG_FILESTREAM;
    return copy_xattr_to(path, fd, clearmask, copymask);
}

// copy current flags and projid to specified file descriptor,
// assuming it will *not* be modified afterwards
int fm_io_posix_xattr::copy_xattr_after(const ft_string &path, int fd) const {
    const ft_u64 clearmask = FS_XFLAG_NOSYMLINKS;
    const ft_u64 copymask = FS_XFLAG_IMMUTABLE | FS_XFLAG_APPEND | FS_XFLAG_SYNC | FS_XFLAG_DAX;
    return copy_xattr_to(path, fd, clearmask, copymask);
}

int fm_io_posix_xattr::copy_xattr_to(const ft_string &path, int fd, ft_u64 clearmask,
                                     ft_u64 copymask) const {
#ifdef FS_IOC_FSGETXATTR
    int err = 0;
    if ((this_flags & copymask) == 0 && this_projid == 0) {
        return err;
    }
    struct fsxattr xattr = {};
    if ((err = ioctl(fd, FS_IOC_FSGETXATTR, &xattr)) < 0) {
        return ff_log(FC_WARN, errno, "failed ioctl(FS_IOC_FSGETXATTR) `%s'", path.c_str());
    }
    xattr.fsx_xflags = (xattr.fsx_xflags & ~clearmask) | (this_flags & copymask);
    xattr.fsx_projid = this_projid;
    if ((err = ioctl(fd, FS_IOC_FSSETXATTR, &xattr)) < 0) {
        return ff_log(FC_WARN, errno, "failed ioctl(FS_IOC_FSSETXATTR) `%s'", path.c_str());
    }
#else
    (void)path;
    (void)fd;
#endif
    return 0;
}

FT_IO_NAMESPACE_END
