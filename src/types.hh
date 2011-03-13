/*
 * types.hh
 *
 *  Created on: Feb 14, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_TYPES_HH
#define FSTRANSLATE_TYPES_HH

#include "check.hh"

#include <stddef.h>      // for size_t
#include <linux/types.h> // for __u32, __u64
#include <sys/types.h>   // for off_t, pid_t, mode_t
#include <sys/stat.h>    // for struct stat
#include <unistd.h>      //  "    "     "

#include "traits.hh"     // for FT_TYPE_TO_UNSIGNED

// ft_size is the type used to represent number of items in memory. when size_t exists, they must be the same type
typedef size_t ft_size;

// ft_off is the type used to represent files length and block devices length. when off_t exists, they must be the same type
typedef off_t  ft_off;

// ft_u64 must be exactly 64-bit unsigned integer. only used for ioctl(FS_IOC_FIEMAP), ioctl(BLKGETSIZE64)
typedef __u64  ft_u64;

// ft_dev is the type used for ID of block devices. when dev_t exists, they must be the same type
typedef dev_t  ft_dev;

// ft_pid is the type used for process PIDs. when ft_pid exists, they must be the same type
typedef pid_t  ft_pid;

// ft_mode is the type used for file/directory permissions. when mode_t exists, they must be the same type
typedef mode_t ft_mode;


// ft_uoff is the unsigned variant of ft_off (in case ft_off is unsigned, they will be the same type)
typedef FT_TYPE_TO_UNSIGNED(ft_off) ft_uoff;

// ft_uint is a medium-size unsigned integer: smaller than ft_uoff,
// but large enough to store blocks count for many devices
typedef unsigned int ft_uint;


// ft_ull must be the largest unsigned integer type existing on the platform
#ifdef FT_HAVE_LONG_LONG
   typedef unsigned long long ft_ull;
#  define FS_ULL "llu"
#else
   typedef unsigned long ft_ull;
#  define FS_ULL "lu"
#endif

// ft_stat is the same as 'struct stat'
typedef struct stat ft_stat;

#endif /* FSTRANSLATE_TYPES_HH */
