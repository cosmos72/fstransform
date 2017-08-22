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
 * dispatch.hh
 *
 *  Created on: Mar 3, 2011
 *      Author: max
 */
#ifndef FSREMAP_WORK_BASE_HH
#define FSREMAP_WORK_BASE_HH

#include "types.hh"    // for ft_uoff
#include "fwd.hh"      // for fr_io, fr_vector<T> forward declarations


FT_NAMESPACE_BEGIN


class fr_dispatch
{
public:
    /**
     * instantiate and run fr_work<T>::main(...)
     * with the smallest T that can represent device blocks count.
     * return 0 if success, else error.
     *
     * implementation: iterates on all configured T and,
     * if both fr_work<T>::check(..) and fr_work<T>::init(..) succeed,
     * calls ff_work<T>::run(), then ff_work<T>::cleanup()
     */
    static int main(fr_vector<ft_uoff> & loop_file_extents,
                    fr_vector<ft_uoff> & free_space_extents,
                    fr_vector<ft_uoff> & to_zero_extents,
                    FT_IO_NS fr_io & io);

};

FT_NAMESPACE_END

#endif /* FSREMAP_WORK_BASE_HH */
