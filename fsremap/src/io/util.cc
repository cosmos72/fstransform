/*
 * io/util.cc
 *
 *  Created on: Mar 6, 2011
 *      Author: max
 */
#include "../first.hh"

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>        // for ENOSYS
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>         // for ENOSYS
#endif

#include "../types.hh"   // for ft_mode
#include "util_posix.hh" // for ff_posix_mkdir()


FT_IO_NAMESPACE_BEGIN


/** create a directory */
int ff_mkdir(const char * path)
{
#ifdef __USE_POSIX
    return ff_posix_mkdir(path);
#else
    return ENOSYS;
#endif
}

FT_IO_NAMESPACE_END
