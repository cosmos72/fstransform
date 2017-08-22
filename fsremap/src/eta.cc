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
 * eta.cc
 *
 *  Created on: Mar 22, 2011
 *      Author: max
 */

#include "first.hh"

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>      // for errno, EDOM
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>       // for errno, EDOM
#endif
#if defined(FT_HAVE_TIME_H)
# include <time.h>     // for time_t, time()
#elif defined(FT_HAVE_CTIME)
# include <ctime>      // for time_t, time()
#endif

#include "eta.hh"        // for ft_eta
#include "misc.hh"       // for ft_now


FT_NAMESPACE_BEGIN


static int ff_least_squares(ft_size N, const double xi[], const double yi[], double & ret_m, double & ret_q);

/** default constructor */
ft_eta::ft_eta(ft_size max_n)
    : this_x(), this_y()
{
    clear(max_n);
}

void ft_eta::clear(ft_size max_n)
{
	this_x.clear();
	this_y.clear();

    this_max_n = max_n;
}


/**
 * add percentage and {current timestamp} to the sliding E.T.A. extrapolation.
 * return number of seconds to E.T.A., or < 0 if not enough data available yet.
 */
double ft_eta::add(double y)
{
	enum { FC_ETA_MIN_N = 3 };

    double x = 0.0;
    if (this_max_n == 0 || ff_now(x) != 0)
        return -1.0; /* also in case of error in gettimeofday */

    /* slide window */
    ft_size n = this_x.size();
    if (n < this_max_n)
    	n++;
    else {
    	this_x.erase(this_x.begin());
    	this_y.erase(this_y.begin());
    }
    this_x.push_back(x);
    this_y.push_back(y);

    if (n < this_max_n && n < FC_ETA_MIN_N)
        return -1.0; /* not enough data available yet */

    /*
     * use the simple linear regression (least squares)
     * formula to find the line best-fitting all points
     */
    double m_all = 0.0, q_all, m_last = 0.0, q_last, m;
    int err = ff_least_squares(n, & this_x[0], & this_y[0], m_all, q_all);
    if (err != 0 || m_all <= 0.0)
        return -1.0; /* ill-conditioned data */

    /* repeat to find the line best-fitting last FC_ETA_MIN_N points */
    err = ff_least_squares(FC_ETA_MIN_N, & this_x[0], & this_y[0], m_last, q_last);
    if (err == 0 && m_last > 0.0)
        /* make a brutal average between the two computed m */
        m = 0.5 * (m_all + m_last);
    else
        m = m_all;

    /* but then cheat and change q to force the best-fitting line to pass from last point */
    double x_left = (1.0 - y) / m;
    if (x_left < 0.0)
        return -1.0; /* E.T.A. is in the past! */

    return x_left;
}

static int ff_least_squares(ft_size N, const double x[], const double y[], double & ret_m, double & ret_q)
{
    double X = 0, X2 = 0; /* X  = sum(x[i]), X2 = sum(x[i]^2) */
    double Y = 0, Y2 = 0; /* Y  = sum(y[i]), Y2 = sum(y[i]^2) */
    double XY = 0;        /* XY = sum(x[i]*y[i]) */
    double x0 = x[0], y0 = y[0], x_, y_;

    for (ft_size i = 0; i < N; i++) {
        x_ = x[i] - x0;
        y_ = y[i] - y0;
        X  += x_; X2 += x_ * x_;
        Y  += y_; Y2 += y_ * y_;
        XY += x_ * y_;
    }
    double v = X2 - X*X/N;
    if (v == 0.0)
        return -EDOM;
    double c = XY - X*Y/N;
    ret_m = c / v;
    /* adjust back for x0, y0 */
    ret_q = (Y - ret_m*X) / N + (y0 - ret_m*x0);
    return 0;
}

FT_NAMESPACE_END
