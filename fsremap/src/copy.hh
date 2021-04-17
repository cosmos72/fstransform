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
 * copy.hh
 *
 *  Created on: Aug 18, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_COPY_HH
#define FSTRANSFORM_COPY_HH

#include "types.hh" // for ft_string

FT_NAMESPACE_BEGIN

void ff_set(ft_string &dst, const ft_string &src);
void ff_set(ft_string &dst, ft_ull src);

void ff_set(ft_ull &dst, const ft_string &src);

void ff_cat(ft_string &dst, const ft_string &src);
void ff_cat(ft_string &dst, ft_ull src);

FT_NAMESPACE_END

#endif /* FSTRANSFORM_COPY_HH */
