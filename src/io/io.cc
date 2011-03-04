/*
 * io/io.cc
 *
 *  Created on: Mar 1, 2011
 *      Author: max
 */

#include "../first.hh"

#include "io.hh"        // for ft_io

FT_IO_NAMESPACE_BEGIN


/** default constructor */
ft_io::ft_io()
    : dev_len(0), eff_block_size_log2(0)
{ }

/**
 * destructor.
 * sub-classes must override it to call close() if they override close()
 */
ft_io::~ft_io()
{ }


/**
 * close this ft_io.
 * sub-classes must override this method to perform appropriate cleanup
 */
void ft_io::close()
{
    dev_len = eff_block_size_log2 = 0;
}

/** compute log2() of effective block size and remember it */
ft_uoff ft_io::effective_block_size_log2(ft_uoff block_size_bitmask)
{
    ft_uoff block_size_log2 = 0;
    if (block_size_bitmask != 0) {
        while ((block_size_bitmask & 1) == 0) {
            block_size_log2++;
            block_size_bitmask >>= 1;
        }
    }
    return eff_block_size_log2 = block_size_log2;
}

/**
 * calls the 3-argument version of read_extents() and, if it succeeds,
 * calls effective_block_size_log2() to compute and remember effective block size
 */
int ft_io::read_extents(ft_vector<ft_uoff> & loop_file_extents,
                        ft_vector<ft_uoff> & free_space_extents)
{
    ft_uoff block_size_bitmask = 0;
    int err = read_extents(loop_file_extents, free_space_extents, block_size_bitmask);
    if (err == 0)
        (void) effective_block_size_log2(block_size_bitmask);
    return err;
}


FT_IO_NAMESPACE_END


