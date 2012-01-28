/*
 * io/util_posix.hh
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */
#ifndef FSREMAP_IO_POSIX_UTIL_HH
#define FSREMAP_IO_POSIX_UTIL_HH

#include "../types.hh" // for ft_uoff, ft_stat, ft_dev, ft_mode */

FT_IO_NAMESPACE_BEGIN

/** invoke ioctl() */
int ff_posix_ioctl(int fd, int request, void * arg);

/** return file stats in (*ret_stat) */
int ff_posix_stat(int fd, ft_stat * ret_stat);

/** return file size in (*ret_size) */
int ff_posix_size(int fd, ft_uoff * ret_size);

/** return block size of file-system containing file */
int ff_posix_blocksize(int fd, ft_uoff * ret_block_size);

/** return ID of device containing file in (*ret_dev) */
int ff_posix_dev(int fd, ft_dev * ret_dev);

/** if file is special block device, return its device ID in (*ret_dev) */
int ff_posix_blkdev_dev(int fd, ft_dev * ret_dev);

/** if file is special block device, return its length in (*ret_dev) */
int ff_posix_blkdev_size(int fd, ft_uoff * ret_size);


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

/**
 * spawn a system command, wait for it to complete and return its exit status.
 * argv[0] is conventionally the program name.
 * argv[1...] are program arguments and must be terminated with a NULL pointer.
 */
int ff_posix_exec(const char * path, const char * const argv[]);

FT_IO_NAMESPACE_END


#endif /* FSREMAP_IO_POSIX_UTIL_HH */
