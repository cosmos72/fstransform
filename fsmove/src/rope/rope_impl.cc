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
 * rope_impl.cc
 *
 *  Created on: Mar 3, 2018
 *      Author: max
 */

#include "first.hh"

#include <new>         // for std::bad_alloc

#if defined(FT_HAVE_STDLIB_H)
# include <stdlib.h>   // for malloc(), free()
#elif defined(FT_HAVE_CSTDLIB)
# include <cstdlib>    // for malloc(), free()
#endif

#if defined(FT_HAVE_STRING_H)
# include <string.h>     // for memcpy()
#elif defined(FT_HAVE_STRING)
# include <cstring>      // for memcmp()
#endif

#include "log.hh"
#include "rope_impl.hh"       // for ft_rope_impl

FT_NAMESPACE_BEGIN


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
	ft_rope_impl * ths = reinterpret_cast<ft_rope_impl *>(bytes);
	new (ths) ft_rope_impl(); // placement new
	if ((ths->suffix_len = suffix_length) != 0)
		memcpy(ths->suffix(), suffix, suffix_length);
	if ((ths->prefix = prefix) != NULL)
		prefix->ref();
	return ths;
}

/** destructor. private use only. call unref() instead */
ft_rope_impl::~ft_rope_impl()
{
	if (prefix != NULL && prefix != this)
		prefix->unref();
	prefix = NULL;
	suffix_len = 0;
}

/** decrement reference count */
void ft_rope_impl::unref()
{
	if (refcount-- != 0)
		return;
	this->~ft_rope_impl();
	delete[]reinterpret_cast<char *>(this);
}


/** convert to ft_string */
void ft_rope_impl::to_string(ft_string & append_dst) const
{
	if (prefix != NULL)
		prefix->to_string(append_dst);
	if (suffix_len == 0)
		return;
	ft_size dst_len = append_dst.size();
	append_dst.resize(dst_len + suffix_len);
	memcpy(&append_dst[dst_len], suffix(), suffix_len);
}

/** compare against char[] */
bool ft_rope_impl::equals(const char other[], ft_size other_len) const
{
	ft_size plen = prefix_len();
	if (plen + suffix_len != other_len)
		return false;
	if (plen != 0)
		if (!prefix->equals(other, plen))
			return false;
	if (suffix_len != 0)
		if (memcmp(suffix(), other + plen, suffix_len))
			return false;
	return true;
}

/** return this hash */
ft_size ft_rope_impl::hash() const
{
	const char * str = suffix();
	ft_size h = prefix ? prefix->hash() : 0;
	ft_size i, n = suffix_len;
	for (i = 0; i < n; i++)
		h = str[i] + (h << 6) + (h << 16) - h;
	ff_log(FC_INFO, 0, "hash = 0x%016llx of {%p, \"%.*s\"}", (unsigned long long)h, prefix, (int)suffix_len, str);
	return h;
}

/** return other's hash */
ft_size ft_rope_impl::hash(const char other[], ft_size other_len)
{
	ft_size h = 0, i, n = other_len;
	for (i = 0; i < n; i++)
		h = other[i] + (h << 6) + (h << 16) - h;
	ff_log(FC_INFO, 0, "hash = 0x%016llx of \"%.*s\"", (unsigned long long)h, (int)other_len, other);
	return h;
}

FT_NAMESPACE_END
