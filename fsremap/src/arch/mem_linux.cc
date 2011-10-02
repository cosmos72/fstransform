/*
 * arch/mem_linux.cc
 *
 *  Created on: Mar 10, 2011
 *      Author: max
 */
#include "../first.hh"

#include "mem_linux.hh"


#ifdef __linux__

#include <cerrno>  // for errno
#include <cstdio>  // for FILE, fopen(), fclose()
#include <cstring> // for strcmp()


#include "../log.hh" // for ff_log()


FT_ARCH_NAMESPACE_BEGIN

/**
 * return an approximation of free system memory in bytes,
 * or 0 if cannot be determined
 */
ft_uoff ff_arch_linux_mem_system_free() {
    FILE * f = fopen("/proc/meminfo", "r");
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
        if ((err = fscanf(f, "%256s %"FT_ULL" %8s\n", label, & n_ull, unit)) <= 0) {
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
            case 'k': scale = 10; break; // 2^10 = 1k
            case 'M': scale = 20; break; // 2^20 = 1M
            case 'G': scale = 30; break; // 2^30 = 1G
            case 'T': scale = 40; break; // 2^40 = 1T
            case 'P': scale = 50; break; // 2^50 = 1P
            case 'E': scale = 60; break; // 2^60 = 1E
            case 'Z': scale = 70; break; // 2^70 = 1Z
            case 'Y': scale = 80; break; // 2^80 = 1Y
            default: scale = 0; break;
        }
        /* overflow? then approximate.. */
        if (scale >= 8*sizeof(ft_ull) || n_ull > (ft_ull)-1 >> scale)
            n_ull = (ft_ull)-1;
        else
            n_ull <<= scale;

        n = (ft_uoff) n_ull;
        /* overflow? then approximate.. */
        if (n < 0 || n_ull != (ft_ull) n || n > (ft_uoff)-1 - total) {
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

