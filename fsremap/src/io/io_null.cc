/*
 * io/io_null.cc
 *
 *  Created on: Mar 4, 2011
 *      Author: max
 */

#include "../first.hh"

#include "../log.hh"       // for ff_log()
#include "../util.hh"      // for ff_pretty_size()
#include "io_null.hh"      // for ft_io_null


FT_IO_NAMESPACE_BEGIN

char const * const ft_io_null::extents_label[ft_io_null::FC_FILE_COUNT] = { "DEVICE-LENGTH", "LOOP-EXTENTS", "FREE-SPACE-EXTENTS" };

char const * const ft_io_null::sim_msg = "SIMULATED ";


/** constructor */
ft_io_null::ft_io_null(fr_job & job)
: super_type(job)
{
    /* tell job that we're a simulation */
    job.simulate_run(true);
}

/** destructor. does nothing */
ft_io_null::~ft_io_null()
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
int ft_io_null::read_extents(fr_vector<ft_uoff> & loop_file_extents,
                             fr_vector<ft_uoff> & free_space_extents,
                             ft_uoff & ret_block_size_bitmask)
{
    return 0;
}

/**
 * close the file descriptors for LOOP-FILE and ZERO-FILE
 *
 * implementation: do nothing and return success
 */
void ft_io_null::close_extents()
{ }

/**
 * create SECONDARY-STORAGE as job.job_dir() + '.storage' and fill it with 'len' bytes of zeros,
 * setup a virtual storage composed by this->primary_storage extents inside DEVICE, plus secondary-storage extents.
 * return 0 if success, else error
 *
 * implementation: do nothing and return success
 */
int ft_io_null::create_storage(ft_size secondary_len, ft_size buffer_len)
{
    double pretty_len = 0.0;
    const char * pretty_label = ff_pretty_size(secondary_len, & pretty_len);
    ff_log(FC_INFO, 0, "%s%s is %.2f %sbytes", sim_msg, label[FC_SECONDARY_STORAGE], pretty_len, pretty_label);

    pretty_label = ff_pretty_size(buffer_len, & pretty_len);
    ff_log(FC_NOTICE, 0, "%sRAM memory buffer is %.2f %sbytes", sim_msg, pretty_len, pretty_label);

    return 0;
}


/**
 * actually copy a list of fragments from DEVICE or FREE-STORAGE, to STORAGE to FREE-DEVICE.
 * must be implemented by sub-classes.
 * note: parameters are in bytes!
 * return 0 if success, else error.
 *
 * implementation: do nothing and return success
 */
int ft_io_null::copy_bytes(fr_dir dir, fr_vector<ft_uoff> & request_vec)
{
    return 0;
}

/**
 * flush any pending copy, i.e. actually perform all queued copies.
 * return 0 if success, else error
 *
 * implementation: do nothing and return success
 */
int ft_io_null::flush_bytes()
{
    return 0;
}

/**
 * write zeroes to device (or to storage).
 * used to remove device-renumbered blocks once relocation is finished
 *
 * implementation: do nothing and return success
 */
int ft_io_null::zero_bytes(fr_to to, ft_uoff offset, ft_uoff length)
{
    return 0;
}

/**
 * write zeroes to primary storage.
 * used to remove primary-storage once relocation is finished
 * and clean the remaped file-system
 *
 * implementation: do nothing and return success
 */
int ft_io_null::zero_primary_storage()
{
    return 0;
}


/**
 * close PRIMARY-STORAGE and SECONDARY-STORAGE. called by work<T>::close_storage()
 *
 * implementation: do nothing and return success
 */
int ft_io_null::close_storage()
{
    return 0;
}


FT_IO_NAMESPACE_END
