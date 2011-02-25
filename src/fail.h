/*
 * fail.h
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_FAIL_H
#define FSTRANSLATE_FAIL_H

/**
 * print fmt and subsequent printf-style args to stderr,
 * followed by ": ", strerror(errno) and "\n"
 * then return 1
 */
int ff_fail(int err, const char * fmt, ...);


#endif /* FSTRANSLATE_FAIL_H */
