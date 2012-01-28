/**
 * this file contains no-longer-used code:
 * the macros conditionally defined here
 * are instead autodetected by 'configure' script
 */

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

