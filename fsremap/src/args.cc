/*
 * args.cc
 *
 *  Created on: Mar 21, 2011
 *      Author: max
 */

#include "first.hh"

#include "args.hh"    // for fr_args

FT_NAMESPACE_BEGIN


/** default constructor */
fr_args::fr_args()
    : root_dir(NULL), io_args(), ui_arg(NULL), umount_cmd(NULL), storage_size(), job_id(0),
      job_clear(FC_CLEAR_AUTODETECT), io_kind(FC_IO_AUTODETECT), ui_kind(FC_UI_NONE),
      force_run(false), simulate_run(false)
{
    ft_size i, n;
    for (i = 0, n = sizeof(io_args)/sizeof(io_args[0]); i < n; i++)
        io_args[i] = NULL;
    for (i = 0, n = sizeof(storage_size)/sizeof(storage_size[0]); i < n; i++)
        storage_size[i] = 0;
}

FT_NAMESPACE_END
