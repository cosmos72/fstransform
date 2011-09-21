/*
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
                    fr_vector<ft_uoff> & free_space_extents, FT_IO_NS fr_io & io);

};

FT_NAMESPACE_END

#endif /* FSREMAP_WORK_BASE_HH */
