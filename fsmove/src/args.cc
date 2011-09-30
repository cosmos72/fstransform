/*
 * args.cc
 *
 *  Created on: Mar 21, 2011
 *      Author: max
 */

#include "first.hh"

#include "args.hh"    // for fm_args

FT_NAMESPACE_BEGIN


/** default constructor */
fm_args::fm_args()
    : io_args(), exclude_list(NULL), io_kind(FC_IO_AUTODETECT), ui_kind(FC_UI_NONE),
      force_run(false), simulate_run(false)
{ }

FT_NAMESPACE_END
