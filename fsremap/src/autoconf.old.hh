/**
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


/** define if fallocate() is supported */
#ifndef FT_HAVE_FALLOCATE
#  if _XOPEN_SOURCE >= 600
#    define FT_HAVE_FALLOCATE
#  endif
#endif /* FT_HAVE_FALLOCATE */

