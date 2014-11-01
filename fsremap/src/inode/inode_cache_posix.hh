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

#ifndef FSTRANSFORM_INODE_CACHE_POSIX_HH
#define FSTRANSFORM_INODE_CACHE_POSIX_HH

#include "inode_cache.hh"       // for ft_inode_cache
#include "../copy.hh"           // for ff_copy()

#include <map>          // for std::map

FT_NAMESPACE_BEGIN

class ft_inode_cache_posix
{
protected:
	ft_string path;

	enum FT_ICP_OPTIONS { FT_ICP_READONLY, FT_ICP_READWRITE };

	static bool readlink(const ft_string & src, ft_string & dst);
	int build_path(const ft_string & rel, ft_string & abs, FT_ICP_OPTIONS options) const;

public:
    /** one-arg constructor */
	explicit ft_inode_cache_posix(const ft_string & init_path);

    /** copy constructor */
	ft_inode_cache_posix(const ft_inode_cache_posix & other);

    /** assignment operator */
    const ft_inode_cache_posix & operator=(const ft_inode_cache_posix & other);

    /** destructor */
    virtual ~ft_inode_cache_posix();

    /* guaranteed NOT to end with '/', unless it's exactly the path "/" */
    const char * get_path() const { return path.c_str(); }

    /* initialize the inode cache. return 0 on success, else return error */
    virtual int init();

    /**
     * return true and set payload of cached inode if found, else add it to cache and return false
     * if false is returned, erase() must be called on the same inode when done with payload!
     */
    bool find_or_add(const ft_string & inode, ft_string & payload);

    /** return true and set payload of cached inode if found, else return false */
    bool find_and_delete(const ft_string & inode, ft_string & result_payload);

    void clear();
};




template<class V>
class ft_inode_cache_posix_v : public ft_inode_cache<V>, public ft_inode_cache_posix
{
private:
	typedef ft_inode_cache<V> super_type;
	typedef ft_inode_cache_posix mixin_type;


public:
    /** default constructor */
	ft_inode_cache_posix_v(const ft_string & init_path, const V & init_zero_payload = V())
		: super_type(init_zero_payload), mixin_type(init_path)
	{ }
    
    /** copy constructor */
	ft_inode_cache_posix_v(const ft_inode_cache_posix_v & other) : super_type(other), mixin_type(other)
	{ }
    
    /** assignment operator */
    virtual const super_type & operator=(const ft_inode_cache_posix_v & other)
    {
    	mixin_type::operator=(other);
    	return super_type::operator=(other);
    }

    /** destructor */
    virtual ~ft_inode_cache_posix_v()
    { }
    
    /* initialize the inode cache. return 0 on success, else return error */
    virtual int init() { return mixin_type::init(); }

    /**
     * return true and set payload of cached inode if found, else add it to cache and return false
     * if false is returned, erase() must be called on the same inode when done with payload!
     */
    virtual bool find_or_add(ft_inode inode, V & payload)
    {
    	ft_string s_inode, s_payload;
    	ff_copy(payload, s_payload);
    	ff_copy(inode, s_inode);
    	bool ret = mixin_type::find_or_add(s_inode, s_payload);
    	ff_copy(s_payload, payload);
    	return ret;
    }

    /** return true and set payload of cached inode if found, else return false */
    virtual bool find_and_delete(ft_inode inode, V & result_payload)
    {
    	ft_string s_inode, s_payload;
    	ff_copy(inode, s_inode);
    	bool ret = mixin_type::find_and_delete(s_inode, s_payload);
    	ff_copy(s_payload, result_payload);
    	return ret;
    }

    virtual void clear()
    {
    	mixin_type::clear();
    }
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_INODE_CACHE_POSIX_HH */
