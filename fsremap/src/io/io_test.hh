/*
 * io/io_test.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSREMAP_IO_IO_TEST_HH
#define FSREMAP_IO_IO_TEST_HH

#include "../types.hh"    // for ft_uoff

#include <cstdio>         // for FILE

#include "io_null.hh"     // for ft_io_null


FT_IO_NAMESPACE_BEGIN

/**
 * "test" class emulating I/O.
 * actually loads extents definition from persistence files
 */
class fr_io_test: public ft_io_null
{
private:
    typedef ft_io_null super_type;

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
     * the trick fr_io_posix uses to implement this method
     * is to fill the device's free space with a ZERO-FILE,
     * and actually retrieve the extents used by ZERO-FILE.
     */
    virtual int read_extents(fr_vector<ft_uoff> & loop_file_extents,
                             fr_vector<ft_uoff> & free_space_extents,
                             ft_uoff & ret_block_size_bitmask);

public:
    /** constructor */
    fr_io_test(fr_job & job);

    /** destructor. calls close() */
    virtual ~fr_io_test();

    /** check for consistency and load LOOP-FILE and ZERO-FILE extents list from files */
    int open(char const* const args[FC_FILE_COUNT]);

    /** return true if this fr_io_posix is currently (and correctly) open */
    virtual bool is_open() const;

    /** close this I/O, including file descriptors to DEVICE, LOOP-FILE and ZERO-FILE */
    virtual void close();

    /**
     * close the file descriptors for LOOP-FILE and ZERO-FILE
     */
    virtual void close_extents();
};

FT_IO_NAMESPACE_END

#endif /* FSREMAP_IO_IO_TEST_HH */
