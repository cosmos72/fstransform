/*
 * fail.c
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */

#include "first.h"

#include <stdio.h>  /* for stderr     */
#include <stdarg.h> /* for vfprintf() */
#include <string.h> /* for strerror() */

#include "fail.h"   /* for ff_fail()  */


/**
 * print fmt and subsequent printf-style args to stderr,
 * and if err != 0 append ": ", strerror(errno) and "\n"
 * finally return 1
 */
int ff_fail(int err, const char * fmt, ...) {
	va_list vargs;

	va_start(vargs, fmt);
	vfprintf(stderr, fmt, vargs);
	va_end(vargs);

	if (err)
		fprintf(stderr, ": %s\n", strerror(err));
	else
		fputc('\n', stderr);
	return 1;
}
