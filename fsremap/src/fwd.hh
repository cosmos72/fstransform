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
 * fwd.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSREMAP_FWD_HH
#define FSREMAP_FWD_HH

#include "check.hh"

FT_NAMESPACE_BEGIN
class fr_args;
class fr_remap;
class fr_dispatch;
class fr_job;

template<typename T> struct fr_extent_key;
template<typename T> struct fr_extent_payload;
template<typename T> class  fr_extent;
template<typename T> class  fr_vector;
template<typename T> class  fr_map;
template<typename T> class  fr_pool_entry;
template<typename T> class  fr_pool;
template<typename T> class  fr_work;
FT_NAMESPACE_END


FT_IO_NAMESPACE_BEGIN
class fr_io;
class fr_io_posix;
class fr_io_test;
class fr_io_self_test;
FT_IO_NAMESPACE_END

FT_UI_NAMESPACE_BEGIN
class fr_ui;
class fr_ui_tty;
FT_UI_NAMESPACE_END

#endif /* FSREMAP_FWD_HH */
