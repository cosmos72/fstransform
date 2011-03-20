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


    int fd[FC_ALL_FILE_COUNT];
    void * storage_mmap, * buffer_mmap;
    ft_size storage_mmap_size, buffer_mmap_size;

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
     * actually copy a single fragment from DEVICE or FREE-STORAGE, to STORAGE to FREE-DEVICE.
     * note: parameters are in bytes!
     * return 0 if success, else error.
     */
    virtual int copy_bytes(const ft_request & request);

    /** internal method called by copy_bytes() to read from DEVICE to mmapped() memory (either BUFFER or STORAGE) */
    int read_bytes(ft_dir dir, ft_uoff from_offset, ft_uoff to_offset, ft_uoff length);

    /** internal method called by copy_bytes() to write from mmapped() memory (either BUFFER or STORAGE) to DEVICE */
    int write_bytes(ft_dir dir, ft_uoff from_offset, ft_uoff to_offset, ft_uoff length);

    /**
     * flush any I/O specific buffer
     * return 0 if success, else error
     * implementation: call msync() because we use a mmapped() buffer for copy()
     * and call sync() because we write() to DEVICE
     */
    virtual int flush_bytes();

    /** internal method, called by flush_bytes() to perform msync() on mmapped storage */
    int msync_bytes(const ft_extent<ft_uoff> & extent) const;

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
