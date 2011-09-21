/*
 * io/extent_posix.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSREMAP_IO_POSIX_EXTENT_HH
#define FSREMAP_IO_POSIX_EXTENT_HH

#include "../fwd.hh"     // for fr_vector<T> forward declaration */
#include "../types.hh"   // for ft_uoff


FT_IO_NAMESPACE_BEGIN

/**
 * retrieves file blocks allocation map (extents) for specified file descriptor
 * and appends them to ret_vector (with user_data = FC_DEFAULT_USER_DATA)
 * in case of failure returns errno-compatible error code, and ret_vector contents will be UNDEFINED.
 *
 * implementation: calls ioctl(FS_IOC_FIEMAP) and if it fails, tries with ioctl(FIBMAP)
 */
int ff_read_extents_posix(int fd, ft_uoff dev_length, fr_vector<ft_uoff> & ret_list, ft_uoff & ret_block_size_bitmask);


FT_IO_NAMESPACE_END


#endif /* FSREMAP_FILEMAP_HH */
