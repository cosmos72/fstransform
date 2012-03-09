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
 * main.hh
 *
 *  Created on: Mar 20, 2011
 *      Author: max
 */

#ifndef FSMOVE_ARGS_HH
#define FSMOVE_ARGS_HH

#include "types.hh"     // for ft_uint, ft_size
#include "io/io.hh"     // for FC_ARGS_COUNT

FT_NAMESPACE_BEGIN

enum fm_io_kind { FC_IO_AUTODETECT, FC_IO_POSIX };
enum fm_ui_kind { FC_UI_NONE };

class fm_args
{
public:
    const char * io_args[FT_IO_NS fm_io::FC_ARGS_COUNT];
    char const * const * exclude_list; // NULL-terminated array of files _not_ to move
    fm_io_kind io_kind;      // if FC_IO_AUTODETECT, will autodetect
    fm_ui_kind ui_kind;      // default is FC_UI_NONE
    bool force_run;          // if true, some sanity checks will be WARNINGS instead of ERRORS
    bool simulate_run;       // if true, move algorithm runs WITHOUT actually moving any file/directory/special-device

    fm_args();
};


FT_NAMESPACE_END

#endif /* FSMOVE_ARGS_HH */
