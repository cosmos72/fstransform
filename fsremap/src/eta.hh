/*
 * eta.hh
 *
 *  Created on: Mar 22, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_ETA_HH
#define FSTRANSFORM_ETA_HH

#include "types.hh"         // for ft_size

#include <vector>           // for std::vector<T>

FT_NAMESPACE_BEGIN

/** estimates time-of-arrival from a sliding window extrapolation of last 5 progress percentages */
class ft_eta
{
private:
    std::vector<double> this_x;
    std::vector<double> this_y;
    ft_size this_max_n;

public:
    enum { DEFAULT_MAX_N = 6 };

    ft_eta(ft_size max_n = DEFAULT_MAX_N);

    /**
     * add percentage and {current timestamp} to the sliding window E.T.A. extrapolation.
     * return number of seconds to E.T.A., or < 0 if not enough data available yet.
     */
    double add(double percentage);

    /* reset this E.T.A. to empty */
    void clear(ft_size max_n = DEFAULT_MAX_N);
};


FT_NAMESPACE_END

#endif /* FSTRANSFORM_ETA_HH */
