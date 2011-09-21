/*
 * io/util.hh
 *
 *  Created on: Mar 6, 2011
 *      Author: max
 */

#ifndef FSREMAP_UTIL_HH
#define FSREMAP_UTIL_HH

#include "../types.hh" // for ft_pid */

FT_IO_NAMESPACE_BEGIN

/** return this process PID in (*ret_pid) */
int ff_pid(ft_pid * ret_pid);

/** create a directory */
int ff_mkdir(const char * path);

FT_IO_NAMESPACE_END

#endif /* FSREMAP_UTIL_HH */
