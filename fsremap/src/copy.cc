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
 * copy.cc
 *
 *  Created on: Aug 18, 2011
 *      Author: max
 */

#include "first.hh"

#include "copy.hh"

#if defined(FT_HAVE_STDIO_H)
#include <stdio.h> /* for snprintf() */
#elif defined(FT_HAVE_CSTDIO)
#include <cstdio> /* for snprintf() */
#endif

FT_NAMESPACE_BEGIN

void ff_set(ft_string &dst, const ft_string &src) {
    dst = src;
}

void ff_set(ft_string &dst, ft_ull src) {
    enum { maxlen = sizeof(ft_ull) * 3 + 1 };
    dst.resize(maxlen);
    char *buf = &dst[0];

    int delta = snprintf(buf, maxlen, "%" FT_XLL, src);
    dst.resize(delta > 0 ? delta : 0);
}

void ff_set(ft_ull &dst, const ft_string &src) {
    dst = 0;
    sscanf(src.c_str(), "%" FT_XLL, &dst);
}

void ff_cat(ft_string &dst, const ft_string &src) {
    dst += src;
}

void ff_cat(ft_string &dst, ft_ull src) {
    enum { maxlen = sizeof(ft_ull) * 3 + 1 };
    size_t oldlen = dst.length();
    dst.resize(oldlen + maxlen);
    char *buf = &dst[oldlen];

    int delta = snprintf(buf, maxlen, "%" FT_XLL, src);
    dst.resize(oldlen + (delta > 0 ? delta : 0));
}

FT_NAMESPACE_END
