/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 * 
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
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
 * io/io_self_test.hh
 *
 *  Created on: Feb 23, 2011
 *      Author: max
 */

#ifndef FSREMAP_IO_IO_SELF_TEST_HH
#define FSREMAP_IO_IO_SELF_TEST_HH

#include "../types.hh"    // for ft_uoff, ft_ull

#include "io_null.hh"     // for ft_io_null


FT_IO_NAMESPACE_BEGIN

/**
 * self-test class:
 * reports random LOOP-FILE and ZERO-FILE extents and emulates I/O
 */
class fr_io_self_test: public ft_io_null
{
private:
    typedef ft_io_null super_type;

    ft_ull this_block_size_log2;

    /** fill ret_extents with random (but consistent) extents. extents will stop at 'length' bytes */
    void invent_extents(fr_map<ft_uoff> & ret_extents, ft_uoff length, ft_uoff & ret_block_size_bitmask) const;

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
     *
     * implementation: fill loop_file_extents and free_space_extents
     * with random (but consistent) data.
     */
    virtual int read_extents(fr_vector<ft_uoff> & loop_file_extents,
                             fr_vector<ft_uoff> & free_space_extents,
                             ft_uoff & ret_block_size_bitmask);

public:
    /** constructor */
    fr_io_self_test(fr_persist & persist);

    /** destructor. calls close() */
    virtual ~fr_io_self_test();

    /** check for consistency and prepare for read_extents() */
    virtual int open(const fr_args & args);

    /** return true if this fr_io_posix is currently (and correctly) open */
    virtual bool is_open() const;

    /** close this I/O, including file descriptors to DEVICE, LOOP-FILE and ZERO-FILE */
    virtual void close();

    /** close any resource associated to LOOP-FILE and ZERO-FILE extents */
    virtual void close_extents();
};

FT_IO_NAMESPACE_END

#endif /* FSREMAP_IO_IO_SELF_TEST_HH */
