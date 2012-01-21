/*
 * util.hh
 *
 *  Created on: Mar 9, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_UTIL_HH
#define FSTRANSFORM_UTIL_HH

#include <cerrno>     // for EINVAL

#include "types.hh"   // for ft_size, ft_uoff

FT_NAMESPACE_BEGIN

template<typename T>
FT_INLINE T ff_min2(T a, T b)
{
    return a < b ? a : b;
}

template<typename T>
FT_INLINE T ff_min3(T a, T b, T c)
{
    return ff_min2(ff_min2(a, b), c);
}

template<typename T>
FT_INLINE T ff_max2(T a, T b)
{
    return a > b ? a : b;
}

template<typename T>
FT_INLINE T ff_max3(T a, T b, T c)
{
    return ff_max2(ff_max2(a, b), c);
}

/**
 * is it possible to sum a and b (a+b), or it will overflow?
 * note: T must be unsigned!
 */
template<typename T>
FT_INLINE bool ff_can_sum(T a, T b)
{
    return a <= ~b;
}



/** convert string to unsigned number */
int ff_str2ull(const char * str, ft_ull * ret_n);

/** convert string with optional [k|M|G|T|P|E|Z|Y] scale to unsigned number */
int ff_str2ull_scaled(const char * str, ft_ull * ret_n);



/** convert ft_ull to T, or return EOVERFLOW if it overflows */
template<typename T>
int ff_narrow(ft_ull ln, T * ret_n)
{
    T n = (T) ln;
    if (n < 0 || ln != (ft_ull) n)
        return EOVERFLOW;
    * ret_n = n;
    return 0;
}

/** convert string to unsigned number */
template<typename T>
int ff_str2un(const char * str, T * ret_n)
{
    ft_ull ln;
    int err = ff_str2ull(str, & ln);
    if (err == 0)
        err = ff_narrow(ln, ret_n);
    return err;
}


/** convert string with optional [k|M|G|T|P|E|Z|Y] scale to unsigned number */
template<typename T>
int ff_str2un_scaled(const char * str, T * ret_n)
{
    ft_ull ln;
    int err = ff_str2ull_scaled(str, & ln);
    if (err == 0)
        err = ff_narrow(ln, ret_n);
    return err;
}



void ff_init_random(ft_ull seed);

/** return a random number in the range [0,max] */
ft_ull ff_random(ft_ull max);




/** return p != NULL ? p : default_p */
template<typename T>
FT_INLINE T * ff_if_null(T * p, T * default_p) { return p != NULL ? p : default_p; }




/**
 * return human-readable representation of length,
 * with [kilo|mega|giga|tera|peta|exa|zeta|yotta] scale as appropriate
 */
const char * ff_pretty_size(ft_uoff length, double * ret_pretty_len);


/**
 * return human-readable representation of time,
 * with [second|minute|hour|day|month|year] scale as appropriate
 */
const char * ff_pretty_time(double time, double * ret_pretty_time);

/**
 * return human-readable representation of time,
 * with [second|minute|hour|day|month|year] scale as appropriate
 */
void ff_pretty_time2(double time,
		ft_ull * ret_pretty_time1, char const ** ret_pretty_label1,
		ft_ull * ret_pretty_time2 = NULL, char const ** ret_pretty_label2 = NULL);


/**
 * return approximate number, rounding to "one-and-a-half" significant digits.
 * if t <= 10, return (ft_ull)(t + 0.5)
 * if t <= 30, return ((ft_ull)(t*5 + 0.5)) / 5
 * if t <= 100, return ((ft_ull)(t*10 + 0.5)) / 10
 * if t <= 300, return ((ft_ull)(t*50 + 0.5)) / 50
 * otherwise return ((ft_ull)(t*100 + 0.5)) / 100
 */
ft_ull ff_pretty_number(double t);


FT_NAMESPACE_END

#endif /* FSTRANSFORM_UTIL_HH */
