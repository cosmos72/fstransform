/*
 * assert.cc
 *
 *  Created on: Mar 13, 2011
 *      Author: max
 */

#include "first.hh"

#if defined(FT_HAVE_STDLIB_H)
# include <stdlib.h>     // for exit()
#elif defined(FT_HAVE_CSTDLIB)
# include <cstdlib>      // for exit()
#endif

#include "assert.hh"     // for ff_assert()
#include "log.hh"        // for ff_log()

FT_NAMESPACE_BEGIN

void ff_assert_fail0(const char * caller_file, const char * caller_func, int caller_line, const char * assertion)
{
    FT_NS ff_logl(caller_file, caller_func, caller_line, FC_FATAL, 0, "internal error! assertion failed: %s", assertion);
    exit(1);
}

FT_NAMESPACE_END
