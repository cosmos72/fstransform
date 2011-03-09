/*
 * io/extent_file.cc
 *
 *  Created on: Mar 3, 2011
 *      Author: max
 */

#include "../first.hh"

#include <cerrno>            // for errno, ENOMEM, EINVAL, EFBIG
#include <istream>           // for std::istream
#include <ostream>           // for std::ostream

#include "../types.hh"       // for ft_off
#include "../extent.hh"      // for ft_extent<T>
#include "../vector.hh"      // for ft_vector<T>
#include "extent_file.hh"    // for ff_read_extents_file()


FT_IO_NAMESPACE_BEGIN

/**
 * retrieves emulated file blocks allocation map (extents) from specified stream
 * and appends them to ret_list (retrieves also user_data)
 * in case of failure returns errno-compatible error code, and ret_list contents will be UNDEFINED.
 *
 * implementation: simply reads the list of triplets (physical, logical, length)
 * stored in the stream as decimal numbers
 */
int ff_read_extents_file(std::istream & is, ft_uoff dev_length, ft_vector<ft_uoff> & ret_list, ft_uoff & ret_block_size_bitmask)
{
    ft_uoff block_size_bitmask = ret_block_size_bitmask;
    ft_extent<ft_uoff> extent;

    while (!is.eof()) {
        is >> extent.physical() >> extent.logical() >> extent.length() >> extent.user_data();
        if (!is.good())
            break;

        block_size_bitmask |= extent.physical() | extent.logical() | extent.length();

        ret_list.push_back(extent);

    } while (0);

    int err;
    if (is.eof()) {
        ret_block_size_bitmask = block_size_bitmask;
        err = 0;
    } else {
        /* no idea what's the problem... */
        err = EINVAL;
    }

    return err;
}


/**
 * writes file blocks allocation map (extents) to specified stream (stores also user_data)
 * in case of failure returns errno-compatible error code.
 *
 * implementation: simply writes the list of triplets (physical, logical, length)
 * into the stream as decimal numbers
 */
int ff_write_extents_file(std::ostream & os, const ft_vector<ft_uoff> & extent_list)
{
    ft_uoff file_length = 0;
    if (!extent_list.empty()) {
        const ft_extent<ft_uoff> & last = extent_list.back();
        // extent_list is ordered either by ->physical or by ->logical
        file_length = last.length() + (last.logical() > last.physical() ? last.logical() : last.physical());
    }

    os << "length " << file_length << '\n';
    ft_vector<ft_uoff>::const_iterator iter = extent_list.begin(), end = extent_list.end();

    os << "physical\tlogical\tlength\tuser_data\n";
    for (; os.good() && iter != end; ++iter) {
        const ft_extent<ft_uoff> & extent = *iter;
        os << extent.physical() << '\t' << extent.logical() << '\t' << extent.length() << '\t' << extent.user_data() << '\n';
    }
    /* no idea what's the problem... */
    return os.good() ? 0 : EINVAL;
}


FT_IO_NAMESPACE_END
