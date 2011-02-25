/*
 * fileutil.h
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */
#ifndef FSTRANSLATE_FILEUTIL_H
#define FSTRANSLATE_FILEUTIL_H

#include "types.h" /* for ff_ioctl(), ff_size(), ff_filedev() */


/** invoke ioctl() */
int ff_ioctl(int fd, int request, void * arg);

/** return file stats in (*ret_stat) */
int ff_stat(int fd, ft_stat * ret_stat);

/** return file size in (*ret_size) */
int ff_size(int fd, ft_off * ret_size);

/** return ID of device containing file in (*ret_dev) */
int ff_dev(int fd, ft_dev * ret_dev);

/** if file is special block device, return its device ID in (*ret_dev) */
int ff_blkdev(int fd, ft_dev * ret_dev);

/** if file is special block device, return its length in (*ret_dev) */
int ff_blkdev_size(int fd, ft_off * ret_size);

#endif /* FSTRANSLATE_FILEUTIL_H */
