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
 * rope_impl.hh
 *
 *  Created on: Mar 3, 2018
 *      Author: max
 */

#ifndef FSTRANSFORM_ROPE_IMPL_HH
#define FSTRANSFORM_ROPE_IMPL_HH

#include "../types.hh"   // for ft_string, ft_size

FT_NAMESPACE_BEGIN

/**
 * short immutable string, with a prefix shared with other rope:s
 */
class ft_rope_impl
{
private:
	enum { PREFIX_SCALE = sizeof(void *) };

	ft_i32 prefix_delta;
	ft_u32 refcount, suffix_len;

	/** default constructor. private use only, call make() instead */
	FT_INLINE ft_rope_impl() : prefix_delta(0), refcount(0), suffix_len(0)
	{ }

	/** copy constructor. forbidden. call make() instead */
	ft_rope_impl(const ft_rope_impl & other);

	/** assignment operator. forbidden */
	const ft_rope_impl & operator=(const ft_rope_impl & other);

	/**
	  * constructor. private use only. call make() instead.
	  * makes a copy of 'suffix'
	  */
	FT_INLINE ft_rope_impl(ft_rope_impl * prefix_ptr, ft_size suffix_length);

	/** destructor. private use only. call unref() instead */
	~ft_rope_impl();

	FT_INLINE char * suffix() {
		return reinterpret_cast<char *>(this) + sizeof(ft_rope_impl);
	}

	FT_INLINE const char * suffix() const {
		return reinterpret_cast<const char *>(this) + sizeof(ft_rope_impl);
	}

	FT_INLINE ft_rope_impl * prefix() const {
		if (!prefix_delta)
	            return NULL;
	        ft_size self = (ft_size)(this);
		return reinterpret_cast<ft_rope_impl *>(self + (ft_size)prefix_delta * PREFIX_SCALE);
	}

	FT_INLINE ft_size prefix_len() const {
		ft_rope_impl * ptr = prefix();
		return ptr ? ptr->len() : 0;
	}

public:
	/** create an empty rope */
	static ft_rope_impl * make();

	/** create a rope with given prefix and suffix. makes a private copy of 'suffix' */
	static ft_rope_impl * make(ft_rope_impl * prefix, const char * suffix, ft_size suffix_length);

	/** increment reference count */
	void ref();

	/** decrement reference count */
	void unref();

	bool empty() const;

	ft_size len() const;

	/** convert to ft_string */
	void to_string(ft_string & append_dst) const;

	/** compare against char[] */
	bool equals(const char other[], ft_size other_len) const;

	/** return this hash */
	ft_size hash() const;

	/** return other's hash */
	static ft_size hash(const char other[], ft_size other_len);
};
FT_NAMESPACE_END

#endif /* FSTRANSFORM_ROPE_IMPL_HH */
