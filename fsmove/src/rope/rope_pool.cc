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
 * rope_pool.cc
 *
 *  Created on: Mar 3, 2018
 *      Author: max
 */

#include "first.hh"

#include "rope_impl.hh"
#include "rope_pool.hh"

FT_NAMESPACE_BEGIN

/** default constructor. */
ft_rope_pool::ft_rope_pool() : count(0), table()
{ }

/** copy constructor. */
ft_rope_pool::ft_rope_pool(const ft_rope_pool & other)
: count(other.count), table(other.table)
{ }

/** assignment operator. */
const ft_rope_pool & ft_rope_pool::operator=(const ft_rope_pool & other)
{
	if (this != & other) {
		count = other.count;
		table = other.table;
	}
	return *this;
}

/** destructor. */
ft_rope_pool::~ft_rope_pool()
{ }

void ft_rope_pool::rehash(ft_size new_len)
{
	// assert((new_len & new_lenn-1)) == 0); // must be power-of-two
	ft_table tmp(new_len);
	tmp.swap(table);
	ft_bucket::const_iterator iter, end;
	for (ft_size i = 0, n = tmp.size(); i < n; i++) {
		const ft_bucket & bucket = tmp[i];
		iter = bucket.begin();
		end = bucket.end();
		for (; iter != end; ++iter) {
			const ft_rope & r = *iter;
		    ft_size index = r.hash() & (new_len - 1);
		    table[index].push_front(r);
		}
	}
}

const ft_rope * ft_rope_pool::find(const char s[], ft_size len) const
{
	ft_size n = table.size();
	if (n == 0)
		return NULL;
	// assert((n & (n-1)) == 0); // must be power-of-two
	ft_size index = ft_rope::hash(s, len) & (n - 1);
	const ft_bucket & bucket = table[index];
	ft_bucket::const_iterator iter = bucket.begin(), end = bucket.end();
	for (; iter != end; ++iter) {
		const ft_rope & r = *iter;
		if (r.equals(s, len))
			return & r;
	}
	return NULL;
}

const ft_rope & ft_rope_pool::find_or_add(const char s[], ft_size len)
{
	const ft_rope * r = find(s, len);
	if (r != NULL)
		return *r;

	enum { MINSPLIT = sizeof(ft_rope_impl) };

	const ft_rope * prefix = NULL;
	const char * suffix = s;
	ft_size suffix_len = len;
	if (len > 2 * MINSPLIT) {
		ft_size split = len - MINSPLIT;
		for (; split > MINSPLIT; split--) {
			if (s[split] == '/') {
				prefix = &find_or_add(s, ++split);
				suffix += split;
				suffix_len -= split;
				break;
			}
		}
	}
	ft_size n = table.size();
	if (count >= n)
		rehash(n = (n ? n * 2 : 64));
	ft_size index = ft_rope::hash(s, len) & (n - 1);
	ft_bucket & bucket = table[index];
	bucket.push_front(ft_rope(prefix, suffix, suffix_len));
	count++;
	return bucket.front();
}

FT_NAMESPACE_END
