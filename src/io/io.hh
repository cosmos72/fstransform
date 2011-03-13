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


FT_IO_NAMESPACE_BEGIN

/**
 * abstract base class for all I/O implementations
 * that actually read and write on DEVICE
 */
class ft_io
{
private:
	ft_vector<ft_uoff> fm_primary_storage;
    ft_extent<ft_uoff> fm_secondary_storage;

    ft_uoff dev_len, eff_block_size_log2;

    ft_job & fm_job;

    /* cannot call copy constructor */
    ft_io(const ft_io &);

    /* cannot call assignment operator */
    const ft_io & operator=(const ft_io &);

protected:
    /** remember device length */
    FT_INLINE void dev_length(ft_uoff dev_length) { dev_len = dev_length; }

    /** compute and return log2() of effective block size and remember it */
    ft_uoff effective_block_size_log2(ft_uoff block_size_bitmask);

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
    FT_INLINE ft_extent<ft_uoff> & secondary_storage() { return fm_secondary_storage; }

public:
    enum {
        FC_DEVICE = 0, FC_LOOP_FILE
    };

    static char const * const label[]; // DEVICE, LOOP-FILE (and also others, but don't tell)

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

    /** return device length, or 0 if not open */
    FT_INLINE ft_uoff dev_length() const { return dev_len; }

    /** return log2 of effective block size, or 0 if not open */
    FT_INLINE ft_uoff effective_block_size_log2() const { return eff_block_size_log2; }



    /** return job */
    FT_INLINE ft_job & job() const { return fm_job; }

    /** return job_id, or 0 if not set */
    FT_INLINE ft_size job_id() const { return fm_job.job_id(); }

    /** return job_dir, or "" if not set */
    FT_INLINE const std::string & job_dir() const { return fm_job.job_dir(); }

    /** return storage_size to use (in bytes), or 0 if not set */
    FT_INLINE ft_size job_storage_size() const { return fm_job.job_storage_size(); }

    /** set storage_size to use (in bytes), or 0 to unset it */
    FT_INLINE void job_storage_size(ft_size len) { fm_job.job_storage_size(len); }

    /** return true if storage_size must be honored EXACTLY (to resume an existent job) */
    FT_INLINE bool job_storage_size_exact() const { return fm_job.job_storage_size_exact(); }

    /** set whether storage_size must be honored EXACTLY (to resume an existent job) */
    FT_INLINE void job_storage_size_exact(bool flag) { fm_job.job_storage_size_exact(flag); }

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
    FT_INLINE ft_vector<ft_uoff> & primary_storage() { return fm_primary_storage; }

    /** get const reference to primary_storage */
    FT_INLINE const ft_vector<ft_uoff> & primary_storage() const { return fm_primary_storage; }

    /**
     * create and open SECONDARY-STORAGE job.job_dir() + '/storage.bin' and fill it with 'secondary_len' bytes of zeros.
     * setup a virtual storage composed by this->primary_storage extents inside DEVICE, plus secondary-storage extents.
     * return 0 if success, else error
     */
    virtual int create_storage(ft_uoff secondary_len) = 0;
};

FT_IO_NAMESPACE_END


#endif /* FSTRANSFORM_IO_IO_HH */
