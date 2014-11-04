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
 * mstring.hh
 *
 *  Created on: Apr 25, 2012
 *      Author: max
 */

#ifndef FSTRANSFORM_MSTRING_HH
#define FSTRANSFORM_MSTRING_HH

#include "types.hh"   // for ft_size

FT_NAMESPACE_BEGIN

/**
 * immutable string. initialized with a const char *,
 * keeps a reference to the char array instead of copying its contents
 */
class ft_mstring
{
private:
	const char * txt;
	ft_size len;

public:
	enum { npos = (ft_size)-1 };

	/** default constructor. */
	ft_mstring();

	/** copy constructor. */
	ft_mstring(const ft_mstring & other);

	/** constructor. shares the char array inside 'other', does not copy it */
	ft_mstring(const ft_mstring & other, ft_size offset, ft_size length);

	/** constructor. shares the char array 'str', does not copy it */
	ft_mstring(const char * str, ft_size str_len);

	/** constructor. shares the char array 'str', does not copy it */
	template<ft_size N>
		ft_mstring(const char (& str)[N])
		: txt(str), len(N-1)
	{ }

	/** assignment operator */
	const ft_mstring & operator=(const ft_mstring & other);

	/** destructor. */
	FT_INLINE ~ft_mstring()
	{ }

	FT_INLINE const char & operator[](ft_size i) const { return txt[i]; }
	FT_INLINE ft_size size() const { return len; }

	// comparison operators
	bool operator==(const ft_mstring & other) const;
	FT_INLINE bool operator!=(const ft_mstring & other) const { return !(*this == other); }

	// comparison operators
	bool operator<(const ft_mstring & other) const;
	FT_INLINE bool operator<=(const ft_mstring & other) const { return !(other < *this); }
	FT_INLINE bool operator> (const ft_mstring & other) const { return other < *this; }
	FT_INLINE bool operator>=(const ft_mstring & other) const { return !(*this < other); }


	ft_size rfind(char ch) const;
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_MSTRING_HH */
