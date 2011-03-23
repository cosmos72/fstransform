/*
 * io/io_self_test.hh
 *
 *  Created on: Feb 23, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_IO_IO_SELF_TEST_HH
#define FSTRANSFORM_IO_IO_SELF_TEST_HH

#include "../types.hh"    // for ft_uoff, ft_ull */

#include "io.hh"          // for ft_io */


FT_IO_NAMESPACE_BEGIN

/**
 * self-test class:
 * reports random LOOP-FILE and ZERO-FILE extents and emulates I/O
 */
class ft_io_self_test: public ft_io
{
private:
    typedef ft_io super_type;

    ft_ull this_block_size_log2;

    /** fill ret_extents with random (but consistent) extents. extents will stop at 'length' bytes */
    void invent_extents(ft_map<ft_uoff> & ret_extents, ft_uoff length, ft_uoff & ret_block_size_bitmask) const;

protected:

    /**
     * retrieve LOOP-FILE extents and FREE-SPACE extents and insert them into
     * the vectors loop_file_extents and free_space_extents.
     * the vectors will be ordered by extent ->logical.
     *
     * return 0 for success, else error (and vectors contents will be UNDEFINED).
     *
     * if success, also returns in ret_effective_block_size_log2 the log2()
     * of device effective block size.
     *
     * implementation: fill loop_file_extents and free_space_extents
     * with random (but consistent) data.
     */
    virtual int read_extents(ft_vector<ft_uoff> & loop_file_extents,
                             ft_vector<ft_uoff> & free_space_extents,
                             ft_uoff & ret_block_size_bitmask);

    /**
     * actually copy a list of fragments from DEVICE or FREE-STORAGE, to STORAGE to FREE-DEVICE.
     * must be implemented by sub-classes.
     * note: parameters are in bytes!
     * return 0 if success, else error.
     *
     * implementation: do nothing and return success
     */
    virtual int copy_bytes(ft_dir dir, ft_vector<ft_uoff> & request_vec);

    /**
     * flush any pending copy, i.e. actually perform all queued copies.
     * return 0 if success, else error
     * on return, 'ret_copied' will be increased by the number of blocks actually copied (NOT queued for copying),
     *
     * implementation: do nothing and return success
     */
    virtual int flush_bytes();


public:
    /** constructor */
    ft_io_self_test(ft_job & job);

    /** destructor. calls close() */
    virtual ~ft_io_self_test();

    /** check for consistency and prepare for read_extents() */
    int open();

    /** return true if this ft_io_posix is currently (and correctly) open */
    virtual bool is_open() const;

    /** close this I/O, including file descriptors to DEVICE, LOOP-FILE and ZERO-FILE */
    virtual void close();

    /** close any resource associated to LOOP-FILE and ZERO-FILE extents */
    virtual void close_extents();

    /**
     * create and open SECONDARY-STORAGE job.job_dir() + '/storage.bin' and fill it with 'secondary_len' bytes of zeros.
     * setup a virtual storage composed by this->primary_storage extents inside DEVICE, plus secondary-storage extents.
     * return 0 if success, else error
     *
     * implementation: do nothing and return success
     */
    virtual int create_storage(ft_size secondary_len, ft_size mem_buffer_len);
};

FT_IO_NAMESPACE_END

#endif /* FSTRANSFORM_IO_IO_SELF_TEST_HH */
