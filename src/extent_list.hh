/*
 * appendable.hh
 *
 *  Created on: Mar 2, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_EXTENT_LIST_HH
#define FSTRANSLATE_EXTENT_LIST_HH

#include "types.hh"    // for ft_uoff


FT_NAMESPACE_BEGIN

/**
 * interface implemented by ft_vector<T>
 *
 * allows ft_io subclasses to read and fill a ft_vector<T> without having to know T.
 *
 * Needed because ft_io subclasses will receive a ft_vector<T>
 * as parameter to virtual methods loop_file_extents(ft_appendable &) and device_extents(ft_appendable &)
 * and C++ virtual methods cannot be templatized (for a very good reason)
 */
class ft_extent_list
{
public:

    /**
     * append a new extent to this container.
     * container must check if new extent touches the last extent already in the container
     * but is assured (and can assume) than new extent will NEVER touch or intersect other extents.
     *
     * container is required to merge the new extent with the last one it contains, if they touch.
     */
    virtual void extent_append(ft_uoff physical, ft_uoff logical, ft_uoff length) = 0;

};

FT_NAMESPACE_END

#endif /* FSTRANSLATE_EXTENT_LIST_HH */
