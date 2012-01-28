/*
 * io/io.cc
 *
 *  Created on: Sep 20, 2011
 *      Author: max
 */

#include "../first.hh"

#include "../args.hh"      // for fm_args
#include "../misc.hh"      // for ff_show_progress(), ff_now()
#include "io.hh"           // for fm_io

FT_IO_NAMESPACE_BEGIN


char const * const fm_io::label[] = {
        "source", "target"
};

/** constructor */
fm_io::fm_io()
    : this_inode_cache(), this_exclude_set(),
      this_source_stat(), this_target_stat(),
      this_source_root(), this_target_root(),
      this_eta(), this_work_done(0), this_work_last_reported(0), this_work_total(0),
      this_force_run(false), this_simulate_run(false)
{ }

/**
 * destructor.
 * sub-classes must override it to call close()
 */
fm_io::~fm_io()
{ }


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
        this_source_stat.set_name("source");
        this_target_stat.set_name("target");
        this_source_root = arg1;
        this_target_root = arg2;
        this_eta.clear();
        this_work_done = this_work_last_reported = this_work_total = 0;
        this_force_run = args.force_run;
        this_simulate_run = args.simulate_run;

        char const * const * exclude_list = args.exclude_list;
        if (exclude_list != NULL) {
            for (; * exclude_list != NULL; ++exclude_list)
                this_exclude_set.insert(* exclude_list);
        }
    } while (0);
    return err;
}


/**
 * set total number of bytes to move (may include estimated overhead for special files, inodes...),
 * reset total number of bytes moved,
 * initialize this_eta to 0% at current time
 */
void fm_io::init_work(ft_uoff work_total)
{
	this_work_total = work_total;
	this_work_done = this_work_last_reported = 0;
	ff_now(this_work_last_reported_time);
	this_eta.add(0.0);
}

/**
 * add to number of bytes moved until now (may include estimated overhead for special files, inodes...)
 * also periodically invokes show_progress()
 */
void fm_io::add_work_done(ft_uoff work_done)
{
	this_work_done += work_done;
	if (this_work_total == 0)
		return;

	ft_uoff delta = this_work_done - this_work_last_reported;
	bool need_show_progress = false;

	if ((this_work_total >> 30) <= 4)
		/* up to 4GB, report approximately every 5% progress */
		need_show_progress = delta >= this_work_total / 20;

	else if (sizeof(ft_uoff) >= 6 && (this_work_total >> 30) <= 100)
		/* up to 100GB, report approximately every 2% progress */
		need_show_progress = delta >= this_work_total / 50;
	else
		/* above 100GB, report approximately every 1GB */
		need_show_progress = delta >= ((ft_uoff)1 << 20);


	if (!need_show_progress)
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

    ff_show_progress(log_level, simul_msg, percentage, work_total - moved_len, " still to move", time_left);
}

/**
 * close this fm_io.
 * sub-classes must override this method to perform appropriate cleanup
 */
void fm_io::close()
{
    this_inode_cache.clear();
    this_exclude_set.clear();
    this_source_stat.clear();
    this_target_stat.clear();
    this_source_root.clear();
    this_target_root.clear();
    this_eta.clear();
    this_work_done = this_work_last_reported = this_work_total = 0;
    this_force_run = false;
    this_simulate_run = false;
}

FT_IO_NAMESPACE_END


