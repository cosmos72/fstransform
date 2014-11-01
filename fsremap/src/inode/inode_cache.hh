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

#ifndef FSTRANSFORM_INODE_CACHE_HH
#define FSTRANSFORM_INODE_CACHE_HH

#include "../types.hh"     // for ft_inode

FT_NAMESPACE_BEGIN

template<class V>
class ft_inode_cache
{
protected:
	V zero_payload;

public:
    /** default constructor */
    ft_inode_cache(const V & init_zero_payload = V()) : zero_payload(init_zero_payload)
    { }

    /** copy constructor */
    ft_inode_cache(const ft_inode_cache<V> & other) : zero_payload(other.zero_payload)
    { }

    /** assignment operator */
    virtual const ft_inode_cache<V> & operator=(const ft_inode_cache<V> & other)
    {
    	if (this != &other)
    		zero_payload = other.zero_payload;
    	return *this;
    }

    /** destructor */
    virtual ~ft_inode_cache()
    { }

    /* initialize the inode-cache. return 0 on success, else return error */
    virtual int init() = 0;

    /**
     * if cached inode found, set payload and return 1.
     * Otherwise add it to cache and return 0.
     * On error, return < 0.
     * if returns 0, erase() must be called on the same inode when done with payload!
     */
    virtual int find_or_add(ft_inode inode, V & inout_payload) = 0;

    /**
     * if cached inode found, set payload, remove inode from cache and return 1.
     * Otherwise return 0. On error, return < 0.
     */
    virtual int find_and_delete(ft_inode inode, V & result_payload) = 0;

    virtual void clear() = 0;
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_INODE_CACHE_HH */
