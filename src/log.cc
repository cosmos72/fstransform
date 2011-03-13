/*
 * log.cc
 *
 *  Created on: Mar 8, 2011
 *      Author: max
 */

#include "first.hh"

#include <cstdio>    // for FILE *, stdout, stderr
#include <cstdarg>   // for va_list and va_{start,end,copy}()
#include <cstring>   // for strerror(), strncmp()
#include <ctime>     // for time(), localtime_r(), localtime(), strftime()

#include <map>       // for std::map<K,V>

#include "types.hh"  // for ft_size
#include "log.hh"    // for ff_log(), ff_vlog() ...


#if defined(FT_HAVE_VA_COPY) || defined(va_copy)
#  define ff_va_copy va_copy
#elif defined(__va_copy)
#  define ff_va_copy __va_copy
#endif

FT_EXTERN_C_BEGIN
FT_NAMESPACE_BEGIN


/* by default, all messages less serious than 'FC_INFO' are suppressed */
static ft_log_level fm_log_threshold = FC_INFO;

static std::map<FILE *, ft_log_fmt> fm_log_stream[FC_FATAL+1];

static char const* const fm_log_label[FC_FATAL+1] =
{
     "TRACE ", "DEBUG ", "INFO  ", "NOTICE", "WARN  ", "ERROR ", "FATAL ",
};

static char const* const fm_log_label_always[FC_FATAL+1] =
{
		"", "", "", "", "WARN", "ERROR", "FATAL ERROR",
};



static bool fm_log_initialized = false;

/**
 * initialize log subsystem:
 * configure stderr to receive all WARN messages or more serious,
 * configure stdout to receive all NOTICE messages or less serious
 */
static void ff_log_init()
{
    if (!fm_log_initialized) {
        fm_log_initialized = true;
#ifdef FT_HAVE_LOCALTIME_R
        tzset();
#endif
        (void) setvbuf(stdout, NULL, _IOLBF, 0);
        (void) setvbuf(stderr, NULL, _IOLBF, 0);

        ff_log_register(stdout, FC_FMT_MSG, FC_TRACE, FC_NOTICE);
        ff_log_register(stderr, FC_FMT_MSG, FC_WARN, FC_FATAL);
    }
}


/**
 * return true if printing specified message level is not suppressed
 */
bool ff_log_is_enabled(ft_log_level level)
{
    return level >= fm_log_threshold;
}

/**
 * tell ff_log() and ff_vlog() to suppress printing of messages less serious than 'level'.
 *
 * by default, all messages less serious than 'FC_INFO' are suppressed
 */
void ff_log_set_threshold(ft_log_level level)
{
    /*
     * note 1.1)
     * log subsystem is automatically initialized upon first call to
     * ff_log(), ff_vlog(), ff_log_register() or ff_log_set_threshold().
     */
    if (!fm_log_initialized)
        ff_log_init();

    fm_log_threshold = level;
}


/**
 * add 'stream' to the list of streams receiving log messages
 * with seriousness between min_level and max_level (inclusive)
 *
 * by default, all WARN messages or more serious are sent to stderr
 * and all NOTICE messages or less serious are sent to stdout
 * note: by default, messages less serious than INFO are suppressed, see ff_log_set_threshold()
 */
void ff_log_register(FILE * f, ft_log_fmt format, ft_log_level min_level, ft_log_level max_level)
{
    /*
     * note 1.1)
     * log subsystem is automatically initialized upon first call to
     * ff_log(), ff_vlog(), ff_log_register() or ff_log_set_threshold().
     */
    if (!fm_log_initialized)
        ff_log_init();

    ft_size i = (ft_size) min_level, n = (ft_size) max_level;
    while (i <= n)
        fm_log_stream[i++][f] = format;
}


/**
 * remove 'stream' from the list of streams receiving log messages
 * with seriousness between min_level and max_level (inclusive)
 */
void ff_log_unregister(FILE * f, ft_log_level min_level, ft_log_level max_level)
{
    ft_size i = (ft_size) min_level, n = (ft_size) max_level;
    while (i <= n)
        fm_log_stream[i++].erase(f);
}


struct ft_log_args
{
    const char * str_now, * file, * file_suffix, * function, * fmt;
    int file_len, line, err;
    ft_log_level level;
    va_list vargs;
};

/**
 * print fmt and subsequent printf-style args to log stream.
 * if err != 0, append ": ", strerror(errno) and "\n"
 * else append "\n"
 */
static void ff_log0(FILE * f, ft_log_fmt format, ft_log_args & args)
{
    switch (format) {
        case FC_FMT_DATETIME_LEVEL_CALLER_MSG:
            fprintf(f, "%s %s [%.*s%s.%s(%d)] ", args.str_now, fm_log_label[args.level],
                    args.file_len, args.file, args.file_suffix, args.function, args.line);
            break;
        case FC_FMT_DATETIME_LEVEL_MSG:
            fprintf(f, "%s %s ", args.str_now, fm_log_label[args.level]);
            break;
        case FC_FMT_LEVEL_MSG:
            fprintf(f, "%s ", fm_log_label[args.level]);
            break;
        case FC_FMT_MSG:
        default:
        	if (args.level >= FC_WARN)
        		/* always mark warnings, errors and fatal errors as such */
        		fprintf(f, "%s: ", fm_log_label_always[args.level]);
            break;
    }

    vfprintf(f, args.fmt, args.vargs);

    if (args.err != 0)
        fprintf(f, ( ff_log_is_reported(args.err)
            ? " (caused by previous error: %s)\n"
            : ": %s\n"
        ), strerror(args.err));
    else
        fputc('\n', f);
}



static const char * ff_strftime();
static void ff_pretty_file(ft_log_args & args);

/**
 * print fmt and subsequent printf-style args to log stream(s).
 * if err != 0, append ": ", strerror(errno) and "\n"
 * else append "\n"
 * finally return err
 */
int ff_logl(const char * file, const char * func, int line, ft_log_level level, int err, const char * fmt, ...)
{
    /*
     * note 1.1)
     * log subsystem is automatically initialized upon first call to
     * ff_log(), ff_vlog(), ff_log_register() or ff_log_set_threshold().
     */
    if (!fm_log_initialized)
        ff_log_init();

    do {
        if (level < fm_log_threshold)
            break;

        std::map<FILE *, ft_log_fmt>::const_iterator
            iter = fm_log_stream[level].begin(),
            end  = fm_log_stream[level].end();
        if (iter == end)
            break;

        ft_log_args args = {
            ff_strftime(), file, "", func, fmt,
            0, line, err,
            level,
            /* va_list vargs */
        };
        ff_pretty_file(args);

        /* iterate on streams configured for 'level' */
        for (; iter != end; ++iter) {
            va_start(args.vargs, fmt);
            ff_log0(iter->first, iter->second, args);
            va_end(args.vargs);
        }
    } while (0);

    /* note 1.2.1) ff_log() and ff_vlog() always return errors as reported (-EINVAL, -ENOMEM...) */
    return ff_log_is_reported(err) ? err : -err;
}




#ifdef ff_va_copy
/**
 * print to log fmt and subsequent printf-style args log stream(s).
 * if err != 0, append ": ", strerror(errno) and "\n"
 * else append "\n"
 * finally return err as reported (flip sign if it was unreported)
 */
int ff_logv(const char * file, const char * func, int line, ft_log_level level, int err, const char * fmt, va_list vargs)
{
    /*
     * note 1.1)
     * log subsystem is automatically initialized upon first call to
     * ff_log(), ff_vlog(), ff_log_register() or ff_log_set_threshold().
     */
    if (!fm_log_initialized)
        ff_log_init();

    do {
        if (level < fm_log_threshold)
            break;

        std::map<FILE *, ft_log_fmt>::const_iterator
            iter = fm_log_stream[level].begin(),
            end  = fm_log_stream[level].end();
        if (iter == end)
            break;

        ft_log_args args = {
            ff_strftime(), file, "", func, fmt,
            0, line, err,
            level,
            /* va_list vargs */
        };
        ff_pretty_file(args);

        /* iterate on streams configured for 'level' */
        for (; iter != end; ++iter) {
            ff_va_copy(args.vargs, vargs);
            ff_log0(iter->first, iter->second, args);
            va_end(args.vargs);
        }
    } while (0);

    /* note 1.2.1) ff_log() and ff_vlog() always return errors as reported (-EINVAL, -ENOMEM...) */
    return ff_log_is_reported(err) ? err : -err;
}
#else
#  warning va_copy() and __va_copy() not found, ff_vlog() will not be compiled
#endif



/**
 * flush all buffered streams used to log messages of specified level
 */
void ff_log_flush(ft_log_level level)
{
    if (level >= fm_log_threshold) {
        std::map<FILE *, ft_log_fmt>::const_iterator
            iter = fm_log_stream[level].begin(),
            end  = fm_log_stream[level].end();

        /* iterate on streams configured for 'level' */
        for (; iter != end; ++iter) {
            fflush(iter->first);
        }
    }
}


enum { FC_SIZEOF_STR_NOW = 21 + 3*(sizeof(time_t)-4) };

static char fm_str_now[FC_SIZEOF_STR_NOW];
static time_t fm_time_now;


/**
 * compute formatted timestamp for current time and return it in a static buffer.
 * used format is "%Y-%m-%d %H:%M:%S"
 */
static const char * ff_strftime()
{
    time_t now = time(NULL);
    if (now != fm_time_now) {
#ifdef FT_HAVE_LOCALTIME_R
        struct tm tm_buf, * tm_now = & tm_buf;
        localtime_r(& now, tm_now);
#else
        struct tm * tm_now = localtime(& now);
#endif
        strftime(fm_str_now, FC_SIZEOF_STR_NOW, "%Y-%m-%d %H:%M:%S", tm_now);
        fm_str_now[FC_SIZEOF_STR_NOW - 1] = '\0';
        fm_time_now = now;
    } else {
        // cached fm_str_now is still good
    }
    return fm_str_now;
}

/**
 * compute pretty-print version of args.file (i.e. __FILE__):
 * skip any '../' or 'src/' prefix,
 * skip filename extension (for example .cc or .hh)
 * and replace final '.t' with '<T>'
 */
static void ff_pretty_file(ft_log_args & args)
{
    const char * file = args.file;
    if (!strncmp(file, "../", 3))
        file += 3;
    if (!strncmp(file, "src/", 3))
        file += 4;
    ft_size file_len = strlen(file);

    /** skip file extension, usually .cc or .hh */
    const char * dot = (const char *) memrchr(file, '.', file_len);
    if (dot != NULL)
        file_len = dot - file;

    /** if file name ends with .t then replace with <T> */
    if (file_len >= 2 && !strncmp(file + file_len - 2, ".t", 2)) {
        file_len -= 2;
        args.file_suffix = "<T>";
    }
    args.file = file;
    /* conversion ft_size -> int: check for overflow, even it may seem silly for a file name */
    args.file_len = file_len;
    if (args.file_len < 0 || file_len != (ft_size) args.file_len)
        args.file_len = 255;
}


FT_NAMESPACE_END
FT_EXTERN_C_END
