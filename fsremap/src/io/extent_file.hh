/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * io/extent_posix.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSREMAP_IO_POSIX_EXTENT_HH
#define FSREMAP_IO_POSIX_EXTENT_HH

#include "../check.hh"

#if defined(FT_HAVE_STDIO_H)
# include <stdio.h>      // for FILE
#elif defined(FT_HAVE_CSTDIO)
# include <cstdio>       // for FILE
#endif

#include "../fwd.hh"     // for fr_vector<T> forward declaration */
#include "../types.hh"   // for ft_uoff

FT_IO_NAMESPACE_BEGIN

/**
 * load file blocks allocation map (extents) previously saved into specified file
 * and appends them to ret_container (retrieves also user_data)
 * in case of failure returns errno-compatible error code, and ret_list contents will be UNDEFINED.
 *
 * implementation: simply reads the list of triplets (physical, logical, length)
 * stored in the stream as decimal numbers
 */
int ff_load_extents_file(FILE * f, fr_vector<ft_uoff> & ret_list, ft_uoff & ret_block_size_bitmask);

/**
 * writes file blocks allocation map (extents) to specified FILE (stores also user_data)
 * in case of failure returns errno-compatible error code.
 *
 * implementation: simply writes the list of triplets (physical, logical, length)
 * into the FILE as decimal numbers
 */
int ff_save_extents_file(FILE * f, const fr_vector<ft_uoff> & extent_list);

/**
 * write 'length' bytes of zeros '\0' into file descriptor and return 0.
 * in case of failure returns errno-compatible error code.
 */
int ff_write_zero_fd(int fd, ft_uoff length);

FT_IO_NAMESPACE_END


#endif /* FSREMAP_FILEMAP_HH */
