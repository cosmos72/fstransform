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
 * cache_zmem.hh
 *
 *  Created on: Jan 30, 2017
 *      Author: max
 */

#ifndef FSTRANSFORM_CACHE_ZMEM_HH
#define FSTRANSFORM_CACHE_ZMEM_HH

#include "cache.hh"      // for ft_cache<K,V>
#include "copy.hh"       // for ff_set()


FT_NAMESPACE_BEGIN

/**
 * compressed in-memory associative array from keys *ft_ull to values (type V).
 * Used to implement inode cache - see cache.hh for details.
 */
template<class K>
class ft_cache_zmem_ks
{
private:
    typedef ft_string V;

    V zero_payload;
    
    zptr_void page[sizeof(K)];
    
public:
    /** default constructor */
    ft_cache_zmem_ks()
    {
        init_pages();
    }
    
    /** copy constructor */
    ft_cache_zmem(const ft_cache_zmem_ks<K> & other)
    {
        init_pages();
        deep_copy_pages(page, other.page);
    }
    
    /** assignment operator */
    const ft_cache_zmem & operator=(const ft_cache_zmem<K,V> & other)
    {
        if (this != &other)
            deep_copy_pages(page, other.page);
        return *this;
    }
    
    /** destructor */
    ~ft_cache_zmem()
    {
        free_pages();
    }
    
    /**
     * if cached inode found, set payload and return 1.
     * Otherwise add it to cache and return 0.
     * On error, return < 0.
     * if returns 0, erase() must be called on the same inode when done with payload!
     */
    int find_or_add(const K key, V & inout_payload)
    {
        ff_assert(inout_payload != this->zero_payload);

        V & value = map[key];
        if (value == this->zero_payload) {
            value = inout_payload;
            return 0;
        }
        inout_payload = value;
        return 1;
    }

    /**
     * if cached key found, set result_payload, remove cached key and return 1.
     * Otherwise return 0. On error, return < 0.
     */
    virtual int find_and_delete(const K key, V & result_payload)
    {
        typename map_type::iterator iter = map.find(key);
        if (iter == map.end())
            return 0;

        result_payload = iter->second;
        map.erase(iter);
        return 1;
    }

    /**
     * if cached inode found, change its payload and return 1.
     * Otherwise return 0. On error, return < 0.
     */
    virtual int find_and_update(const K key, const V & new_payload)
    {
        typename map_type::iterator iter = map.find(key);
        if (iter == map.end())
            return 0;
        
        iter->second = new_payload;
        return 1;
    }
    
    virtual void clear()
    {
        map.clear();
    }
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_CACHE_MEM_HH */
