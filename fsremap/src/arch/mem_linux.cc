/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
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
 * arch/mem_linux.cc
 *
 *  Created on: Mar 10, 2011
 *      Author: max
 */
#include "../first.hh"

#ifdef __linux__

#include "mem_linux.hh"

#if defined(FT_HAVE_ERRNO_H)
#include <errno.h> // for errno
#elif defined(FT_HAVE_CERRNO)
#include <cerrno> // for errno
#endif
#if defined(FT_HAVE_STDIO_H)
#include <stdio.h> // for FILE, fopen(), fclose()
#elif defined(FT_HAVE_CSTDIO)
#include <cstdio> // for FILE, fopen(), fclose()
#endif
#if defined(FT_HAVE_STRING_H)
#include <string.h> // for strcmp()
#elif defined(FT_HAVE_CSTRING)
#include <cstring> // for strcmp()
#endif

#include "../log.hh" // for ff_log()

FT_ARCH_NAMESPACE_BEGIN

/**
 * return an approximation of free system memory in bytes,
 * or 0 if cannot be determined
 */
ft_uoff ff_arch_linux_mem_system_free() {
    FILE *f = fopen("/proc/meminfo", "r");
    if (f == NULL) {
        ff_log(FC_WARN, errno, "failed to open /proc/meminfo, maybe /proc is not mounted?");
        return 0;
    }
    char label[256], unit[8];
    ft_ull n_ull = 0, scale;
    ft_uoff total = 0, n;
    ft_uint left = 3;
    int err;

    while (left != 0) {
        if ((err = fscanf(f, "%255s %" FT_ULL " %7s\n", label, &n_ull, unit)) <= 0) {
            if (ferror(f))
                ff_log(FC_WARN, errno, "error reading /proc/meminfo");
            break;
        }
        if (err != 3)
            continue;

        label[255] = '\0'; // in case it's missing
        unit[7] = '\0';    // idem

        if (strcmp(label, "MemFree:") && strcmp(label, "Buffers:") && strcmp(label, "Cached:"))
            continue;
        switch (unit[0]) {
        case 'k':
            scale = 10;
            break; // 2^10 = 1k
        case 'M':
            scale = 20;
            break; // 2^20 = 1M
        case 'G':
            scale = 30;
            break; // 2^30 = 1G
        case 'T':
            scale = 40;
            break; // 2^40 = 1T
        case 'P':
            scale = 50;
            break; // 2^50 = 1P
        case 'E':
            scale = 60;
            break; // 2^60 = 1E
        case 'Z':
            scale = 70;
            break; // 2^70 = 1Z
        case 'Y':
            scale = 80;
            break; // 2^80 = 1Y
        default:
            scale = 0;
            break;
        }
        /* overflow? then approximate.. */
        if (scale >= 8 * sizeof(ft_ull) || n_ull > (ft_ull)-1 >> scale)
            n_ull = (ft_ull)-1;
        else
            n_ull <<= scale;

        n = (ft_uoff)n_ull;
        /* overflow? then approximate.. */
        if ((ft_ull)n != n_ull || n > (ft_uoff)-1 - total) {
            total = (ft_uoff)-1;
            break;
        }
        total += n;
        left--;
    }
    if (fclose(f) != 0)
        ff_log(FC_WARN, errno, "error closing /proc/meminfo");
    return total;
}

FT_ARCH_NAMESPACE_END

#endif /* __linux__ */
