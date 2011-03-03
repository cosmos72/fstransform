/*
 * io/posix_extent.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_IO_POSIX_EXTENT_HH
#define FSTRANSLATE_IO_POSIX_EXTENT_HH

#include "../fwd.hh"   // for ft_extent_list forward declaration */

#include "../types.hh" // for ft_uoff

FT_IO_NAMESPACE_BEGIN

/**
 * retrieves file blocks allocation map (extents) for specified file descriptor
 * and appends them to ret_container.
 * in case of failure returns errno-compatible error code, and ret_list contents will be UNDEFINED.
 *
 * implementation: calls ioctl(FS_IOC_FIEMAP) and if it fails, tries with ioctl(FIBMAP)
 *
 * must (and will) also check that device blocks count can be represented by ret_list,
 * by calling ret_list.extent_set_range(block_size, block_count)
 */
int ff_posix_extents(int fd, ft_uoff dev_length, ft_extent_list & ret_list);


FT_IO_NAMESPACE_END


#endif /* FSTRANSLATE_FILEMAP_HH */
