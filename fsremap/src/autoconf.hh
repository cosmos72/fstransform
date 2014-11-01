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
 * autoconf.hh
 *
 * defines compiler, headers and library features.
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_AUTOCONF_HH
#define FSTRANSFORM_AUTOCONF_HH

#include "check.hh"
#include "ft_config.hh"


/** define if compiler supports the types (long long) and (unsigned long long) */
#if !defined(FT_HAVE_LONG_LONG) || !defined(FT_HAVE_UNSIGNED_LONG_LONG)
#  if defined(__GNUC__)
#    define FT_HAVE_LONG_LONG
#    define FT_HAVE_UNSIGNED_LONG_LONG
#  endif
#endif /* FT_HAVE_LONG_LONG */

/* (long long) and (unsigned long long) are useful only if they are BOTH supported */
#ifndef FT_HAVE_LONG_LONG
# undef FT_HAVE_UNSIGNED_LONG_LONG
#endif
#ifndef FT_HAVE_UNSIGNED_LONG_LONG
# undef FT_HAVE_LONG_LONG
#endif



/** define if va_copy() macro is supported (copies va_list variadic arguments) */
#ifndef FT_HAVE_VA_COPY
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#    define FT_HAVE_VA_COPY
#  endif
#endif


/** define to compiler's own version of 'inline' keyword */
#ifndef FT_INLINE
#  if defined(__cplusplus)
     /* inline is standard keyword in C++ */
#    define FT_INLINE inline
#  elif defined(__GNUC__)
     /* prefer __inline__ to inline, the former works even in gcc strict-ansi mode */
#    define FT_INLINE __inline__
#  else
#    define FT_INLINE
#  endif
#endif /* FT_INLINE */


/**
 * define if extern "C" { ... } is understood by the compiler and needed to get C linkage.
 * should be defined in C++, and undefined in C.
 */
#ifndef FT_HAVE_EXTERN_C
#  if defined(__cplusplus)
#    define FT_HAVE_EXTERN_C
#  endif
#endif /* FT_HAVE_EXTERN_C */


/**
 * define if namespace Foo { ... } is understood by the compiler.
 * should be defined in C++, and undefined in C.
 */
#ifndef FT_HAVE_NAMESPACE
#  if defined(__cplusplus)
#    define FT_HAVE_NAMESPACE
#  endif
#endif /* FT_HAVE_NAMESPACE */


/** define to compiler's own version of __FILE__ or equivalent */
#ifndef FT_THIS_FILE
#  define FT_THIS_FILE __FILE__
#endif

/** define to compiler's own version of __LINE__ or equivalent  */
#ifndef FT_THIS_LINE
#  define FT_THIS_LINE __LINE__
#endif

/** define to compiler's own version of __func__, __FUNCTION__ or equivalent (don't use __PRETTY_FUNCTION__) */
#ifndef FT_THIS_FUNCTION
#  define FT_THIS_FUNCTION __FUNCTION__
#endif


#endif /* FSTRANSFORM_AUTOCONF_HH */
