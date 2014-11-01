/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 * 
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * io/util_posix.cc
 *
 *  Created on: Mar 27, 2012
 *      Author: max
 */
#include "../first.hh"

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>        // for errno, ECHILD
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>         // for errno, ECHILD
#endif
#if defined(FT_HAVE_STDLIB_H)
# include <stdlib.h>       // for exit()
#elif defined(FT_HAVE_CSTDLIB)
# include <cstdlib>        // for exit()
#endif

#ifdef FT_HAVE_UNISTD_H
# include <unistd.h>       // for dup2(), close(), fork(), execvp()
#endif
#ifdef FT_HAVE_FCNTL_H
# include <fcntl.h>        // for open()
#endif
#ifdef FT_HAVE_SYS_TYPES_H
# include <sys/types.h>    // for   "   , waitpid()
#endif
#ifdef FT_HAVE_SYS_STAT_H
# include <sys/stat.h>     // for   "
#endif
#ifdef FT_HAVE_SYS_WAIT_H
# include <sys/wait.h>     // for waitpid()
#endif

#include "../log.hh"      // for ff_log()
#include "util_posix.hh"  // for ff_posix_exec_silent()


FT_IO_NAMESPACE_BEGIN



/**
 * spawn a system command, typically with fork()+execv(), wait for it to complete and return its exit status.
 * argv[0] is conventionally the program name.
 * argv[1...] are program arguments and must be terminated with a NULL pointer.
 */
int ff_posix_exec_silent(const char * path, const char * const argv[])
{
    int err;
    pid_t pid = ::fork();
    if (pid == 0) {
        /* child */
        int dev_null = ::open("/dev/null", O_RDWR);
        if (dev_null >= 0) {
            (void) ::dup2(dev_null, 0);
            (void) ::dup2(dev_null, 1);
            (void) ::dup2(dev_null, 2);
            if (dev_null > 2)
                (void) ::close(dev_null);
        } else
            ff_log(FC_DEBUG, errno, "open('/dev/null') failed");

        ::execvp(path, (char * const *)argv);

        /* if we reach here, execvp() failed! */
        ff_log(FC_DEBUG, err = errno, "execvp(%s) failed");

        /* exit() can only return one-byte exit status */
        err &= 0xff;
        if (err == 0) {
            err = (ECHILD > 0 ? ECHILD : -ECHILD) & 0xff;
            if (err == 0)
                err = 1;
        }
        ::exit(err);
    } else if (pid == (pid_t)-1) {
        ff_log(FC_WARN, errno, "fork() failed");
    } else {
        /* parent */
        err = -ECHILD; // assume failure unless proved successful...
        int status = 0;

        if (waitpid(pid, & status, 0/*options*/) != pid) {
            err = ff_log(FC_WARN, errno, "error in waitpid(), assuming command '%s' failed", path);
            if (err == 0)
                err = -ECHILD;
        } else if (WIFEXITED(status)) {
            status = WEXITSTATUS(status);
            if (status == 0)
                err = 0; // proved successful!
            else
                ff_log(FC_DEBUG, 0, "command '%s' exited with non-zero exit status %d", path, status);

        } else if (WIFSIGNALED(status))
            ff_log(FC_DEBUG, 0, "command '%s' died with signal %d", path, (int)WTERMSIG(status));
        else
            ff_log(FC_WARN, 0, "waitpid() returned unknown status %d, assuming command '%s' failed", status, path);
    }
    return err;
}

FT_IO_NAMESPACE_END
