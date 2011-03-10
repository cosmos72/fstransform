/*
 * io/io.cc
 *
 *  Created on: Mar 1, 2011
 *      Author: max
 */

#include "../first.hh"

#include <cerrno>          // for ENOMEM, EINVAL...
#include <cstdlib>         // for malloc()
#include <cstring>         // for strlen(), memcpy()
#include <fstream>         // for std::ofstream

#include <string>          // for std::string

#include "io.hh"           // for ft_io
#include "extent_file.hh"  // for ff_write_extents_file()

FT_IO_NAMESPACE_BEGIN


/** constructor */
ft_io::ft_io(ft_job & job)
    : dev_len(0), eff_block_size_log2(0), fm_job(job)
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

/**
 * saves extents to files job.job_dir() + '/loop_extents.txt' and job.job_dir() + '/free_space_extents.txt'
 * by calling the function ff_write_extents_file()
 */
int ft_io::write_extents(const ft_vector<ft_uoff> & loop_file_extents,
                         const ft_vector<ft_uoff> & free_space_extents)
{
    static char const* const filename[] = { "loop_extents.txt", "free_space_extents.txt" };
    enum { FC_FILE_COUNT = sizeof(filename)/sizeof(filename[0]) };
    std::string path;
    const std::string & job_dir = fm_job.job_dir();
    ft_size i;
    int err = 0;
    for (i = 0; i < FC_FILE_COUNT; i++) {
        path = job_dir;
        path += filename[i];
        std::ofstream os(path.c_str(), std::ios_base::out|std::ios_base::trunc);

        if (!os.good()) {
            /* no idea what's the problem... */
            err = EINVAL;
            break;
        }
        if ((err = ff_write_extents_file(os, i == 0 ? loop_file_extents : free_space_extents)) != 0)
            break;
    }
    return err;
}



FT_IO_NAMESPACE_END


