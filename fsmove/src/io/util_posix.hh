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
 * io/util_posix.hh
 *
 *  Created on: Mar 27, 2012
 *      Author: max
 */
#ifndef FSMOVE_IO_POSIX_UTIL_HH
#define FSMOVE_IO_POSIX_UTIL_HH

#include "../types.hh"

FT_IO_NAMESPACE_BEGIN

/**
 * spawn a system command, wait for it to complete and return its exit status.
 * argv[0] is conventionally the program name.
 * argv[1...] are program arguments and must be terminated with a NULL pointer.
 */
int ff_posix_exec_silent(const char * path, const char * const argv[]);

FT_IO_NAMESPACE_END


#endif /* FSREMAP_IO_POSIX_UTIL_HH */
