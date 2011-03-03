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
#include "../extent_list.hh" // for ft_extent_list


FT_IO_NAMESPACE_BEGIN

/**
 * abstract base class for all I/O implementations
 * that actually read and write on DEVICE
 */
class ft_io
{
private:
    ft_uoff dev_len;

    /* cannot call copy constructor */
    ft_io(const ft_io &);

    /* cannot call assignment operator */
    const ft_io & operator=(const ft_io &);

protected:
    /** remember device length */
    FT_INLINE void dev_length(ft_uoff dev_length) { dev_len = dev_length; }

    /**
     * retrieve LOOP-FILE extents and insert them into ret_list.
     * return 0 for success, else error (and ret_list contents will be UNDEFINED).
     *
     * must be overridden by sub-classes.
     * implementations must also check that device blocks count can be represented by ret_list,
     * by calling ret_list.extent_set_range(block_size, block_count)
     */
    virtual int loop_file_extents_list(ft_extent_list & ret_list) = 0;

    /**
     * retrieve FREE SPACE extents and insert them into ret_list.
     * a possible trick subclasses may use to implement this method
     * is to fill the device free space with a ZERO-FILE,
     * and actually retrieve the extents used by ZERO-FILE.
     *
     * return 0 for success, else error (and ret_list contents will be UNDEFINED).
     */
    virtual int free_space_extents_list(ft_extent_list & ret_list) = 0;

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

    /**
     * retrieve LOOP-FILE extents and insert them into ret_map.
     * return 0 for success, else error (and ret_map contents will be unchanged).
     *
     * also checks that device blocks count can be represented by T
     */
    template<class T>
    int loop_file_extents(ft_map<T> & ret_map);

    /**
     * given LOOP-FILE extents, compute DEVICE extents and insert them into ret_map.
     *
     * implementation: computes union of specified loop_file_map
     * and free_space_extents_list(), then complements the union.
     *
     * return 0 for success, else error (and ret_map contents will be unchanged).
     */
    template<class T>
    int device_extents(const ft_map<T> & loop_file_map, ft_map<T> & ret_map);
};

FT_IO_NAMESPACE_END

#include "io.template.hh"


#endif /* FSTRANSFORM_IO_IO_HH */
