/*
 * io/util.cc
 *
 *  Created on: Mar 6, 2011
 *      Author: max
 */
#include "../first.hh"

#include <cerrno>        // for ENOSYS

#include "../types.hh"   // for ft_pid, ft_mode
#include "util_posix.hh" // for ff_posix_pid(), ff_posix_mkdir()


FT_IO_NAMESPACE_BEGIN


/** return this process PID in (*ret_pid) */
int ff_pid(ft_pid * ret_pid)
{
#ifdef __USE_POSIX
    return ff_posix_pid(ret_pid);
#else
    return ENOSYS;
#endif
}

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
