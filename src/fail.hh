/*
 * fail.hh
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_FAIL_HH
#define FSTRANSLATE_FAIL_HH

#include "check.hh"

#include <stdarg.h> // for va_list */


FT_EXTERN_C_BEGIN
FT_NAMESPACE_BEGIN

/**
 * print fmt and subsequent printf-style args to stderr,
 * if err != 0, append ": ", strerror(errno) and "\n"
 * else append "\n"
 * finally return 1
 */
int ff_fail(int err, const char * fmt, ...);

/**
 * print fmt and subsequent printf-style args to stderr,
 * if err != 0, append ": ", strerror(errno) and "\n"
 * else append "\n"
 * finally return 1
 */
int ff_vfail(int err, const char * fmt, va_list args);


FT_NAMESPACE_END
FT_EXTERN_C_END

#endif /* FSTRANSLATE_FAIL_HH */
