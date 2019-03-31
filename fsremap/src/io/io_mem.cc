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
 * io/io_mem.cc
 *
 *  Created on: Mar 31, 2019
 *      Author: max
 */

#include "../first.hh"

#include "../log.hh"       // for ff_log()
#include "../misc.hh"      // for ff_pretty_size()
#include "io_mem.hh"       // for ft_io_mem


FT_IO_NAMESPACE_BEGIN

char const * const ft_io_mem::extents_label[ft_io_mem::FC_EXTENTS_FILE_COUNT] = {
		"DEVICE-LENGTH", "LOOP-EXTENTS", "FREE-SPACE-EXTENTS", "TO-ZERO-EXTENTS"
};

char const * const ft_io_mem::sim_msg = "(simulated) ";


/** constructor */
ft_io_mem::ft_io_mem(fr_persist & persist)
  : super_type(persist)
{
    /* tell job that we're a simulation */
    job().simulate_run(true);
}

/** destructor. */
ft_io_mem::~ft_io_mem()
{ }


/**
 * retrieve LOOP-FILE extents and FREE-SPACE extents and append them into
 * the vectors loop_file_extents and free_space_extents.
 * the vectors will be ordered by extent ->logical.
 *
 * return 0 for success, else error (and vectors contents will be UNDEFINED).
 *
 * implementation: does nothing.
 */
int ft_io_mem::read_extents(fr_vector<ft_uoff> & FT_ARG_UNUSED(loop_file_extents),
                             fr_vector<ft_uoff> & FT_ARG_UNUSED(free_space_extents),
                             fr_vector<ft_uoff> & FT_ARG_UNUSED(to_zero_extents),
                             ft_uoff & FT_ARG_UNUSED(ret_block_size_bitmask))
{
    return 0;
}

/**
 * close the file descriptors for LOOP-FILE and ZERO-FILE
 *
 * implementation: do nothing and return success
 */
void ft_io_mem::close_extents()
{ }

/**
 * create SECONDARY-STORAGE as job.job_dir() + '.storage' and fill it with 'len' bytes of zeros,
 * setup a virtual storage composed by this->primary_storage extents inside DEVICE, plus secondary-storage extents.
 * return 0 if success, else error
 *
 * implementation: do nothing and return success
 */
int ft_io_mem::create_storage(ft_size secondary_len, ft_size buffer_len)
{
	const ft_uoff block_size_log2 = effective_block_size_log2();
	const ft_uoff block_size_mask = ((ft_uoff)1 << block_size_log2) - 1;
	ff_assert((secondary_len & block_size_mask) == 0);
	ff_assert((buffer_len & block_size_mask) == 0);

    /**
     * recompute primary_len... we could receive it from caller, but it's redundant
     */
    ft_uoff primary_len = 0;
    fr_vector<ft_uoff>::iterator begin = primary_storage().begin(), iter, end = primary_storage().end();
    for (iter = begin; iter != end; ++iter) {
    	const ft_uoff primary_extent_len = iter->second.length;
    	ff_assert((primary_extent_len & block_size_mask) == 0);
        primary_len += primary_extent_len;
    }

	device.resize(dev_length() >> block_size_log2);
	storage.resize((primary_len >> block_size_log2) + (secondary_len >> block_size_log2));
	mem.resize(buffer_len >> block_size_log2);

    double pretty_len = 0.0;
    const char * pretty_label = ff_pretty_size(secondary_len, & pretty_len);
    ff_log(FC_INFO, 0, "%s%s is %.2f %sbytes", sim_msg, label[FC_SECONDARY_STORAGE], pretty_len, pretty_label);

    pretty_label = ff_pretty_size(buffer_len, & pretty_len);
    ff_log(FC_NOTICE, 0, "%sRAM memory buffer is %.2f %sbytes", sim_msg, pretty_len, pretty_label);

    return 0;
}

/**
 * call umount(8) on dev_path()
 *
 * implementation: do nothing and return success
 */
int ft_io_mem::umount_dev()
{
    return 0;
}

/**
 * actually copy a list of fragments from DEVICE or FREE-STORAGE, to STORAGE to FREE-DEVICE.
 * must be implemented by sub-classes.
 * note: parameters are in bytes!
 * return 0 if success, else error.
 *
 * implementation: perform the copy on in-memory buffers
 */
int ft_io_mem::flush_copy_bytes(fr_dir dir, fr_vector<ft_uoff> & request_vec)
{
    return 0;
}

/**
 * flush any pending copy, i.e. actually perform all queued copies.
 * return 0 if success, else error
 *
 * implementation: do nothing and return success
 */
int ft_io_mem::flush_bytes()
{
    return 0;
}

/**
 * write zeroes to device (or to storage).
 * used to remove device-renumbered blocks once remapping is finished
 *
 * implementation: do nothing and return success
 */
int ft_io_mem::zero_bytes(fr_to FT_ARG_UNUSED(to), ft_uoff FT_ARG_UNUSED(offset), ft_uoff FT_ARG_UNUSED(length))
{
    return 0;
}

/**
 * write zeroes to primary storage.
 * used to remove primary-storage once remapping is finished
 * and clean the remaped file-system
 *
 * implementation: do nothing and return success
 */
int ft_io_mem::zero_primary_storage()
{
    return 0;
}


/**
 * close PRIMARY-STORAGE and SECONDARY-STORAGE. called by work<T>::close_storage()
 *
 * implementation: do nothing and return success
 */
int ft_io_mem::close_storage()
{
    return 0;
}


FT_IO_NAMESPACE_END
