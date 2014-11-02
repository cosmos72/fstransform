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

#ifndef FSTRANSFORM_CACHE_ADAPTOR_HH
#define FSTRANSFORM_CACHE_ADAPTOR_HH

#include "cache.hh"    // for ft_cache<K,V>
#include "../copy.hh"  // for ff_set()

FT_NAMESPACE_BEGIN

template<class Cache, class K, class V>
class ft_cache_adaptor : public ft_cache<K, V>, public Cache
{
private:
	typedef ft_cache<K,V> super_type;
	typedef ft_cache_adaptor<Cache,K,V> this_type;

	typedef Cache                        mixin_type;
	typedef typename Cache::key_type     mixin_key_type;
	typedef typename Cache::payload_type mixin_payload_type;

public:
    /** default constructor */
	ft_cache_adaptor(const V & init_zero_payload = V()) : super_type(init_zero_payload), mixin_type()
    { }
    
    /** copy constructor */
	ft_cache_adaptor(const this_type & other) : super_type(other), mixin_type(other)
    { }
    
    /** assignment operator */
    virtual const super_type & operator=(const this_type & other)
    {
        mixin_type::operator=(other);
        return super_type::operator=(other);
    }
    
    /** destructor */
    virtual ~ft_cache_adaptor()
    { }
    
    /**
     * if cached inode found, set payload and return 1.
     * Otherwise add it to cache and return 0.
     * On error, return < 0.
     * if returns 0, erase() must be called on the same inode when done with payload!
     */
    virtual int find_or_add(const K key, V & inout_payload)
    {
    	mixin_key_type m_key;
    	ff_set(m_key, key);

    	mixin_payload_type m_payload;
    	ff_set(m_payload, inout_payload);

        int err = mixin_type::find_or_add(m_key, m_payload);
        ff_set(inout_payload, m_payload);

        return err;
    }

    /**
     * if cached key found, set payload, remove cached key and return 1.
     * Otherwise return 0. On error, return < 0.
     */
    virtual int find_and_delete(const K key, V & result_payload)
    {
    	mixin_key_type m_key;
    	ff_set(m_key, key);

    	mixin_payload_type m_payload;
        int err = mixin_type::find_and_delete(m_key, m_payload);
        ff_set(result_payload, m_payload);

        return err;
    }

    virtual void clear()
    {
        mixin_type::clear();
    }
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_CACHE_ADAPTOR_HH */
