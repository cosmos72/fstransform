/*
 * util.cc
 *
 *  Created on: Mar 9, 2011
 *      Author: max
 */

#include "first.hh"

#include "util.hh"    // for ff_pretty_size(), ff_strtoull()

FT_NAMESPACE_BEGIN


int ff_str2uoff(const char * str, ft_uoff * ret_n)
{
    ft_uoff n = 0;
    char ch;
    while ((ch = *str++) >= '0' && ch <= '9') {
        n *= 10;
        n += (ft_uoff) (ch - '0');
    }
    if (ch == '\0') {
        * ret_n = n;
        return 0;
    }
    return EINVAL;
}




static char const* fc_pretty_size[] = {
        "", "kilo", "mega", "giga", "tera", "peta", "exa", "zeta", "yotta"
};

enum { fc_pretty_size_len = sizeof(fc_pretty_size)/sizeof(fc_pretty_size[0]) };

const char * ff_pretty_size(ft_uoff len, double * ret_pretty_len)
{
    double pretty_len = (double) len;
    ft_size i;
    for (i = 0; i < fc_pretty_size_len-1 && pretty_len >= 1024.0; i++)
        pretty_len *= .0009765625; // exactly 1/1024
    * ret_pretty_len = pretty_len;
    return fc_pretty_size[i];
}

FT_NAMESPACE_END
