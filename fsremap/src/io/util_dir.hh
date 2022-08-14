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
 * io/util.hh
 *
 *  Created on: Mar 6, 2011
 *      Author: max
 */

#ifndef FSREMAP_UTIL_DIR_HH
#define FSREMAP_UTIL_DIR_HH

#include "../types.hh"

FT_IO_NAMESPACE_BEGIN

/**
 * create a directory, return 0 (success) or error.
 * note: path MUST NOT end with '/'
 *
 * depending on the caller expectations whether the directory
 * can already exist or not, error==EEXIST may also indicate success
 */
int ff_mkdir(const char * path, ft_mode mode = 0755);

/**
 * same as ff_mkdir(), plus calls ff_log(WARN) on errors.
 * if error==EEXIST, it is returned but not logged.
 */
int ff_mkdir_or_warn(const char * path, ft_mode mode = 0755);

/**
 * create a directory, return 0 (success) or error.
 *
 * depending on the caller expectations whether the directory
 * can already exist or not, error==EEXIST may also indicate success
 */
int ff_mkdir_recursive(const ft_string & path);

int ff_remove_recursive(const ft_string & path);

FT_IO_NAMESPACE_END

#endif /* FSREMAP_UTIL_DIR_HH */
