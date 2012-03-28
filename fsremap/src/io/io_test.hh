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
 * io/io_test.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSREMAP_IO_IO_TEST_HH
#define FSREMAP_IO_IO_TEST_HH

#include "../types.hh"     // for ft_uoff, ft_size

#if defined(FT_HAVE_STDIO_H)
# include <stdio.h>        // for FILE. also for fopen(), fclose() used in io_test.cc
#elif defined(FT_HAVE_CSTDIO)
# include <cstdio>         // for FILE. also for fopen(), fclose() used in io_test.cc
#endif

#include "io_null.hh"      // for ft_io_null


FT_IO_NAMESPACE_BEGIN

/**
 * "test" class emulating I/O.
 * actually loads extents definition from persistence files
 */
class fr_io_test: public ft_io_null
{
private:
    typedef ft_io_null super_type;

    FILE * this_f[FC_FILE_COUNT];

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
     * the trick fr_io_posix uses to implement this method
     * is to fill the device's free space with a ZERO-FILE,
     * and actually retrieve the extents used by ZERO-FILE.
     */
    virtual int read_extents(fr_vector<ft_uoff> & loop_file_extents,
                             fr_vector<ft_uoff> & free_space_extents,
                             ft_uoff & ret_block_size_bitmask);

public:
    /** constructor */
    fr_io_test(fr_persist & persist);

    /** destructor. calls close() */
    virtual ~fr_io_test();

    /** check for consistency and load LOOP-FILE and ZERO-FILE extents list from files */
    virtual int open(const fr_args & args);

    /** return true if this fr_io_posix is currently (and correctly) open */
    virtual bool is_open() const;

    /** close this I/O, including file descriptors to DEVICE, LOOP-FILE and ZERO-FILE */
    virtual void close();

    /**
     * close the file descriptors for LOOP-FILE and ZERO-FILE
     */
    virtual void close_extents();
};

FT_IO_NAMESPACE_END

#endif /* FSREMAP_IO_IO_TEST_HH */
