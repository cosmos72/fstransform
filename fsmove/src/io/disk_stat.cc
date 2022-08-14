/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 2 of the License, or
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
 * io/disk_stat.cc
 *
 *  Created on: Oct 05, 2011
 *      Author: max
 */

#include "../first.hh"

#include "disk_stat.hh" // for fm_disk_stat
#include "../log.hh"    // for ff_log()
#include "../misc.hh"   // for ff_pretty_size()

FT_IO_NAMESPACE_BEGIN

/** constructor */
fm_disk_stat::fm_disk_stat() : this_name(), this_total(0), this_free(0) {
}

/** clear all data stored in this object */
void fm_disk_stat::clear() {
    this_name.clear();
    this_total = this_free = 0;
}

/**
 * set the free disk space.
 * returns 0, or error if free disk space becomes critically low
 */
int fm_disk_stat::set_free(ft_uoff free) {
    this_free = free;
    int err = 0;
    if (is_too_low_free_space(free)) {
        double pretty_size = 0.0;
        const char *pretty_label = ff_pretty_size(free, &pretty_size);
        ff_log(FC_ERROR, 0,
               "free space on %s device is critically low: only %.2f %sbytes left, "
               "ABORTING!!!",
               this_name.c_str(), pretty_size, pretty_label);
        err = -ENOSPC;
    }
    return err;
}

/**
 * return true if 'free' amount of free space would trigger a 'critically low
 * free space' error
 */
bool fm_disk_stat::is_too_low_free_space(ft_uoff free) const {
    /**
     * if file system is smaller than 6GB, critically low free space is 96kbytes.
     * if file system is between 6GB and 64TB, critically low free space is total
     * disk space divided 65536 (i.e. 0.0015%). if file system is larger than
     * 64TB, critically low free space is 1Gbyte.
     */
    return free <= THRESHOLD_MIN || (free <= THRESHOLD_MAX && free <= (this_total >> 16));
}

FT_IO_NAMESPACE_END
