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
 * mstring.hh
 *
 *  Created on: Apr 25, 2012
 *      Author: max
 */

#include "first.hh"

#include "misc.hh"       // for ft_min2()
#include "mstring.hh"    // for ft_mstring

#if defined(FT_HAVE_STRING_H)
# include <string.h>     // for memcmp(), memcpy(), memrchr()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>      // for memcmp(), memcpy(), memrchr()
#endif

FT_NAMESPACE_BEGIN

/** default constructor. */
ft_mstring::ft_mstring()
: txt(NULL), len(0)
{ }

/** copy constructor. */
ft_mstring::ft_mstring(const ft_mstring & other)
: txt(other.txt), len(other.len)
{ }


/** constructor. shares the char array inside 'other', does not copy it */
ft_mstring::ft_mstring(const ft_mstring & other, ft_size offset, ft_size length)
: txt(other.txt + ff_min2(offset, other.len)),
  len(other.len <= offset ? 0 : ff_min2(length, other.len - offset))
{ }

/** constructor. shares the char array 'str', does not copy it */
ft_mstring::ft_mstring(const char * str, ft_size str_len)
: txt(str), len(str_len)
{ }


/** assignment operator */
const ft_mstring & ft_mstring::operator=(const ft_mstring & other)
{
	// ok also if this == & other
	txt = other.txt;
	len = other.len;

	return * this;
}

// comparison operator
bool ft_mstring::operator==(const ft_mstring & other) const
{
	if (len != other.len)
		return false;
	if (txt == other.txt)
		// also catches this == & other
		return true;
	return memcmp(txt, other.txt, len) == 0;
}

// comparison operator
bool ft_mstring::operator<(const ft_mstring & other) const
{
	if (len != 0 && other.len != 0 && txt != other.txt)
	{
		int cmp = memcmp(txt, other.txt, ff_min2(len, other.len));
		if (cmp != 0)
			// different contents
			return cmp < 0;
	}
	// at least one empty string, or same contents in the common prefix.
	// also catches this == & other
	return len < other.len;
}



ft_size ft_mstring::rfind(char ch) const
{
	if (len != 0) {
		const char * match = (const char *) memrchr(txt, (unsigned char) ch, len);
		if (match != NULL)
			return match - txt;
	}
	return npos;
}

FT_NAMESPACE_END
