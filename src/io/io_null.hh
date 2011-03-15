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
class ft_io_null: public ft_io
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

    /**
     * copy a single fragment from DEVICE to FREE-STORAGE, or from STORAGE to FREE-DEVICE or from DEVICE to FREE-DEVICE
     * (STORAGE to FREE-STORAGE copies could be supported easily, but are not considered useful)
     * note: parameters are in bytes!
     * note: this implementation will do nothing, increase ret_copied by length, and return success
     *
     * return 0 if success, else error
     *
     * on return, 'ret_queued' will be increased by the number of bytes actually copied or queued for copying,
     * which could be > 0 even in case of errors
     */
    virtual int copy_bytes(ft_uoff from_physical, ft_uoff to_physical, ft_uoff length, ft_uoff & ret_queued, ft_dir dir);

    /**
     * return number of blocks queued for copying.
     */
    virtual ft_uoff queued_bytes() const;

    /**
     * flush any pending copy, i.e. actually perform all queued copies.
     * return 0 if success, else error
     * on return, 'ret_copied' will be increased by the number of blocks actually copied (NOT queued for copying),
     *
     * implementation: do nothing and return success
     */
    virtual int flush_bytes(ft_uoff & ret_copied);


public:
    /** constructor */
    ft_io_null(ft_job & job);

    /** destructor. calls close() */
    virtual ~ft_io_null();

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
     * create SECONDARY-STORAGE as job.job_dir() + '.storage' and fill it with 'len' bytes of zeros,
     * setup a virtual storage composed by this->primary_storage extents inside DEVICE, plus secondary-storage extents.
     * return 0 if success, else error
     *
     * implementation: do nothing and return success
     */
    virtual int create_secondary_storage(ft_uoff len);
};

FT_IO_NAMESPACE_END

#endif /* FSTRANSFORM_IO_IO_EMUL_HH */
