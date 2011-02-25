/*
 * fiemap.h
 *
 *  Created on: Feb 14, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_FILEMAP_H
#define FSTRANSLATE_FILEMAP_H

#include "map.h" /* for ft_map */

/**
 * retrieves file blocks allocation map for specified file descriptor
 * and returns pointer to allocated struct in (*ret_map);
 * in case of failure returns operating system error (and *ret_map will be NULL).
 *
 * struct returned in *ret_map must be freed by caller with free() when not needed anymore.
 *
 * implementation: calls ioctl(FS_IOC_FIEMAP) and if it fails, tries with ioctl(FIBMAP)
 */
int ff_filemap(int fd, ft_map ** ret_map);


#endif /* FSTRANSLATE_FILEMAP_H */
