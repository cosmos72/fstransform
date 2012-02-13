/*
 * misc.cc
 *
 *  Created on: Mar 9, 2011
 *      Author: max
 */

#include "first.hh"

#if defined(FT_HAVE_STDLIB_H)
# include <stdlib.h>   // for srandom(), random(), rand(), srand()
#elif defined(FT_HAVE_CSTDLIB)
# include <cstdlib>    // for srandom(), random(), rand(), srand()
#endif
#if defined(FT_HAVE_TIME_H)
# include <time.h>     // for time()
#elif defined(FT_HAVE_CTIME)
# include <ctime>      // for time()
#endif

#ifdef FT_HAVE_SYS_TIME_H
# include <sys/time.h>   // for struct timeval, gettimeofday()
#endif


#include "misc.hh"    // declarations of the functions defined here

FT_NAMESPACE_BEGIN

static const ft_ull max_n_div_10 = (ft_ull)-1 / 10;
const char max_n_rem_10 = (char)((ft_ull)-1 % 10);

static int ff_str2ull_(const char *& str, ft_ull * ret_n)
{
    ft_ull n = 0;
    char ch;
    while ((ch = *str) >= '0' && ch <= '9') {
        str++;
        ch -= '0';
        if (n < max_n_div_10 || (n == max_n_div_10 && ch <= max_n_rem_10)) {
            n *= 10;
            n += ch;
        } else
            return EOVERFLOW;
    }
    * ret_n = n;
    return 0;
}

int ff_str2ull(const char * str, ft_ull * ret_n)
{
    ft_ull n = 0;
    int err = ff_str2ull_(str, & n);
    if (err == 0 && *str != '\0')
        return EINVAL;
    if (err == 0)
        * ret_n = n;
    return err;
}


/** convert string with optional [k|M|G|T|P|E|Z|Y] scale to unsigned number */
int ff_str2ull_scaled(const char * str, ft_ull * ret_n)
{
    ft_ull scale, n = 0;
    int err;
    do {
        if ((err = ff_str2ull_(str, & n)) != 0)
            break;

        switch (*str) {
            case '\0': scale = 0; * ret_n = n; return err;
            case 'k': scale = 10; break;
            case 'M': scale = 20; break;
            case 'G': scale = 30; break;
            case 'T': scale = 40; break;
            case 'P': scale = 50; break;
            case 'E': scale = 60; break;
            case 'Z': scale = 70; break;
            case 'Y': scale = 80; break;
            default: err = EINVAL; break;
        }
        if (err != 0)
            break;

        /* overflow? */
        if (scale >= 8*sizeof(ft_ull) || n > (ft_ull)-1 >> scale) {
            err = EOVERFLOW;
            break;
        }
        n <<= scale;
        * ret_n = n;
    } while (0);
    return err;
}


int ff_now(double & ret_time) {
    int err;
#ifdef FT_HAVE_GETTIMEOFDAY
    struct timeval tv;
    if ((err = gettimeofday(& tv, NULL)) == 0) {
        ret_time = (double) tv.tv_sec + (double) tv.tv_usec * 1e-6;
        return err;
    }
    /* error in gettimeofday(), fall back on time() */
#else
# warning gettimeofday() not found on this platform. timestamps and "time left" estimations will be less accurate
#endif
    errno = err = 0;
    time_t now = time(NULL);
    if (now == (time_t)-1)
        err = -errno;
    if (err == 0)
        ret_time = (double) now;
    return err;
}



#if defined(FT_HAVE_SRANDOM) && defined(FT_HAVE_RANDOM)
# define ff_misc_random_init(seed) srandom(seed)
# define ff_misc_random()          random()
#else
# define ff_misc_random_init(seed) srand(seed)
# define ff_misc_random()          rand()
#endif

/** return a random number in the range [0,n] */
ft_ull ff_random(ft_ull n)
{
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        ff_misc_random_init(time(NULL));
    }

    ft_ull r;
    if (n == 0)
        return 0;
    if (n < RAND_MAX) {
        ft_ull max = RAND_MAX - (RAND_MAX % (n + 1));
        do {
            r = ff_misc_random();
        } while (r > max);
        return r / (max / (n + 1));
    }
    if (n == RAND_MAX)
        return ff_misc_random();
    const ft_ull max_p_1 = (ft_ull)RAND_MAX + 1;
    const ft_ull n_hi = (n + RAND_MAX) / max_p_1;
    do {
        r = ff_random(n_hi) * max_p_1 + ff_misc_random();
    } while (r > n);
    return r;
}







static char const* fc_pretty_size_unit[] = {
    "", "kilo", "mega", "giga", "tera", "peta", "exa", "zeta", "yotta",
};

enum { fc_pretty_size_len = sizeof(fc_pretty_size_unit)/sizeof(fc_pretty_size_unit[0]) };

/**
 * return human-readable representation of len,
 * with [kilo|mega|giga|tera|peta|exa|zeta|yotta] scale as appropriate
 */
const char * ff_pretty_size(ft_uoff len, double * ret_pretty_len)
{
    double pretty_len = (double) len;
    ft_size i;
    for (i = 0; i < fc_pretty_size_len-1 && pretty_len >= 1024.0; i++)
        pretty_len *= .0009765625; // exactly 1/1024
    * ret_pretty_len = pretty_len;
    return fc_pretty_size_unit[i];
}






static char const* fc_pretty_time_unit[] = {
    "second", "minute", "hour", "day", "month", "year",
};
static const double fc_pretty_time[] = {
    1.0, 60.0, 3600.0, 86400.0, 86400.0 * 30, 86400.0 * 365,
};

enum { fc_pretty_time_len = sizeof(fc_pretty_time)/sizeof(fc_pretty_time[0]) };

/**
 * return human-readable representation of time,
 * with [second|minute|hour|day|month|year] scale as appropriate
 */
char const * ff_pretty_time(double time, double * ret_pretty_time)
{
    ft_size i = 0;
    for (; i < fc_pretty_time_len - 1; i++) {
        if (time < fc_pretty_time[i+1])
            break;
    }
    * ret_pretty_time = time / fc_pretty_time[i];
    return fc_pretty_time_unit[i];
}

/**
 * return human-readable representation of time,
 * with [second|minute|hour|day|month|year] scale as appropriate
 */
void ff_pretty_time2(double time,
        ft_ull * ret_pretty_time1, char const ** ret_pretty_label1,
        ft_ull * ret_pretty_time2, char const ** ret_pretty_label2)
{
    ft_size i = 0;
    for (; i < fc_pretty_time_len - 1; i++) {
        if (time < fc_pretty_time[i+1])
            break;
    }
    double time1 = time / fc_pretty_time[i];

    * ret_pretty_time1 = ff_pretty_number(time1);
    * ret_pretty_label1 = fc_pretty_time_unit[i];
    if (ret_pretty_time2 == NULL || ret_pretty_label2 == NULL)
        return;

    * ret_pretty_time2 = 0;
    * ret_pretty_label2 = NULL;

    if (i == 0 || time1 <= 1.0 || time1 > 1.9)
        return;

    time -= (ft_ull) time1 * fc_pretty_time[i];
    if (time <= 0.0)
        return;

    ft_size j = 0;
    for (j = 0; j < fc_pretty_time_len - 1; j++) {
        if (time < fc_pretty_time[j+1])
            break;
    }
    if (j + 1 == i) {
        * ret_pretty_time1 = 1;
        * ret_pretty_time2 = ff_pretty_number(time / fc_pretty_time[j]);
        * ret_pretty_label2 = fc_pretty_time_unit[j];
    }
}

/**
 * return approximate number, rounding to "one-and-a-half" significant digits.
 * if t <= 10, return (ft_ull)(t + 0.5)
 * if t <= 30, return ((ft_ull)(t/5 + 0.5)) * 5
 * if t <= 100, return ((ft_ull)(t/10 + 0.5)) * 10
 * if t <= 300, return ((ft_ull)(t/50 + 0.5)) * 50
 * otherwise return ((ft_ull)(t/100 + 0.5)) * 100
 */
ft_ull ff_pretty_number(double t) {
    ft_ull n;
    if (t <= 10.0)
        n = (ft_ull)(t + 0.5);
    else if (t <= 30.0)
        n = 5 * (ft_ull)(t*0.2 + 0.5);
    else if (t <= 100.0)
        n = 10 * (ft_ull)(t*0.1 + 0.5);
    else if (t <= 300.0)
        n = 50 * (ft_ull)(t*0.02 + 0.5);
    else
        n = 100 * (ft_ull)(t*0.01 + 0.5);
    return n;
}

/**
 * Show progress. logs a message like
 * {prefix}progress: {percentage}% done, {bytes_left} bytes{suffix}, estimated {time_left} left"
 *
 * if time_left < 0, omits the part "estimated {time_left} left"
 */
void ff_show_progressl(const char * caller_file, const char * caller_func, int caller_line,
        ft_log_level log_level, const char * prefix, double percentage,
        ft_uoff bytes_left, const char * suffix, double time_left)
{
    double pretty_len = 0.0;
    const char * pretty_label = ff_pretty_size(bytes_left, & pretty_len);

    if (time_left < 0) {
        ff_log(log_level, 0, "%sprogress: %4.1f%% done, %5.1f %sbytes%s",
                prefix, percentage, pretty_len, pretty_label, suffix);
        return;
    }

    ft_ull time_left1 = 0, time_left2 = 0;
    const char * time_left_label1 = NULL, * time_left_label2 = NULL;

    ff_pretty_time2(time_left, & time_left1, & time_left_label1, & time_left2, & time_left_label2);

    /* we write something like "1 hour and 20 minutes" instead of just "1 hour" or "1.3 hours" */
    if (time_left_label2 != NULL) {
        ff_logl(caller_file, caller_func, caller_line,
                log_level, 0, "%sprogress: %4.1f%% done, %5.1f %sbytes%s, estimated %2"FT_ULL" %s%s and %2"FT_ULL" %s%s left",
                prefix, percentage, pretty_len, pretty_label, suffix,
                time_left1, time_left_label1, (time_left1 != 1 ? "s": ""),
                time_left2, time_left_label2, (time_left2 != 1 ? "s": ""));
    } else {
        ff_logl(caller_file, caller_func, caller_line,
                log_level, 0, "%sprogress: %4.1f%% done, %5.1f %sbytes%s, estimated %2"FT_ULL" %s%s left",
                prefix, percentage, pretty_len, pretty_label, suffix,
                time_left1, time_left_label1, (time_left1 != 1 ? "s": ""));
    }
}

FT_NAMESPACE_END
