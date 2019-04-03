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
 * pool.hh
 *
 *  Created on: Mar 8, 2011
 *      Author: max
 */

#include "first.hh"

#include "assert.hh"      // for ff_assert macro
#include "pool.hh"        // for fr_pool<T>

FT_NAMESPACE_BEGIN


template<typename T>
fr_pool<T>::fr_pool(fr_map<T> & map) : backing_map(map)
{
    init();
}


/** initialize this pool to reflect contents of backing fr_map<T> */
template<typename T>
void fr_pool<T>::init()
{
    map_iterator begin = backing_map.begin(), iter = backing_map.end();
    /*
     * iterate backward to have lower physicals in the last std::vector<T> positions,
     * so that they will be used first
     */
    while (begin != iter)
        insert0(--iter);
}

/** insert into this pool an extent _ALREADY_ present in backing map */
template<typename T>
void fr_pool<T>::insert0(map_iterator map_iter)
{
    (*this)[map_iter->second.length].push_back(map_iter);
}


/**
 * "allocate" from the single extent 'iter' in this pool and shrink it
 * to store the single extent 'map_iter'.
 * remove allocated (and renumbered) extent from map and write it into map_allocated
 */
template<typename T>
void fr_pool<T>::allocate_unfragmented(map_iterator map_iter, fr_map<T> & map, fr_map<T> & map_allocated, iterator iter)
{
    map_value_type & map_value = * map_iter;
    T physical = map_value.first.physical;
    T length = map_value.second.length;
    ft_size user_data = map_value.second.user_data;

    /* check that 'iter' extent is big enough to fit map_iter */
    fr_pool_entry<T> & pool_entry = iter->second;
    map_iterator pool_iter = pool_entry.back();
    map_mapped_type & pool_value = pool_iter->second;
    T pool_value_logical = pool_value.logical;
    T pool_value_length = iter->first;
    ff_assert(pool_value_length == pool_value.length);
    ff_assert(pool_value_length >= length);

    /* update maps to reflect allocation */
    map_allocated.insert(physical, pool_value_logical, length, user_data);
    map.remove(map_iter);

    /* remove extent from pool_entry */
    pool_entry.pop_back();
    /* if pool_entry is empty, remove it from this pool */
    if (pool_entry.empty())
        super_type::erase(iter);

    /* shrink 'iter' extent inside backing map */
    pool_iter = backing_map.remove_front(pool_iter, length);
    if (pool_iter != backing_map.end())
        /* we have a remainder: reinsert it into this pool */
        insert0(pool_iter);
}

/**
 * "allocate" a single fragment from this pool to store the single extent 'map_iter'.
 * shrink extent from map (leaving unallocated portion) and write the allocated portion into map_allocated.
 *
 * return iterator to remainder of extent that still needs to be allocated
 */
template<typename T>
typename fr_pool<T>::map_iterator fr_pool<T>::allocate_fragment(map_iterator map_iter, fr_map<T> & map, fr_map<T> & map_allocated)
{
    map_value_type & map_value = * map_iter;
    T physical = map_value.first.physical;
    T length = map_value.second.length;
    ft_size user_data = map_value.second.user_data;

    ff_assert(!this->empty());
    iterator iter = this->end(); // use the largest extent we have
    --iter;
    T pool_value_length = iter->first;
    fr_pool_entry<T> & pool_entry = iter->second;
    map_iterator pool_iter = pool_entry.back();
    map_mapped_type & pool_value = pool_iter->second;
    T pool_value_logical = pool_value.logical;
    ff_assert(pool_value_length == pool_value.length);
    ff_assert(pool_value_length < length);

    /* update maps to reflect partial allocation */
    map_allocated.insert(physical, pool_value_logical, pool_value_length, user_data);
    map_iter = map.remove_front(map_iter, pool_value_length);

    /* remove extent from pool_entry */
    pool_entry.pop_back();
    /* if pool_entry is empty, remove it from this pool */
    if (pool_entry.empty())
        super_type::erase(iter);

    /* remove 'iter' extent from backing map */
    backing_map.remove(pool_iter);

    /* return iterator to remainder of extent that still needs to be allocated */
    return map_iter;
}



/*
 * "allocate" (and remove) extents from this pool to store map extents using a best-fit strategy.
 * remove allocated (and renumbered) extents from map and write them into map_allocated,
 * fragmenting them if needed
 */
template<typename T>
void fr_pool<T>::allocate_all(fr_map<T> & map, fr_map<T> & map_allocated)
{
    map_iterator map_iter = map.begin(), map_iter_tmp, map_end = map.end();
    while (map_iter != map_end && !this->empty()) {
        map_iter_tmp = map_iter;
        ++map_iter;
        allocate(map_iter_tmp, map, map_allocated);
    }
}

/**
 * "allocate" using a best-fit strategy (and remove) extents from this pool
 * to store the single extent 'map_iter', which must belong to 'map'.
 * remove allocated (and renumbered) extent from map and write it into map_allocated,
 * fragmenting it if needed.
 */
template<typename T>
void fr_pool<T>::allocate(map_iterator map_iter, fr_map<T> & map, fr_map<T> & map_allocated)
{
    iterator iter, end = this->end();
    T length;

    while ((length = map_iter->second.length) != 0 && !this->empty()) {
        if ((iter = this->lower_bound(length)) != end) {
            /* found a pool entry big enough to fit extent remainder */
            allocate_unfragmented(map_iter, map, map_allocated, iter);
            return;
        }
        /* no pool entry is big enough: we need to fragment the extent */
        map_iter = allocate_fragment(map_iter, map, map_allocated);
    }
}

FT_NAMESPACE_END
