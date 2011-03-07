/*
 * io/util_posix.hh
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */
#ifndef FSTRANSLATE_IO_POSIX_UTIL_HH
#define FSTRANSLATE_IO_POSIX_UTIL_HH

#include "../types.hh" // for ft_off, ft_stat, ft_dev, ft_u64 */

FT_IO_NAMESPACE_BEGIN

/** invoke ioctl() */
int ff_posix_ioctl(int fd, int request, void * arg);

/** return file stats in (*ret_stat) */
int ff_posix_stat(int fd, ft_stat * ret_stat);

/** return file size in (*ret_size) */
int ff_posix_size(int fd, ft_uoff * ret_size);

/** return ID of device containing file in (*ret_dev) */
int ff_posix_dev(int fd, ft_dev * ret_dev);

/** if file is special block device, return its device ID in (*ret_dev) */
int ff_posix_blkdev_dev(int fd, ft_dev * ret_dev);

/** if file is special block device, return its length in (*ret_dev) */
int ff_posix_blkdev_size(int fd, ft_uoff * ret_size);


/** return this process PID in (*ret_pid) */
int ff_posix_pid(ft_pid * ret_pid);

/** create a directory */
int ff_posix_mkdir(const char * path, ft_mode mode = 0755);

FT_IO_NAMESPACE_END


#endif /* FSTRANSLATE_IO_POSIX_UTIL_HH */
