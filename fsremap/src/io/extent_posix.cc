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
 * io/extent_posix.cc
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "../first.hh" // for FT_*TEMPLATE* macros */

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>        // for errno, ENOMEM, EINVAL, EFBIG
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>         // for errno, ENOMEM, EINVAL, EFBIG
#endif
#if defined(FT_HAVE_STDLIB_H)
# include <stdlib.h>       // for malloc(), free()
#elif defined(FT_HAVE_CSTDLIB)
# include <cstdlib>        // for malloc(), free()
#endif
#if defined(FT_HAVE_STRING_H)
# include <string.h>       // for memset()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>        // for memset()
#endif

#ifdef FT_HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>    // for ioctl()
#endif

#ifdef FT_HAVE_LINUX_FS_H
# include <linux/fs.h>     // for FS_IOC_FIEMAP, FIBMAP */
#endif
#ifdef FT_HAVE_LINUX_FIEMAP_H
 /* if <linux/fs.h> defines FS_IOC_FIEMAP, <linux/fiemap.h> is supposed to exist */
# include <linux/fiemap.h> // for struct fiemap and struct fiemap_extent. 
#endif

#include <utility>         // for std::pair<T1,T2> and std::make_pair()
#include <vector>          // for std::vector<T>


#include "../log.hh"       // for ff_log() */
#include "../traits.hh"    // for FT_TYPE_TO_UNSIGNED(T) */
#include "../types.hh"     // for ft_off */
#include "../extent.hh"    // for fr_extent<T>, fr_map<T>, ff_filemap() */
#include "../vector.hh"    // for fr_vector<T> */
#include "extent_posix.hh" // for ff_read_extents_posix() */
#include "util_posix.hh"   // for ff_posix_ioctl(), ff_posix_size() */

FT_IO_NAMESPACE_BEGIN

/**
 * retrieves file blocks allocation map (extents) for specified file descriptor
 * and appends them to ret_vector (with user_data = FC_DEFAULT_USER_DATA).
 * in case of failure returns errno-compatible error code, and ret_vector contents will be UNDEFINED.
 *
 * must (and will) also check that device size can be represented by ret_list,
 *
 * implementation: calls ioctl(FIBMAP)
 */
static int ff_posix_fibmap(int fd, ft_uoff dev_length, fr_vector<ft_uoff> & ret_list, ft_uoff & ret_block_size_bitmask)
{
#ifdef FIBMAP
    ft_uoff file_length, file_block_count, dev_block_count;
    ft_uoff ioctl_n = 0, block_size = 0, logical_uoff, physical_uoff;

    ft_size extent_n = ret_list.size();

    /* lower-level API ff_posix_ioctl(FIGETBSZ) and ff_posix_ioctl(FIBMAP) need these to be int */
    int err = 0, block_size_int, logical, physical;

    do {
        if ((err = ff_posix_ioctl(fd, FIGETBSZ, & block_size_int))) {
            err = ff_log(FC_ERROR, err, "ff_posix_fibmap(): error in ioctl(%d, FIGETBSZ)", fd);
            break;
        }

        block_size = (ft_uoff) block_size_int;
        // ft_uoff is expected to be unsigned and wider than int,
        // but explicitly checking for overflow is always safer than "expecting"
        if (block_size < 0 || block_size_int != (int) block_size) {
            /* overflow! give up. */
            err = ff_log(FC_ERROR, EFBIG, "ff_posix_fibmap(): error, block_size = %"FT_ULL" overflows type (ft_uoff)", (ft_ull) block_size_int);
            break;
        }

        if ((err = ff_posix_size(fd, & file_length))) {
            err = ff_log(FC_ERROR, err, "ff_posix_fibmap(): error in stat(%d)", fd);
            break;
        }

        // number of blocks in the device
        dev_block_count = (dev_length + block_size - 1) / block_size;

        // number of blocks in the file
        file_block_count = (file_length + block_size - 1) / block_size;

        // ioctl(FIBMAP) wants an (int logical) and returns an (int physical)
        // in units of one block, so we must check for overflow!
        int n = (int) file_block_count;
        int m = (int) dev_block_count;

        if (m < 0 || dev_block_count != (ft_uoff) m
            || n < 0 || file_block_count != (ft_uoff) n)
        {
            /* overflow! give up. */
            err = ff_log(FC_ERROR, EFBIG, "ff_posix_fibmap(): error, dev_block_count = %"FT_ULL", file_block_count = %"FT_ULL" overflow type (int)",
                         (ft_ull) dev_block_count, (ft_ull) file_block_count);
            break;
        }

        for (logical = 0; logical < n; logical++) {
            physical = logical;
            ioctl_n++;
            if ((err = ff_posix_ioctl(fd, FIBMAP, & physical))) {
                err = ff_log(FC_ERROR, err, "ff_posix_fibmap(): error in ioctl(%d, FIBMAP, %"FT_ULL")", fd, (ft_ull) logical);
                break;
            }
            /* FIBMAP reports holes (i.e. unallocated blocks in the file) as physical == 0. ugly */
            if (physical != 0) {
                physical_uoff = (ft_uoff) physical * block_size;
                logical_uoff = (ft_uoff) logical * block_size;

                /* this is painful... FIBMAP reports one block per call */
                ret_list.append(physical_uoff, logical_uoff, block_size, FC_DEFAULT_USER_DATA);
            }
        }
    } while (0);

    if (err == 0) {
        static ft_ull log_count = 0;

        if (log_count++ == 5)
            ff_log(FC_DEBUG, 0, "decreasing to level TRACE any further DEBUG message 'ioctl(FIBMAP) successful'");

        extent_n = ret_list.size() - extent_n;

        ff_log(log_count < 5 ? FC_DEBUG : FC_TRACE, 0, "ioctl(%d, FIBMAP) successful: retrieved %"FT_ULL" extent%s in %"FT_ULL" call%s",
                fd, (ft_ull) extent_n, extent_n == 1 ? "" : "s", (ft_ull) ioctl_n, ioctl_n == 1 ? "" : "s");
        /* keep track of bits used by extents. needed to compute effective block size */
        ret_block_size_bitmask |= block_size;
    }
    return err;
#else
    return ENOSYS;
#endif /* FIBMAP */
}


#ifdef FS_IOC_FIEMAP
static int ff_linux_fiemap(int fd, ft_uoff file_start, ft_uoff file_end, ft_u32 extent_n, struct fiemap * k_map)
{
    ft_size k_len = sizeof(struct fiemap) + extent_n * sizeof(struct fiemap_extent);

    memset(k_map, 0, k_len);

    k_map->fm_start = (ft_u64) file_start;
    k_map->fm_length = (ft_u64) (file_end - file_start);
    k_map->fm_flags = FIEMAP_FLAG_SYNC;
    k_map->fm_extent_count = extent_n;

    int err = 0;
    if ((err = ff_posix_ioctl(fd, FS_IOC_FIEMAP, k_map)) != 0) {
        static ft_ull log_count = 0;
        if (log_count++ == 5)
            ff_log(FC_DEBUG, 0, "decreasing to level TRACE any further DEBUG message 'ioctl(FIEMAP) failed'");

        /* do not mark the error as reported, this is just a DEBUG message */
        ff_log(log_count < 5 ? FC_DEBUG : FC_TRACE, 0,
                "ioctl(%d, FIEMAP, extents[%"FT_ULL"]) failed (%s), falling back on ioctl(FIBMAP) ...",
                fd, (ft_ull) extent_n, strerror(err));
    }
    return err;
}
#endif /* FS_IOC_FIEMAP */

/*
 * retrieves file blocks allocation map (extents) for specified file descriptor
 * and appends them to ret_vector (with user_data = FC_DEFAULT_USER_DATA).
 * in case of failure returns errno-compatible error code and ret_vector contents will be UNCHANGED.
 *
 * must (and will) also check that device size can be represented by ret_list
 *
 * implementation: calls ioctl(FS_IOC_FIEMAP)
 */
static int ff_linux_fiemap(int fd, fr_vector<ft_uoff> & ret_list, ft_uoff & ret_block_size_bitmask)
{
#ifdef FS_IOC_FIEMAP
    ft_uoff file_start = 0, file_size;
    int err;

    if ((err = ff_posix_size(fd, & file_size)) || file_size == 0)
        return err;

    fr_vector<ft_uoff> tmp_list;

    enum { K_EXTENT_N = 1024, K_SIZEOF_FIEMAP = sizeof(struct fiemap) + K_EXTENT_N * sizeof(struct fiemap_extent) };
    char buf[K_SIZEOF_FIEMAP];
    struct fiemap * k_map = (struct fiemap *) buf;
    ft_uoff ioctl_n = 0, block_size_bitmask = ret_block_size_bitmask;

    // call ioctl() repeatedly until we retrieve all extents
    while (ioctl_n++, (err = ff_linux_fiemap(fd, file_start, file_size, K_EXTENT_N, k_map)) == 0) {

        ft_u32 i, extent_n = k_map->fm_mapped_extents;
        if (extent_n == 0) {
            /* we did not get any extent... bail out */
            ff_log(FC_WARN, 0, "ioctl(%d, FS_IOC_FIEMAP) is refusing to return any extent after file offset = %"FT_ULL
                    ", falling back on ioctl(FIBMAP) ...", fd, (ft_ull) file_start);
            /* mark the error as reported, WARN is quite a severe level */
            err = -ENOSYS; /* ioctl(FS_IOC_FIEMAP) not working as expected... */
            break;
        }

        const struct fiemap_extent & last_e = k_map->fm_extents[extent_n - 1];
        const ft_uoff new_file_start = (ft_uoff) last_e.fe_logical + (ft_uoff) last_e.fe_length;
        if (new_file_start <= file_start) {
            ff_log(FC_WARN, 0, "ioctl(%d, FS_IOC_FIEMAP) returned extents ending at %"FT_ULL", i.e. _before_ start of requested range [%"FT_ULL", %"FT_ULL"]"
                    ", falling back on ioctl(FIBMAP) ...", fd, (ft_ull) new_file_start, (ft_ull) file_start, (ft_ull) file_size);
            /* mark the error as reported, WARN is quite a severe level */
            err = -ENOSYS; /* ioctl(FS_IOC_FIEMAP) not working as expected... */
            break;
        }

        tmp_list.reserve(tmp_list.size() + extent_n);

        for (i = 0; i < extent_n; i++) {
            const struct fiemap_extent & e = k_map->fm_extents[i];

            ft_u32 flag = e.fe_flags & (FIEMAP_EXTENT_UNKNOWN | FIEMAP_EXTENT_ENCODED);

            if (flag) {
                ff_log(FC_DEBUG, 0, "ioctl(%d, FS_IOC_FIEMAP, extents[%"FT_ULL"]) returned unsupported %s%s%s extents, falling back on ioctl(FIBMAP) ...",
                       fd, (ft_ull) extent_n,
                       (flag & FIEMAP_EXTENT_UNKNOWN ? "UNKNOWN" : ""),
                       (flag == (FIEMAP_EXTENT_UNKNOWN|FIEMAP_EXTENT_ENCODED) ? "+" : ""),
                       (flag & FIEMAP_EXTENT_ENCODED ? "ENCODED" : "")
                );
                // do not mark the error as reported, this is just a DEBUG message
                err = ENOSYS;
                break;
            }
            /*
             * keep track of bits used by all physical, logical and lengths.
             * needed to check against block size
             */
            block_size_bitmask |= e.fe_physical | e.fe_logical | e.fe_length;

            // save what we retrieved
            tmp_list.append((ft_uoff) e.fe_physical,
                            (ft_uoff) e.fe_logical,
                            (ft_uoff) e.fe_length,
                            (e.fe_flags & FIEMAP_EXTENT_UNWRITTEN) ? FC_EXTENT_ZEROED : FC_DEFAULT_USER_DATA);
        }
        if (err != 0 || (last_e.fe_flags & FIEMAP_EXTENT_LAST))
            break;

        // no FIEMAP_EXTENT_LAST found, we did not get all the extents. keep trying...
        if (new_file_start >= file_size)
            // should not happen, but not too dangerous
            file_size = new_file_start + 1;
        file_start = new_file_start;
    }
    if (err != 0)
        return err;

    ft_size extent_n = tmp_list.size();

    /* ok, no strange extents: we can now add them to ret_list */
    ret_list.reserve(ret_list.size() + extent_n);
    ret_list.append_all(tmp_list);

    static ft_ull log_count = 0;
    if (log_count++ == 5)
        ff_log(FC_DEBUG, 0, "decreasing to level TRACE any further DEBUG message 'ioctl(FIEMAP) successful'");

    ff_log(log_count < 5 ? FC_DEBUG : FC_TRACE, 0, "ioctl(%d, FIEMAP) successful: retrieved %"FT_ULL" extent%s in %"FT_ULL" call%s",
            fd, (ft_ull) extent_n, extent_n == 1 ? "" : "s", (ft_ull) ioctl_n, ioctl_n == 1 ? "" : "s");
    ret_block_size_bitmask = block_size_bitmask;

    return err;
#else
    return ENOSYS;
#endif /* FS_IOC_FIEMAP */
}



/**
 * retrieves file blocks allocation map (extents) for specified file descriptor
 * and appends them to ret_vector (with user_data = FC_DEFAULT_USER_DATA) sorted by ->logical
 * in case of failure returns errno-compatible error code, and ret_vector contents will be UNDEFINED.
 *
 * implementation: calls ioctl(FS_IOC_FIEMAP) and if it fails, tries with ioctl(FIBMAP)
 */
int ff_read_extents_posix(int fd, ft_uoff dev_length, fr_vector<ft_uoff> & ret_list, ft_uoff & ret_block_size_bitmask)
{
    int err;
    do {
        err = ff_linux_fiemap(fd, ret_list, ret_block_size_bitmask);
        if (err != 0) {
            int err2 = ff_posix_fibmap(fd, dev_length, ret_list, ret_block_size_bitmask);
            if (err2 != 0) {
                if (!ff_log_is_reported(err))
                    err = ff_log(FC_ERROR, err,  "%s", "failed to list file blocks with ioctl(FS_IOC_FIEMAP)");
                if (!ff_log_is_reported(err2))
                    err2 = ff_log(FC_ERROR, err2, "%s", "failed to list file blocks with ioctl(FIBMAP)");
            }
            err = err2;
        }
    } while (0);

    return err;
}

FT_IO_NAMESPACE_END
