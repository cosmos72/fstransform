/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 2 of the License, or
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
 * map.cc
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "first.hh"      // for FT_*TEMPLATE* macros */

#ifdef FT_HAVE_EXTERN_TEMPLATE
#  include "map.t.hh"
   FT_TEMPLATE_INSTANTIATE(FT_TEMPLATE_map_hh)
#endif /* FT_HAVE_EXTERN_TEMPLATE */
