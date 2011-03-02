/*
 * io/posix_extent.cc
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "../first.hh" // for FT_*EXTERN_TEMPLATE* macros */

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
 * implementation: calls ioctl(FIBMAP)
 */
static int ff_posix_fibmap(int fd, ft_extent_list & ret_list)
{
    /* lower-level API ff_posix_ioctl(FIGETBSZ) and ff_posix_ioctl(FIBMAP) need these to be int */
    int err = 0, logical, physical, block_size;
    ft_uoff file_size, block_count;

    do {
        if ((err = ff_posix_ioctl(fd, FIGETBSZ, & block_size)))
            break;

        if ((err = ff_posix_size(fd, & file_size)))
            break;

        /* number of blocks in the file */
        block_count = (file_size + block_size - 1) / block_size;

        /*
         * ioctl(FIBMAP) wants an (int) as block number,
         * so we need this possibly narrowing cast
         * => we must check for overflow
         */
        int n = (int) block_count;
        if (n < 0 || block_count != (ft_uoff) n) {
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
                ret_list.extent_append((ft_uoff) physical * block_size, (ft_uoff) logical * block_size, (ft_uoff) block_size);
        }
    } while (0);

    return err;
}



static int ff_linux_fiemap_allocate_and_ioctl(int fd, ft_size extent_n, struct fiemap ** pk_map) {
    struct fiemap * k_map;
    ft_size k_len = sizeof(struct fiemap) + extent_n * sizeof(struct fiemap_extent);
    int err;

    * pk_map = k_map = (struct fiemap *) malloc(k_len);
    if (k_map == NULL)
        return ENOMEM; /* Out of memory */
    memset(k_map, 0, k_len);

    k_map->fm_start = 0L;
    k_map->fm_length = ~(ft_off)0L;
    k_map->fm_flags = FIEMAP_FLAG_SYNC | FIEMAP_FLAG_XATTR;
    k_map->fm_extent_count = extent_n;

    if ((err = ff_posix_ioctl(fd, FS_IOC_FIEMAP, k_map)) != 0) {
        free(k_map);
        * pk_map = NULL;
        return err;
    }
    return 0;
}

/*
 * retrieves file blocks allocation map (extents) for specified file descriptor
 * and appends them to ret_vector.
 * in case of failure returns errno-compatible error code and ret_vector contents will be UNCHANGED.
 *
 * implementation: calls ioctl(FS_IOC_FIEMAP)
 */
static int ff_linux_fiemap(int fd, ft_extent_list & ret_list) {
    struct fiemap * k_map = NULL;
    struct fiemap_extent * k_extent;
    ft_uoff file_size;
    ft_size i, extent_n = 0;
    int err;


    do {
        /* make sure we can represent file extents with current choice of 'T' */
        if ((err = ff_posix_size(fd, & file_size)))
            break;

        /* first pass: call ioctl() and ask how many extents are needed */
        if ((err = ff_linux_fiemap_allocate_and_ioctl(fd, extent_n, & k_map)))
            break;

        extent_n = k_map->fm_mapped_extents;
        free(k_map);

        /* second pass: allocate enough extents and call ioctl() again to retrieve them */
        if ((err = ff_linux_fiemap_allocate_and_ioctl(fd, extent_n, & k_map)))
            break;

        extent_n = k_map->fm_mapped_extents;
        k_extent = k_map->fm_extents;

        /* perform a first loop, checking for unsupported extents */
        for (i = 0; i < extent_n; i++) {
            if (k_extent[i].fe_flags & (FIEMAP_EXTENT_UNKNOWN | FIEMAP_EXTENT_ENCODED)) {
                err = ENOSYS;
                break;
            }
        }
        if (err != 0)
            break;

        /* ok, no strange extents: we can now add them to ret_list */
        for (i = 0; i < extent_n; i++) {
            if (k_extent[i].fe_flags & FIEMAP_EXTENT_UNWRITTEN)
                continue;

            ret_list.extent_append((ft_uoff) k_extent[i].fe_physical, (ft_uoff) k_extent[i].fe_logical, (ft_uoff) k_extent[i].fe_length);
        }
    } while (0);

    free(k_map);
    return err;
}


/**
 * retrieves file blocks allocation map (extents) for specified file descriptor
 * and appends them to ret_vector.
 * in case of failure returns errno-compatible error code, and ret_vector contents will be UNDEFINED.
 *
 * implementation: calls ioctl(FS_IOC_FIEMAP) and if it fails, tries with ioctl(FIBMAP)
 */
int ff_posix_extents(int fd, ft_extent_list & ret_list) {
    int err1 = ff_linux_fiemap(fd, ret_list), err2 = 0;
    if (err1 != 0) {
        err2 = ff_posix_fibmap(fd, ret_list);
        if (err2 != 0) {
            ff_fail(err1, "%s", "failed to list file blocks with ioctl(FS_IOC_FIEMAP)");
            ff_fail(err2, "%s", "failed to list file blocks with ioctl(FIBMAP)");
        }
    }
    return err1 == 0 ? err1 : err2;
}

FT_IO_NAMESPACE_END
