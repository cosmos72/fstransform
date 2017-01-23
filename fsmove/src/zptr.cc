/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2017 Massimiliano Ghilardi
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
 * zmem.hh
 *
 *  Created on: Jan 22, 2017
 *      Author: max
 */

#include "first.hh"

#include "zptr.hh"

#if defined(FT_HAVE_STDLIB_H)
# include <stdlib.h>       // for malloc(), realloc(), free()
#elif defined(FT_HAVE_CSTDLIB)
# include <cstdlib>        // for malloc(), realloc(), free()
#endif


FT_NAMESPACE_BEGIN

zptr_handle zptr_alloc(ft_size size) {
    return (zptr_handle)malloc(size);
}
void zptr_free(zptr_handle handle, ft_size size) {
    free((void *)handle);
}

void zptr_deflate(zptr_handle handle) {
}

void * zptr_inflate(zptr_handle handle) {
    return (void *)handle;
}

FT_NAMESPACE_END


