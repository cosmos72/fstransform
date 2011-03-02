/*
 * types.hh
 *
 *  Created on: Feb 14, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_TYPES_HH
#define FSTRANSLATE_TYPES_HH

#include "check.hh"

#include <stddef.h>      // for size_t        */
#include <linux/types.h> // for __u32, __u64  */
#include <sys/types.h>   // for off_t         */
#include <sys/stat.h>    // for struct stat   */
#include <unistd.h>      /*  "    "     "     */

#include "traits.hh"     // for FT_TYPE_TO_UNSIGNED

typedef size_t ft_size;  // type used to represent number of items in memory. just an alias for size_t
typedef off_t  ft_off;   // type used to represent length of a file or block device, at least as wide as off_t
typedef __u32  ft_u32;   // 32-bit unsigned integer
typedef __u64  ft_u64;   // 64-bit unsigned integer
typedef dev_t  ft_dev;   // type used for ID of block devices


typedef FT_TYPE_TO_UNSIGNED(ft_off) ft_uoff; // unsigned variant of ft_off, in case ft_off is signed


typedef struct stat ft_stat;

#endif /* FSTRANSLATE_TYPES_HH */
