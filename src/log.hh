/*
 * log.hh
 *
 *  Created on: Mar 8, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_LOG_HH
#define FSTRANSFORM_LOG_HH

#include "check.hh"

#include <stdio.h>  // for FILE *
#include <stdarg.h> // for va_list

FT_EXTERN_C_BEGIN
FT_NAMESPACE_BEGIN

typedef enum ft_level_e { FC_TRACE, FC_DEBUG, FC_INFO, FC_NOTICE, FC_WARN, FC_ERROR, FC_FATAL } ft_level;


/**
 * initialize log subsystem:
 * configure stderr to receive all WARN messages or more serious,
 * configure stdout to receive all NOTICE messages or less serious
 */
void ff_log_init();

/**
 * print fmt and subsequent printf-style args to log stream(s).
 * if err != 0, append ": ", strerror(errno) and "\n"
 * else append "\n"
 * finally return err
 */
int ff_log(ft_level level, int err, const char * fmt, ...);

/**
 * print to log fmt and subsequent printf-style args log stream(s).
 * if err != 0, append ": ", strerror(errno) and "\n"
 * else append "\n"
 * finally return err
 */
int ff_vlog(ft_level level, int err, const char * fmt, va_list args);

/**
 * flush all buffered streams used to log messages of specified level
 */
void ff_log_flush(ft_level level);

/**
 * add 'stream' to the list of streams receiving log messages
 * with seriousness between min_level and max_level (inclusive)
 *
 * by default, all WARN messages or more serious are sent to stderr
 * and all NOTICE messages or less serious are sent to stdout
 * note: by default, messages less serious than INFO are suppressed, see ff_log_set_threshold()
 */
void ff_log_register(FILE * stream, ft_level min_level = FC_TRACE, ft_level max_level = FC_FATAL);

/**
 * remove 'stream' from the list of streams receiving log messages
 * at least as serious as 'level'.
 */
void ff_log_unregister(FILE * stream, ft_level min_level = FC_TRACE, ft_level max_level = FC_FATAL);

/**
 * tell ff_log() and ff_vlog() to suppress printing of messages less serious than 'level'.
 *
 * by default, all messages less serious than 'INFO' are suppressed
 */
void ff_log_set_threshold(ft_level level);

/**
 * return true if printing specified message level is not suppressed
 */
bool ff_log_is_enabled(ft_level level);


FT_NAMESPACE_END
FT_EXTERN_C_END


#endif /* FSTRANSFORM_LOG_HH */
