/*
 * common header for fsattr, fsmove, fsremap
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
 * first.hh
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_FIRST_HH
#define FSTRANSFORM_FIRST_HH

#if defined(__USE_ANSI) || defined(_FEATURES_H) || defined(FSTRANSFORM_FEATURES_HH) || defined(FSTRANSFORM_AUTOCONF_HH)
#  error "first.hh" must be included before any other #include
#endif

/* put here any option/define/... affecting system-wide includes */

/*
 * _GNU_SOURCE implies _LARGEFILE_SOURCE.
 * together with _FILE_OFFSET_BITS=64 they should set off_t to be at least 64 bits wide
 */
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#ifndef _LARGEFILE_SOURCE
#  define _LARGEFILE_SOURCE
#endif
#ifndef _FILE_OFFSET_BITS
#  define _FILE_OFFSET_BITS 64
#endif


/* put here any option/define/... affecting the whole program (headers and source files) */

#include "autoconf.hh"

#ifdef FT_HAVE_FEATURES_H
# include <features.h>
#endif

#include "features.hh"



#endif /* FSTRANSFORM_FIRST_HH */
