/*
 * file_extent.template.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "first.hh"

#include <cerrno>         /* for errno, ENOMEM, EINVAL, EFBIG */
#include <climits>        /* for INT_MAX */
#include <cstdlib>        /* for qsort() */
#include <cstring>        /* for memset() */

#include <limits>         /* for std::numeric_limits<T> */
#include <utility>        /* for std::pair<T1,T2> and std::make_pair() */
#include <vector>         /* for std::vector<T> */

#include <linux/fs.h>     /* for FS_IOC_FIEMAP, FIBMAP */
#include <linux/fiemap.h> /* for struct fiemap and struct fiemap_extent */

#include "fail.hh"        /* for ff_fail() */
#include "traits.hh"      /* for FT_TYPE_TO_UNSIGNED(T) */
#include "types.hh"       /* for ft_off */
#include "extent.hh"      /* for ft_extent<T>, ft_map<T>, ff_filemap() */
#include "vector.hh"      /* for ft_vector<T> */
#include "file_extent.hh" /* for ff_file_extents() */
#include "file_util.hh"   /* for ff_ioctl(), ff_size() */

/**
 * if file size can be retrieved and it can be represented by T,
 * set (* ret_size) to file size and return 0,
 * else return error (if file size cannot be represented by T, error is EFBIG)
 */
template<typename T>
static T ff_file_size(int fd, T * ret_size)
{
    /* not 'T file_size' because lower-level API ff_size() expect this to be ft_off */
    ft_off file_size;

    int err = 0;

    do {
        if ((err = ff_size(fd, & file_size)))
            break;

        typedef typename FT_TYPE_TO_UNSIGNED(ft_off) ft_off_unsigned;
        typedef typename FT_TYPE_TO_UNSIGNED(T)      T_unsigned;

        /* avoid comparison between signed and unsigned */
        if ((ft_off_unsigned) file_size > (T_unsigned) std::numeric_limits<T>::max()) {
            /*
             * overflow! we cannot represent file size - and extents! - with current choice of 'T'
             * Give up.
             */
            err = EFBIG;
            break;
        }
        /** this possibly narrowing cast is safe, we just checked for overflow */
        * ret_size = (T) file_size;

    } while (0);

    return err;
}


/**
 * retrieves file blocks allocation map (extents) for specified file descriptor
 * and appends them to ret_vector.
 * in case of failure returns errno-compatible error code, and ret_vector contents will be unchanged.
 *
 * implementation: calls ioctl(FIBMAP)
 */
template<typename T>
static int ff_file_fibmap(int fd, ft_vector<T> & ret_vector)
{
    /* lower-level API ff_ioctl(FIGETBSZ) and ff_ioctl(FIBMAP) need these to be int */
    int err = 0, logical, physical, block_size;
    ft_size original_vector_size = ret_vector.size();
    T file_size, block_count;

    do {
        if ((err = ff_ioctl(fd, FIGETBSZ, & block_size)))
            break;

        if ((err = ff_file_size(fd, & file_size)))
            break;
        
        /* number of blocks in the file */
        block_count = (file_size + block_size - 1) / block_size;
        if (block_count > INT_MAX) {
            /*
             * overflow! ioctl(FIBMAP) wants an (int) as block number,
             * but this file contains more than INT_MAX blocks.
             * Give up.
             */
            err = EFBIG;
            break;
        }
        /* this possibly narrowing cast is now safe, we just checked for overflow */
        int n = (int) block_count;
        
        for (logical = 0; logical < n; logical++) {
            physical = logical;
            if ((err = ff_ioctl(fd, FIBMAP, & physical)))
                break;
            /* FIBMAP reports holes (i.e. unallocated blocks in the file) as physical == 0 */
            if (physical == 0)
                continue;
            ret_vector.append((T) physical * block_size, (T) logical * block_size, (T) block_size);
        }
    } while (0);

    if (err)
        ret_vector.resize(original_vector_size);
    return err;
}



static int ff_file_fiemap_allocate_and_ioctl(int fd, ft_size extent_n, struct fiemap ** pk_map) {
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
 * retrieves file blocks allocation map (extents) for specified file descriptor
 * and appends them to ret_vector.
 * in case of failure returns errno-compatible error code and ret_vector contents will be unchanged.
 *
 * implementation: calls ioctl(FS_IOC_FIEMAP)
 */
template<typename T>
static int ff_file_fiemap(int fd, ft_vector<T> & ret_vector) {
    struct fiemap * k_map = NULL;
    struct fiemap_extent * k_extent;
    T file_size;
    ft_size i, extent_n = 0, original_vector_size = ret_vector.size();
    int err;


    do {
        /* make sure we can represent file extents with current choice of 'T' */
        if ((err = ff_file_size(fd, & file_size)))
            break;

        /* first pass: call ioctl() and ask how many extents are needed */
        if ((err = ff_file_fiemap_allocate_and_ioctl(fd, extent_n, & k_map)))
            break;

        extent_n = k_map->fm_mapped_extents;
        free(k_map);

        /* second pass: allocate enough extents and call ioctl() again to retrieve them */
        if ((err = ff_file_fiemap_allocate_and_ioctl(fd, extent_n, & k_map)))
            break;

        extent_n = k_map->fm_mapped_extents;
        k_extent = k_map->fm_extents;

        ret_vector.reserve(extent_n);
        ft_extent<T> extent;

        for (i = 0; i < extent_n; i++) {
            if (k_extent[i].fe_flags & (FIEMAP_EXTENT_UNKNOWN | FIEMAP_EXTENT_ENCODED)) {
                err = ENOSYS;
                break;
            }
            if (k_extent[i].fe_flags & FIEMAP_EXTENT_UNWRITTEN) {
                continue;
            }
            ret_vector.append((T) k_extent[i].fe_physical, (T) k_extent[i].fe_logical, (T) k_extent[i].fe_length);
        }
    } while (0);

    free(k_map);
    if (err)
        ret_vector.resize(original_vector_size);
    return err;
}


/**
 * retrieves file blocks allocation map (extents) for specified file descriptor
 * and appends them to ret_vector.
 * in case of failure returns errno-compatible error code, and ret_vector contents will be unchanged.
 *
 * implementation: calls ioctl(FS_IOC_FIEMAP) and if it fails, tries with ioctl(FIBMAP)
 */
template<typename T>
int ff_file_extents(int fd, ft_vector<T> & ret_vector) {
    int err1 = ff_file_fiemap(fd, ret_vector), err2 = 0;
    if (err1 != 0) {
        err2 = ff_file_fibmap(fd, ret_vector);
        if (err2 != 0) {
        	ff_fail(err1, "%s", "failed to list file blocks with ioctl(FS_IOC_FIEMAP)");
        	ff_fail(err2, "%s", "failed to list file blocks with ioctl(FIBMAP)");
        }
    }
    return err1 == 0 ? err1 : err2;
}
