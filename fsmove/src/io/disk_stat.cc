/*
 * io/disk_stat.cc
 *
 *  Created on: Oct 05, 2011
 *      Author: max
 */

#include "../first.hh"

#include "../log.hh"       // for ff_log()
#include "../util.hh"      // for ff_pretty_size()
#include "disk_stat.hh"    // for fm_disk_stat

FT_IO_NAMESPACE_BEGIN


/** constructor */
fm_disk_stat::fm_disk_stat()
    : this_name(), this_total(0), this_free(0)
{ }

/** clear all data stored in this object */
void fm_disk_stat::clear()
{
    this_name.clear();
    this_total = this_free = 0;
}

/**
 * set the free disk space.
 * returns 0, or error if free disk space becomes critically low
 */
int fm_disk_stat::set_free(ft_uoff free)
{
    this_free = free;
    int err = 0;
    /**
     * if file systems is smaller than 64GB, critically low free space is 1Mbyte.
     * if file systems is between 64GB and 64TB, critically low free space is total disk space divided 65536 (i.e. 0.0015%).
     * if file systems is larger than 64TB, critically low free space is 1Gbyte.
     */
    if (free <= _1Gbyte
            && (free <= _1Mbyte || free <= (this_total >> 16))) {
        double pretty_size = 0.0;
        const char * pretty_label = ff_pretty_size(free, & pretty_size);
        ff_log(FC_ERROR, 0, "free space on %s device is critically low: only %.2f %sbytes left, ABORTING!!!", this_name.c_str(), pretty_size, pretty_label);
        err = -ENOSPC;
    }
    return err;
}

FT_IO_NAMESPACE_END


