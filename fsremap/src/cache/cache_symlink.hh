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
 * key_cache.hh
 *
 *  Created on: Aug 18, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_CACHE_SYMLINK_HH
#define FSTRANSFORM_CACHE_SYMLINK_HH

#include "cache_adaptor.hh"  // for ft_cache_adaptor<K,V,Cache>

#include <map>          // for std::map

FT_NAMESPACE_BEGIN

/**
 * symlink-based associative array from keys (strings) to values (strings).
 * Used to implement inode cache - see cache.hh for details.
 */
class ft_cache_symlink
{
protected:
	ft_string path;

	enum FT_ICP_OPTIONS { FT_ICP_READONLY, FT_ICP_READWRITE };

	static int readlink(const ft_string & src, ft_string & dst);
	int build_path(const ft_string & rel, ft_string & abs, FT_ICP_OPTIONS options) const;

public:
	typedef ft_string key_type;
	typedef ft_string payload_type;


    /** one-arg constructor */
	explicit ft_cache_symlink();

    /** copy constructor */
	ft_cache_symlink(const ft_cache_symlink & other);

    /** assignment operator */
    const ft_cache_symlink & operator=(const ft_cache_symlink & other);

    /** destructor */
    virtual ~ft_cache_symlink();

    /* guaranteed NOT to end with '/', unless it's exactly the path "/" */
    const char * get_path() const { return path.c_str(); }

    /* initialize the cache. return 0 on success, else return error */
    int init(const ft_string & init_path);

    /**
     * if cached key found, set payload and return 1.
     * Otherwise add it to cache and return 0.
     * On error, return < 0.
     * if returns 0, erase() must be called on the same key when done with payload!
     */
    int find_or_add(const ft_string & key, ft_string & payload);

    /**
     * if cached key found, set result_payload, remove cached key and return 1.
     * Otherwise return 0. On error, return < 0.
     */
    int find_and_delete(const ft_string & key, ft_string & result_payload);

    /**
     * if cached inode found, change its payload and return 1.
     * Otherwise return 0. On error, return < 0.
     */
    int find_and_update(const ft_string key, const ft_string & new_payload);

    void clear();
};



/**
 * symlink-based associative array from keys (type K) to values (type V).
 * Used to implement inode cache - see cache.hh for details.
 */
template<class K, class V>
class ft_cache_symlink_kv : public ft_cache_adaptor<ft_cache_symlink,K,V>
{
private:
	typedef ft_cache_symlink mixin_type;
	typedef ft_cache_adaptor<mixin_type,K,V> super_type;


public:
    /** default constructor */
	ft_cache_symlink_kv(const V & init_zero_payload = V())
		: super_type(init_zero_payload)
	{ }
    
    /** destructor */
    virtual ~ft_cache_symlink_kv()
    { }

    /* initialize the cache. return 0 on success, else return error */
    int init(const V & path) {
    	ft_string s_path;
    	ff_set(s_path, path);
    	return mixin_type::init(s_path);
    }
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_CACHE_SYMLINK_HH */
