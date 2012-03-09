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
 * args.cc
 *
 *  Created on: Mar 21, 2011
 *      Author: max
 */

#include "first.hh"

#include "args.hh"    // for fr_args

FT_NAMESPACE_BEGIN


/** default constructor */
fr_args::fr_args()
    : root_dir(NULL), io_args(), ui_arg(NULL), umount_cmd(NULL), storage_size(), job_id(0),
      job_clear(FC_CLEAR_AUTODETECT), io_kind(FC_IO_AUTODETECT), ui_kind(FC_UI_NONE),
      force_run(false), simulate_run(false), ask_questions(true)
{
    ft_size i, n;
    for (i = 0, n = sizeof(io_args)/sizeof(io_args[0]); i < n; i++)
        io_args[i] = NULL;
    for (i = 0, n = sizeof(storage_size)/sizeof(storage_size[0]); i < n; i++)
        storage_size[i] = 0;
}

FT_NAMESPACE_END
