/*
 * io/util_posix.hh
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */
#ifndef FSREMAP_IO_POSIX_UTIL_HH
#define FSREMAP_IO_POSIX_UTIL_HH

#include "../types.hh" // for ft_off, ft_stat, ft_dev */

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


/**
 * seek file descriptor to specified position from file beginning.
 * note: if an error is returned, file descriptor position will be undefined!
 */
int ff_posix_lseek(int fd, ft_uoff pos);

/**
 * read from a file descriptor.
 * keep retrying in case of EINTR or short reads.
 * on return, ret_length will be increased by the number of bytes actually read
 */
int ff_posix_read(int fd, void * mem, ft_uoff length);


/**
 * write to a file descriptor.
 * keep retrying in case of EINTR or short writes.
 * on return, ret_length will be increased by the number of bytes actually written
 */
int ff_posix_write(int fd, const void * mem, ft_uoff length);


FT_IO_NAMESPACE_END


#endif /* FSREMAP_IO_POSIX_UTIL_HH */
