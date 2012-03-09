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
 * dispatch.cc
 *
 *  Created on: Mar 3, 2011
 *      Author: max
 */

#include "first.hh"

#include "dispatch.hh"    // for fr_dispatch
#include "work.hh"        // for fr_work<T>

FT_NAMESPACE_BEGIN


/**
 * instantiate and run fr_work<T>
 * with the smallest T that can represent device blocks count.
 * return 0 if success, else error.
 *
 * implementation: iterates on all known configured T and,
 * if both fr_work<T>::check() and fr_work<T>::init() succeed,
 * calls fr_work<T>::run(), then fr_work<T>::cleanup()
 */
int fr_dispatch::main(fr_vector<ft_uoff> & loop_file_extents,
                           fr_vector<ft_uoff> & free_space_extents, FT_IO_NS fr_io & io)
{
    if (fr_work<ft_uint>::check(io) == 0) {
        ff_log(FC_INFO, 0, "using reduced (%"FT_ULL" bit) remapping algorithm", (ft_ull)8*sizeof(ft_uint));
        return fr_work<ft_uint>::main(loop_file_extents, free_space_extents, io);
    }
    ff_log(FC_INFO, 0, "using full (%"FT_ULL" bit) remapping algorithm", (ft_ull)8*sizeof(ft_uoff));
    return fr_work<ft_uoff>::main(loop_file_extents, free_space_extents, io);
}

FT_NAMESPACE_END
