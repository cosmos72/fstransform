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
 *  Created on: Mar 1, 2011
 *      Author: max
 */

#include "../first.hh"

#if defined(FT_HAVE_STDIO_H)
# include <stdio.h>        // for fopen(), fclose()
#elif defined(FT_HAVE_CSTDIO)
# include <cstdio>         // for fopen(), fclose()
#endif


#include "../log.hh"       // for ff_log()
#include "../misc.hh"      // for ff_can_sum()
#include "../ui/ui.hh"     // for fr_ui
#include "io.hh"           // for fr_io
#include "extent_file.hh"  // for ff_write_extents_file()

FT_IO_NAMESPACE_BEGIN


char const * const fr_io::label[] = {
        "device", "loop-file", "zero-file", "secondary-storage", "primary-storage", "storage", "free-space"
};
char const * const fr_io::LABEL[] = {
        "DEVICE", "LOOP-FILE", "ZERO-FILE", "SECONDARY-STORAGE", "PRIMARY-STORAGE", "STORAGE", "FREE-SPACE"
};


char const* const fr_io::extents_filename[FC_IO_EXTENTS_COUNT] = {
    "/loop_extents.txt", "/free_space_extents.txt", "/to_zero_extents.txt"
};


/** constructor */
fr_io::fr_io(fr_persist & persist)
    : this_primary_storage(), request_vec(), this_dev_length(0), this_loop_file_length(0), this_eff_block_size_log2(0),
      this_dev_path(NULL), this_cmd_umount(NULL), this_job(persist.job()), this_persist(persist), this_ui(NULL),
      request_dir(FC_INVALID2INVALID), this_delegate_ui(false)
{
    this_secondary_storage.clear();
}

/**
 * destructor.
 * sub-classes must override it to call close() if they override close()
 */
fr_io::~fr_io()
{ }

/**
 * open this fr_io.
 * sub-classes must override this method to perform appropriate initialization,
 * and the first thing sub-classes open() must do is to call fr_io::open().
 */
int fr_io::open(const fr_args & args)
{
    this_cmd_umount = args.cmd_umount;
    return 0;
}

/**
 * close this fr_io.
 * sub-classes must override this method to perform appropriate cleanup
 */
void fr_io::close()
{
    this_primary_storage.clear();
    this_secondary_storage.clear();
    request_vec.clear();
    request_dir = FC_INVALID2INVALID;

    this_dev_length = this_eff_block_size_log2 = 0;
    this_dev_path = NULL;
}



/** compute and return log2() of effective block size */
ft_uoff fr_io::effective_block_size_log2(ft_uoff block_size_bitmask)
{
    ft_uoff block_size_log2 = 0;
    if (block_size_bitmask != 0) {
        while ((block_size_bitmask & 1) == 0) {
            block_size_log2++;
            block_size_bitmask >>= 1;
        }
    }
    return block_size_log2;
}

/* return (-)EOVERFLOW if request from/to + length overflow specified maximum value */
int fr_io::validate(const char * type_name, ft_uoff type_max, fr_dir dir, ft_uoff from, ft_uoff to, ft_uoff length)
{
    to = ff_max2(from, to);
    if (!ff_can_sum(to, length) || length > type_max || to > type_max - length) {
        return ff_log(FC_FATAL, EOVERFLOW, "internal error! %s to %s io.copy(dir = %d, from_physical = %" FT_ULL ", to_physical = %" FT_ULL ", length = %" FT_ULL ")"
                      " overflows maximum allowed (%s)%" FT_ULL ,
                      ff_is_from_dev(dir) ? label[FC_DEVICE] : label[FC_STORAGE],
                      ff_is_to_dev(dir) ? label[FC_DEVICE] : label[FC_STORAGE],
                      (int)dir, (ft_ull)from, (ft_ull)to, (ft_ull)length, type_name, (ft_ull)type_max);
    }
    return 0;
}


/**
 * if replaying an existing job, calls ff_load_extents_file() to load saved extents files.
 * otherwise calls the 4-argument version of read_extents() and, if it succeeds,
 * calls effective_block_size_log2() to compute and remember effective block size
 */
int fr_io::read_extents(fr_vector<ft_uoff> & loop_file_extents,
                        fr_vector<ft_uoff> & free_space_extents,
                        fr_vector<ft_uoff> & to_zero_extents)
{

    ft_uoff block_size_bitmask = 0;
    int err;
    if (is_replaying())
    	err = load_extents(loop_file_extents, free_space_extents, to_zero_extents, block_size_bitmask);
    else
    	err = read_extents(loop_file_extents, free_space_extents, to_zero_extents, block_size_bitmask);

    if (err == 0) {
        this_eff_block_size_log2 = effective_block_size_log2(block_size_bitmask);
        ff_log(FC_INFO, 0, "%s effective block size = %" FT_ULL , label[FC_DEVICE], (ft_ull) 1 << this_eff_block_size_log2);
    }
    return err;
}



/**
 * loads extents from files 'loop_extents.txt', 'free_space_extents.txt' and 'to_zero_extents.txt'
 * inside folder job.job_dir() by calling the function ff_load_extents_file()
 * if successful, calls effective_block_size_log2() to compute and remember effective block size
 */
int fr_io::load_extents(fr_vector<ft_uoff> & loop_file_extents,
						fr_vector<ft_uoff> & free_space_extents,
					    fr_vector<ft_uoff> & to_zero_extents, ft_uoff & ret_block_size_bitmask)
{
	fr_vector<ft_uoff> * ret_extents[] = { & loop_file_extents, & free_space_extents, & to_zero_extents };
    ft_string path;
    ft_uoff block_size_bitmask = 0;
    const ft_string & job_dir = this_job.job_dir();
    FILE * f = NULL;
    const char * path_cstr = NULL;
    int err = 0;

    for (ft_size i = 0; err == 0 && i < FC_IO_EXTENTS_COUNT; i++) {
        path = job_dir;
        path += extents_filename[i];
        path_cstr = path.c_str();
        if ((f = fopen(path_cstr, "r")) == NULL) {
        	if (i == FC_IO_EXTENTS_TO_ZERO)
        		ff_log(FC_WARN, errno, "this job is probably from version 0.9.3, cannot open persistence file '%s'", path_cstr);
        	else
        		err = ff_log(FC_ERROR, errno, "error opening persistence file '%s'", path_cstr);
        	break;
        }
        if ((err = ff_load_extents_file(f, * ret_extents[i], block_size_bitmask)) != 0)
            err = ff_log(FC_ERROR, err, "error reading persistence file '%s'", path_cstr);

        if (fclose(f) != 0) {
            ff_log(FC_WARN, errno, "warning: failed to close persistence file '%s'", path_cstr);
            f = NULL;
        }
    }
    if (err == 0)
    	ret_block_size_bitmask = block_size_bitmask;

    return err;
}

/**
 * saves extents to files job.job_dir() + '/loop_extents.txt' and job.job_dir() + '/free_space_extents.txt'
 * by calling the function ff_save_extents_file()
 */
int fr_io::save_extents(const fr_vector<ft_uoff> & loop_file_extents,
                        const fr_vector<ft_uoff> & free_space_extents,
                        const fr_vector<ft_uoff> & to_zero_extents) const
{
	const fr_vector<ft_uoff> * extents[] = { & loop_file_extents, & free_space_extents, & to_zero_extents };
    ft_string path;
    const ft_string & job_dir = this_job.job_dir();
    FILE * f = NULL;
    const char * path_cstr = NULL;
    int err = 0;

    for (ft_size i = 0; err == 0 && i < FC_IO_EXTENTS_COUNT; i++) {
        path = job_dir;
        path += extents_filename[i];
        path_cstr = path.c_str();
        if ((f = fopen(path_cstr, "w")) == NULL) {
            err = ff_log(FC_ERROR, errno, "error opening persistence file '%s'", path_cstr);
            break;
        }
        if ((err = ff_save_extents_file(f, * extents[i])) != 0)
            err = ff_log(FC_ERROR, err, "error writing to persistence file '%s'", path_cstr);

        if (fclose(f) != 0) {
            ff_log(FC_WARN, errno, "error closing persistence file '%s'", path_cstr);
            f = NULL;
        }
    }
    return err;
}

/**
 * called once by work<T>::relocate() immediately before starting the remapping phase.
 *
 * must be overridden by sub-classes to check that last device block to be written is actually writable.
 * Reason: at least on Linux, if a filesystems is smaller than its containing device, it often limits to its length the writable blocks in the device.
 *
 * default implementation: do nothing and return success (0)
 */
int fr_io::check_last_block()
{
	return 0;
}


/**
 * perform buffering and coalescing of copy requests.
 * queues a copy of single fragment from DEVICE or FREE-STORAGE, to STORAGE to FREE-DEVICE.
 * calls flush_queue() as needed to actually perform any copy that cannot be buffered or coalesced.
 * note: parameters are in bytes!
 * return 0 if success, else error
 */
int fr_io::copy_bytes(fr_dir dir, ft_uoff from_physical, ft_uoff to_physical, ft_uoff length)
{
    int err = 0;
    if (!request_vec.empty() && request_dir != dir) {
        if ((err = flush_queue()) != 0)
            return err;
    }
    if ((err = validate("ft_uoff", (ft_uoff)-1, dir, from_physical, to_physical, length)) != 0)
        return err;

	// do NOT actually show anything while replaying persistence
    if (this_ui != 0 && !this_delegate_ui && !is_replaying())
        this_ui->show_io_copy(dir, from_physical, to_physical, length);

    request_dir = dir;
    request_vec.append(from_physical, to_physical, length, FC_DEFAULT_USER_DATA);
    return err;
}


/**
 * flush any pending copy, i.e. actually perform all queued copies.
 * return 0 if success, else error
 * on return, 'ret_copied' will be increased by the number of blocks actually copied (NOT queued for copying),
 * which could be > 0 even in case of errors
 */
int fr_io::flush_queue()
{
    int err = 0;
    if (!request_vec.empty()) {

    	// do NOT actually copy anything while replaying persistence
    	if (!is_replaying())
    		err = flush_copy_bytes(request_dir, request_vec);

    	request_vec.clear();
        request_dir = FC_INVALID2INVALID;
    }
    return err;
}

/**
 * flush any I/O specific buffer
 * return 0 if success, else error
 * default implementation: do nothing
 */
int fr_io::flush_bytes()
{
    return 0;
}

/**
 * flush any pending copy (call copy_bytes() through flush_queue()),
 * then flush any I/O specific buffer (call flush_bytes()).
 * Finall, call ui->show_io_flush() if needed
 * return 0 if success, else error
 */
int fr_io::flush()
{
    int err = flush_queue();

    // do NOT actually copy anything while replaying persistence
    if (!is_replaying()) {
    	if (err == 0)
    		err = flush_bytes();
    	if (err == 0 && this_ui != 0 && !this_delegate_ui)
    		this_ui->show_io_flush();
    }
    return err;
}

/** called to remove storage from file system if execution is successful */
int fr_io::remove_storage_after_success()
{
	return 0;
}

FT_IO_NAMESPACE_END


