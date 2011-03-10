/*
 * dispatch.hh
 *
 *  Created on: Mar 3, 2011
 *      Author: max
 */
#ifndef FSTRANSFORM_WORK_BASE_HH
#define FSTRANSFORM_WORK_BASE_HH

#include "types.hh"    // for ft_uoff
#include "fwd.hh"      // for ft_io, ft_vector<T> forward declarations


FT_NAMESPACE_BEGIN


class ft_dispatch
{
public:
    /**
     * instantiate and run ft_work<T>::main(...)
     * with the smallest T that can represent device blocks count.
     * return 0 if success, else error.
     *
     * implementation: iterates on all configured T and,
     * if both ft_work<T>::check(..) and ft_work<T>::init(..) succeed,
     * calls ff_work<T>::run(), then ff_work<T>::quit()
     */
    static int main(ft_vector<ft_uoff> & loop_file_extents,
                    ft_vector<ft_uoff> & free_space_extents, FT_IO_NS ft_io & io);

};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_WORK_BASE_HH */
