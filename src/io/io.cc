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

#include "../log.hh"       // for ff_log()
#include "../util.hh"      // for ff_can_sum()
#include "../ui/ui.hh"     // for ft_ui
#include "io.hh"           // for ft_io
#include "extent_file.hh"  // for ff_write_extents_file()

FT_IO_NAMESPACE_BEGIN


char const * const ft_io::label[] = {
        "device", "loop-file", "zero-file", "secondary-storage", "primary-storage", "storage", "free-space"
};


char const* const ft_io::extents_filename[FC_IO_EXTENTS_FILE_COUNT] = {
    "/loop_extents.txt", "/free_space_extents.txt"
};


/** constructor */
ft_io::ft_io(ft_job & job)
    : this_primary_storage(), request_vec(), this_dev_length(0), this_eff_block_size_log2(0),
      this_dev_path(NULL), this_job(job), request_dir(FC_INVALID2INVALID), this_delegate_ui(false)
{
    this_secondary_storage.clear();
}

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
    this_primary_storage.clear();
    this_secondary_storage.clear();
    request_vec.clear();
    request_dir = FC_INVALID2INVALID;

    this_dev_length = this_eff_block_size_log2 = 0;
    this_dev_path = NULL;
}

/** compute and return log2() of effective block size and remember it */
ft_uoff ft_io::effective_block_size_log2(ft_uoff block_size_bitmask)
{
    ft_uoff block_size_log2 = 0;
    if (block_size_bitmask != 0) {
        while ((block_size_bitmask & 1) == 0) {
            block_size_log2++;
            block_size_bitmask >>= 1;
        }
    }
    return this_eff_block_size_log2 = block_size_log2;
}

/* return (-)EOVERFLOW if request from/to + length overflow specified maximum value */
int ft_io::validate(const char * type_name, ft_uoff type_max, ft_dir dir, ft_uoff from, ft_uoff to, ft_uoff length)
{
    to = ff_max2(from, to);
    if (!ff_can_sum(to, length) || length > type_max || to > type_max - length) {
        return ff_log(FC_FATAL, EOVERFLOW, "internal error! %s to %s io.copy(dir = %d, from_physical = %"FS_ULL", to_physical = %"FS_ULL", length = %"FS_ULL")"
                      " overflows maximum allowed (%s)%"FS_ULL,
                      ff_is_from_dev(dir) ? label[FC_DEVICE] : label[FC_STORAGE],
                      ff_is_to_dev(dir) ? label[FC_DEVICE] : label[FC_STORAGE],
                      (int)dir, (ft_ull)from, (ft_ull)to, (ft_ull)length, type_name, (ft_ull)type_max);
    }
    return 0;
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
    if (err == 0) {
        ft_uoff eff_block_size_log2 = effective_block_size_log2(block_size_bitmask);
        ff_log(FC_INFO, 0, "%s effective block size = %"FS_ULL, label[FC_DEVICE], (ft_ull) 1 << eff_block_size_log2);
    }
    return err;
}



/**
 * loads extents from file job.job_dir() + '/loop_extents.txt' and job.job_dir() + '/free_space_extents.txt'
 * by calling the function ff_load_extents_file()
 * if successful, calls effective_block_size_log2() to compute and remember effective block size
 */
int ft_io::load_extents(ft_vector<ft_uoff> & loop_file_extents,
                        ft_vector<ft_uoff> & free_space_extents)
{
    std::string path;
    const std::string & job_dir = this_job.job_dir();
    FILE * f = NULL;
    const char * path_cstr = NULL;
    int err = 0;

    ft_uoff eff_block_size_log2 = 0;

    for (ft_size i = 0; err == 0 && i < FC_IO_EXTENTS_FILE_COUNT; i++) {
        path = job_dir;
        path += extents_filename[i];
        path_cstr = path.c_str();
        if ((f = fopen(path_cstr, "r")) == NULL) {
            err = ff_log(FC_ERROR, errno, "error opening persistence file '%s'", path_cstr);
            break;
        }
        if ((err = ff_load_extents_file(f, (i == 0 ? loop_file_extents : free_space_extents), eff_block_size_log2)) != 0)
            err = ff_log(FC_ERROR, err, "error reading persistence file '%s'", path_cstr);

        if (fclose(f) != 0) {
            ff_log(FC_WARN, errno, "warning: failed to close persistence file '%s'", path_cstr);
            f = NULL;
        }
    }
    if (err == 0)
        this_eff_block_size_log2 = eff_block_size_log2;

    return err;
}

/**
 * saves extents to files job.job_dir() + '/loop_extents.txt' and job.job_dir() + '/free_space_extents.txt'
 * by calling the function ff_save_extents_file()
 */
int ft_io::save_extents(const ft_vector<ft_uoff> & loop_file_extents,
                        const ft_vector<ft_uoff> & free_space_extents) const
{
    std::string path;
    const std::string & job_dir = this_job.job_dir();
    FILE * f = NULL;
    const char * path_cstr = NULL;
    int err = 0;
    for (ft_size i = 0; err == 0 && i < FC_IO_EXTENTS_FILE_COUNT; i++) {
        path = job_dir;
        path += extents_filename[i];
        path_cstr = path.c_str();
        if ((f = fopen(path_cstr, "w")) == NULL) {
            err = ff_log(FC_ERROR, errno, "error opening persistence file '%s'", path_cstr);
            break;
        }
        if ((err = ff_save_extents_file(f, i == 0 ? loop_file_extents : free_space_extents)) != 0)
            err = ff_log(FC_ERROR, err, "error writing to persistence file '%s'", path_cstr);

        if (fclose(f) != 0) {
            ff_log(FC_WARN, errno, "error closing persistence file '%s'", path_cstr);
            f = NULL;
        }
    }
    return err;
}



/**
 * perform buffering and coalescing of copy requests.
 * queues a copy of single fragment from DEVICE or FREE-STORAGE, to STORAGE to FREE-DEVICE.
 * calls flush_queue() as needed to actually perform any copy that cannot be buffered or coalesced.
 * note: parameters are in bytes!
 * return 0 if success, else error
 */
int ft_io::copy_queue(ft_dir dir, ft_uoff from_physical, ft_uoff to_physical, ft_uoff length)
{
    int err = 0;
    if (!request_vec.empty() && request_dir != dir) {
        if ((err = flush_queue()) != 0)
            return err;
    }
    if ((err = validate("ft_uoff", (ft_uoff)-1, dir, from_physical, to_physical, length)) != 0)
        return err;

    if (this_ui != 0 && !this_delegate_ui)
        this_ui->show_io_copy(dir, from_physical, to_physical, length);

    request_dir = dir;
    request_vec.append(from_physical, to_physical, length, FC_DEFAULT_USER_DATA);
    return err;
}


/**
 * flush any pending copy, i.e. actually perform all queued copies.
 * return 0 if success, else error
 * on return, 'ret_copied' will be increased by the number of blocks actually copied (NOT queued for copying),
 * which could be > 0 even in case of errors
 */
int ft_io::flush_queue()
{
    int err = 0;
    if (!request_vec.empty()) {
        err = copy_bytes(request_dir, request_vec);
        request_vec.clear();
        request_dir = FC_INVALID2INVALID;
    }
    return err;
}

/**
 * flush any I/O specific buffer
 * return 0 if success, else error
 * default implementation: do nothing
 */
int ft_io::flush_bytes()
{
    return 0;
}

/**
 * flush any pending copy (call copy_bytes() through flush_queue()),
 * plus flush any I/O specific buffer (call flush_bytes())
 * return 0 if success, else error
 */
int ft_io::flush()
{
    int err = flush_queue();
    if (err == 0)
        err = flush_bytes();
    if (err == 0 && this_ui != 0 && !this_delegate_ui)
        this_ui->show_io_flush();
    return err;
}

FT_IO_NAMESPACE_END


