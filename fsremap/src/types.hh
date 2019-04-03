/*
 * common types for fsattr, fsmove, fsremap
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

// ft_ssize is the signed version of ft_size. when ssize_t exists, they must be the same type
typedef ssize_t ft_ssize;

// ft_off is the type used to represent files length and block devices length. when off_t exists, they must be the same type
typedef off_t  ft_off;

// ft_u64 must be exactly 64-bit unsigned integer. used for ioctl(FS_IOC_FIEMAP), ioctl(BLKGETSIZE64), e4_cpu2fs() and e4_fs2cpu()
typedef uint64_t ft_u64;

// ft_u32 must be exactly 32-bit unsigned integer. used for ioctl(FS_IOC_FIEMAP), e4_cpu2fs() and e4_fs2cpu()
typedef uint32_t ft_u32;

// ft_i32 must be exactly 32-bit signed integer. used for ft_rope_impl
typedef int32_t ft_i32;

// ft_u16 must be exactly 16-bit unsigned integer. only used for e4_cpu2fs() and e4_fs2cpu()
typedef uint16_t ft_u16;

// ft_u8 must be exactly 8-bit unsigned integer. only used for e4_cpu2fs() and e4_fs2cpu()
typedef uint8_t  ft_u8;

// ft_dev is the type used for ID of block devices. when dev_t exists, they must be the same type
typedef dev_t  ft_dev;

// ft_mode is the type used for file/directory permissions. when mode_t exists, they must be the same type
typedef mode_t ft_mode;

// ft_inode is the type used for inode numbers. when ino_t exists, they must be the same type
typedef ino_t ft_inode;

// ft_nlink is the type used for inode link count. when nlink_t exists, they must be the same type
typedef nlink_t ft_nlink;

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
