/*
 * io/io.cc
 *
 *  Created on: Sep 20, 2011
 *      Author: max
 */

#include "../first.hh"

#include <string>          // for ft_string

#include "../args.hh"      // for fm_args
#include "../log.hh"       // for ff_log()
#include "io.hh"           // for fm_io

FT_IO_NAMESPACE_BEGIN


char const * const fm_io::label[] = {
        "source", "target"
};

/** constructor */
fm_io::fm_io()
    : this_source_root(), this_target_root()
{ }

/**
 * destructor.
 * sub-classes must override it to call close()
 */
fm_io::~fm_io()
{ }


/**
 * open this fm_io.
 * sub-classes must override this method to perform appropriate initialization
 */
int fm_io::open(const fm_args & args)
{
    const char * arg1 = args.io_args[FC_SOURCE_ROOT], * arg2 = args.io_args[FC_TARGET_ROOT];
    int err = 0;
    do {
        if (arg1 == NULL) {
            ff_log(FC_ERROR, 0, "missing arguments: %s %s", label[FC_SOURCE_ROOT], label[FC_TARGET_ROOT]);
            err = -EINVAL;
            break;
        }
        if (arg2 == NULL) {
            ff_log(FC_ERROR, 0, "missing argument: %s", label[FC_TARGET_ROOT]);
            err = -EINVAL;
            break;
        }
        this_source_root = arg1;
        this_target_root = arg2;
    } while (0);
    return err;
}

/**
 * close this fm_io.
 * sub-classes must override this method to perform appropriate cleanup
 */
void fm_io::close()
{
    this_source_root.clear();
    this_target_root.clear();
}

FT_IO_NAMESPACE_END


