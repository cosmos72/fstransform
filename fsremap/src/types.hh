/*
 * types.hh
 *
 *  Created on: Feb 14, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_TYPES_HH
#define FSTRANSFORM_TYPES_HH

#include "check.hh"

#if defined(FT_HAVE_STDDEF_H)
# include <stddef.h>     // for size_t
#elif defined(FT_HAVE_CSTDDEF)
# include <cstddef>      // for size_t
#endif
#if defined(FT_HAVE_STDINT_H)
# include <stdint.h>     // for uint64_t, uint32_t
#elif defined(FT_HAVE_CSTDINT)
# include <cstdint>      // for uint64_t, uint32_t
#endif

#ifdef FT_HAVE_SYS_TYPES_H
# include <sys/types.h>   // for off_t, pid_t, mode_t, ino_t
#endif
#ifdef FT_HAVE_SYS_STAT_H
# include <sys/stat.h>    // for struct stat
#endif
#ifdef FT_HAVE_UNISTD_H
# include <unistd.h>      //  "    "     "
#endif

#include <string>        // for std::string

#include "traits.hh"     // for FT_TYPE_TO_UNSIGNED

// ft_size is the type used to represent number of items in memory. when size_t exists, they must be the same type
typedef size_t ft_size;

// ft_off is the type used to represent files length and block devices length. when off_t exists, they must be the same type
typedef off_t  ft_off;

// ft_u64 must be exactly 64-bit unsigned integer. only used for ioctl(FS_IOC_FIEMAP), ioctl(BLKGETSIZE64)
typedef uint64_t ft_u64;

// ft_u32 must be exactly 32-bit unsigned integer. only used for ioctl(FS_IOC_FIEMAP)
typedef uint32_t ft_u32;

// ft_dev is the type used for ID of block devices. when dev_t exists, they must be the same type
typedef dev_t  ft_dev;

// ft_mode is the type used for file/directory permissions. when mode_t exists, they must be the same type
typedef mode_t ft_mode;

// ft_inode is the type used for inode numbers. when ino_t exists, they must be the same type
typedef ino_t ft_inode;

// ft_uoff is the unsigned variant of ft_off (in case ft_off is unsigned, they will be the same type)
typedef FT_TYPE_TO_UNSIGNED(ft_off) ft_uoff;

// ft_uint is a medium-size unsigned integer: smaller than ft_uoff,
// but large enough to store blocks count for many devices
typedef unsigned int ft_uint;


// ft_ull must be the largest unsigned integer type existing on the platform
#ifdef FT_HAVE_LONG_LONG
   typedef unsigned long long ft_ull;
#  define FT_ULL "llu"
#  define FT_XLL "llx"
#  define FT_OLL "llo" /* octal */
#else
   typedef unsigned long ft_ull;
#  define FT_ULL "lu"
#  define FT_XLL "lx"
#  define FT_OLL "lo" /* octal */
#endif

// ft_stat is the same as 'struct stat'
typedef struct stat ft_stat;

// ft_string must be functionally equivalent to 'std::string', i.e. have a compatible API
typedef std::string ft_string;


#endif /* FSTRANSFORM_TYPES_HH */
