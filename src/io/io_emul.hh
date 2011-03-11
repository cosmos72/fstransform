/*
 * io/io_posix.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_IO_IO_EMUL_HH
#define FSTRANSFORM_IO_IO_EMUL_HH

#include "../types.hh"    // for ft_uoff */

#include <iosfwd>         // for std::ifstream forward declaration

#include "io.hh"          // for ft_io */


FT_IO_NAMESPACE_BEGIN

/**
 * "dummy" class emulating I/O
 */
class ft_io_emul: public ft_io
{
public:
    enum {
        FC_LOOP_EXTENTS = 0,
        FC_FREE_SPACE_EXTENTS,
        FC_FILE_COUNT // must be equal to count of preceding enum constants
    };

    static char const * const label[]; // LOOP-EXTENTS and FREE-SPACE-EXTENTS

private:
    typedef ft_io super_type;

    std::ifstream * is[FC_FILE_COUNT];

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

public:
    /** constructor */
    ft_io_emul(ft_job & job);

    /** destructor. calls close() */
    virtual ~ft_io_emul();

    /** check for consistency and open LOOP-FILE extents file and ZERO-FILE extents file */
    int open(char const* const paths[FC_FILE_COUNT]);

    /** return true if this ft_io_posix is currently (and correctly) open */
    virtual bool is_open() const;

    /** close this I/O, including file descriptors to DEVICE, LOOP-FILE and ZERO-FILE */
    virtual void close();

    /**
     * close the file descriptors for LOOP-FILE and ZERO-FILE
     */
    virtual void close_extents();

    /**
     * create and open file job.job_dir() + '/storage.bin' and fill it with job.job_storage_size() bytes of zeros.
     * return 0 if success, else error
     *
     * implementation: do nothing and return success
     */
    virtual int create_storage();
};

FT_IO_NAMESPACE_END

#endif /* FSTRANSFORM_IO_IO_EMUL_HH */
