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
 * io/util_posix.cc
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */
#include "../first.hh"

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>        // for errno, EOVERFLOW, ENOTBLK, EINTR
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>         // for errno, EOVERFLOW, ENOTBLK, EINTR
#endif

#if defined(FT_HAVE_STDLIB_H)
# include <stdlib.h>       // for exit(), posix_fallocate()
#elif defined(FT_HAVE_CSTDLIB)
# include <cstdlib>        // for exit(), posix_fallocate()
#endif

#ifdef FT_HAVE_FCNTL_H
# include <fcntl.h>        // for fallocate()
#endif
#ifdef FT_HAVE_UNISTD_H
# include <unistd.h>       // for fork(), execvp()
#endif
#ifdef FT_HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>    // for ioctl()
#endif
#ifdef FT_HAVE_SYS_STAT_H
# include <sys/stat.h>     // for stat()
#endif
#ifdef FT_HAVE_SYS_TYPES_H
# include <sys/types.h>    // for waitpid()
#endif
#ifdef FT_HAVE_SYS_WAIT_H
# include <sys/wait.h>     // for    "
#endif
#ifdef FT_HAVE_SYS_DISKLABEL_H
# include <sys/disklabel.h> // for struct disklabel on *BSD
#endif
#ifdef FT_HAVE_LINUX_FS_H
# include <linux/fs.h>     // for BLKGETSIZE64 on Linux
#endif

#include "../types.hh"    // for ft_u64, ft_stat
#include "../log.hh"      // for ff_log()
#include "../misc.hh"     // for ff_min2<T>()
#include "util_posix.hh"  // for ff_posix_ioctl(), ff_posix_stat(), ff_posix_size(), ff_filedev()


FT_IO_NAMESPACE_BEGIN



/** invoke ioctl() */
int ff_posix_ioctl(int fd, int request, void * arg)
{
    return ioctl(fd, request, arg) == 0 ? 0 : errno;
}

/** return file stats in (*ret_stat) */
int ff_posix_stat(int fd, ft_stat * ret_stat)
{
    int err = fstat(fd, ret_stat);
    if (err != 0)
        err = errno;
    return err;
}

/** return file stats in (*ret_stat) */
int ff_posix_stat(const char * path, ft_stat * ret_stat)
{
    int err = lstat(path, ret_stat);
    if (err != 0)
    	err = ff_log(FC_ERROR, errno, "error in lstat(%s)", path);
    return err;
}

/** return block size of file-system containing file */
int ff_posix_blocksize(int fd, ft_uoff * ret_block_size)
{
    ft_stat st_buf;
    int err = ff_posix_stat(fd, & st_buf);
    if (err == 0) {
        ft_uoff block_size;
        if ((err = ff_narrow(st_buf.st_blksize, & block_size)) == 0)
        	* ret_block_size = block_size;
    }
    return err;
}

/** return file size in (*ret_size) */
int ff_posix_size(int fd, ft_uoff * ret_size)
{
    ft_stat st_buf;
    int err = ff_posix_stat(fd, & st_buf);
    if (err == 0) {
        ft_uoff file_size;
        if ((err = ff_narrow(st_buf.st_size, & file_size)) == 0)
        	* ret_size = file_size;
    }
    return err;
}

/** return ID of device containing file in (*ret_dev) */
int ff_posix_dev(int fd, dev_t * ret_dev)
{
    ft_stat st_buf;
    int err = ff_posix_stat(fd, & st_buf);
    if (err == 0)
        * ret_dev = st_buf.st_dev;
    return err;
}



/** if file is special block device, return its device ID in (*ret_dev) */
int ff_posix_blkdev_dev(int fd, ft_dev * ret_dev)
{
    ft_stat st_buf;
    int err = ff_posix_stat(fd, & st_buf);
    if (err == 0) {
        if ((st_buf.st_mode & S_IFBLK))
            * ret_dev = st_buf.st_rdev;
        else
            err = ENOTBLK;
    }
    return err;
}

/** if file is special block device, return its length in (*ret_size) */
int ff_posix_blkdev_size(int fd, ft_uoff * ret_size)
{
#if defined(DIOCGDINFO) && defined(FT_HAVE_STRUCT_DISKLABEL_D_SECSIZE) && defined(FT_HAVE_STRUCT_DISKLABEL_D_SECPERUNIT)
    // *BSD
    struct disklabel dl;
    int err = ff_posix_ioctl(fd, DIOCGDINFO, & dl);
    if (err == 0) {
        if (dl.d_secsize <= 0 || dl.d_secperunit <= 0)
            err = EINVAL; // invalid size
        else {
            ft_uoff dev_sector_size = (ft_uoff) dl.d_secsize;
            ft_uoff dev_sector_count = (ft_uoff) dl.d_secperunit;
            if (dev_sector_size != dl.d_secsize || dev_sector_count != dl.d_secperunit)
                err = EOVERFLOW; // sector size or sector count cannot be represented by ft_uoff!
            else {
                ft_uoff dev_size = dev_sector_size * dev_sector_count;
                // check for multiplication overflow
                if (dev_size / dev_sector_size != dev_sector_count)
                     err = EOVERFLOW; // device size cannot be represented by ft_uoff!
                else
                    * ret_size = dev_size;
            }
        }
    }
#elif defined(BLKGETSIZE64)
    // Linux
    ft_u64 size_u64 = 0;
    int err = ff_posix_ioctl(fd, BLKGETSIZE64, & size_u64);
    if (err == 0) {
        if (size_u64 <= 0)
            err = EINVAL; // invalid size
        else if (size_u64 > (ft_uoff)-1)
            err = EOVERFLOW; // device size cannot be represented by ft_uoff!
        else
            * ret_size = (ft_uoff) size_u64;
    }
#else
    // Linux, obsolete: BLKGETSIZE returns device size DIVIDED 512
    unsigned long size_div_512 = 0;
    int err = ff_posix_ioctl(fd, BLKGETSIZE, & size_div_512);
    if (err == 0) {
        if (size_div_512 <= 0)
            err = EINVAL; // invalid size
        else if (size_div_512 > ((ft_uoff)-1 >> 9))
            err = EOVERFLOW; // device size cannot be represented by ft_uoff!
        else
            * ret_size = (ft_uoff) size_div_512 << 9;
    }
#endif
    return err;
}




/**
 * seek file descriptor to specified position from file beginning.
 * note: if an error is returned, file descriptor position will be undefined!
 */
int ff_posix_lseek(int fd, ft_uoff pos)
{
    off_t pos_s = (off_t)pos;

    if (pos_s < 0 || pos != (ft_uoff) pos_s)
        return EOVERFLOW;
    if ((pos_s = lseek(fd, pos_s, SEEK_SET)) < 0)
        return errno;
    if (pos != (ft_uoff) pos_s)
        return ESPIPE;
    return 0;
}



/**
 * read from a file descriptor.
 * keep retrying in case of EINTR or short reads.
 */
int ff_posix_read(int fd, void * mem, ft_uoff length)
{
    ft_uoff chunk, max = (ft_uoff)((size_t)(ssize_t)-1 >> 1); /**< max = std::numeric_limits<ssize_t>::max() */
    ssize_t got;

    while (length != 0) {
        chunk = ff_min2(length, max);
        while ((got = ::read(fd, mem, (size_t)chunk)) < 0 && errno == EINTR)
            ;
        if (got < 0)
            return errno;
        if (got == 0)
            /* end-of-file */
            break;
        if ((ft_uoff) got >= length)
            break;
        mem = (void *)((char *)mem + got);
        length -= (ft_uoff) got;
    }
    return 0;
}


/**
 * write to a file descriptor.
 * keep retrying in case of EINTR or short writes.
 */
int ff_posix_write(int fd, const void * mem, ft_uoff length)
{
    ft_uoff chunk, max = (ft_uoff)((size_t)(ssize_t)-1 >> 1); /**< max = std::numeric_limits<ssize_t>::max() */
    ssize_t sent;

    while (length != 0) {
        chunk = ff_min2(length, max);
        while ((sent = ::write(fd, mem, (size_t)chunk)) < 0 && errno == EINTR)
            ;
        if (sent < 0)
            return errno;
        if (sent == 0)
            /* end-of-file */
            break;
        if ((ft_uoff) sent >= length)
            break;
        mem = (const void *)((const char *)mem + sent);
        length -= (ft_uoff) sent;
    }
    return 0;
}

/**
 * preallocate and fill with zeroes 'length' bytes on disk for a file descriptor.
 * uses fallocate() if available, else posix_fallocate(), else plain write() loop.
 */
int ff_posix_fallocate(int fd, ft_off length, const ft_string & err_msg)
{
	int err = 0;
#if defined(FT_HAVE_FALLOCATE)
	if ((err = fallocate(fd, 0, 0, length)) != 0)
#elif defined(FT_HAVE_POSIX_FALLOCATE)
	if ((err = posix_fallocate(fd, 0, length)) != 0)
#endif /* FT_HAVE_FALLOCATE */
	{
		/* fall back on write() */
		enum { zero_len = 64*1024 };
		char zero[zero_len];
		ft_off pos = 0;
		ft_size chunk;

		while (pos < length) {
			// safe cast ft_uoff -> ft_size, the value is <= zero_len
			chunk = (ft_size) ff_min2<ft_off>(zero_len, length - pos);
			if ((err = ff_posix_write(fd, zero, chunk)) != 0) {
				err = ff_log(FC_ERROR, errno, "%s", err_msg.c_str());
				break;
			}
			pos += chunk;
		}
	}
	return err;
}

/**
 * spawn a system command, typically with fork()+execv(), wait for it to complete and return its exit status.
 * argv[0] is conventionally the program name.
 * argv[1...] are program arguments and must be terminated with a NULL pointer.
 */
int ff_posix_exec(const char * path, const char * const argv[])
{
    int err;
    pid_t pid = ::fork();
    if (pid == 0) {
        /* child */
        ::execvp(path, (char * const *)argv);

        /* if we reach here, execvp() failed! */
        ff_log(FC_ERROR, err = errno, "execvp(%s) failed");

        /* exit() can only return one-byte exit status */
        err &= 0xff;
        if (err == 0) {
            err = (ECHILD > 0 ? ECHILD : -ECHILD) & 0xff;
            if (err == 0)
                err = 1;
        }
        ::exit(err);
    } else if (pid == (pid_t)-1) {
        err = ff_log(FC_ERROR, errno, "fork() failed");
    } else {
        /* parent */
        err = -ECHILD; // assume failure unless proved successful...
        int status = 0;

        if (waitpid(pid, & status, 0/*options*/) != pid) {
        	err = ff_log(FC_ERROR, errno, "error in waitpid(), assuming command '%s' failed", path);
        	if (err == 0)
        		err = -ECHILD;
        } else if (WIFEXITED(status)) {

            status = WEXITSTATUS(status);
            if (status == 0)
                err = 0; // proved successful!
            else
                ff_log(FC_ERROR, 0, "command '%s' exited with non-zero exit status %d", path, status);

        } else if (WIFSIGNALED(status))
            ff_log(FC_ERROR, 0, "command '%s' died with signal %d", path, (int)WTERMSIG(status));
        else
            ff_log(FC_ERROR, 0, "waitpid() returned unknown status %d, assuming command '%s' failed", status, path);
    }
    return err;
}

FT_IO_NAMESPACE_END
