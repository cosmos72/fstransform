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
 * io/io_test.cc
 *
 *  Created on: Mar 4, 2011
 *      Author: max
 */

#include "../first.hh"

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>        // for errno, EISCONN, ENOTCONN
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>         // for errno, EISCONN, ENOTCONN
#endif

#include "../log.hh"       // for ff_log()
#include "../args.hh"      // for fr_args
#include "../misc.hh"      // for ff_str2un_scaled()
#include "extent_file.hh"  // for ff_read_extents_file()
#include "io_test.hh"      // for fr_io_test


FT_IO_NAMESPACE_BEGIN

/** constructor */
fr_io_test::fr_io_test(fr_persist & persist)
: super_type(persist)
{
    /* mark this_f[] as invalid: they are not open yet */
    for (ft_size i = 0; i < FC_EXTENTS_FILE_COUNT; i++)
        this_f[i] = NULL;
}

/** destructor. calls close() */
fr_io_test::~fr_io_test()
{
    close();
}

/** return true if this fr_io_test is currently (and correctly) open */
bool fr_io_test::is_open() const
{
    return dev_length() != 0;
}

/** check for consistency and load LOOP-FILE and ZERO-FILE extents list from files */
int fr_io_test::open(const fr_args & args)
{
    if (is_open()) {
        // already open!
        ff_log(FC_ERROR, 0, "unexpected call to io_test::open(), I/O is already open");
        return EISCONN;
    }
    int err = fr_io::open(args);
    if (err != 0)
        return err;

    char const* const* io_args = args.io_args;
    ft_uoff dev_len;
    ft_size i = FC_DEVICE_LENGTH;
    err = ff_str2un_scaled(io_args[i], & dev_len);
    if (err != 0) {
        return ff_log(FC_ERROR, errno, "error parsing %s '%s'", extents_label[i], io_args[i]);
    }

    if (!is_replaying()) {
		for (i = FC_DEVICE_LENGTH+1; i < FC_EXTENTS_FILE_COUNT; i++) {
			if ((this_f[i] = fopen(io_args[i], "r")) == NULL) {
				err = ff_log(FC_ERROR, errno, "error opening %s '%s'", extents_label[i], io_args[i]);
				break;
			}
		}
    }
    if (err == 0) {
        dev_length(dev_len);
        dev_path("<test-device>");
    } else
        close();

    return err;
}


/** close a single descriptor/stream */
void fr_io_test::close0(ft_size i)
{
    if (this_f[i] != NULL) {
        if (fclose(this_f[i]) != 0)
            ff_log(FC_WARN, errno, "warning: failed to close %s", extents_label[i]);
        this_f[i] = NULL;
    }
}

/**
 * close file descriptors.
 * return 0 for success, 1 for error (prints by itself error message to stderr)
 */
void fr_io_test::close()
{
    close_extents();
    super_type::close();
}

/**
 * close the file descriptors for LOOP-FILE and ZERO-FILE
 */
void fr_io_test::close_extents()
{
    for (ft_size i = 0; i < FC_EXTENTS_FILE_COUNT; i++)
        close0(i);
}


/** return true if this I/O has open descriptors/streams to LOOP-FILE and FREE-SPACE */
bool fr_io_test::is_open_extents() const
{
    ft_size i, n = FC_EXTENTS_FILE_COUNT;
    for (i = FC_DEVICE_LENGTH+1; i < n; i++)
        if (this_f[i] == NULL)
            break;
    return i == n;
}




/**
 * retrieve LOOP-FILE extents, FREE-SPACE extents and any additional extents to be ZEROED
 * and insert them into the vectors loop_file_extents, free_space_extents and to_zero_extents
 * the vectors will be ordered by extent ->logical (for to_zero_extents, ->physical and ->logical will be the same).
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
 * this implementation simply reads extents from persistence files.
 */
int fr_io_test::read_extents(fr_vector<ft_uoff> & loop_file_extents,
                         fr_vector<ft_uoff> & free_space_extents,
                         fr_vector<ft_uoff> & to_zero_extents,
                         ft_uoff & ret_block_size_bitmask)
{
    ft_uoff block_size_bitmask = ret_block_size_bitmask;
    int err = 0;
    do {
        if (!is_open_extents()) {
            ff_log(FC_ERROR, 0, "unexpected call to io_test::read_extents(), I/O is not open");
            err = -ENOTCONN; // not open!
            break;
        }
        fr_vector<ft_uoff> * ret_extents[FC_EXTENTS_FILE_COUNT] = {
        	& loop_file_extents, & free_space_extents, & to_zero_extents,
        };
        for (ft_size i = FC_LOOP_EXTENTS; i < FC_EXTENTS_FILE_COUNT; i++) {
			/* ff_load_extents_file() appends to fr_vector<ft_uoff>, does NOT overwrite it */
			if ((err = ff_load_extents_file(this_f[i], * ret_extents[i], block_size_bitmask)) != 0) {
				err = ff_log(FC_ERROR, err, "error reading %s extents from save-file", extents_label[i]);
				break;
			}
        }
    } while (0);

    if (err == 0)
        ret_block_size_bitmask = block_size_bitmask;

    return err;
}

FT_IO_NAMESPACE_END
