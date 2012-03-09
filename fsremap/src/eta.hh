/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 * 
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
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

/** estimates time-of-arrival from a sliding window extrapolation of last 3 progress percentages */
class ft_eta
{
private:
    std::vector<double> this_x;
    std::vector<double> this_y;
    ft_size this_max_n;

public:
    enum { DEFAULT_MAX_N = 12 };

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
