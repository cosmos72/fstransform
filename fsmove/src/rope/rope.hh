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
 * rope.hh
 *
 *  Created on: Mar 3, 2018
 *      Author: max
 */

#ifndef FSTRANSFORM_ROPE_HH
#define FSTRANSFORM_ROPE_HH

#include "../types.hh"   // for ft_string, ft_size

FT_NAMESPACE_BEGIN

class ft_rope_impl;

// -----------------------------------------------------------------------------
/**
 * short immutable string, with a prefix shared with other rope:s
 */
class ft_rope
{
private:
	ft_rope_impl * p;

public:
	/** default constructor. */
	FT_INLINE ft_rope() : p(NULL)
	{ }

	/** copy constructor. */
	ft_rope(const ft_rope & other);

	/** constructor from ft_string */
	explicit ft_rope(const ft_string & other);

	/** constructor with prefix and suffix. makes a private copy of 'suffix' */
	ft_rope(const ft_rope * prefix, const char * suffix, ft_size suffix_length);

	/** assignment operator */
	const ft_rope & operator=(const ft_rope & other);

	/** destructor */
	~ft_rope();

	/**
	 * identity comparison: returns true ONLY if pointing to the same ft_rope_impl.
	 * enough for the comparison against empty ft_rope() made by ft_cache_mem<K,V>.
	 * does NOT compare the char[] array!
	 */
	FT_INLINE bool operator==(const ft_rope & other) const {
		return p == other.p;
	}
	FT_INLINE bool operator!=(const ft_rope & other) const {
		return p != other.p;
	}

	bool empty() const;

	/* validate ft_rope_impl pointer */
	void validate() const;

	/** convert to ft_string */
	void to_string(ft_string & append_dst) const;

	/** compare against char[] */
	bool equals(const char other[], ft_size other_len) const;

	/** compare against ft_string */
	FT_INLINE bool equals(const ft_string & other) const {
		return equals(other.c_str(), other.size());
	}

	/** return this hash */
	ft_size hash() const;

	/** return other's hash */
	static ft_size hash(const char other[], ft_size other_len);

	/** return other's hash */
	static FT_INLINE ft_size hash(const ft_string & other) {
		return hash(other.c_str(), other.size());
	}
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_ROPE_HH */
