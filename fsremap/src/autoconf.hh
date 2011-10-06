/*
 * autoconf.hh
 *
 * defines compiler features.
 * ideally, this file should be generated by a './configure' script from autotools
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_AUTOCONF_HH
#define FSTRANSFORM_AUTOCONF_HH

#include "check.hh"


/** define if compiler supports the types (long long) and (unsigned long long) */
#ifndef FT_HAVE_LONG_LONG
#  if defined(__GNUC__)
#    define FT_HAVE_LONG_LONG
#  endif
#endif /* FT_HAVE_LONG_LONG */


/** define if <termios.h> exists and is usable */
#ifndef FT_HAVE_TERMIOS_H
#  if defined(__unix__)
#    define FT_HAVE_TERMIOS_H
#  endif
#endif /* FT_HAVE_TERMIOS_H */


/** define if localtime_r() is supported - otherwise, localtime() will be used */
#ifndef FT_HAVE_LOCALTIME_R
#  if _POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _BSD_SOURCE || _SVID_SOURCE || _POSIX_SOURCE
#    define FT_HAVE_LOCALTIME_R
#  endif
#endif /* FT_HAVE_LOCALTIME_R */


/** define if posix_fallocate() is supported */
#ifndef FT_HAVE_POSIX_FALLOCATE
#  if _XOPEN_SOURCE >= 600
#    define FT_HAVE_POSIX_FALLOCATE
#  endif
#endif /* FT_HAVE_POSIX_FALLOCATE */

/** define if umount() is supported */
#ifndef FT_HAVE_UMOUNT
# if defined(__linux__)
#    define FT_HAVE_UMOUNT
# endif
#endif

/** define if va_copy() macro is supported (copies va_list variadic arguments) */
#ifndef FT_HAVE_VA_COPY
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#    define FT_HAVE_VA_COPY
#  endif
#endif


/**
 * define if C++ compiler supports g++-style explicit template instantiation:
 * extern template ...<>; // declaration
 * template ...<>;        // instantiation
 */
#ifndef FT_HAVE_EXTERN_TEMPLATE
#  if defined(__cplusplus) && defined(__GNUC__)
#    define FT_HAVE_EXTERN_TEMPLATE
#  endif
#endif /* FT_HAVE_EXTERN_TEMPLATE */



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