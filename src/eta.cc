/*
 * eta.cc
 *
 *  Created on: Mar 22, 2011
 *      Author: max
 */

#include "first.hh"

#include <cerrno>        // for ERANGE
#include <sys/time.h>    // for time_t, gettimeofday()

#include "eta.hh"        // for ft_eta


FT_NAMESPACE_BEGIN


static int ff_least_squares(ft_size N, const double xi[], const double yi[], double & ret_m, double & ret_q);

static int ff_now(double & ret_now);


/** default constructor */
ft_eta::ft_eta()
    : this_y(), this_x()
{
    clear();
}

void ft_eta::init()
{
    (void) ff_now(this_x0);
}

void ft_eta::clear()
{
    for (ft_size i = 0; i < FC_ETA_N; i++)
        this_y[i] = this_x[i] = 0.0;
    this_x0 = 0.0;
    this_n = 0;
}


/**
 * add percentage and {current timestamp} to the sliding E.T.A. extrapolation.
 * return number of seconds to E.T.A., or < 0 if not enough data available yet.
 */
double ft_eta::add(double percentage)
{
    double x = 0.0, y = percentage;
    if (ff_now(x) != 0)
        return -1.0; /* error in gettimeofday */

    /* slide window */
    for (ft_size i = FC_ETA_N - 1; i != 0; i--) {
        this_y[i] = this_y[i - 1];
        this_x[i] = this_x[i - 1];
    }
    this_y[0] = percentage;
    this_x[0] = x -= this_x0;
    if (this_n < FC_ETA_N && ++this_n < FC_ETA_MIN_N)
        return -1.0; /* not enough data available yet */

    /*
     * use the simple linear regression (least squares)
     * formula to find the line best-fitting all points
     */
    double m_all = 0.0, q_all, m_last = 0.0, q_last, m;
    int err = ff_least_squares(this_n, this_x, this_y, m_all, q_all);
    if (err != 0 || m_all <= 0.0)
        return -1.0; /* ill-conditioned data */

    /* repeat to find the line best-fitting last FC_ETA_MIN_N points */
    err = ff_least_squares(FC_ETA_MIN_N, this_x, this_y, m_last, q_last);
    if (err == 0 && m_last > 0.0)
        /* make a brutal average between the two computed m */
        m = 0.5 * (m_all + m_last);
    else
        m = m_all;

    /* but then cheat and compute x@(y=1) forcing the best-fitting line to pass from last point */
    double x_1 = x + (1.0 - y) / m;
    if (x_1 <= x)
        return -1.0; /* E.T.A. is in the past! */

    return x_1 - x;
}

static int ff_now(double & ret_time)
{
    struct timeval tv;
    if (gettimeofday(& tv, NULL) != 0)
        return errno; /* error in gettimeofday() */
    ret_time = (double) tv.tv_sec + (double) tv.tv_usec * 1e-6;
    return 0;
}

static int ff_least_squares(ft_size N, const double x[], const double y[], double & ret_m, double & ret_q)
{
    double X = 0, X2 = 0; /* X = sum(x[i]), X2 = sum(x[i]^2) */
    double Y = 0, Y2 = 0; /* Y = sum(y[i]), Y2 = sum(y[i]^2) */
    double XY = 0;       /* XY = sum(x[i]*y[i]) */
    double x_, y_;

    for (ft_size i = 0; i < N; i++) {
        x_ = x[i]; y_ = y[i];
        X  += x_; X2 += x_ * x_;
        Y  += y_; Y2 += y_ * y_;
        XY += x_ * y_;
    }
    double v = X2 - X*X/N;
    if (v == 0)
        return -ERANGE;
    double c = XY - X*Y/N;
    ret_m = c / v;
    ret_q = (Y - ret_m*X) / N;
    return 0;
}

FT_NAMESPACE_END
