/*
 * fsmount_kernel - invoke raw linux syscall mount(), bypassing /sbin/mount
 *
 * Copyright (C) 2017 Massimiliano Ghilardi
 * 
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 2 of the License, or
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
 * main.cc
 *  Created on: Jan 15, 2012
 *      Author: max
 */

#include "first.hh"

#include "log.hh"

#include <string>
#include <vector>

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>     // for errno
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>      // for errno
#endif

#if defined(FT_HAVE_STRING_H)
# include <string.h>     // for memcmp(), strcmp(), strdup() ...
#elif defined(FT_HAVE_CSTRING)
# include <cstring>      // for memcmp(), strcmp(), strdup() ...
#endif

#if defined(FT_HAVE_SYS_MOUNT_H)
# include <sys/mount.h>  // for mount()
#endif


#ifndef MS_BIND
# define MS_BIND 0
#endif
#ifndef MS_DIRSYNC
# define MS_DIRSYNC 0 
#endif
#ifndef MS_LAZYTIME
# define MS_LAZYTIME 0 
#endif
#ifndef MS_REC
# define MS_REC 0 
#endif
#ifndef MS_RELATIME
# define MS_RELATIME 0 
#endif
#ifndef MS_STRICTATIME
# define MS_STRICTATIME 0 
#endif
#ifndef MS_SILENT
# define MS_SILENT 0 
#endif


FT_NAMESPACE_BEGIN

static const char * program_name = "fsmount_kernel";

static const char * source = NULL;
static const char * target = NULL;
static const char * fstype = NULL;
static char * options = NULL;
static unsigned long mountflags = 0;

/** print command-line usage to stdout and return err */
int usage(int err)
{
    ff_log(FC_NOTICE, 0, "Usage: %s [-o OPTIONS] -t FILESYSTEM_TYPE SOURCE TARGET\n", program_name);
    ff_log(FC_NOTICE, 0, "Call the kernel to mount a filesystem,");
    ff_log(FC_NOTICE, 0, "ignoring any userspace helper or wrapper (fuse...)");

    ff_log(FC_NOTICE, 0,
           "Options:\n"
           "  --                end of options. treat subsequent parameters as arguments\n"
           "                        even if they start with '-'\n"
           "  -t <type>         set the filesystem type. MANDATORY.\n"
           "  -o <list>         comma-separated list of mount options.\n"
           "  -h, --help        display this help and exit\n"
           "      --version     output version information and exit\n");
    return err;
}

/** output version information and return 0 */
int version()
{
    return ff_log(FC_NOTICE, 0,
            "fsmount_kernel (fstransform utilities) " FT_VERSION "\n"
            "Copyright (C) 2011-2017 Massimiliano Ghilardi\n"
            "\n"
            "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
            "This is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n");
}

int parse_options()
{
    char * sep, * src = options, * dst = options;
    unsigned long len;
    while (src != NULL)
    {
        sep = strchr(src, ',');
        len = sep ? sep - src : strlen(src);
        
        if (len == 5 && !memcmp(src, "atime", len))
            mountflags &= ~MS_NOATIME;
        else if (len == 5 && !memcmp(src, "async", len))
            mountflags &= ~MS_SYNCHRONOUS;
        else if (len == 4 && !memcmp(src, "bind", len))
            mountflags |= MS_BIND;
        else if (len == 3 && !memcmp(src, "dev", len))
            mountflags &= ~MS_NODEV;
        else if (len == 8 && !memcmp(src, "diratime", len))
            mountflags &= ~MS_NODIRATIME;
        else if (len == 7 && !memcmp(src, "dirsync", len))
            mountflags |= MS_DIRSYNC;
        else if (len == 4 && !memcmp(src, "exec", len))
            mountflags &= ~MS_NOEXEC;
        else if (len == 8 && !memcmp(src, "lazytime", len))
            mountflags |= MS_LAZYTIME;
        else if (len == 4 && !memcmp(src, "mand", len))
            mountflags |= MS_MANDLOCK;
        else if (len == 6 && !memcmp(src, "nomand", len))
            mountflags &= ~MS_MANDLOCK;
        else if (len == 7 && !memcmp(src, "noatime", len))
            mountflags |= MS_NOATIME;
        else if (len == 5 && !memcmp(src, "nodev", len))
            mountflags |= MS_NODEV;
        else if (len == 10 && !memcmp(src, "nodiratime", len))
            mountflags |= MS_NODIRATIME;
        else if (len == 6 && !memcmp(src, "noexec", len))
            mountflags |= MS_NOEXEC;
        else if (len == 10 && !memcmp(src, "norelatime", len))
            mountflags &= ~MS_RELATIME;
        else if (len == 13 && !memcmp(src, "nostrictatime", len))
            mountflags &= ~MS_STRICTATIME;
        else if (len == 6 && !memcmp(src, "nosuid", len))
            mountflags |= MS_NOSUID;
        else if (len == 5 && !memcmp(src, "rbind", len))
            mountflags |= MS_BIND | MS_REC;
        else if (len == 8 && !memcmp(src, "relatime", len))
            mountflags |= MS_RELATIME;
        else if (len == 7 && !memcmp(src, "remount", len))
            mountflags |= MS_REMOUNT;
        else if (len == 2 && !memcmp(src, "ro", len))
            mountflags |= MS_RDONLY;
        else if (len == 2 && !memcmp(src, "rw", len))
            mountflags &= ~MS_RDONLY;
        else if (len == 5 && !memcmp(src, "slave", len))
            mountflags |= MS_SLAVE;
        else if (len == 6 && !memcmp(src, "shared", len))
            mountflags |= MS_SHARED;
        else if (len == 4 && !memcmp(src, "suid", len))
            mountflags &= ~MS_NOSUID;
        else if (len == 7 && !memcmp(src, "private", len))
            mountflags |= MS_PRIVATE;
        else if (len == 10 && !memcmp(src, "unbindable", len))
            mountflags |= MS_UNBINDABLE;
        else if (len == 6 && !memcmp(src, "silent", len))
            mountflags |= MS_SILENT;
        else if (len == 11 && !memcmp(src, "strictatime", len))
            mountflags |= MS_STRICTATIME;
        else if (len == 4 && !memcmp(src, "sync", len))
            mountflags |= MS_SYNCHRONOUS;
        else {
            if (dst != options)
                *dst++ = ',';
            memmove(dst, src, len);
            dst += len;
        }
        src = sep + (sep != NULL);
    }
    if (dst)
        *dst++ = '\0';
    
    if (!source) {
        ff_log(FC_ERROR, 0, "%s: missing argument SOURCE", program_name);
        return usage(1);
    }
    if (mountflags & MS_REMOUNT) {
        return 0;
    }
    if (mountflags & MS_BIND) {
        if (target)
            return 0;
        ff_log(FC_ERROR, 0, "%s: missing argument TARGET, required by options 'bind' and 'rbind'", program_name);
        return usage(1);
    }
    if (mountflags & (MS_SHARED|MS_PRIVATE|MS_SLAVE|MS_UNBINDABLE)) {
        return 0;
    }
    if (!target || !fstype) {
        ff_log(FC_ERROR, 0, "%s: missing %s%s%s", program_name,
               (target ? "" : "argument TARGET"),
               (target || fstype ? "" : " and "),
               (fstype ? "" : "option -t FILESYSTEM_TYPE"));
        return usage(1);
    }
    return 0;
}

int do_mount()
{
    int err = mount(source, target, fstype, mountflags, options);
    if (err != 0)
        err = ff_log(FC_ERROR, errno, "syscall mount(source='%s', target='%s', fstype='%s', mountflags=0x%lx, options='%s') failed",
                     source, (target ? target : ""), (fstype ? fstype : ""), mountflags, (options ? options : ""));
    return err;
}

int main(int argc, char ** argv)
{
    const char * arg;
    bool allow_options = true;
    
    if (*argv)
        program_name = *argv++;
    
    while ((arg = *argv++))
    {
        if (allow_options && arg[0] == '-') {
            if (!strcmp(arg, "--"))
                allow_options = false;
            else if (!strcmp(arg, "-h") || !strcmp(arg, "--help"))
                return usage(0);
            else if (!strcmp(arg, "--version"))
                return version();
            else if (*argv && !strcmp(arg, "-t")) {
                fstype = *argv++;
            } else if (*argv && !strcmp(arg, "-o")) {
                options = *argv++;
            } else {
                ff_log(FC_NOTICE, 0, "%s: invalid option -- '%s'\n", program_name, argv);
                return usage(1);
            }
        } else {
            if (!source)
                source = arg;
            else if (!target)
                target = arg;
            else {
                ff_log(FC_NOTICE, 0, "%s: unexpected argument -- '%s'", program_name, arg);
                return usage(1);
            }
        }
    }
    if (options && !(options = strdup(options))) {
        return ff_log(FC_ERROR, errno, "Out of memory!");
    }

    int err = parse_options();
    if (err != 0)
        return err;

    return do_mount();
}

FT_NAMESPACE_END
    
int main(int argc, char ** argv)
{
    return FT_NS main(argc, argv);
}
