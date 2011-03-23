/*
 * io/io_test.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_IO_IO_EMUL_HH
#define FSTRANSFORM_IO_IO_EMUL_HH

#include "../types.hh"    // for ft_uoff

#include <cstdio>         // for FILE

#include "io.hh"          // for ft_io


FT_IO_NAMESPACE_BEGIN

/**
 * "test" class emulating I/O
 */
class ft_io_test: public ft_io
{
public:
    enum {
        FC_LOOP_EXTENTS = 0,
        FC_FREE_SPACE_EXTENTS,
        FC_FILE_COUNT // must be equal to count of preceding enum constants
    };

    static char const * const extents_label[]; // LOOP-EXTENTS and FREE-SPACE-EXTENTS

private:
    typedef ft_io super_type;

    FILE * this_f[FC_FILE_COUNT];

protected:

    /** return true if this I/O has open descriptors/streams to LOOP-FILE and FREE-SPACE */
    bool is_open_extents() const;

    /** close a single descriptor/stream */
    void close0(ft_size which);

    /**
     * retrieve LOOP-FILE extents and FREE-SPACE extents and insert them into
     * the vectors loop_file_extents and free_space_extents.
     * the vectors will be ordered by extent ->logical.
     *
     * return 0 for success, else error (and vectors contents will be UNDEFINED).
     *
     * if success, also returns in ret_effective_block_size_log2 the log2()
     * of device effective block size.
     * the device effective block size is defined as follows:
     * it is the largest power of 2 that exactly divides all physical,
     * logical and lengths in all returned extents (both for LOOP-FILE
     * and for FREE-SPACE) and that also exactly exactly divides device length.
     *
     * the trick ft_io_posix uses to implement this method
     * is to fill the device's free space with a ZERO-FILE,
     * and actually retrieve the extents used by ZERO-FILE.
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
    ft_io_test(ft_job & job);

    /** destructor. calls close() */
    virtual ~ft_io_test();

    /** check for consistency and load LOOP-FILE and ZERO-FILE extents list from files */
    int open(char const* const path[FC_FILE_COUNT], ft_uoff dev_len);

    /** return true if this ft_io_posix is currently (and correctly) open */
    virtual bool is_open() const;

    /** close this I/O, including file descriptors to DEVICE, LOOP-FILE and ZERO-FILE */
    virtual void close();

    /**
     * close the file descriptors for LOOP-FILE and ZERO-FILE
     */
    virtual void close_extents();

    /**
     * create SECONDARY-STORAGE as job.job_dir() + '.storage' and fill it with 'len' bytes of zeros,
     * setup a virtual storage composed by this->primary_storage extents inside DEVICE, plus secondary-storage extents.
     * return 0 if success, else error
     *
     * implementation: do nothing and return success
     */
    virtual int create_storage(ft_size secondary_len, ft_size buffer_len);
};

FT_IO_NAMESPACE_END

#endif /* FSTRANSFORM_IO_IO_EMUL_HH */
