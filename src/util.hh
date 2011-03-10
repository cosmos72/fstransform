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

const char * ff_pretty_size(ft_uoff len, double * ret_pretty_len);

int ff_str2uoff(const char * str, ft_uoff * ret_n);

/** convert string to unsigned number */
template<typename T>
int ff_str2un(const char * str, T * ret_n)
{
    ft_uoff buf_n;
    int err = ff_str2uoff(str, & buf_n);
    if (err == 0) {
        T n = (T) buf_n;
        if (n < 0 || buf_n != (ft_uoff) n)
            err = EINVAL;
        else
            * ret_n = n;
    }
    return err;
}

FT_NAMESPACE_END

#endif /* FSTRANSFORM_UTIL_HH */
