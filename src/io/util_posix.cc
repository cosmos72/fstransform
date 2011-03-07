/*
 * io/util_posix.cc
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */
#include "../first.hh"

#include <cerrno>        // for errno           */
#include <sys/ioctl.h>   // for ioctl()          */
#include <linux/fs.h>    // for BLKGETSIZE64     */

#include "../types.hh"   // for ft_u64, ft_stat  */
#include "util_posix.hh" // for ff_posix_ioctl(), ff_posix_stat(), ff_posix_size(), ff_filedev() */


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


/** return file size in (*ret_size) */
int ff_posix_size(int fd, ft_uoff * ret_size)
{
	ft_stat st_buf;
	int err = ff_posix_stat(fd, & st_buf);
	if (err == 0) {
	    ft_uoff file_size = (ft_uoff) st_buf.st_size;
	    if ((ft_off) file_size == st_buf.st_size)
	        * ret_size = file_size;
	    else
	        err = EFBIG; // file size cannot be represented by ft_uoff!
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
	ft_u64 size_buf;
	int err = ff_posix_ioctl(fd, BLKGETSIZE64, & size_buf);
	if (err == 0) {
        ft_uoff dev_size = (ft_uoff) size_buf;
        if ((ft_u64) dev_size == size_buf)
            * ret_size = dev_size;
        else
            err = EFBIG; // device size cannot be represented by ft_uoff!
	}
	return err;
}


/** return this process PID in (*ret_pid) */
int ff_posix_pid(ft_pid * ret_pid)
{
    * ret_pid = getpid();
    return 0;
}

/** create a directory */
int ff_posix_mkdir(const char * path, ft_mode mode)
{
    return mkdir(path, mode);
}

FT_IO_NAMESPACE_END
