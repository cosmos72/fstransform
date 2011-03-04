/*
 * fwd.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_FWD_HH
#define FSTRANSFORM_FWD_HH

#include "check.hh"

FT_NAMESPACE_BEGIN

class ft_transform;
class ft_work_dispatch;

template<typename T> struct ft_extent_key;
template<typename T> struct ft_extent_payload;
template<typename T> class  ft_extent;
template<typename T> class  ft_vector;
template<typename T> class  ft_map;
template<typename T> class  ft_work;

FT_NAMESPACE_END
FT_IO_NAMESPACE_BEGIN

class ft_io;

FT_IO_NAMESPACE_END

#endif /* FSTRANSFORM_FWD_HH */
