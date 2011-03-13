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
static FT_INLINE T ff_min2(T a, T b)
{
    return a < b ? a : b;
}

template<typename T>
static FT_INLINE T ff_max2(T a, T b)
{
    return a > b ? a : b;
}

/**
 * return human-readable representation of len,
 * with [kilo|mega|giga|tera|peta|exa|zeta|yotta] scale as appropriate
 */
const char * ff_pretty_size(ft_uoff len, double * ret_pretty_len);


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

FT_NAMESPACE_END

#endif /* FSTRANSFORM_UTIL_HH */
