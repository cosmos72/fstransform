/*
 * filemap.c
 *
 *  Created on: Feb 22, 2011
 *      Author: max
 */

#include "first.h"

#include <errno.h>         /* for errno, ENOMEM, EINVAL, EFBIG */
#include <stdlib.h>        /* for qsort() */
#include <string.h>        /* for memset() */
#include <limits.h>        /* for INT_MAX */

#include <linux/fs.h>      /* for FS_IOC_FIEMAP, FIBMAP */
#include <linux/fiemap.h>  /* for struct fiemap and struct fiemap_extent */

#include "fail.h"          /* for ff_fail() */
#include "filemap.h"       /* for ft_extent, ft_map, ff_map_*() */
#include "fileutil.h"      /* for ff_ioctl(), ff_size() */


/**
 * alternative ff_filemap() implementation using ioctl(FIBMAP) instead of ioctl(FS_IOC_FIEMAP),
 * in case the latter is not available / not supported
 */
static int ff_fibmap(int fd, ft_map ** pret_map) {
    off_t i, n;
    int err = 0, block, blocksize;

    if (pret_map == NULL)
        return EINVAL; /* Invalid argument */

    do {
        if ((err = ff_ioctl(fd, FIGETBSZ, & blocksize)))
            break;
        if ((err = ff_size(fd, & n)))
            break;
        
        /* number of blocks in the file */
        n = (n + blocksize - 1) / blocksize;
        if (n > INT_MAX) {
            /*
             * overflow! ioctl(FIBMAP) wants an (int) as block number,
             * but this file contains more than INT_MAX blocks.
             * Give up.
             */
            err = EFBIG; 
            break;
        }
        
        ft_map * map, * new_map;
        /*
         * allocate initial (empty) map even if n == 0.
         * caller expects (* pret_map) to be != NULL on successful return
         */
        if ((map = ff_map_alloc(ft_map_default_capacity)) == NULL) {
        	err = ENOMEM;
            break;
        }
        
        for (i = 0; i < n; i++) {
            block = i;
            if ((err = ff_ioctl(fd, FIBMAP, &block)))
                break;
            /* FIBMAP reports holes (i.e. unallocated blocks in the file) as physical = 0 */
            if (block == 0)
                continue;
            if ((new_map = ff_map_append(map, (ft_off) i * blocksize, (ft_off) block * blocksize, (ft_off) blocksize)) != NULL)
            	map = new_map;
            else {
            	err = ENOMEM;
                break;
            }
        }
        if (err == 0)
            /*
             * we almost surely return a map with capacity > map->fc_size,
             * but it's not a problem... just some wasted memory until map is freed
             */
            * pret_map = map;

    } while (0);
    return err;
}



static int ff_fiemap_allocate_and_ioctl(int fd, ft_size extent_n, struct fiemap ** pk_map) {
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

    if ((err = ff_ioctl(fd, FS_IOC_FIEMAP, k_map)) != 0) {
        free(k_map);
        * pk_map = NULL;
        return err;
    }
    return 0;
}

/*
 * ff_filemap() implementation using ioctl(FS_IOC_FIEMAP)
 */
static int ff_fiemap(int fd, ft_map ** pret_map) {
    struct fiemap * k_map;
    struct fiemap_extent * k_extent;
    ft_map * ret_map;
    ft_extent * ret_extent;
    ft_size ret_len, i, extent_n = 0;
    int err;

    if (pret_map == NULL)
        return EINVAL; /* Invalid argument */
    ret_map = NULL;

    do {
        /* first pass: call ioctl() and ask how many extents are needed */
        if ((err = ff_fiemap_allocate_and_ioctl(fd, extent_n, & k_map)))
            break;

        extent_n = k_map->fm_mapped_extents;
        free(k_map);

        /* second pass: allocate enough extents and call ioctl() again to retrieve them */
        if ((err = ff_fiemap_allocate_and_ioctl(fd, extent_n, & k_map)))
            break;

        extent_n = k_map->fm_mapped_extents;
        ret_len = sizeof(ft_map) + extent_n * sizeof(ft_extent);
        ret_map = (ft_map *) malloc(ret_len);
        if (ret_map == NULL) {
            err = ENOMEM; /* Out of memory */
            break;
        }
        memset(ret_map, 0, ret_len);

        k_extent = k_map->fm_extents;
        ret_extent = ret_map->fc_extents;
        ret_map->fc_size = extent_n;
        for (i = 0; i < extent_n; i++) {
            if (k_extent[i].fe_flags & (FIEMAP_EXTENT_UNKNOWN | FIEMAP_EXTENT_ENCODED)) {
                err = ENOSYS;
                break;
            }
            if (k_extent[i].fe_flags & FIEMAP_EXTENT_UNWRITTEN) {
                continue;
            }
            ret_extent[i].fc_logical  = k_extent[i].fe_logical;
            ret_extent[i].fc_physical = k_extent[i].fe_physical;
            ret_extent[i].fc_length   = k_extent[i].fe_length;
        }
    } while (0);


    free(k_map);
    if (err != 0) {
        free(ret_map);
        ret_map = 0;
    }
    * pret_map = ret_map;
    return err;
}


/**
 * retrieves file blocks allocation map for specified file descriptor
 * and returns pointer to allocated struct in (*ret_map);
 * in case of failure returns operating system error (and *ret_map will be NULL).
 *
 * struct returned in *ret_map must be freed by caller with free() when not needed anymore.
 *
 * implementation: calls ff_fiemap() and if it fails, tries with ff_fibmap().
 */
int ff_filemap(int fd, ft_map ** pret_map) {
    int err1 = ff_fiemap(fd, pret_map), err2 = 0;
    if (err1) {
        err2 = ff_fibmap(fd, pret_map);
        if (err2) {
        	ff_fail(err1, "%s", "failed to get file blocks with ioctl(FS_IOC_FIEMAP)");
        	ff_fail(err2, "%s", "failed to get file blocks with ioctl(FIBMAP)");
        }
    }
    return err1 == 0 ? err1 : err2;
}
