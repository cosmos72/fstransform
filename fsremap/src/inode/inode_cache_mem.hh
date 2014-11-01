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
 * inode_cache.hh
 *
 *  Created on: Aug 18, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_INODE_CACHE_MEM_HH
#define FSTRANSFORM_INODE_CACHE_MEM_HH

#include "inode_cache.hh"  // for ft_inode_cache

#include <map>          // for std::map

#include "../assert.hh"    // for ff_assert()

FT_NAMESPACE_BEGIN

template<class V>
class ft_inode_cache_mem : public ft_inode_cache<V>
{
private:
	typedef ft_inode_cache<V> super_type;

	typedef std::map<ft_inode, V> map_type;

	map_type map;

public:
    /** default constructor */
	ft_inode_cache_mem(const V & init_zero_payload = V()) : super_type(init_zero_payload), map()
    { }
    
    /** copy constructor */
	ft_inode_cache_mem(const ft_inode_cache_mem & other) : super_type(other), map(other.map)
    { }
    
    /** assignment operator */
    virtual const super_type & operator=(const ft_inode_cache_mem & other)
    {
        if (this != &other)
            map = other.map;
        return super_type::operator=(other);
    }
    
    /** destructor */
    virtual ~ft_inode_cache_mem()
    { }
    
    /**
     * return true and set payload of cached inode if found, else add it to cache and return false
     * if false is returned, erase() must be called on the same inode when done with payload!
     */
    virtual bool find_or_add(ft_inode inode, V & payload)
    {
        ff_assert(payload != this->zero_payload);

        V & value = map[inode];
        if (value == this->zero_payload) {
            value = payload;
            return false;
        }
        payload = value;
        return true;
    }

    /** return true and set payload of cached inode if found, else return false */
    virtual bool find(ft_inode inode, V & result_payload) const
    {
        typename map_type::const_iterator iter = map.find(inode);
        if (iter == map.end())
            return false;

        result_payload = iter->second;
        return true;
    }

    /**
     * must be called if and only if find_or_add(inode) returned false
     */
    virtual void erase(ft_inode inode)
    {
        ft_size erased = map.erase(inode);

        ff_assert(erased == 1);
    }

    virtual void clear()
    {
        map.clear();
    }
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_INODE_CACHE_MEM_HH */
