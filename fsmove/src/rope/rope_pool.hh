/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 2 of the License, or
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
 * rope_pool.hh
 *
 *  Created on: Mar 3, 2018
 *      Author: max
 */

#ifndef FSTRANSFORM_ROPE_POOL_HH
#define FSTRANSFORM_ROPE_POOL_HH

#include <list>
#include <vector>

#include "../types.hh"  // for ft_string
#include "rope.hh"      // for ft_rope
#include "rope_list.hh" // for ft_rope_list

FT_NAMESPACE_BEGIN

// -----------------------------------------------------------------------------
/**
 * pool of ropes. useful to compress large sets of repetitive strings,
 * in particular for the strings in the inode cache ft_cache<ft_inode, ft_rope>
 */
class ft_rope_pool
{
private:
	typedef ft_rope_list ft_bucket;
	typedef std::vector<ft_bucket> ft_table;

	ft_size count;
	ft_table table;

	void rehash(ft_size new_len);

public:
	/** default constructor. */
	ft_rope_pool();

	/** copy constructor. */
	ft_rope_pool(const ft_rope_pool & other);

	/** assignment operator. */
	const ft_rope_pool & operator=(const ft_rope_pool & other);

	/* destructor. */
	~ft_rope_pool();

	/** returned pointer is valid only until pool is rehashed.
	  * copy returned ft_rope if you need it further */
	const ft_rope * find(const char s[], ft_size len) const;

	FT_INLINE const ft_rope * find(const ft_string & s) const {
		return find(s.c_str(), s.size());
	}

	ft_rope make(const char s[], ft_size len);

	FT_INLINE ft_rope make(const ft_string & s) {
		return make(s.c_str(), s.size());
	}
	void erase(const ft_string & s);
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_ROPE_POOL_HH */
