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
#include "../map.hh"         // for ft_map<T>


FT_IO_NAMESPACE_BEGIN

/**
 * abstract base class for all I/O implementations
 * that actually read and write on DEVICE
 */
class ft_io
{
private:
    ft_uoff dev_len, eff_block_size_log2;

    /* cannot call copy constructor */
    ft_io(const ft_io &);

    /* cannot call assignment operator */
    const ft_io & operator=(const ft_io &);

protected:
    /** remember device length */
    FT_INLINE void dev_length(ft_uoff dev_length) { dev_len = dev_length; }

    /** compute log2() of effective block size and remember it */
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

public:
    enum {
        FC_DEVICE = 0, FC_LOOP_FILE
    };

    static char const * const label[]; // DEVICE, LOOP-FILE (and ZERO-FILE, but don't tell)

    /** default constructor */
    ft_io();

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

    /**
     * calls the 3-argument version of read_extents() and, if it succeeds,
     * calls effective_block_size_log2() to compute and remember effective block size
     */
    int read_extents(ft_vector<ft_uoff> & loop_file_extents,
                     ft_vector<ft_uoff> & free_space_extents);
};

FT_IO_NAMESPACE_END


#endif /* FSTRANSFORM_IO_IO_HH */
