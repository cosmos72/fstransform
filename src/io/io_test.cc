/*
 * io/io_test.cc
 *
 *  Created on: Mar 4, 2011
 *      Author: max
 */

#include "../first.hh"

#include <cerrno>          // for errno, EISCONN...
#include <fstream>         // for std::ifstream

#include "../log.hh"       // for ff_log()
#include "extent_file.hh"  // for ff_read_extents_file()
#include "io_test.hh"      // for ft_io_test


FT_IO_NAMESPACE_BEGIN

char const * const ft_io_test::extents_label[ft_io_test::FC_FILE_COUNT] = { "LOOP-EXTENTS", "FREE-SPACE-EXTENTS" };

/** constructor */
ft_io_test::ft_io_test(ft_job & job)
: super_type(job)
{
    /* mark fd[] as invalid: they are not open yet */
    for (ft_size i = 0; i < FC_FILE_COUNT; i++)
        this_f[i] = NULL;
}

/** destructor. calls close() */
ft_io_test::~ft_io_test()
{
    close();
}

/** return true if this ft_io_test is currently (and correctly) open */
bool ft_io_test::is_open() const
{
    return dev_length() != 0;
}

/** check for consistency and load LOOP-FILE and ZERO-FILE extents list from files */
int ft_io_test::open(char const* const path[FC_FILE_COUNT], ft_uoff dev_len)
{
    if (is_open()) {
        // already open!
        ff_log(FC_ERROR, 0, "unexpected call, I/O is already open");
        return EISCONN;
    }

    int err = 0;

    for (ft_size i = 0; i < FC_FILE_COUNT; i++) {
        if ((this_f[i] = fopen(path[i], "r")) == NULL) {
            err = ff_log(FC_ERROR, errno, "error opening %s '%s'", extents_label[i], path[i]);
            break;
        }
    }

    if (err == 0) {
        dev_length(dev_len);
        dev_path("<test-device>");
    } else
        close();

    return err;
}


/** close a single descriptor/stream */
void ft_io_test::close0(ft_size i)
{
    if (this_f[i] != NULL) {
        if (fclose(this_f[i]) != 0)
            ff_log(FC_WARN, errno, "warning: failed to close %s", extents_label[i]);
        this_f[i] = NULL;
    }
}

/**
 * close file descriptors.
 * return 0 for success, 1 for error (prints by itself error message to stderr)
 */
void ft_io_test::close()
{
    close_extents();
    super_type::close();
}

/**
 * close the file descriptors for LOOP-FILE and ZERO-FILE
 */
void ft_io_test::close_extents()
{
    for (ft_size i = 0; i < FC_FILE_COUNT; i++)
        close0(i);
}


/** return true if this I/O has open descriptors/streams to LOOP-FILE and FREE-SPACE */
bool ft_io_test::is_open_extents() const
{
    ft_size i = 0, n = FC_FILE_COUNT;
    for (i = 0; i < n; i++)
        if (this_f[i] == NULL)
            break;
    return i == n;
}




/**
 * retrieve LOOP-FILE extents and FREE-SPACE extents and append them into
 * the vectors loop_file_extents and free_space_extents.
 * the vectors will be ordered by extent ->logical.
 *
 * return 0 for success, else error (and vectors contents will be UNDEFINED).
 *
 * implementation: load extents list from files
 * (for example they could be the job persistence files)
 */
int ft_io_test::read_extents(ft_vector<ft_uoff> & loop_file_extents,
                             ft_vector<ft_uoff> & free_space_extents,
                             ft_uoff & ret_block_size_bitmask)
{
    ft_uoff block_size_bitmask = ret_block_size_bitmask;
    int err = 0;
    do {
        if (!is_open_extents()) {
            err = ENOTCONN; // not open!
            break;
        }
        /* ff_load_extents_file() appends to ft_vector<ft_uoff>, does NOT overwrite it */
        if ((err = ff_load_extents_file(this_f[FC_LOOP_EXTENTS], loop_file_extents, block_size_bitmask)) != 0)
            break;
        if ((err = ff_load_extents_file(this_f[FC_FREE_SPACE_EXTENTS], free_space_extents, block_size_bitmask)) != 0)
            break;
    } while (0);

    if (err == 0)
        ret_block_size_bitmask = block_size_bitmask;

    return err;
}


/**
 * create SECONDARY-STORAGE as job.job_dir() + '.storage' and fill it with 'len' bytes of zeros,
 * setup a virtual storage composed by this->primary_storage extents inside DEVICE, plus secondary-storage extents.
 * return 0 if success, else error
 *
 * implementation: do nothing and return success
 */
int ft_io_test::create_secondary_storage(ft_uoff secondary_len, ft_uoff buffer_len)
{
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
int ft_io_test::copy_bytes(ft_dir dir, ft_vector<ft_uoff> & request_vec)
{
    return 0;
}

/**
 * flush any pending copy, i.e. actually perform all queued copies.
 * return 0 if success, else error
 *
 * implementation: do nothing and return success
 */
int ft_io_test::flush_bytes()
{
    return 0;
}



FT_IO_NAMESPACE_END
