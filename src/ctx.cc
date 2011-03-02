/*
 * ctx.cc
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#include "first.hh"

#include <cerrno>    /* for errno      */

#include "fail.hh"   /* for ff_fail()  */
#include "ctx.hh"    /* for ft_ctx     */


const char * ft_ctx::fc_dev_label = "DEVICE";
const char * const ft_ctx::fc_file_label[FC_FILE_COUNT] = { "LOOP-FILE", "ZERO-FILE" };


/** default constructor */
ft_ctx::ft_ctx()
    : fm_dev_length(0)
{
    /* mark fm_file_fd[] as invalid: they are not open yet */
    for (ft_size i = 0; i < FC_FILE_COUNT; i++)
        fm_file_fd[i] = -1;
}

/** destructor. calls close() */
ft_ctx::~ft_ctx()
{
    close();
}

/** return true if this ft_main is currently (and correctly) open */
bool ft_ctx::is_open() const
{
    bool flag = fm_dev_length != 0;
    for (ft_size i = 0; i < FC_FILE_COUNT; i++) {
        if (fm_file_fd[i] < 0) {
            flag = false;
            break;
        }
    }
    return flag;
}

/**
 * close file descriptors.
 * return 0 for success, 1 for error (prints by itself error message to stderr)
 */
void ft_ctx::close()
{
    fm_dev_length = 0;
    for (ft_size i = 0; i < FC_FILE_COUNT; i++) {
        if (fm_file_fd[i] >= 0) {
            if (::close(fm_file_fd[i]) != 0) {
                ff_fail(errno, "warning: closing %s file descriptor [%d] failed", fc_file_label[i], fm_file_fd[i]);
            }
            fm_file_fd[i] = -1;
        }
    }
}
