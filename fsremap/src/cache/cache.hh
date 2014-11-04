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

#ifndef FSTRANSFORM_CACHE_HH
#define FSTRANSFORM_CACHE_HH

FT_NAMESPACE_BEGIN

/*
 * Base class for generic key->value caches.
 *
 * Used to implement inode cache, needed to detect multiple files
 * hard-linked together (i.e. having the same inode) and let fsmove
 * create an accurate replica of them.
 */
template<class K, class V>
class ft_cache
{
protected:
	V zero_payload;

public:
    /** default constructor */
    ft_cache(const V & init_zero_payload = V()) : zero_payload(init_zero_payload)
    { }

    /** copy constructor */
    ft_cache(const ft_cache<K,V> & other) : zero_payload(other.zero_payload)
    { }

    /** assignment operator */
    virtual const ft_cache<K,V> & operator=(const ft_cache<K,V> & other)
    {
    	if (this != &other)
    		zero_payload = other.zero_payload;
    	return *this;
    }

    /** destructor */
    virtual ~ft_cache()
    { }

    /**
     * if cached inode found, set payload and return 1.
     * Otherwise add it to cache and return 0.
     * On error, return < 0.
     * if returns 0, erase() must be called on the same inode when done with payload!
     */
    virtual int find_or_add(const K key, V & inout_payload) = 0;

    /**
     * if cached inode found, set payload, remove inode from cache and return 1.
     * Otherwise return 0. On error, return < 0.
     */
    virtual int find_and_delete(const K key, V & result_payload) = 0;

    virtual void clear() = 0;
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_CACHE_HH */
