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
 * args.cc
 *
 *  Created on: Mar 21, 2011
 *      Author: max
 */

#include "first.hh"

#include "args.hh"    // for fm_args

FT_NAMESPACE_BEGIN


/** default constructor */
fm_args::fm_args()
	: program_name("fsmove"),
      io_args(), exclude_list(NULL), inode_cache_path(NULL),
      io_kind(FC_IO_AUTODETECT), ui_kind(FC_UI_NONE),
      force_run(false), simulate_run(false)
{ }

FT_NAMESPACE_END
