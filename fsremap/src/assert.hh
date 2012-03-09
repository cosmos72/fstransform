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
 * assert.h
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_ASSERT_HH
#define FSTRANSFORM_ASSERT_HH

#include "check.hh"

FT_EXTERN_C_BEGIN
FT_NAMESPACE_BEGIN

void ff_assert_fail0(const char * caller_file, const char * caller_func, int caller_line, const char * assertion);

FT_NAMESPACE_END
FT_EXTERN_C_END

#define ff_assert(expr) do { if (!(expr)) ff_assert_fail0(FT_THIS_FILE, FT_THIS_FUNCTION, FT_THIS_LINE, #expr ); } while (0)

#define ff_assert_fail(msg) ff_assert_fail0(FT_THIS_FILE, FT_THIS_FUNCTION, FT_THIS_LINE, (msg))


#endif /* FSTRANSFORM_ASSERT_HH */
