/*
 * file_extent.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_FILEMAP_HH
#define FSTRANSLATE_FILEMAP_HH

#include "fwd.hh" /* for ft_vector<T> forward declaration */

/**
 * retrieves file blocks allocation map (extents) for specified file descriptor
 * and appends them to ret_vector.
 * in case of failure returns errno-compatible error code, and ret_vector contents will be unchanged.
 *
 * implementation: calls ioctl(FS_IOC_FIEMAP) and if it fails, tries with ioctl(FIBMAP)
 */
template<class T>
int ff_file_extents(int fd, ft_vector<T> & ret_vector);




#ifdef FT_HAVE_EXTERN_TEMPLATE
#  define FT_EXTERN_TEMPLATE_file_extents(T)                       int ff_file_extents<T>(int fd, ft_vector<T> & ret_vector);
#  define FT_EXTERN_TEMPLATE_file_extent_hh(ft_prefix, ft_list_t)  ft_list_t(ft_prefix, FT_EXTERN_TEMPLATE_file_extents)
   FT_EXTERN_TEMPLATE_DECLARE(FT_EXTERN_TEMPLATE_file_extent_hh)
#else
#  include "file_extent.template.hh"
#endif /* FT_EXTERN_TEMPLATE */


#endif /* FSTRANSLATE_FILEMAP_HH */
