/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 2 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * io/io_test.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSREMAP_IO_IO_NULL_HH
#define FSREMAP_IO_IO_NULL_HH

#include "../types.hh"    // for ft_uoff

#include "io.hh"          // for fr_io


FT_IO_NAMESPACE_BEGIN

/**
 * "null" class emulating I/O
 */
class ft_io_null: public fr_io
{
public:
    enum {
        FC_DEVICE_LENGTH = 0,
        FC_LOOP_EXTENTS,
        FC_FREE_SPACE_EXTENTS,
        FC_TO_ZERO_EXTENTS,
        FC_EXTENTS_FILE_COUNT // must be equal to count of preceding enum constants
    };

    static char const * const extents_label[]; // DEVICE-LENGTH, LOOP-EXTENTS, FREE-SPACE-EXTENTS and TO-ZERO-EXTENTS
    static char const * const sim_msg; // "(simulated) "

private:
    typedef fr_io super_type;

protected:

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
     * implementation: does nothing.
     */
    virtual int read_extents(fr_vector<ft_uoff> & loop_file_extents,
                             fr_vector<ft_uoff> & free_space_extents,
                             fr_vector<ft_uoff> & to_zero_extents,
                             ft_uoff & ret_block_size_bitmask);

    /**
     * actually copy a list of fragments from DEVICE or FREE-STORAGE, to STORAGE to FREE-DEVICE.
     * must be implemented by sub-classes.
     * note: parameters are in bytes!
     * return 0 if success, else error.
     *
     * implementation: do nothing and return success
     */
    virtual int flush_copy_bytes(fr_dir dir, fr_vector<ft_uoff> & request_vec);

    /**
     * flush any pending copy, i.e. actually perform all queued copies.
     * return 0 if success, else error
     * on return, 'ret_copied' will be increased by the number of blocks actually copied (NOT queued for copying),
     *
     * implementation: do nothing and return success
     */
    virtual int flush_bytes();

    /**
     * write zeroes to device (or to storage).
     * used to remove device-renumbered blocks once remapping is finished
     *
     * implementation: do nothing and return success
     */
    virtual int zero_bytes(fr_to to, ft_uoff offset, ft_uoff length);

public:
    /** constructor */
    ft_io_null(fr_persist & persist);

    /** destructor. does nothing. */
    virtual ~ft_io_null();

    /**
     * close the file descriptors for LOOP-FILE and ZERO-FILE
     *
     * implementation: do nothing and return success
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

    /**
     * call umount(8) on dev_path()
     *
     * implementation: do nothing and return success
     */
    virtual int umount_dev();

    /**
     * write zeroes to primary storage.
     * used to remove primary-storage once remapping is finished
     * and clean the remaped file-system
     *
     * implementation: do nothing and return success
     */
    virtual int zero_primary_storage();

    /**
     * close PRIMARY-STORAGE and SECONDARY-STORAGE. called by work<T>::close_storage()
     *
     * implementation: do nothing and return success
     */
    virtual int close_storage();
};

FT_IO_NAMESPACE_END

#endif /* FSREMAP_IO_IO_NULL_HH */
