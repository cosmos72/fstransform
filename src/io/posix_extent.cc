/*
 * io/posix_extent.cc
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "../first.hh" // for FT_*TEMPLATE* macros */

/*
 * io/posix_extent.template.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "../first.hh"

#include <cerrno>         // for errno, ENOMEM, EINVAL, EFBIG */
#include <cstdlib>        // for qsort() */
#include <cstring>        // for memset() */

#include <utility>        // for std::pair<T1,T2> and std::make_pair() */
#include <vector>         // for std::vector<T> */

#include <linux/fs.h>     // for FS_IOC_FIEMAP, FIBMAP */
#include <linux/fiemap.h> // for struct fiemap and struct fiemap_extent */

#include "../fail.hh"        // for ff_fail() */
#include "../traits.hh"      // for FT_TYPE_TO_UNSIGNED(T) */
#include "../types.hh"       // for ft_off */
#include "../extent.hh"      // for ft_extent<T>, ft_map<T>, ff_filemap() */
#include "../vector.hh"      // for ft_vector<T> */
#include "posix_extent.hh"   // for ff_posix_extents() */
#include "posix_util.hh"     // for ff_posix_ioctl(), ff_posix_size() */

FT_IO_NAMESPACE_BEGIN


/**
 * retrieves file blocks allocation map (extents) for specified file descriptor
 * and appends them to ret_vector.
 * in case of failure returns errno-compatible error code, and ret_vector contents will be UNDEFINED.
 *
 * must (and will) also check that device blocks count can be represented by ret_list,
 * by calling ret_list.extent_set_range(block_size, block_count)
 *
 * implementation: calls ioctl(FIBMAP)
 */
static int ff_posix_fibmap(int fd, ft_uoff block_size_log2, ft_extent_list & ret_list)
{
    ft_uoff file_length, file_block_count, block_size = (ft_uoff)1 << block_size_log2;

    /* lower-level API ff_posix_ioctl(FIGETBSZ) and ff_posix_ioctl(FIBMAP) need these to be int */
    int err = 0, logical, physical;

    do {
        if ((err = ff_posix_size(fd, & file_length)))
            break;

        // number of blocks in the file
        file_block_count = (file_length + block_size - 1) / block_size;

        // ioctl(FIBMAP) wants an (int) as block number,
        // so we must check for overflow!
        int n = (int) file_block_count;
        if (n < 0 || file_block_count != (ft_uoff) n) {
            /* overflow! give up. */
            err = EFBIG;
            break;
        }

        for (logical = 0; logical < n; logical++) {
            physical = logical;
            if ((err = ff_posix_ioctl(fd, FIBMAP, & physical)))
                break;
            /* FIBMAP reports holes (i.e. unallocated blocks in the file) as physical == 0 */
            if (physical != 0)
                /* this is painful... FIBMAP reports one block per call */
                ret_list.extent_append((ft_uoff) physical, (ft_uoff) logical, (ft_uoff) 1);
        }
    } while (0);

    return err;
}

/** report an extent incompatible with block size */
static int ff_linux_fiemap_inconsistency(const struct fiemap_extent * extent, ft_uoff block_size)
{
#ifdef FT_HAVE_LONG_LONG
    ff_fail(0, "error in ff_linux_fiemap(): ioctl(FS_IOC_FIEMAP) extent "
            "physical = %llu, logical = %llu, length = %llu is not multiple of block size (%llu)",
            (unsigned long long) extent->fe_physical, (unsigned long long) extent->fe_logical,
            (unsigned long long) extent->fe_length, (unsigned long long) block_size);
#else
    ff_fail(0, "error in ff_linux_fiemap(): ioctl(FS_IOC_FIEMAP) extent "
            "physical = %lu, logical = %lu, length = %lu is not multiple of block size (%lu)",
            (unsigned long) extent->fe_physical, (unsigned long) extent->fe_logical,
            (unsigned long) extent->fe_length, (unsigned long) block_size);
#endif
    return EINVAL;
}


static int ff_linux_fiemap_allocate_and_ioctl(int fd, ft_uoff file_length, ft_size extent_n, struct fiemap ** ret_k_map) {
    struct fiemap * k_map;
    ft_size k_len = sizeof(struct fiemap) + extent_n * sizeof(struct fiemap_extent);
    int err;

    k_map = (struct fiemap *) malloc(k_len);
    if (k_map == NULL)
        return ENOMEM; /* Out of memory */
    memset(k_map, 0, k_len);

    k_map->fm_start = 0L;
    k_map->fm_length = (ft_u64) file_length;
    k_map->fm_flags = FIEMAP_FLAG_SYNC;
    k_map->fm_extent_count = extent_n;

    if ((err = ff_posix_ioctl(fd, FS_IOC_FIEMAP, k_map)) != 0) {
        free(k_map);
        k_map = NULL;
    }
    * ret_k_map = k_map;
    return err;
}

/*
 * retrieves file blocks allocation map (extents) for specified file descriptor
 * and appends them to ret_vector.
 * in case of failure returns errno-compatible error code and ret_vector contents will be UNCHANGED.
 *
 * must (and will) also check that device blocks count can be represented by ret_list,
 * by calling ret_list.extent_set_range(block_size, block_count)
 *
 * implementation: calls ioctl(FS_IOC_FIEMAP)
 */
static int ff_linux_fiemap(int fd, ft_uoff block_size_log2, ft_extent_list & ret_list) {
    struct fiemap * k_map = NULL;
    struct fiemap_extent * k_extent;
    ft_uoff file_length, block_size_mask, block_size = (ft_uoff) 1 << block_size_log2;
    ft_size i, extent_n = 0;
    int err;

    do {
        /* make sure we can represent file extents with current choice of 'T' */
        if ((err = ff_posix_size(fd, & file_length)))
            break;

        /* first pass: call ioctl() and ask how many extents are needed */
        if ((err = ff_linux_fiemap_allocate_and_ioctl(fd, file_length, extent_n, & k_map)))
            break;

        extent_n = k_map->fm_mapped_extents;
        free(k_map);
        k_map = NULL;

        /* second pass: allocate enough extents and call ioctl() again to retrieve them */
        if ((err = ff_linux_fiemap_allocate_and_ioctl(fd, file_length, extent_n, & k_map)))
            break;

        extent_n = k_map->fm_mapped_extents;
        k_extent = k_map->fm_extents;

        /*
         * perform a first loop, checking for unsupported extents
         * and computing an effective block size
         */
        for (i = 0; i < extent_n; i++) {
            if (k_extent[i].fe_flags & (FIEMAP_EXTENT_UNKNOWN | FIEMAP_EXTENT_ENCODED)) {
                err = ENOSYS;
                break;
            }
            /* keep track of bits used by all physical, logical and lengths.
             * needed to check against block size */
            block_size_mask = k_extent[i].fe_physical | k_extent[i].fe_logical | k_extent[i].fe_length;

            /* check that received block size is compatible with extents (it could be sub-optimal though) */
            if (block_size_mask & (block_size - 1)) {
                err = ff_linux_fiemap_inconsistency(& k_extent[i], block_size);
                break;
            }
        }
        if (err != 0)
            break;



        /* ok, no strange extents: we can now add them to ret_list */
        for (i = 0; i < extent_n; i++) {
            if (k_extent[i].fe_flags & FIEMAP_EXTENT_UNWRITTEN)
                continue;

            ret_list.extent_append((ft_uoff) k_extent[i].fe_physical >> block_size_log2,
                                   (ft_uoff) k_extent[i].fe_logical  >> block_size_log2,
                                   (ft_uoff) k_extent[i].fe_length   >> block_size_log2);
        }
    } while (0);

    if (k_map != NULL)
        free(k_map);
    return err;
}

/** compute log2(block size) */
int ff_util_log2(ft_uoff n, ft_uoff * ret_log2) {
    if (n & (n - 1)) {
#ifdef FT_HAVE_LONG_LONG
        ff_fail(0, "error in ff_posix_extents(): block size (%llu) returned by ioctl(FIGETBSZ) is not a power of 2",
                (unsigned long long) n);
#else
        ff_fail(0, "error in ff_posix_extents(): block size (%lu) returned by ioctl(FIGETBSZ) is not a power of 2",
                (unsigned long) n);
#endif
        return EINVAL;
    }

    ft_uoff log2 = 0;
    while ((n >>= 1) != 0)
        log2++;

    * ret_log2 = log2;

    return 0;
}


/**
 * retrieves file blocks allocation map (extents) for specified file descriptor
 * and appends them to ret_vector.
 * in case of failure returns errno-compatible error code, and ret_vector contents will be UNDEFINED.
 *
 * implementation: calls ioctl(FS_IOC_FIEMAP) and if it fails, tries with ioctl(FIBMAP)
 *
 * must (and will) also check that device blocks count can be represented by ret_list,
 * by calling ret_list.extent_set_range(block_size, block_count)
 */
int ff_posix_extents(int fd, ft_uoff dev_length, ft_extent_list & ret_list) {
    ft_uoff block_size, block_size_log2, dev_block_count;
    int block_size_int;
    int err;

    do {
        if ((err = ff_posix_ioctl(fd, FIGETBSZ, & block_size_int)))
            break;
        block_size = (ft_uoff) block_size_int;
        // ft_uoff is expected to be unsigned and wider than int,
        // but explicitly checking for overflow is always safer than expecting
        if (block_size < 0 || block_size_int != (int) block_size) {
            /* overflow! give up. */
            err = EFBIG;
            break;
        }
        /* compute log2(block_size) */
        if ((err = ff_util_log2(block_size, & block_size_log2)))
            break;

        // number of blocks in the device
        dev_block_count = (dev_length + block_size - 1) / block_size;

        // check that device blocks count can be represented by ret_list,
        // by calling ret_list.extent_set_range(block_size, block_count)
        if ((err = ret_list.extent_set_range(block_size, dev_block_count)))
            break;

        err = ff_linux_fiemap(fd, block_size_log2, ret_list);
        if (err != 0) {
            int err2 = ff_posix_fibmap(fd, block_size_log2, ret_list);
            if (err2 == 0)
                err = err2;
            else {
                ff_fail(err,  "%s", "failed to list file blocks with ioctl(FS_IOC_FIEMAP)");
                ff_fail(err2, "%s", "failed to list file blocks with ioctl(FIBMAP)");
            }
        }

    } while (0);

    return err;
}

FT_IO_NAMESPACE_END
