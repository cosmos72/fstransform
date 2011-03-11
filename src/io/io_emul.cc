/*
 * io/io_emul.cc
 *
 *  Created on: Mar 4, 2011
 *      Author: max
 */

#include "../first.hh"

#include <cerrno>          // for errno, EISCONN...
#include <fstream>         // for std::ifstream

#include "../log.hh"       // for ff_log()
#include "extent_file.hh"  // for ff_read_extents_file()
#include "io_emul.hh"      // for ft_io_emul


FT_IO_NAMESPACE_BEGIN

char const * const ft_io_emul::label[ft_io_emul::FC_FILE_COUNT] = { "LOOP-EXTENTS", "FREE-SPACE-EXTENTS" };

/** constructor */
ft_io_emul::ft_io_emul(ft_job & job)
: super_type(job)
{
    /* mark fd[] as invalid: they are not open yet */
    for (ft_size i = 0; i < FC_FILE_COUNT; i++)
        is[i] = NULL;
}

/** destructor. calls close() */
ft_io_emul::~ft_io_emul()
{
    close();
}

/** return true if this ft_io_emul is currently (and correctly) open */
bool ft_io_emul::is_open() const
{
    return dev_length() != 0;
}

/** check for consistency and open LOOP-EXTENTS and FREE-SPACE-EXTENTS */
int ft_io_emul::open(char const* const path[FC_FILE_COUNT])
{
    if (is_open()) {
        // already open!
        ff_log(FC_ERROR, 0, "unexpected call, I/O is already open");
        return EISCONN;
    }

    std::string str;
    ft_uoff lengths[FC_FILE_COUNT];
    ft_size i;
    int err = 0;

    do {
        for (i = 0; i < FC_FILE_COUNT; i++) {
            is[i] = new std::ifstream(path[i], std::ios_base::in);
            if (! is[i]->good()) {
                err = ff_log(FC_ERROR, errno, "error opening %s '%s'", label[i], path[i]);
                break;
            }

            /* both LOOP-EXTENTS and FREE-SPACE-EXTENTS start with:
             * length {file_size}\n
             * physical logical length user_data\n
             */
            (* is[i]) >> str >> lengths[i] >> str >> str >> str >> str;
            if (! is[i]->good()) {
                err = ff_log(FC_ERROR, errno, "error reading 'length' from %s '%s'", label[i], path[i]);
                break;
            }
        }
    } while (0);

    if (err == 0)
        /* use emulated LOOP length as DEVICE length */
        dev_length(lengths[FC_LOOP_EXTENTS]);
    else
        close();

    return err;
}


/** close a single descriptor/stream */
void ft_io_emul::close0(ft_size i)
{
    if (is[i] != NULL) {
        delete is[i];
        is[i] = NULL;
    }
}

/**
 * close file descriptors.
 * return 0 for success, 1 for error (prints by itself error message to stderr)
 */
void ft_io_emul::close()
{
    for (ft_size i = 0; i < FC_FILE_COUNT; i++)
        close0(i);
    super_type::close();
}

/**
 * close the file descriptors for LOOP-FILE and ZERO-FILE
 */
void ft_io_emul::close_extents()
{
    ft_size which[] = { FC_LOOP_EXTENTS, FC_FREE_SPACE_EXTENTS };
    for (ft_size i = 0; i < sizeof(which)/sizeof(which[0]); i++)
        close0(which[i]);
}


/** return true if this I/O has open descriptors/streams to LOOP-FILE and FREE-SPACE */
bool ft_io_emul::is_open_extents() const
{
    bool flag = false;
    if (dev_length() != 0) {
        ft_size which[] = { FC_LOOP_EXTENTS, FC_FREE_SPACE_EXTENTS };
        ft_size i, n = sizeof(which)/sizeof(which[0]);
        for (i = 0; i < n; i++)
            if (is[which[i]] == NULL)
                break;
        flag = i == n;
    }
    return flag;
}




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
int ft_io_emul::read_extents(ft_vector<ft_uoff> & loop_file_extents,
                             ft_vector<ft_uoff> & free_space_extents,
                             ft_uoff & ret_block_size_bitmask)
{
    ft_uoff block_size_bitmask = ret_block_size_bitmask;
    int err = 0;
    do {
        if (!is_open_extents()) {
            err = ENOTCONN; // not open!
            break;
        }

        /* ff_emul_extents() appends into ft_vector<T>, does NOT overwrite it */
        if ((err = ff_read_extents_file(* is[FC_LOOP_EXTENTS], dev_length(), loop_file_extents, block_size_bitmask)) != 0)
            break;
        if ((err = ff_read_extents_file(* is[FC_FREE_SPACE_EXTENTS], dev_length(), free_space_extents, block_size_bitmask)) != 0)
            break;

    } while (0);

    if (err == 0)
        ret_block_size_bitmask = block_size_bitmask;

    return err;
}



/**
 * create and open file job.job_dir() + '/storage.bin' and fill it with job.job_storage_size() bytes of zeros.
 * return 0 if success, else error
 *
 * implementation: do nothing and return success
 */
int ft_io_emul::create_storage()
{
    return 0;
}


FT_IO_NAMESPACE_END
