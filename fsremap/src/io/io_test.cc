/*
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
fr_io_test::fr_io_test(fr_job & job)
: super_type(job)
{
    /* mark fd[] as invalid: they are not open yet */
    for (ft_size i = 0; i < FC_FILE_COUNT; i++)
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
        ff_log(FC_ERROR, 0, "unexpected call, I/O is already open");
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

    for (i = FC_DEVICE_LENGTH+1; i < FC_FILE_COUNT; i++) {
        if ((this_f[i] = fopen(io_args[i], "r")) == NULL) {
            err = ff_log(FC_ERROR, errno, "error opening %s '%s'", extents_label[i], io_args[i]);
            break;
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
    for (ft_size i = 0; i < FC_FILE_COUNT; i++)
        close0(i);
}


/** return true if this I/O has open descriptors/streams to LOOP-FILE and FREE-SPACE */
bool fr_io_test::is_open_extents() const
{
    ft_size i, n = FC_FILE_COUNT;
    for (i = FC_DEVICE_LENGTH+1; i < n; i++)
        if (this_f[i] == NULL)
            break;
    return i == n;
}




/**
 * retrieve LOOP-FILE extents and FREE-SPACE extents and append them into
 * the vectors loop_file_extents and free_space_extents.
 * the vectors will be ordered by extent ->logical.
 *
 * return 0 for success, else error (and vectors contents will be UNDEFINED).
 *
 * implementation: load extents list from files
 * (for example they could be the job persistence files)
 */
int fr_io_test::read_extents(fr_vector<ft_uoff> & loop_file_extents,
                             fr_vector<ft_uoff> & free_space_extents,
                             ft_uoff & ret_block_size_bitmask)
{
    ft_uoff block_size_bitmask = ret_block_size_bitmask;
    int err = 0;
    do {
        if (!is_open_extents()) {
            err = ENOTCONN; // not open!
            break;
        }
        /* ff_load_extents_file() appends to fr_vector<ft_uoff>, does NOT overwrite it */
        if ((err = ff_load_extents_file(this_f[FC_LOOP_EXTENTS], loop_file_extents, block_size_bitmask)) != 0) {
            err = ff_log(FC_ERROR, err, "error reading %s extents from save-file", label[FC_LOOP_FILE]);
            break;
        }
        if ((err = ff_load_extents_file(this_f[FC_FREE_SPACE_EXTENTS], free_space_extents, block_size_bitmask)) != 0) {
            err = ff_log(FC_ERROR, err, "error reading %s extents from save-file", label[FC_FREE_SPACE]);
            break;
        }
    } while (0);

    if (err == 0)
        ret_block_size_bitmask = block_size_bitmask;

    return err;
}

FT_IO_NAMESPACE_END
