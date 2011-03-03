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
     * store block_size into this container, and check that container is able
     * to store physical, logical and length values up to block_count.
     *
     * if container stores them in type T, it must check that block_count does not overflow T,
     * and immediately return EFBIG if it overflows.
     *
     * return 0 if success, else error
     */
    virtual int extent_set_range(ft_uoff block_size, ft_uoff block_count) = 0;

    /**
     * append a new extent to this container.
     * container must check if new extent touches the last extent already in the container
     * but is assured (and can assume) than new extent will NEVER touch or intersect other extents.
     *
     * container is also assured (and can assume) that physical, logical and length will never exceed
     * the device blocks count already passed to extent_set_range()
     *
     * container is required to merge the new extent with the last one it contains, if they touch.
     *
     */
    virtual void extent_append(ft_uoff physical, ft_uoff logical, ft_uoff length) = 0;

};

FT_NAMESPACE_END

#endif /* FSTRANSLATE_EXTENT_LIST_HH */
