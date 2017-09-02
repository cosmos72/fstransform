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
 * assert.cc
 *
 *  Created on: Mar 13, 2011
 *      Author: max
 */

#include "first.hh"

#if defined(FT_HAVE_STDLIB_H)
# include <stdlib.h>     // for exit()
#elif defined(FT_HAVE_CSTDLIB)
# include <cstdlib>      // for exit()
#endif

#include "assert.hh"     // for ff_assert()
#include "log.hh"        // for ff_log()

FT_NAMESPACE_BEGIN

void ff_assert_fail0(const char * caller_file, int caller_file_len, const char * caller_func, int caller_line, const char * assertion)
{
    ff_logl(caller_file, caller_file_len, caller_func, caller_line, FC_FATAL, 0, "internal error! assertion failed: %s", assertion);
    ft_log_appender::flush_all(FC_FATAL);
    exit(1);
}

FT_NAMESPACE_END
