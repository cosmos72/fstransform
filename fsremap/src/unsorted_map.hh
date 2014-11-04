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
 * inode_cache.hh
 *
 *  Created on: Aug 18, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_UNSORTED_MAP_HH
#define FSTRANSFORM_UNSORTED_MAP_HH

#include "check.hh"

#if defined(FT_HAVE_STD_UNORDERED_MAP)

# ifdef FT_HAVE_UNORDERED_MAP
#  include <unordered_map>
# endif
# define ft_unsorted_map std::unordered_map

#elif defined(FT_HAVE_STD_TR1_UNORDERED_MAP)

# ifdef FT_HAVE_TR1_UNORDERED_MAP
#  include <tr1/unordered_map>
# endif
# define ft_unsorted_map std::tr1::unordered_map

#endif

#endif /* FSTRANSFORM_UNSORTED_MAP_HH */
