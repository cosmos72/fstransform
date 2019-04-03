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
 * io/io.hh
 *
 *  Created on: Sep 20, 2011
 *      Author: max
 */

#ifndef FSMOVE_IO_IO_HH
#define FSMOVE_IO_IO_HH

#include "../check.hh"

#include "../types.hh"       // for ft_string, ft_uoff
#include "../eta.hh"         // for ft_eta
#include "../log.hh"         // for ft_log_level, also for ff_log() used by io.cc
#include "../fwd.hh"         // for fm_args
#include "../cache/cache.hh" // for ft_cache<K,V>

#include "disk_stat.hh"      // for fm_disk_stat

#include <set>               // for std::set


FT_IO_NAMESPACE_BEGIN

/**
 * abstract base class for all I/O implementations
 * that actually move files around
 */
class fm_io {

private:
    ft_cache<ft_inode, ft_string> * this_inode_cache;
    std::set<ft_string> this_exclude_set;

    fm_disk_stat this_source_stat, this_target_stat;
    ft_string this_source_root, this_target_root;

    ft_eta  this_eta;
    ft_uoff this_work_total, this_work_report_threshold;
    ft_uoff this_work_done,  this_work_last_reported;
    double this_work_last_reported_time;

    const char * this_progress_msg;

    bool this_force_run, this_simulate_run;

    /**
     * returns error if source or target file-system are almost full (typical threshold is 97%)
     */
    int is_almost_full(const fm_disk_stat & stat) const;

protected:

    int inode_cache_find_or_add(ft_inode inode, ft_string & path);

    int inode_cache_find_and_delete(ft_inode inode, ft_string & path);

    FT_INLINE fm_disk_stat & source_stat() { return this_source_stat; }
    FT_INLINE fm_disk_stat & target_stat() { return this_target_stat; }

    FT_INLINE void progress_msg(const char * msg) { this_progress_msg = msg; }

    /**
     * use source_stat and target_stat to compute total number of bytes to move
     * (may include estimated overhead for special files, inodes...),
     * reset total number of bytes moved,
     * initialize this_eta to 0% at current time
     *
     * returns error if source or target file-system are almost full (typical threshold is 97%)
     */
    int init_work();

    /**
     * add to number of bytes moved until now (may include estimated overhead for special files, inodes...)
     * also periodically invokes show_progress()
     */
    void add_work_done(ft_uoff work_done);

    /** show human-readable progress indication, bytes still to move, and estimated time left */
    void show_progress(ft_log_level log_level = FC_NOTICE);

public:
    enum {
        FC_SOURCE_ROOT = 0, FC_TARGET_ROOT,
        FC_ARGS_COUNT = 2, // must be equal to count of preceding enum constants
    };

    static char const * const label[]; // source, target
    static char const * const LABEL[]; // SOURCE, TARGET

    /** constructor */
    fm_io();

    /**
     * destructor.
     * sub-classes must override it to call close()
     */
    virtual ~fm_io();

    /** return true if this ft_io is currently (and correctly) open */
    virtual bool is_open() const = 0;

    /**
     * open this fm_io.
     * sub-classes must override this method to perform appropriate initialization
     */
    virtual int open(const fm_args & args);

    /** core of recursive move algorithm, actually moves the whole source tree into target */
    virtual int move() = 0;

    /**
     * close this fm_io.
     * sub-classes must override this method to perform appropriate cleanup
     */
    virtual void close();

    /**
     * return the set of source files NOT to move
     */
    FT_INLINE const std::set<ft_string> & exclude_set() const { return this_exclude_set; }

    /**
     * return the top-most source path to move from
     */
    FT_INLINE const ft_string & source_root() const { return this_source_root; }

    /**
     * return the top-most target path to move to
     */
    FT_INLINE const ft_string & target_root() const { return this_target_root; }

    /**
     * return the force_run flag
     */
    FT_INLINE bool force_run() const { return this_force_run; }

    /**
     * return the simulate_run flag
     */
    FT_INLINE bool simulate_run() const { return this_simulate_run; }
};



FT_IO_NAMESPACE_END

#endif /* FSMOVE_IO_IO_HH */
