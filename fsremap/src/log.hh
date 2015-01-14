/*
 * logging utilities for fsattr, fsmove, fsremap
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
 * log.hh
 *
 *  Created on: Mar 8, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_LOG_HH
#define FSTRANSFORM_LOG_HH

#include "first.hh"

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>      /* for EINVAL */
#elif defined(FT_HAVE_CERRNO) && defined(__cplusplus)
# include <cerrno>       /* for EINVAL */
#endif
#if defined(FT_HAVE_STDARG_H)
# include <stdarg.h>     /* for va_list. also for va_start(), va_end(), va_copy() used by log.cc */
#elif defined(FT_HAVE_CSTDARG) && defined(__cplusplus)
# include <cstdarg>      /* for va_list. also for va_start(), va_end(), va_copy() used by log.cc */
#endif
#if defined(FT_HAVE_STDIO_H)
# include <stdio.h>      /* for FILE. also for stdout, stderr, fprintf(), fileno() used by log.cc */
#elif defined(FT_HAVE_CSTDIO) && defined(__cplusplus)
# include <cstdio>       /* for FILE. also for stdout, stderr, fprintf(), fileno() used by log.cc */
#endif

#include <set>           /* for std::set<T>  */
#include <map>           /* for std::map<K,V> */

#include "mstring.hh"    /* ft_mstring */

FT_NAMESPACE_BEGIN


/**
 * note 1.1)
 * log subsystem is automatically initialized and configured upon first call to
 * ff_log(), ff_vlog(), ff_log_register_range(), ff_log_unregister_range() or ff_log_set_threshold().
 *
 * automatic configuration is:
 * print to stdout all INFO and NOTICE messages, with format FC_FMT_MSG
 * print to stderr all WARN, ERROR and FATAL messages, with format FC_FMT_MSG
 */


/* FC_FATAL is reserved for things that should not happen, i.e. bugs in the program or in the operating system. */
enum ft_log_level {
    FC_LEVEL_NOT_SET = 0,
    FC_DUMP, FC_TRACE, FC_DEBUG, FC_INFO, FC_NOTICE, FC_WARN, FC_ERROR, FC_FATAL,
    FC_SHOW_DEFAULT_LEVEL = FC_TRACE
};

enum ft_log_fmt {
    FC_FMT_MSG, /* message only */
    FC_FMT_LEVEL_MSG, /* level + message */
    FC_FMT_DATETIME_LEVEL_MSG, /* datetime + level + message */
    FC_FMT_DATETIME_LEVEL_CALLER_MSG, /* datetime + level + [file.func(line)] + message */
};

enum ft_log_color {
    FC_COL_AUTO = 0,
    FC_COL_NONE,
    FC_COL_ANSI,
};

/**
 * print to log fmt and subsequent printf-style args log stream(s).
 * if err != 0, append ": ", strerror(errno) and "\n"
 * else append "\n"
 * finally return err
 */
#define ff_log_is_enabled(level)        ff_logl_is_enabled(FT_THIS_FILE, sizeof(FT_THIS_FILE)-1, level)
#define ff_log(level, err, ...)         ff_logl(FT_THIS_FILE, sizeof(FT_THIS_FILE)-1, FT_THIS_FUNCTION, FT_THIS_LINE, level, err, __VA_ARGS__)
#define ff_vlog(level, err, fmt, vargs) ff_logv(FT_THIS_FILE, sizeof(FT_THIS_FILE)-1, FT_THIS_FUNCTION, FT_THIS_LINE, level, err, fmt, vargs)

bool ff_logl_is_enabled(const char * caller_file, int caller_file_len, ft_log_level level);
int  ff_logl(const char * caller_file, int caller_file_len, const char * caller_func, int caller_line, ft_log_level level, int err, const char * fmt, ...);
int  ff_logv(const char * caller_file, int caller_file_len, const char * caller_func, int caller_line, ft_log_level level, int err, const char * fmt, va_list args);

const char * ff_log_level_to_string(ft_log_level level);


#if defined(EINVAL) && EINVAL < 0
#  define ff_log_is_reported(err) ((err) >= 0)
#else
#  define ff_log_is_reported(err) ((err) <= 0)
#endif

struct ft_log_event
{
    const char * str_now, * file, * file_suffix, * function, * fmt;
    int file_len, line, err;
    ft_log_level level;
    va_list vargs;
};

class ft_log_appender;
typedef std::set<ft_log_appender *> ft_log_appenders;
typedef ft_log_appenders::iterator ft_log_appenders_iterator;
typedef ft_log_appenders::const_iterator ft_log_appenders_citerator;


class ft_log_appender {
private:
    FILE * stream;
    ft_log_fmt format;
    ft_log_level min_level, max_level;
    ft_log_color color;
    
    /** destructor. */
    ~ft_log_appender();

    /** list of all appenders */
    static ft_log_appenders & get_all_appenders();

public:
    /** constructor. */
    ft_log_appender(FILE * stream, ft_log_fmt format = FC_FMT_MSG,
                    ft_log_level min_level = FC_DUMP, ft_log_level max_level = FC_FATAL,
                    ft_log_color color = FC_COL_AUTO);
    
    FT_INLINE void set_format(ft_log_fmt format) { this->format = format; }

    FT_INLINE void set_min_level(ft_log_level min_level) { this->min_level = min_level; }
    FT_INLINE void set_max_level(ft_log_level max_level) { this->max_level = max_level; }
    
    /** write a log message to stream */
    void append(ft_log_event & event);

    /** flush this appender */
    void flush();
    
    /** flush all buffered streams used to log messages for specified level */
    static void flush_all(ft_log_level level);

    /** set format, min level and color of this appender */
    void reconfigure(ft_log_fmt format_except_fatal = FC_FMT_MSG, ft_log_level stdout_min_level = FC_LEVEL_NOT_SET, ft_log_color color = FC_COL_AUTO);

    /** set format, min level and color of all appenders */
    static void reconfigure_all(ft_log_fmt format_except_fatal = FC_FMT_MSG, ft_log_level stdout_min_level = FC_LEVEL_NOT_SET, ft_log_color color = FC_COL_AUTO);
};


class ft_log;
typedef std::map<ft_mstring, ft_log *> all_loggers_type;
typedef all_loggers_type::iterator all_loggers_iterator;
typedef all_loggers_type::const_iterator all_loggers_citerator;


class ft_log {
private:
    friend class ft_log_appender;

    const ft_mstring * name;
    ft_log * parent;
    ft_log_appenders appenders;
    ft_log_level level; /* events less severe than level will be suppressed */
    mutable ft_log_level effective_level, threshold_level;

    /** initialize loggers and appenders. */
    static void initialize();

    /** return map of all existing loggers. */
    static all_loggers_type & get_all_loggers();
    
    /** constructor. */
    ft_log(const ft_mstring & name, ft_log * parent, ft_log_level level = FC_LEVEL_NOT_SET);

    /** destructor. */
    ~ft_log();

    /** find or create a parent logger given child name. */
    static ft_log & get_parent(const ft_mstring & child_logger_name);

    /**
     * invalidate all cached effective_level and threshold_level.
     * needed after some logger->level is changed.
     */
    static void invalidate_all_cached_levels();

    /** log a message (skip threshold_level check) */
    void append(ft_log_event & event);
    
public:

    /** return root logger */
    static ft_log & get_root_logger();

    /** find or create a logger by name */
    static ft_log & get_logger(const ft_mstring & logger_name);

    /** log a message (unless its level is suppressed) */
    void log(ft_log_event & event);

    /** return true if level is enabled (i.e. not suppressed) for this logger. */
    FT_INLINE bool is_enabled(ft_log_level level) const { return level >= get_threshold_level(); }


    /** get logger name. */
    FT_INLINE const ft_mstring & get_name() const { return * name; }
    
    /** return the level, i.e. least serious level that is not suppressed. */
    FT_INLINE ft_log_level get_level() const { return level; }

    /** return the effective level: if level is set return it, otherwise return parent effective level. */
    ft_log_level get_effective_level() const;

    /** return the threshold level: the minimum of this and all ancestor's levels. */
    ft_log_level get_threshold_level() const;

    /** set the level, i.e. least serious level that is not suppressed. */
    FT_INLINE void set_level(ft_log_level level) { this->level = level; invalidate_all_cached_levels(); }

    /** add an appender */
    void add_appender(ft_log_appender & appender);
    
    /** remove an appender */
    void remove_appender(ft_log_appender & appender);
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_LOG_HH */
