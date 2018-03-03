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

#include "first.hh"

#include "rope_test.hh"
#include "rope_pool.hh"

FT_NAMESPACE_BEGIN

#define REF(s) (s), (sizeof(s)-1)

bool rope_test()
{
	ft_rope_pool pool;
	pool.find_or_add(REF("very_very_very/long_long_long/name_name_name/some_some_some/file_file_file_1"));
	pool.find_or_add(REF("very_very_very/long_long_long/name_name_name/some_some_some/file_file_file_2"));
	pool.find_or_add(REF("very_very_very/long_long_long/name_name_name/some_some_some/file_file_file_3"));
	return true;
}


FT_NAMESPACE_END
