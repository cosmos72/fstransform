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
    : dev_len(0)
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
    dev_len = 0;
}



FT_IO_NAMESPACE_END


