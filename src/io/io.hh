/*
 * io/io.hh
 *
 *  Created on: Mar 1, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_IO_IO_HH
#define FSTRANSFORM_IO_IO_HH

#include "../check.hh"

#include "../types.hh"       // for ft_uoff
#include "../job.hh"         // for ft_job
#include "../extent.hh"      // for ft_extent<T>
#include "../vector.hh"      // for ft_vector<T>
#include "../map.hh"         // for ft_map<T>

#include "request.hh"        // for ft_request


FT_IO_NAMESPACE_BEGIN

/**
 * abstract base class for all I/O implementations
 * that actually read and write on DEVICE
 */
class ft_io
{
public:
    enum {
        FC_DEVICE = 0, FC_LOOP_FILE
    };

    static char const * const label[]; // DEVICE, LOOP-FILE (and also others, but don't tell)

private:
	ft_vector<ft_uoff> this_primary_storage;
    ft_extent<ft_uoff> this_secondary_storage;
    ft_vector<ft_uoff> request_vec;

    ft_uoff this_dev_length, this_eff_block_size_log2;
    const char * this_dev_path;
    ft_job & this_job;
    ft_dir request_dir;


    /* cannot call copy constructor */
    ft_io(const ft_io &);

    /* cannot call assignment operator */
    const ft_io & operator=(const ft_io &);

protected:
    enum {
        FC_ZERO_FILE = FC_LOOP_FILE + 1,
        FC_SECONDARY_STORAGE,
        FC_PRIMARY_STORAGE,
        FC_STORAGE,
        FC_FREE_SPACE,
    };

    /** remember device length */
    FT_INLINE void dev_length(ft_uoff dev_length) { this_dev_length = dev_length; }

    /** remember device path */
    FT_INLINE void dev_path(const char * dev_path) { this_dev_path = dev_path; }

    /** compute and return log2() of effective block size and remember it */
    ft_uoff effective_block_size_log2(ft_uoff block_size_bitmask);

    /* return (-)EOVERFLOW if request from/to + length overflow specified maximum value */
    static int validate(const char * type_name, ft_uoff type_max, ft_dir dir, ft_uoff from, ft_uoff to, ft_uoff length);

    /* return (-)EOVERFLOW if request from/to + length overflow specified maximum value */
    FT_INLINE static int validate(const char * type_name, ft_uoff type_max, ft_dir dir, const ft_extent<ft_uoff> & extent)
    {
        return validate(type_name, type_max, dir, extent.physical(), extent.logical(), extent.length());
    }

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
     * must be overridden by sub-classes.
     *
     * a common trick subclasses may use to implement this method
     * is to fill the device's free space with a ZERO-FILE,
     * and actually retrieve the extents used by ZERO-FILE.
     */
    virtual int read_extents(ft_vector<ft_uoff> & loop_file_extents,
                             ft_vector<ft_uoff> & free_space_extents,
                             ft_uoff & ret_effective_block_size_log2) = 0;

    /**
     * get writable reference to secondary_storage.
     * must be called by create_storage() to set the details of secondary_storage
     */
    FT_INLINE ft_extent<ft_uoff> & secondary_storage() { return this_secondary_storage; }

    /**
     * perform buffering and coalescing of copy requests.
     * queues a copy of single fragment from DEVICE or FREE-STORAGE, to STORAGE to FREE-DEVICE.
     * calls flush_queue() as needed to actually perform any copy that cannot be buffered or coalesced.
     * note: parameters are in bytes!
     * return 0 if success, else error
     */
    int copy_queue(ft_dir dir, ft_uoff from_physical, ft_uoff to_physical, ft_uoff length);

    /**
     * flush any pending copy, i.e. actually call copy_bytes(request).
     * return 0 if success, else error
     * on return, 'ret_copied' will be increased by the number of blocks actually copied (NOT queued for copying),
     * which could be > 0 even in case of errors
     */
    int flush_queue();


    /**
     * actually copy a list of fragments from DEVICE or FREE-STORAGE, to STORAGE to FREE-DEVICE.
     * must be implemented by sub-classes.
     * note: parameters are in bytes!
     * return 0 if success, else error.
     */
    virtual int copy_bytes(ft_dir dir, ft_vector<ft_uoff> & request_vec) = 0;

    /**
     * flush any I/O specific buffer
     * return 0 if success, else error
     * default implementation: do nothing
     */
    virtual int flush_bytes();


public:
    /** constructor */
    ft_io(ft_job & job);

    /**
     * destructor.
     * sub-classes must override it to call close() if they override close()
     */
    virtual ~ft_io();

    /** return true if this ft_io is currently (and correctly) open */
    virtual bool is_open() const = 0;

    /**
     * close this ft_io.
     * sub-classes must override this method to perform appropriate cleanup
     */
    virtual void close();

    /** return device length (in bytes), or 0 if not open */
    FT_INLINE ft_uoff dev_length() const { return this_dev_length; }

    /** return device path, or NULL if not open */
    FT_INLINE const char * dev_path() const { return this_dev_path; }

    /** return log2 of effective block size, or 0 if not open */
    FT_INLINE ft_uoff effective_block_size_log2() const { return this_eff_block_size_log2; }



    /** return job */
    FT_INLINE ft_job & job() const { return this_job; }

    /** return job_id, or 0 if not set */
    FT_INLINE ft_size job_id() const { return this_job.job_id(); }

    /** return job_dir, or "" if not set */
    FT_INLINE const std::string & job_dir() const { return this_job.job_dir(); }

    /** return storage_size to use (in bytes), or 0 if not set */
    FT_INLINE ft_size job_storage_size(ft_storage_size which) const { return this_job.job_storage_size(which); }

    /** set storage_size to use (in bytes), or 0 to unset it */
    FT_INLINE void job_storage_size(ft_storage_size which, ft_size len) { this_job.job_storage_size(which, len); }

    /**
     * return true if I/O classes should be less strict on sanity checks
     * and generate WARNINGS (and keep going) for failed sanity checks
     * instead of generating ERRORS (and quitting)
     */
    FT_INLINE bool force_run() const { return this_job.force_run(); }

    /** return true if subclasses should simulate run, i.e. run WITHOUT reading or writing device blocks */
    FT_INLINE bool simulate_run() const { return this_job.simulate_run(); }

    /**
     * calls the 3-argument version of read_extents() and, if it succeeds,
     * calls effective_block_size_log2() to compute and remember effective block size
     */
    int read_extents(ft_vector<ft_uoff> & loop_file_extents,
                     ft_vector<ft_uoff> & free_space_extents);

    /**
     * saves extents to files job.job_dir() + '/loop_extents.txt' and job.job_dir() + '/free_space_extents.txt'
     * by calling the function ff_write_extents_file()
     */
    int write_extents(const ft_vector<ft_uoff> & loop_file_extents,
                      const ft_vector<ft_uoff> & free_space_extents);

    /**
     * close the file descriptors for LOOP-FILE and ZERO-FILE
     */
    virtual void close_extents() = 0;



    /**
     * get writable reference to primary_storage.
     * must be called before create_storage() to set the details of primary_storage
     */
    FT_INLINE ft_vector<ft_uoff> & primary_storage() { return this_primary_storage; }

    /** get const reference to primary_storage */
    FT_INLINE const ft_vector<ft_uoff> & primary_storage() const { return this_primary_storage; }

    /**
     * create and open SECONDARY-STORAGE job.job_dir() + '/storage.bin' and fill it with 'secondary_len' bytes of zeros.
     * setup a virtual storage composed by this->primary_storage extents inside DEVICE, plus secondary-storage extents.
     * return 0 if success, else error
     */
    virtual int create_storage(ft_size secondary_len, ft_size mem_buffer_len) = 0;

    /**
     * copy a single fragment from DEVICE to FREE-STORAGE, or from STORAGE to FREE-DEVICE or from DEVICE to FREE-DEVICE
     * (STORAGE to FREE-STORAGE copies could be supported easily, but are not considered useful)
     * note: parameters are in bytes!
     * note: implementations may accumulate (queue) copy requests, actual copy and error reporting may be delayed until flush()
     *
     * return 0 if success, else error
     *
     * on return, 'ret_copied' will be increased by the number of blocks actually copied or queued for copying,
     * which could be > 0 even in case of errors
     */
    template<typename T>
    int copy(ft_dir dir, T from_physical, T to_physical, T length)
    {
        /** TODO: move buffering to work<T> */
        return copy_queue(dir,
                          (ft_uoff)from_physical << this_eff_block_size_log2,
                          (ft_uoff)to_physical << this_eff_block_size_log2,
                          (ft_uoff)length  << this_eff_block_size_log2);
    }

    /**
     * flush any pending copy (call copy_bytes() through flush_queue()),
     * plus flush any I/O specific buffer (call flush_bytes())
     * return 0 if success, else error
     */
    int flush();
};



FT_IO_NAMESPACE_END



#endif /* FSTRANSFORM_IO_IO_HH */
