/*
 * request.cc
 *
 *  Created on: Mar 15, 2011
 *      Author: max
 */

#include "../first.hh"

#include <cerrno>       // for ENOTDIR, EINVAL

#include "../assert.hh" // for ff_assert()
#include "../extent.hh" // for ft_dir, ff_{from,to}_{dev,storage}()
#include "../util.hh"   // for ff_max3<T>(), ff_can_sum<T>()
#include "request.hh"   // for ft_request
#include "io_posix.hh"  // for ft_io_posix::{label, FC_DEVICE, FC_STORAGE}


FT_IO_NAMESPACE_BEGIN

/** construct a request with no pending copies */
ft_request::ft_request()
{
    clear();
}

/** forget and discard any requested copy */
void ft_request::clear()
{
    this_from = this_to = this_length = 0;
    this_dir = FC_STORAGE2STORAGE; /* use FC_STORAGE2STORAGE as 'invalid dir' */
}


/** assert class invariant: ff_can_sum(ff_max2(this_from, this_to), this_length) */
#define ff_invariant() ff_assert(ff_can_sum(ff_max2(this_from, this_to), this_length))


/**
 * forget any requested copy and set this request to specified values.
 * returns EOVERFLOW if max(from,to)+length overflows ft_uoff
 */
int ft_request::assign(ft_uoff from, ft_uoff to, ft_uoff length, ft_dir dir)
{
    if (!ff_can_sum(ff_max2(from, to), length))
        return EOVERFLOW;

    this_from = from;
    this_to = to;
    this_length = length;
    this_dir = dir;
    return 0;
}



/**
 * coalesce this request with specified values, or return error if they cannot be coalesced.
 * possible errors:
 * ENOTDIR   the two requests are not in the same direction
 * EINVAL    the two requests are not consecutive, or coalescing them would overflow
 */
int ft_request::coalesce(ft_uoff from, ft_uoff to, ft_uoff length, ft_dir dir)
{
    ff_invariant();

    if (this_length == 0)
        return assign(from, to, length, dir);

    if (this_dir != dir)
        return ENOTDIR;

    if (!ff_can_sum(ff_max3(from, to, this_length), length))
        return EOVERFLOW;

    if (from + length == this_from && to + length == this_to) {
        this_from = from;
        this_to = to;

    } else if (this_from + this_length == from && this_to + this_length == to) {
        /* nothing to do here */
    } else
        return EINVAL;

    this_length += length;
    this_dir = dir;

    ff_invariant();
    return 0;
}

/**
 * remove 'length' bytes from the beginning of this request
 */
int ft_request::remove_front(ft_uoff length)
{
    ff_invariant();

    int err = 0;
    if (this_length == length)
        clear();
    else if (this_length > length) {
        // invariant: ff_can_sum(ff_max2(this_from, this_to), this_length)
        this_from += length;
        this_to += length;
        ff_invariant();
    } else {
        err = EOVERFLOW;
    }
    return err;
}


const char * ft_request::label_from() const
{
    return ft_io_posix::label[ff_is_from_dev(this_dir) ? ft_io_posix::FC_DEVICE : ft_io_posix::FC_STORAGE];
}

const char * ft_request::label_to() const
{
    return ft_io_posix::label[ff_is_to_dev(this_dir) ? ft_io_posix::FC_DEVICE : ft_io_posix::FC_STORAGE];
}

FT_IO_NAMESPACE_END
