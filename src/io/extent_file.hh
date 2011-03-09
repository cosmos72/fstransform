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

#include <iosfwd>        // for std::istream forward declaration


FT_IO_NAMESPACE_BEGIN

/**
 * retrieves emulated file blocks allocation map (extents) from specified stream
 * and appends them to ret_container (retrieves also user_data)
 * in case of failure returns errno-compatible error code, and ret_list contents will be UNDEFINED.
 *
 * implementation: simply reads the list of triplets (physical, logical, length)
 * stored in the stream as decimal numbers
 */
int ff_read_extents_file(std::istream & is, ft_uoff dev_length, ft_vector<ft_uoff> & ret_list, ft_uoff & ret_block_size_bitmask);

/**
 * writes file blocks allocation map (extents) to specified stream (stores also user_data)
 * in case of failure returns errno-compatible error code.
 *
 * implementation: simply writes the list of triplets (physical, logical, length)
 * into the stream as decimal numbers
 */
int ff_write_extents_file(std::ostream & os, const ft_vector<ft_uoff> & extent_list);

FT_IO_NAMESPACE_END


#endif /* FSTRANSLATE_FILEMAP_HH */
