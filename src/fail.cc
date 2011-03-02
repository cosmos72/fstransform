/*
 * fail.c
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */

#include "first.hh"

#include <cstdio>  /* for stderr     */
#include <cstdarg> /* for vfprintf() */
#include <cstring> /* for strerror() */

#include "fail.hh"   /* for ff_fail()  */

/**
 * print fmt and subsequent printf-style args to stderr,
 * if err != 0, append ": ", strerror(errno) and "\n"
 * else append "\n"
 * finally return 1
 */
int ff_vfail(int err, const char * fmt, va_list args)
{
    vfprintf(stderr, fmt, args);

    if (err != 0)
        fprintf(stderr, ": %s\n", strerror(err));
    else
        fputc('\n', stderr);
    return 1;
}

/**
 * print fmt and subsequent printf-style args to stderr,
 * if err != 0, append ": ", strerror(errno) and "\n"
 * else append "\n"
 * finally return 1
 */
int ff_fail(int err, const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    ff_vfail(err, fmt, args);
    va_end(args);

    return 1;
}

