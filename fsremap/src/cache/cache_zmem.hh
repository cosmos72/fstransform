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
 *  Created on: Feb 5, 2017
 *      Author: max
 */

#ifndef FSTRANSFORM_CACHE_ZMEM_HH
#define FSTRANSFORM_CACHE_ZMEM_HH

#include "cache.hh"          // for ft_cache<K,V>
#include "../log.hh"         // for ff_log()
#include "../types.hh"       // for ft_inode
#include "../zmem/ztree.hh"  // for ztree<T>

FT_NAMESPACE_BEGIN

/**
 * compressed in-memory associative array from keys (ft_ull) to values (V).
 * Used to implement inode cache - see cache.hh for details.
 */

class ft_cache_zmem_is : public ft_cache<ft_inode, ft_string>
{
private:
    ztree<char *> tree;
   
    /** copy constructor - NOT IMPLEMENTED */
    ft_cache_zmem_is(const ft_cache_zmem_is & other);
    
    /** assignment operator - NOT IMPLEMENTED */
    const ft_cache_zmem_is & operator=(const ft_cache_zmem_is & other);
    
public:
    /** default constructor */
    ft_cache_zmem_is();

    /** destructor */
    ~ft_cache_zmem_is();
   
    virtual int find_or_add(const ft_inode key, ft_string & inout_payload);

    
    /**
     * if cached key found, set result_payload, remove cached key and return 1.
     * Otherwise return 0. On error, return < 0.
     */
    virtual int find_and_delete(const ft_inode key, ft_string & result_payload);
    
    /**
     * if cached inode found, change its payload and return 1.
     * Otherwise return 0. On error, return < 0.
     */
    virtual int find_and_update(const ft_inode key, const ft_string & new_payload);
    
    virtual void clear();
};



template<class K, class V>
  class ft_cache_zmem;


template<>
  class ft_cache_zmem<ft_inode, ft_string> : public ft_cache_zmem_is
  { };



FT_NAMESPACE_END

#endif /* FSTRANSFORM_CACHE_ZMEM_HH */
