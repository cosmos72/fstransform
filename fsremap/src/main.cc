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
 * main.cc
 *
 *  Created on: Mar 1, 2011
 *      Author: max
 */
#include "first.hh"



#undef FR_TEST_MAP
#undef FR_TEST_MAP_MERGE
#undef FR_TEST_VECTOR_COMPOSE
#undef FR_TEST_RANDOM
#undef FR_TEST_IOCTL_FIEMAP
#undef FR_TEST_WRITE_ZEROES
#undef FR_TEST_PRETTY_TIME




#if defined(FR_TEST_MAP)

# include "map.hh"
# include "work.hh"
FT_NAMESPACE_BEGIN

#define FR_MAIN(argc, argv) FT_NS test_map<ft_uint>()

template<typename T>
static int test_map() {
    fr_map<T> map;
    map.insert(3, 3, 1, FC_DEFAULT_USER_DATA);
    map.insert(0, 0, 3, FC_DEFAULT_USER_DATA);
    fr_work<T>::show("test map", "", 1, map, FC_INFO);
    return 0;
}
FT_NAMESPACE_END

#elif defined(FR_TEST_MAP_MERGE)

#include "log.hh"
#include "map.hh"
#include "vector.hh"
FT_NAMESPACE_BEGIN

template<typename T>
static void test_show(const char * label1, const char * label2, const fr_vector<T> v) {
    v.show(label1, label2, 1, FC_INFO);
}

#define FR_MAIN(argc, argv) FT_NS test_map_merge<ft_uoff>()

template<typename T>
static int test_map_merge() {
    fr_vector<T> v1, v2;

    v1.append(39174144, 4589568,  31744, FC_DEFAULT_USER_DATA);
    v2.append(70565888, 4476928, 114688, FC_DEFAULT_USER_DATA);
    test_show("v1", "", v1);
    test_show("v2", "", v2);

    // use fr_map<T>::merge_shift() to merge.
    // unluckily it merges based on ->physical, so we must transpose the vectors
    v1.transpose();
    v2.transpose();

    fr_map<ft_uoff> map;
    enum { NO_SHIFT = 0 };
    map.append0_shift(v1, NO_SHIFT);

    map.merge_shift(v2, NO_SHIFT, FC_PHYSICAL1);

    v1.assign(map.begin(), map.end());
    v1.transpose();

    test_show("v1", " after merge", v1);

    return 0;
}
FT_NAMESPACE_END

#elif defined(FR_TEST_VECTOR_COMPOSE)

#include "log.hh"
#include "vector.hh"
FT_NAMESPACE_BEGIN

template<typename T>
static void test_show(const char * label, const fr_vector<T> v) {
    v.show(label, "", 1, FC_INFO);
}

#define FR_MAIN(argc, argv) FT_NS test_vector_compose<ft_uoff>()

template<typename T>
static int test_vector_compose() {
    fr_vector<T> v1, v2, result, unmapped;
    T bitmask = 0;
    v1.append( 0, 10,100, FC_DEFAULT_USER_DATA);

    v2.append( 4,  0, 30, FC_DEFAULT_USER_DATA);
    v2.append(54, 30, 50, FC_DEFAULT_USER_DATA);
    v2.append(94, 80, 40, FC_DEFAULT_USER_DATA);

    int err = result.compose(v1, v2, bitmask, unmapped);
    if (err == 0) {
        ff_log(FC_INFO, 0, "block_size_bitmask = 0x%" FT_XLL, (ft_ull) bitmask);
        test_show("result", result);
        test_show("unmapped", unmapped);
    }
    return err ? 1 : 0;
}
FT_NAMESPACE_END

#elif defined(FR_TEST_RANDOM)

# include "assert.hh"
# include "log.hh"
# include "misc.hh"

FT_NAMESPACE_BEGIN

#define FR_MAIN(argc, argv) FT_NS test_random(argc, argv)
static int test_random(int argc, char ** argv) {
   ft_ull i, n = 1000, max = 7;
   if (argc > 1)
       ff_str2ull(argv[1], & max);
   if (argc > 2)
       ff_str2ull(argv[2], & n);

   
   if (max > 0x10000) {
       for (i = 0; i < n; i++)
           ff_log(FC_INFO, 0, "ff_random(%20"  FT_ULL ") = %20"  FT_ULL , max, ff_random(max));
   } else {
       ft_ull r;
       ft_size * hist = new ft_size[max + 1];
       for (i = 0; i <= max; i++)
       hist[i] = 0;
       for (i = 0; i < n; i++) {
       r = ff_random(max);
       ff_assert(r <= max);
       ++hist[r];
       }
       for (i = 0; i <= max; i++)
           ff_log(FC_INFO, 0, "histogram[%"  FT_ULL "] = %"  FT_ULL , i, (ft_ull)hist[i]);
       delete[] hist;
   }
   return 0;
}
FT_NAMESPACE_END

#elif defined(FR_TEST_IOCTL_FIEMAP)

# include "log.hh"
# include "vector.hh"
# include "io/extent_posix.hh"
# include "io/util_posix.hh"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

FT_NAMESPACE_BEGIN

using FT_IO_NS ff_posix_size;
using FT_IO_NS ff_read_extents_posix;

#define FR_MAIN(argc, argv) FT_NS test_ioctl_fiemap(argc, argv)
static int test_ioctl_fiemap(int argc, char ** argv)
{
    ft_log::get_root_logger().set_level(FC_DEBUG);
    ft_log_appender::redefine(stdout, FC_FMT_DATETIME_LEVEL_CALLER_MSG, FC_DEBUG, FC_NOTICE);
    ft_log_appender::redefine(stderr, FC_FMT_DATETIME_LEVEL_CALLER_MSG, FC_WARN);

    fr_vector<ft_uoff> extents;
    ft_uoff dev_size = 0x7FFFFFFFul, block_size_bitmask;
    const char * path;
    int fd, err = 0;

    while ((path = * ++argv) != NULL) {
        if ((fd = ::open(path, O_RDONLY)) < 0) {
            err = ff_log(FC_ERROR, errno, "error opening file '%s'", path);
            break;
        }
        extents.clear();
        block_size_bitmask = 0;

        if ((err = ff_read_extents_posix(fd, dev_size, extents, block_size_bitmask)) != 0)
            break;

        ft_size n = extents.size();

        ff_log(FC_INFO, 0, "# %4"  FT_ULL " extent%s in %s", (ft_ull) n, (n == 1 ? " " : "s"), path);
        ff_log(FC_INFO, 0, "#  extent           physical         logical      length  user_data");

        for (ft_size i = 0; i < n; i++) {
            fr_extent<ft_uoff> & e = extents[i];

            ff_log(FC_INFO, 0, "#%8"  FT_ULL "\t%12"  FT_ULL "\t%12"  FT_ULL "\t%8"  FT_ULL "\t(%"  FT_ULL ")", (ft_ull)i,
                   (ft_ull) e.physical(), (ft_ull) e.logical(), (ft_ull) e.length(), (ft_ull) e.user_data());
        }
    }
    return err;
}
FT_NAMESPACE_END


#elif defined(FR_TEST_WRITE_ZEROES)

FT_IO_NAMESPACE_BEGIN
int ff_zero_loop_file_holes(int argc, char ** argv);
FT_IO_NAMESPACE_END
# define FR_MAIN(argc, argv) FT_IO_NS ff_zero_loop_file_holes(argc, argv)


#elif defined(FR_TEST_PRETTY_TIME)


#include "misc.hh"
#include "log.hh"

FT_NAMESPACE_BEGIN
int ff_test_pretty_time(int argc, char ** argv) {
    ft_log_level log_level = FT_NS FC_INFO;
    double time, percentage = 0.0, pretty_len = 0;
    const char * simul_msg = "", * pretty_label = "";

    for (time = 0.4; time < 800000000.0; time *= 1.1) {
        ft_ull time_left1 = 0, time_left2 = 0;
        const char * time_left_label1 = NULL, * time_left_label2 = NULL;

        ff_pretty_time2(time, & time_left1, & time_left_label1, & time_left2, & time_left_label2);

        /* we write something like "1 hour and 20 minutes" instead of just "1 hour" or "1.3 hours" */
        if (time_left_label2 != NULL) {
            ff_log(log_level, 0, "%sprogress: %4.1f%% done, %.2f %sbytes still to relocate, estimated %"  FT_ULL " %s%s and %"  FT_ULL " %s%s left",
                    simul_msg, percentage, pretty_len, pretty_label,
                    time_left1, time_left_label1, (time_left1 != 1 ? "s": ""),
                    time_left2, time_left_label2, (time_left2 != 1 ? "s": ""));
        } else {
            ff_log(log_level, 0, "%sprogress: %4.1f%% done, %.2f %sbytes still to relocate, estimated %"  FT_ULL " %s%s left",
                    simul_msg, percentage, pretty_len, pretty_label,
                    time_left1, time_left_label1, (time_left1 != 1 ? "s": ""));
        }
    }
    return 0;
}
FT_NAMESPACE_END
# define FR_MAIN(argc, argv) FT_NS ff_test_pretty_time(argc, argv)



#else /* actual fstranform program */

# include "remap.hh"    // for fr_remap
# define FR_MAIN(argc, argv) FT_NS fr_remap::main(argc, argv)

#endif /* defined(FT_TEST_*) */


int main(int argc, char ** argv) {
    return FR_MAIN(argc, argv);
}
