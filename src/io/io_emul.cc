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

/** default constructor */
ft_io_emul::ft_io_emul()
: super_type()
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
    bool flag = false;
    if (dev_length() != 0) {
        ft_size i;
        for (i = 0; i < FC_FILE_COUNT; i++)
            if (is[i] == NULL)
                break;
        flag = i == FC_FILE_COUNT;
    }
    return flag;
}

/** check for consistency and open LOOP-EXTENTS and FREE-SPACE-EXTENTS */
int ft_io_emul::open(char const* const path[FC_FILE_COUNT])
{
    if (is_open())
        return EISCONN; // already open!

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

/**
 * close file descriptors.
 * return 0 for success, 1 for error (prints by itself error message to stderr)
 */
void ft_io_emul::close()
{
    for (ft_size i = 0; i < FC_FILE_COUNT; i++) {
        if (is[i] != NULL) {
            delete is[i];
            is[i] = NULL;
        }
    }
    super_type::close();
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
        if (!is_open()) {
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

FT_IO_NAMESPACE_END
