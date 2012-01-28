/*
 * eta.hh
 *
 *  Created on: Mar 22, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_ETA_HH
#define FSTRANSFORM_ETA_HH

#include "types.hh"         // for ft_size

FT_NAMESPACE_BEGIN

/** estimates time-of-arrival from a sliding window extrapolation of last 5 progress percentages */
class ft_eta
{
private:
    enum { FC_ETA_N = 5 };
    double this_x[FC_ETA_N];
    double this_y[FC_ETA_N];
    ft_size this_n;

public:
    ft_eta();

    /**
     * add percentage and {current timestamp} to the sliding window E.T.A. extrapolation.
     * return number of seconds to E.T.A., or < 0 if not enough data available yet.
     */
    double add(double percentage);

    /* reset this E.T.A. to empty */
    void clear();
};


FT_NAMESPACE_END

#endif /* FSTRANSFORM_ETA_HH */
