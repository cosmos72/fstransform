/*
 * fileutil.c
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */
#include "first.hh"

#include <cerrno>        /* for errno       */

#include <sys/ioctl.h>   /* for ioctl() */

#include <linux/fs.h>   /* for BLKGETSIZE64 */
#include <linux/types.h>/* for __u64        */

#include "file_util.hh"  /* for ft_stat, ff_ioctl(), ff_stat(), ff_size(), ff_filedev() */


typedef __u64 ft_u64;


/** invoke ioctl() */
int ff_ioctl(int fd, int request, void * arg) {
	return ioctl(fd, request, arg) == 0 ? 0 : errno;
}

/** return file stats in (*ret_stat) */
int ff_stat(int fd, ft_stat * ret_stat) {
	int err = fstat(fd, ret_stat);
	if (err != 0)
		err = errno;
	return err;
}


/** return file size in (*ret_size) */
int ff_size(int fd, off_t * ret_size) {
	ft_stat st;
	int err = ff_stat(fd, & st);
	if (err == 0)
		* ret_size = st.st_size;
	return err;
}

/** return ID of device containing file in (*ret_dev) */
int ff_dev(int fd, dev_t * ret_dev) {
	ft_stat st;
	int err = ff_stat(fd, & st);
	if (err == 0)
		* ret_dev = st.st_dev;
	return err;
}



/** if file is special block device, return its device ID in (*ret_dev) */
int ff_blkdev(int fd, ft_dev * ret_dev) {
	ft_stat st;
	int err = ff_stat(fd, & st);
	if (err == 0) {
		if ((st.st_mode & S_IFBLK))
			* ret_dev = st.st_rdev;
		else
			err = ENOTBLK;
	}
	return err;
}

/** if file is special block device, return its length in (*ret_size) */
int ff_blkdev_size(int fd, ft_off * ret_size) {
	ft_u64 dev_size;
	int err = ff_ioctl(fd, BLKGETSIZE64, & dev_size);
	if (err == 0) {
		/* maybe ft_off is not wide enough to represent (ft_u64) dev_size ? */
		if (dev_size == (ft_u64)(ft_off) dev_size)
			* ret_size = (ft_off) dev_size;
		else
			err = EFBIG;
	}
	return err;
}

