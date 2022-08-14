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
 * rope_impl.cc
 *
 *  Created on: Mar 3, 2018
 *      Author: max
 */

#include "../first.hh"

#include <new>           // for std::bad_alloc
#include <stdexcept>     // for std::length_error

#if defined(FT_HAVE_STDLIB_H)
# include <stdlib.h>     // for malloc(), free()
#elif defined(FT_HAVE_CSTDLIB)
# include <cstdlib>      // for malloc(), free()
#endif

#if defined(FT_HAVE_STRING_H)
# include <string.h>     // for memcmp(), memcpy()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>      // for memcmp(), memcpy()
#endif

#include "../log.hh"
#include "rope_impl.hh"

FT_NAMESPACE_BEGIN

static bool unaligned_malloc_warn_once = true;

// -----------------------------------------------------------------------------
/** create an empty rope */
ft_rope_impl * ft_rope_impl::make()
{
	return make(NULL, NULL, 0);
}

/** create a rope with given prefix and suffix. makes a private copy of 'suffix' */
ft_rope_impl * ft_rope_impl::make(ft_rope_impl * prefix, const char * suffix, ft_size suffix_length)
{
	char * bytes = reinterpret_cast<char *>(malloc(sizeof(ft_rope_impl) + suffix_length));
	if (bytes == NULL)
		throw std::bad_alloc();

	if (((ft_size)bytes % PREFIX_SCALE) != 0 && unaligned_malloc_warn_once) {
		unaligned_malloc_warn_once = false;
		ff_log(FC_WARN, 0, "malloc() returned unaligned pointer %p, ft_rope will not be memory-saving", bytes);
	}
	if (!prefix || ((ft_size)bytes % PREFIX_SCALE) == ((ft_size)prefix % PREFIX_SCALE)) {
		// check whether prefix_delta can point to prefix
		ft_i32 delta = prefix ? (ft_i32)(((ft_size)prefix - (ft_size)bytes) / PREFIX_SCALE) : 0;
		if (!prefix || (ft_size)bytes + (ft_size)delta * PREFIX_SCALE == (ft_size)prefix) {
			if (suffix_length != (ft_u32)suffix_length)
				throw std::length_error("string too long for ft_rope");
			ft_rope_impl * ths = reinterpret_cast<ft_rope_impl *>(bytes);
			new (ths) ft_rope_impl(); // placement new
			if ((ths->suffix_len = suffix_length) != 0)
				memcpy(ths->suffix(), suffix, suffix_length);
			ths->prefix_delta = delta;
		   	if (prefix != NULL)
				prefix->ref();
			return ths;
		}
	}
	// cannot point to prefix... copy it
	free(bytes);
	ft_size prefix_len = prefix->len();
	if (prefix_len + suffix_length != (ft_u32)(prefix_len + suffix_length))
		throw std::length_error("string too long for ft_rope");
	bytes = reinterpret_cast<char *>(malloc(sizeof(ft_rope_impl) + prefix_len + suffix_length));
	if (bytes == NULL)
		throw std::bad_alloc();
	ft_rope_impl * ths = reinterpret_cast<ft_rope_impl *>(bytes);
	new (ths) ft_rope_impl(); // placement new
	char * dst = ths->suffix() + prefix_len;
	if ((ths->suffix_len = suffix_length) != 0)
		memcpy(dst, suffix, suffix_length);
	do {
		if ((prefix_len = prefix->suffix_len) != 0)
			memcpy(dst -= prefix_len, prefix->suffix(), prefix_len);
		prefix = prefix->prefix();
	} while (prefix);
	return ths;
}

/** destructor. private use only. call unref() instead */
ft_rope_impl::~ft_rope_impl()
{
	if (prefix_delta != 0) {
		prefix()->unref();
		prefix_delta = 0;
	}
	suffix_len = 0;
}

/** increment reference count */
void ft_rope_impl::ref()
{
	switch (refcount) {
	case (ft_u32)-1:
		break;
	case (ft_u32)-2:
		ff_log(FC_DEBUG, 0, "ft_rope %p refcount reached 2^32-1", this);
		// FALLTHROUGH
	default:
		refcount++;
		break;
	}
}


/** decrement reference count */
void ft_rope_impl::unref()
{
	if (refcount == (ft_u32)-1 || refcount-- != 0)
		return;
	this->~ft_rope_impl();
	free(this);
}

bool ft_rope_impl::empty() const
{
	const ft_rope_impl * p = this;
	do {
		if (p->suffix_len != 0)
			return false;
	} while ((p = p->prefix()) != NULL);
	return true;
}

ft_size ft_rope_impl::len() const
{
	const ft_rope_impl * p = this;
	ft_size n = 0;
	do {
		n += p->suffix_len;
	} while ((p = p->prefix()) != NULL);
	return n;
}


/** convert to ft_string */
void ft_rope_impl::to_string(ft_string & append_dst) const
{
	ft_size ths_len = len();
	if (ths_len == 0)
		return;
	ft_size dst_len = append_dst.size();
	append_dst.resize(dst_len + ths_len);
	char * end_dst = &append_dst[0] + dst_len + ths_len;
	const ft_rope_impl * p = this;
	do {
		if (p->suffix_len != 0)
			memcpy(end_dst -= p->suffix_len, p->suffix(), p->suffix_len);
	} while ((p = p->prefix()) != NULL);
}

/** compare against char[] */
bool ft_rope_impl::equals(const char other[], ft_size other_len) const
{
	ft_size ths_len = len();
	if (ths_len != other_len)
		return false;
	other += other_len;
	const ft_rope_impl * p = this;
	do {
		if (p->suffix_len != 0)
			if (memcmp(other -= p->suffix_len, p->suffix(), p->suffix_len))
				return false;
	} while ((p = p->prefix()) != NULL);
	return true;
}

/** return this hash */
ft_size ft_rope_impl::hash() const
{
	const ft_rope_impl * p = this;
	ft_size h = 0;
	do {
		const char * str = p->suffix(), * end = str + p->suffix_len;
		while (str != end)
			h = *--end + (h << 16) + (h << 6) - h;
	} while ((p = p->prefix()) != NULL);
	// ff_log(FC_INFO, 0, "hash = 0x%016llx of {%p, \"%.*s\"}", (unsigned long long)h, prefix(), (int)suffix_len, suffix());
	return h;
}

/** return other's hash */
ft_size ft_rope_impl::hash(const char other[], ft_size other_len)
{
	const char * end = other + other_len;
	ft_size h = 0;
	while (other != end)
		h = *--end + (h << 16) + (h << 6) - h;
	// ff_log(FC_INFO, 0, "hash = 0x%016llx of \"%.*s\"", (unsigned long long)h, (int)other_len, other);
	return h;
}

FT_NAMESPACE_END
