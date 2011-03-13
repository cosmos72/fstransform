/*
 * dispatch.cc
 *
 *  Created on: Mar 3, 2011
 *      Author: max
 */

#include "first.hh"

#include "dispatch.hh"    // for ft_dispatch
#include "work.hh"        // for ft_work<T>

FT_NAMESPACE_BEGIN


/**
 * instantiate and run ft_work<T>
 * with the smallest T that can represent device blocks count.
 * return 0 if success, else error.
 *
 * implementation: iterates on all known configured T and,
 * if both ft_work<T>::check() and ft_work<T>::init() succeed,
 * calls ft_work<T>::run(), then ft_work<T>::quit()
 */
int ft_dispatch::main(ft_vector<ft_uoff> & loop_file_extents,
                           ft_vector<ft_uoff> & free_space_extents, FT_IO_NS ft_io & io)
{
    if (ft_work<ft_uint>::check(io) == 0) {
    	ff_log(FC_DEBUG, 0, "using reduced (%"FS_ULL" bit) relocation algorithm: lower memory usage", (ft_ull)8*sizeof(ft_uint));
        return ft_work<ft_uint>::main(loop_file_extents, free_space_extents, io);
    }
	ff_log(FC_DEBUG, 0, "using full (%"FS_ULL" bit) relocation algorithm", (ft_ull)8*sizeof(ft_uoff));
    return ft_work<ft_uoff>::main(loop_file_extents, free_space_extents, io);
}

FT_NAMESPACE_END
