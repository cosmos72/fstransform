/*
 * io/io_posix.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_IO_IO_POSIX_HH
#define FSTRANSFORM_IO_IO_POSIX_HH

#include "../types.hh"    // for ft_uoff */
#include "io.hh"          // for ft_io */


FT_IO_NAMESPACE_BEGIN

/**
 * class performing I/O on POSIX systems
 */
class ft_io_posix: public ft_io
{
public:
    enum {
        FC_DEVICE = ft_io::FC_DEVICE,
        FC_LOOP_FILE = ft_io::FC_LOOP_FILE,
        FC_ZERO_FILE,
        FC_FILE_COUNT, // must be equal to count of preceding enum constants,
        FC_SECONDARY_STORAGE = FC_FILE_COUNT,
        FC_ALL_FILE_COUNT,
        FC_PRIMARY_STORAGE = FC_ALL_FILE_COUNT,
        FC_STORAGE,
        FC_FREE_SPACE,
    };

private:
    typedef ft_io super_type;


    /**
     * simple I/O buffering class for ft_io_posix
     */
    class ft_queue
    {
    public:
        ft_uoff from_physical;
        ft_uoff to_physical;
        ft_uoff length;
        ft_dir  dir;

        /** construct a queue with no pending copies */
        ft_queue();

        /**
         * forget and discard any pending copy.
         */
        void clear();
    };


    int fd[FC_ALL_FILE_COUNT];
    void * storage_mmap;
    ft_size storage_mmap_size;

    ft_queue queue;

protected:

    /** return true if a single descriptor/stream is open */
    bool is_open0(ft_size which) const;

    /** close a single descriptor/stream */
    void close0(ft_size which);

    /** return true if this I/O has open descriptors/streams to LOOP-FILE and FREE-SPACE */
    bool is_open_extents() const;

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
     * replace a part of the mmapped() storage_mmap area with specified storage_extent,
     * and store mmapped() address into storage_extent.user_data().
     * return 0 if success, else error.
     *
     * note: fd shoud be this->fd[FC_DEVICE] for primary storage,
     * or this->fd[FC_SECONDARY_STORAGE] for secondary storage
     */
    int replace_storage_mmap(int fd, const char * label, ft_extent<ft_uoff> & storage_extent,
    		 ft_size extent_index, ft_size & mem_offset);

    /**
     * create and open SECONDARY-STORAGE in job.job_dir() + '.storage'
     * and fill it with 'secondary_len' bytes of zeros. do not mmap() it.
     * return 0 if success, else error
     */
    int create_secondary_storage(ft_uoff secondary_len);

    /** close and munmap() PRIMARY-STORAGE and SECONDARY-STORAGE. called by close() */
    void close_storage();


    /**
     * copy a single fragment from DEVICE to FREE-STORAGE, or from STORAGE to FREE-DEVICE or from DEVICE to FREE-DEVICE
     * (STORAGE to FREE-STORAGE copies could be supported easily, but are not considered useful)
     * note: parameters are in bytes!
     * note: this implementation will accumulate (queue) some copy requests, actual copy and error reporting may be delayed until flush()
     *
     * return 0 if success, else error
     *
     * on return, 'ret_queued' will be increased by the number of bytes actually copied or queued for copying,
     * which could be > 0 even in case of errors
     */
    virtual int copy_bytes(ft_uoff from_physical, ft_uoff to_physical, ft_uoff length, ft_uoff & ret_queued, ft_dir dir);


    /**
     * called by copy_bytes() after checking for invalid parameters.
     * queues the copy request if possible, else calls flush() and copies copy request to ft_queue
     * note: parameters are in bytes!
     *
     * return 0 if success, else error
     *
     * on return, 'ret_queued' will be increased by the number of bytes actually copied or queued for copying,
     * which could be > 0 even in case of errors
     */
    int queue_copy(ft_uoff from_physical, ft_uoff to_physical, ft_uoff length, ft_uoff & ret_queued, ft_dir dir);

    /**
     * flush any pending copy, i.e. actually perform all queued copies.
     * return 0 if success, else error
     *
     * on return, 'ret_copied' will be increased by the number of blocks actually copied (NOT queued for copying),
     * which could be > 0 even in case of errors
     */
    virtual int flush_bytes(ft_uoff & ret_copied);

    /**
     * return number of blocks queued for copying.
     */
    virtual ft_uoff queued_bytes() const;

public:
    /** constructor */
    ft_io_posix(ft_job & job);

    /** destructor. calls close() */
    virtual ~ft_io_posix();

    /** check for consistency and open DEVICE, LOOP-FILE and ZERO-FILE */
    int open(char const* const paths[FC_FILE_COUNT]);

    /** return true if this ft_io_posix is currently (and correctly) open */
    virtual bool is_open() const;

    /** close this I/O, including file descriptors to DEVICE, LOOP-FILE, ZERO-FILE and SECONDARY-STORAGE */
    virtual void close();

    /**
     * close the file descriptors for LOOP-FILE and ZERO-FILE
     */
    virtual void close_extents();

    /**
     * create and open SECONDARY-STORAGE in job.job_dir() + '.storage',
     * fill it with 'secondary_len' bytes of zeros and mmap() it.
     *
     * then mmap() together into consecutive RAM this->primary_storage extents and secondary_storage extents.
     *
     * return 0 if success, else error
     */
    virtual int create_storage(ft_uoff secondary_len);
};

FT_IO_NAMESPACE_END

#endif /* FSTRANSFORM_IO_IO_POSIX_HH */
