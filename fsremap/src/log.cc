/*
 * log.cc
 *
 *  Created on: Mar 8, 2011
 *      Author: max
 */

#include "first.hh"

#if defined(FT_HAVE_STRING_H)
# include <string.h>     // for strerror(), strncmp()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>      // for strerror(), strncmp()
#endif
#if defined(FT_HAVE_TIME_H)
# include <time.h>       // for time(), localtime_r(), localtime(), strftime()
#elif defined(FT_HAVE_CTIME)
# include <ctime>        // for time(), localtime_r(), localtime(), strftime()
#endif

#include <utility>       // for std::make_pair()

#include "types.hh"      // for ft_size
#include "log.hh"        // for ff_log(), ff_vlog() ...


#if defined(FT_HAVE_VA_COPY) || defined(va_copy)
#  define ff_va_copy va_copy
#elif defined(__va_copy)
#  define ff_va_copy __va_copy
#else
#  error both va_copy() and __va_copy() are missing, cannot compile log.cc
#endif

FT_NAMESPACE_BEGIN

static char const* const this_log_label[FC_FATAL+1] =
{
     "", "DUMP  ", "TRACE ", "DEBUG ", "INFO  ", "NOTICE", "WARN  ", "ERROR ", "FATAL ",
};

static char const* const this_log_label_always[FC_FATAL+1] =
{
    "", "", "", "", "", "", "WARN: ", "ERROR: ", "FATAL: ",
};




static std::list<ft_log_appender *> * fc_log_all_appenders = NULL;
static std::map<ft_string, ft_log *> * fc_log_all_loggers = NULL;
static ft_log * fc_log_root_logger = NULL;
static bool fc_log_initialized = false;


/** list of all appenders */
std::list<ft_log_appender *> & ft_log_appender::get_all_appenders()
{
    if (!fc_log_initialized)
    	ft_log::initialize();
    return * fc_log_all_appenders;
}




/** constructor. */
ft_log_appender::ft_log_appender(FILE * my_stream, ft_log_fmt my_format, ft_log_level my_min_level, ft_log_level my_max_level)
    : stream(my_stream), format(my_format), min_level(my_min_level), max_level(my_max_level)
{
    get_all_appenders().push_back(this);
}

/** destructor. */
ft_log_appender::~ft_log_appender()
{
    get_all_appenders().remove(this);
}



/**
 * write a log message to stream.
 * 
 * print fmt and subsequent printf-style args to log stream.
 * if err != 0, append ": ", strerror(errno) and "\n"
 * else append "\n"
 */
void ft_log_appender::append(ft_log_event & event)
{
    const ft_log_level level = event.level;
    if (level < min_level || level > max_level)
        return;
        
    switch (format) {
        case FC_FMT_DATETIME_LEVEL_CALLER_MSG:
            fprintf(stream, "%s %s [%.*s%s.%s(%d)] ", event.str_now, this_log_label[level],
                    event.file_len, event.file, event.file_suffix, event.function, event.line);
            break;
        case FC_FMT_DATETIME_LEVEL_MSG:
            fprintf(stream, "%s %s ", event.str_now, this_log_label[level]);
            break;
        case FC_FMT_LEVEL_MSG:
            fprintf(stream, "%s ", this_log_label[level]);
            break;
        case FC_FMT_MSG:
        default:
            /* always mark warnings, errors and fatal errors as such */
            fprintf(stream, "%s", this_log_label_always[level]);
            break;
    }

    va_list vargs;
    ff_va_copy(vargs, event.vargs);
    vfprintf(stream, event.fmt, vargs);
    va_end(vargs);

    if (event.err != 0) {
        bool is_reported = ff_log_is_reported(event.err);
        fprintf(stream, is_reported ? " (caused by previous error: %s)\n" : ": %s\n", strerror(is_reported ? -event.err : event.err));
    } else
        fputc('\n', stream);
}

/** flush this appender */
void ft_log_appender::flush()
{
    fflush(stream);
}


/** flush all buffered streams used to log messages for specified level */
void ft_log_appender::flush_all(ft_log_level level)
{
    std::list<ft_log_appender *> & all_appenders = get_all_appenders();
    std::list<ft_log_appender *>::const_iterator iter = all_appenders.begin(), end = all_appenders.end();

    /* iterate on streams configured for 'level' */
    for (; iter != end; ++iter) {
        ft_log_appender * appender = *iter;
        if (appender->min_level <= level && appender->max_level >= level)
            appender->flush();
    }
}

/** set format and min/max levels of this appender */
void ft_log_appender::redefine(ft_log_fmt format, ft_log_level min_level, ft_log_level max_level)
{
	this->format = format;
	this->min_level = min_level;
	this->max_level = max_level;
}

/** set format and min/max levels of all appenders attached to stream */
void ft_log_appender::redefine(FILE * stream, ft_log_fmt format, ft_log_level min_level, ft_log_level max_level)
{
    if (!fc_log_initialized)
    	ft_log::initialize();

    std::list<ft_log_appender *> & all_appenders = get_all_appenders();
    std::list<ft_log_appender *>::const_iterator iter = all_appenders.begin(), end = all_appenders.end();
    
    for (; iter != end; ++iter) {
        ft_log_appender * appender = *iter;
        if (appender->stream == stream)
            appender->redefine(format, min_level, max_level);
    }
}








/** return map of all existing loggers. */
std::map<ft_string, ft_log *> & ft_log::get_all_loggers()
{
    if (!fc_log_initialized)
    	ft_log::initialize();
    return * fc_log_all_loggers;
}

/** return root logger. by default, all messages less serious than 'FC_INFO' are suppressed on all streams */
ft_log & ft_log::get_root_logger()
{
    if (!fc_log_initialized)
    	ft_log::initialize();
    return * fc_log_root_logger;
}



/**
 * initialize log subsystem. automatic configuration is:
 *
 * print to stderr all INFO and NOTICE messages, with format FC_FMT_MSG
 * print to stdout all WARN, ERROR and FATAL messages, with format FC_FMT_MSG
 */
void ft_log::initialize()
{
    if (fc_log_initialized)
        return;
    
    fc_log_initialized = true;

    if (fc_log_all_appenders == NULL)
    	fc_log_all_appenders = new std::list<ft_log_appender *>();
    if (fc_log_all_loggers == NULL)
    	fc_log_all_loggers = new std::map<ft_string, ft_log *>();
    if (fc_log_root_logger == NULL)
    	fc_log_root_logger = new ft_log("", NULL, FC_INFO);

    ft_log & root_logger = ft_log::get_root_logger();
#ifdef FT_HAVE_TZSET
    tzset();
#endif
    (void) setvbuf(stdout, NULL, _IOLBF, 0);
    (void) setvbuf(stderr, NULL, _IOLBF, 0);

    root_logger.add_appender(* new ft_log_appender(stdout, FC_FMT_MSG, FC_INFO, FC_NOTICE));
    root_logger.add_appender(* new ft_log_appender(stderr, FC_FMT_MSG, FC_WARN));
}


/** constructor. */
ft_log::ft_log(const ft_string & my_name, ft_log * my_parent, ft_log_level my_level)
    : parent(my_parent), appenders(), level(my_level)
{
    std::pair<all_loggers_iterator, bool> iter_pair = get_all_loggers().insert(std::make_pair(my_name, this));
    name = & iter_pair.first->first;
}

/** destructor. */
ft_log::~ft_log()
{
    get_all_loggers().erase(* name);
    if (parent == NULL)
        ft_log_appender::flush_all(FC_DUMP);
}


/** find or create parent logger given child name. */
ft_log & ft_log::get_parent(const ft_string & child_logger_name)
{
    ft_size slash = child_logger_name.rfind('/');
    if (slash == ft_string::npos)
        return get_root_logger();

    ft_string parent_name(child_logger_name, 0, slash);
    return get_logger(parent_name);
}

/** find or create a logger by name */
ft_log & ft_log::get_logger(const ft_string & logger_name)
{
    std::map<ft_string, ft_log *> & all_loggers = get_all_loggers();
    all_loggers_iterator iter = all_loggers.find(logger_name);
    if (iter != all_loggers.end())
        return * iter->second;
        
    ft_log & parent = get_parent(logger_name);

    return * new ft_log(logger_name, & parent);
}

/** log a message (skip level check) */
void ft_log::append(ft_log_event & event)
{
    ft_log * logger = this;
    do {
        std::list<ft_log_appender *>::const_iterator iter = logger->appenders.begin(), end = logger->appenders.end();
        for (; iter != end; ++iter)
            (* iter)->append(event);

        logger = logger->parent;
    } while (logger != NULL);
}


/** log a message (unless it's suppressed) */
void ft_log::log(ft_log_event & event)
{
    ft_log_level level = get_effective_level();
    
    if (event.level >= level)
        append(event);
}

/** return the effective level: if level is set return it, otherwise return parent effective level. */
ft_log_level ft_log::get_effective_level() const
{
    const ft_log * logger = this;
    ft_log_level level;
    do {
        level = logger->level;
        logger = logger->parent;
    } while (level == FC_LEVEL_NOT_SET && logger != NULL);

    return level;
}

/** add an appender */
void ft_log::add_appender(ft_log_appender & appender)
{
    appenders.push_back(& appender);
}
    
/** remove an appender */
void ft_log::remove_appender(ft_log_appender & appender)
{
    // remove last occurrence of appender
    std::list<ft_log_appender *>::iterator begin = appenders.begin(), iter = appenders.end();
    while (iter != begin) {
        if (*--iter == & appender) {
            appender.flush();
            appenders.erase(iter);
            return;
        }
    }
}


static const char * ff_strftime();
static void ff_pretty_file(ft_log_event & event);


bool ff_logl_is_enabled(const char * caller_file, ft_log_level level)
{
    ft_log_event event = {
        "", caller_file, "", "", "",
        0, 0, 0,
        level,
        /* va_list vargs */
    };
    ff_pretty_file(event);
    
    ft_string logger_name(event.file, event.file_len);
    ft_log & logger = ft_log::get_logger(logger_name);
    
    return logger.is_enabled(level);
}


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
    ft_log_event event = {
        ff_strftime(), file, "", func, fmt,
        0, line, err,
        level,
        /* va_list vargs */
    };
    ff_pretty_file(event);

    ft_string logger_name(event.file, event.file_len);
    ft_log & logger = ft_log::get_logger(logger_name);
    
    va_start(event.vargs, fmt);
    logger.log(event);
    va_end(event.vargs);
    
    /* note 1.2.1) ff_log() and ff_vlog() always return errors as reported (-EINVAL, -ENOMEM...) */
    return ff_log_is_reported(err) ? err : -err;
}




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
    ft_log_event event = {
        ff_strftime(), file, "", func, fmt,
        0, line, err,
        level,
        /* va_list vargs */
    };
    ff_pretty_file(event);

    ft_string logger_name(event.file, event.file_len);
    ft_log & logger = ft_log::get_logger(logger_name);
    
    ff_va_copy(event.vargs, vargs);
    logger.log(event);
    va_end(event.vargs);
    
    /* note 1.2.1) ff_log() and ff_vlog() always return errors as reported (-EINVAL, -ENOMEM...) */
    return ff_log_is_reported(err) ? err : -err;
}



enum { FC_SIZEOF_STR_NOW = 21 + 3*(sizeof(time_t)-4) };

static char this_str_now[FC_SIZEOF_STR_NOW];
static time_t this_time_now;


/**
 * compute formatted timestamp for current time and return it in a static buffer.
 * used format is "%Y-%m-%d %H:%M:%S"
 */
static const char * ff_strftime()
{
    time_t now = time(NULL);
    if (now != this_time_now) {
#ifdef FT_HAVE_LOCALTIME_R
        struct tm tm_buf, * tm_now = & tm_buf;
        localtime_r(& now, tm_now);
#else
        struct tm * tm_now = localtime(& now);
#endif
        strftime(this_str_now, FC_SIZEOF_STR_NOW, "%Y-%m-%d %H:%M:%S", tm_now);
        this_str_now[FC_SIZEOF_STR_NOW - 1] = '\0';
        this_time_now = now;
    } else {
        // cached this_str_now is still good
    }
    return this_str_now;
}

/**
 * compute pretty-print version of args.file (i.e. __FILE__):
 * skip any '../' or 'src/' prefix,
 * skip filename extension (for example .cc or .hh)
 * and replace final '.t' with '<T>'
 */
static void ff_pretty_file(ft_log_event & event)
{
    const char * file = event.file;
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
        event.file_suffix = "<T>";
    }
    event.file = file;
    /* conversion ft_size -> int: check for overflow, even it may seem silly for a file name */
    event.file_len = file_len;
    if (event.file_len < 0 || file_len != (ft_size) event.file_len)
        event.file_len = 255;
}


FT_NAMESPACE_END
