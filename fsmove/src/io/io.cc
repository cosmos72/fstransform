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
 * io/io.cc
 *
 *  Created on: Sep 20, 2011
 *      Author: max
 */

#include "../first.hh"

#include "../args.hh"      // for fm_args
#include "../assert.hh"    // for ff_assert()
#include "../misc.hh"      // for ff_show_progress(), ff_now()
#include "io.hh"           // for fm_io

#include "cache/cache_mem.hh"     // for ft_cache_mem
#include "cache/cache_symlink.hh" // for ft_cache_symlink_kv

#if defined(FT_HAVE_MATH_H)
# include <math.h>         // for sqrt()
#elif defined(FT_HAVE_CMATH)
# include <cmath>          // for sqrt()
#endif

#if defined(FT_HAVE_STRING_H)
# include <string.h>       // for strcmp()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>        // for strcmp()
#endif


FT_IO_NAMESPACE_BEGIN


char const * const fm_io::label[] = {
    "source", "target"
};
char const * const fm_io::LABEL[] = {
    "SOURCE", "TARGET"
};

/** constructor */
fm_io::fm_io() : this_inode_cache(NULL), this_exclude_set(),
    this_source_stat(), this_target_stat(),
    this_source_root(), this_target_root(),
    this_eta(), this_work_total(0), this_work_report_threshold(0),
    this_work_done(0), this_work_last_reported(0), 
    this_work_last_reported_time(0.0),
    this_progress_msg(NULL), this_force_run(false), this_simulate_run(false)
{ }

/**
 * destructor.
 * sub-classes must override it to call close()
 */
fm_io::~fm_io()
{
    // in case it's != NULL
    delete_inode_cache();
}

void fm_io::delete_inode_cache()
{
    delete this_inode_cache;
    this_inode_cache = NULL;
}

/**
 * open this fm_io.
 * sub-classes must override this method to perform appropriate initialization
 */
int fm_io::open(const fm_args & args)
{
    const char * arg1 = args.io_args[FC_SOURCE_ROOT], * arg2 = args.io_args[FC_TARGET_ROOT];
    
    int err = 0;
    do {
        if (arg1 == NULL) {
            ff_log(FC_ERROR, 0, "missing arguments: %s %s", label[FC_SOURCE_ROOT], label[FC_TARGET_ROOT]);
            err = -EINVAL;
            break;
        }
        if (arg2 == NULL) {
            ff_log(FC_ERROR, 0, "missing argument: %s", label[FC_TARGET_ROOT]);
            err = -EINVAL;
            break;
        }
        const char * inode_cache_path = args.inode_cache_path;
        delete_inode_cache();
        if (inode_cache_path != NULL)
        {
            ft_cache_symlink_kv<ft_inode, ft_string> * icp = new ft_cache_symlink_kv<ft_inode, ft_string>(inode_cache_path);
            err = icp->init(inode_cache_path);
            if (err != 0)
            {
                delete icp;
                break;
            }
            // icp->get_path() removes trailing '/' unless it's exactly the path "/"
            inode_cache_path = icp->get_path();
            this_inode_cache = icp;
        }
        else
            this_inode_cache = new ft_cache_mem<ft_inode, ft_string>();
        
        
        
        this_source_stat.set_name("source");
        this_target_stat.set_name("target");
        this_source_root = arg1;
        this_target_root = arg2;
        this_eta.clear();
        this_work_total = this_work_report_threshold = this_work_done = this_work_last_reported = 0;
        this_force_run = args.force_run;
        this_simulate_run = args.simulate_run;
        this_progress_msg = " still to move";
        
        char const * const * exclude_list = args.exclude_list;
        if (exclude_list != NULL) {
            for (; * exclude_list != NULL; ++exclude_list)
                this_exclude_set.insert(* exclude_list);
        }
        // do NOT move the inode-cache!
        if (inode_cache_path != NULL)
        {
            this_exclude_set.insert(inode_cache_path);
        }
        
    } while (0);
    return err;
}


int fm_io::inode_cache_find_or_add(ft_inode inode, ft_string & path)
{
    ft_size root_len = this_target_root.length();
    ff_assert(path.length() >= root_len && path.compare(0, root_len, this_target_root) == 0);
    
    ft_string short_path = path.substr(root_len);
    int err = this_inode_cache->find_or_add(inode, short_path);
    if (err == 1)
        path = this_target_root + short_path;
    return err;
}

int fm_io::inode_cache_find_and_delete(ft_inode inode, ft_string & path)
{
    ft_size root_len = this_target_root.length();
    ff_assert(path.length() >= root_len && path.compare(0, root_len, this_target_root) == 0);
    
    ft_string short_path = path.substr(root_len);
    int err = this_inode_cache->find_and_delete(inode, short_path);
    if (err == 1)
        path = this_target_root + short_path;
    return err;
}

/**
 * returns error if source or target file-system are almost full (typical threshold is 97%)
 */
int fm_io::is_almost_full(const fm_disk_stat & stat) const
{
    ft_uoff total = stat.get_total();
    int err = 0;
    if (total != 0) {
        ft_uoff used = stat.get_used();
        double percentage = 100.0 * ((double) used / total);
        if (percentage > 97.0) {
            bool can_force = percentage <= 99.0;
            bool is_warn = this_force_run && can_force;
            
            
            ff_log(is_warn ? FC_WARN : FC_ERROR, 0, "%s file-system is %4.1f%% full%s%s",
                   stat.get_name().c_str(), percentage,
                   (is_warn ? ", continuing anyway due to -f" : ", cowardly refusing to run"),
                   (!is_warn && can_force ? ". use option '-f' to override this safety check AT YOUR OWN RISK" : ""));
            
            err = is_warn ? 0 : -ENOSPC;
        }
    }
    return err;
}

/**
 * set total number of bytes to move (may include estimated overhead for special files, inodes...),
 * reset total number of bytes moved,
 * initialize this_eta to 0% at current time
 * 
 * returns error if source or target file-system are almost full (typical threshold is 97%)
 */
int fm_io::init_work()
{
    int err = is_almost_full(source_stat());
    if (err == 0)
        err = is_almost_full(target_stat());
    if (err != 0)
        return err;
    
    ft_uoff source_used = source_stat().get_used();
    ft_uoff target_used = target_stat().get_used();
    ft_uoff work_total = source_used > target_used ? source_used - target_used : 0;
    
    this_work_total = work_total;
    
    ft_uoff work_total_GB = work_total >> 30;
    
    if (work_total_GB <= 4)
        /* up to 4GB, report approximately every 5% progress */
        this_work_report_threshold = work_total / 20;
    
    else if (work_total_GB <= 100)
        /* up to 100GB, report approximately every 2% progress */
        this_work_report_threshold = work_total / 50;
    else
        /* above 100GB, report approximately every 2GB * sqrt(size/100GB) */
        this_work_report_threshold = (ft_uoff)
            (((ft_uoff)2 << 30) * sqrt(0.01 * work_total_GB));
    
    this_work_done = this_work_last_reported = 0;
    ff_now(this_work_last_reported_time);
    this_eta.add(0.0);
    return err;
}

/**
 * add to number of bytes moved until now (may include estimated overhead for special files, inodes...)
 * also periodically invokes show_progress()
 */
void fm_io::add_work_done(ft_uoff work_done)
{
    this_work_done += work_done;
    if (this_work_total == 0
        || this_work_report_threshold == 0
        || this_work_done - this_work_last_reported < this_work_report_threshold)
        return;
    
    this_work_last_reported = this_work_done;
    ff_now(this_work_last_reported_time);
    show_progress(FC_INFO);
}

/** show human-readable progress indication, bytes still to move, and estimated time left */
void fm_io::show_progress(ft_log_level log_level)
{
    ft_uoff moved_len = this_work_done, work_total = this_work_total;
    
    if (work_total == 0)
        return;
    if (moved_len > work_total)
        moved_len = work_total;
    
    double percentage = 0.0, time_left = -1.0;
    
    percentage = moved_len / (double)work_total;
    time_left = this_eta.add(percentage);
    percentage *= 100.0;
    
    const char * simul_msg = "";
    if (simulate_run()) {
        simul_msg = "(simulated) ";
        time_left = -1.0;
    }
    
    ff_show_progress(log_level, simul_msg, percentage, work_total - moved_len, this_progress_msg, time_left);
}

/**
 * close this fm_io.
 * sub-classes must override this method to perform appropriate cleanup
 */
void fm_io::close()
{
    this_exclude_set.clear();
    this_source_stat.clear();
    this_target_stat.clear();
    this_source_root.clear();
    this_target_root.clear();
    this_eta.clear();
    this_work_done = this_work_last_reported = this_work_total = 0;
    this_progress_msg = NULL;
    this_force_run = this_simulate_run = false;
    
    delete_inode_cache();
}

FT_IO_NAMESPACE_END
