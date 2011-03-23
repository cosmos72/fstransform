/*
 * io/extent_posix.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_IO_POSIX_EXTENT_HH
#define FSTRANSLATE_IO_POSIX_EXTENT_HH

#include "../fwd.hh"     // for ft_vector<T> forward declaration */
#include "../types.hh"   // for ft_uoff

#include <cstdio>        // for FILE


FT_IO_NAMESPACE_BEGIN

/**
 * load file blocks allocation map (extents) previously saved into specified file
 * and appends them to ret_container (retrieves also user_data)
 * in case of failure returns errno-compatible error code, and ret_list contents will be UNDEFINED.
 *
 * implementation: simply reads the list of triplets (physical, logical, length)
 * stored in the stream as decimal numbers
 */
int ff_load_extents_file(FILE * f, ft_vector<ft_uoff> & ret_list, ft_uoff & ret_block_size_bitmask);

/**
 * writes file blocks allocation map (extents) to specified FILE (stores also user_data)
 * in case of failure returns errno-compatible error code.
 *
 * implementation: simply writes the list of triplets (physical, logical, length)
 * into the FILE as decimal numbers
 */
int ff_save_extents_file(FILE * f, const ft_vector<ft_uoff> & extent_list);

/**
 * write 'length' bytes of zeros '\0' into file descriptor and return 0.
 * in case of failure returns errno-compatible error code.
 */
int ff_write_zero_fd(int fd, ft_uoff length);

FT_IO_NAMESPACE_END


#endif /* FSTRANSLATE_FILEMAP_HH */
