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
 * rope.cc
 *
 *  Created on: Mar 3, 2018
 *      Author: max
 */

#include "../first.hh"

#include <stdexcept>     // for std::invalid_argument

#include "rope.hh"       // for ft_rope
#include "rope_impl.hh"  // for ft_rope_impl

FT_NAMESPACE_BEGIN

/** copy constructor. */
ft_rope::ft_rope(const ft_rope & other) : p(NULL)
{
	other.validate();
	if (other.p != NULL) {
		other.p->ref();
		p = other.p;
	}
}

/** constructor from ft_string */
ft_rope::ft_rope(const ft_string & other) : p(NULL)
{
	if (other.size() != 0)
		p = ft_rope_impl::make(NULL, other.c_str(), other.size());
}

/** constructor with prefix and suffix. makes a private copy of 'suffix' */
ft_rope::ft_rope(const ft_rope * prefix, const char * suffix, ft_size suffix_length) : p(NULL)
{
	if (prefix)
		prefix->validate();
	ft_rope_impl * prefix_p = prefix ? prefix->p : NULL;
	if (prefix_p != NULL || suffix_length != 0)
		p = ft_rope_impl::make(prefix_p, suffix, suffix_length);
}

/** assignment operator */
const ft_rope & ft_rope::operator=(const ft_rope & other)
{
	validate();
	other.validate();
	if (this->p != other.p) {
		if (p != NULL) {
			p->unref();
			p = NULL;
		}
		if ((other.p != NULL)) {
			other.p->ref();
			p = other.p;
		}
	}
	return *this;
}

/* destructor */
ft_rope::~ft_rope()
{
	validate();
	if (p != NULL) {
		p->unref();
		p = NULL;
	}
}

bool ft_rope::empty() const
{
	return p == NULL || p->empty();
}

/* validate ft_rope_impl pointer */
void ft_rope::validate() const
{
	if (p == NULL)
		return;
	ft_size n = (ft_size)p;
	if ((n & 7) || n <= 0xffff)
		throw std::invalid_argument("invalid ft_rope_impl pointer");
}



/** convert to ft_string */
void ft_rope::to_string(ft_string & append_dst) const
{
	validate();
	if (p != NULL)
		p->to_string(append_dst);
}

/** compare against char[] */
bool ft_rope::equals(const char other[], ft_size other_len) const
{
	validate();
	if (p == NULL)
		return other_len == 0;
	return p->equals(other, other_len);
}

/** return this hash */
ft_size ft_rope::hash() const
{
	validate();
	return p ? p->hash() : ft_rope_impl::hash(NULL, 0);
}

/** return other's hash */
ft_size ft_rope::hash(const char other[], ft_size other_len)
{
	return ft_rope_impl::hash(other, other_len);
}


FT_NAMESPACE_END
