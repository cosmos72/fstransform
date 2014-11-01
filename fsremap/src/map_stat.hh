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
 * map_stat.hh
 *
 *  Created on: Mar 12, 2011
 *      Author: max
 */

#ifndef FSREMAP_MAP_STAT_HH
#define FSREMAP_MAP_STAT_HH

#include "map.hh"    // for fr_map<T>


FT_NAMESPACE_BEGIN

template<typename T>
class fr_map_stat : public fr_map<T>
{
private:
    typedef fr_map<T> super_type;
    typedef typename fr_map<T>::iterator iterator;

    T this_total_count; /**< total length (number of blocks) in this map extents */
    T this_used_count;  /**< used  length (number of blocks) in this map extents */

public:
    // construct empty fr_map_stat
    fr_map_stat();

    // destructor
    ~fr_map_stat();

    // duplicate a fr_map_stat, i.e. initialize this fr_map as a copy of other.
    fr_map_stat(const fr_map<T> & other);
    // duplicate a fr_map_stat, i.e. initialize this fr_map as a copy of other.
    fr_map_stat(const fr_map_stat<T> & other);

    // copy fr_map_stat, i.e. set fr_map_stat fr_map contents as a copy of other's contents.
    const fr_map_stat<T> & operator=(const fr_map<T> & other);
    // copy fr_map_stat, i.e. set fr_map_stat fr_map contents as a copy of other's contents.
    const fr_map_stat<T> & operator=(const fr_map_stat<T> & other);

    /** clear this fr_map_stat. also sets total_count, used_count and free_count to zero */
    void clear();

    /** same as super_type::insert(T,T,T,ft_size), but also updates used_count() */
    FT_INLINE iterator stat_insert(T physical, T logical, T length, ft_size user_data)
    {
        this_used_count += length;
        return super_type::insert(physical, logical, length, user_data);
    }

    /** same as super_type::remove(iterator), but also updates used_count() */
    FT_INLINE void stat_remove(iterator iter)
    {
        super_type::remove(iter);
        this_used_count -= iter->second.length;
    }

    /** same as super_type::remove(T, T, T), but also updates used_count() */
    FT_INLINE void stat_remove(T physical, T logical, T length)
    {
        super_type::remove(physical, logical, length);
        this_used_count -= length;
    }

    /**
     * remove an initial part of an existing extent from this fr_map.
     * returns iterator to new, smaller extent, or end() if the whole extent was removed
     */
    FT_INLINE void stat_remove_front(iterator iter, T shrink_length)
    {
        super_type::remove_front(iter, shrink_length);
        this_used_count -= shrink_length;
    }


    FT_INLINE T total_count() const { return this_total_count; }
    FT_INLINE T used_count() const { return this_used_count; }
    FT_INLINE T free_count() const { return this_total_count - this_used_count; }

    FT_INLINE void total_count(T n) { this_total_count = n; }

    FT_INLINE void used_count(T n) { this_used_count = n; }
    FT_INLINE void used_count_add(T n) { this_used_count += n; }
    FT_INLINE void used_count_sub(T n) { this_used_count -= n; }
};

FT_NAMESPACE_END


#ifdef FT_HAVE_EXTERN_TEMPLATE
#  define FT_TEMPLATE_map_stat_hh(ft_prefix, T) ft_prefix class FT_NS fr_map_stat< T >;
   FT_TEMPLATE_DECLARE(FT_TEMPLATE_map_stat_hh)
#else
#  include "map_stat.t.hh"
#endif /* FT_HAVE_EXTERN_TEMPLATE */

#endif /* FSREMAP_MAP_STAT_HH */
