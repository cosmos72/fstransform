/*
 * main.cc
 *
 *  Created on: Mar 1, 2011
 *      Author: max
 */
#include "first.hh"



#undef FR_TEST_MAP
#undef FR_TEST_RANDOM
#undef FR_TEST_WRITE_ZEROES




#if defined(FR_TEST_MAP)

# include "map.hh"
# include "work.hh"
#define FR_MAIN(argc, argv) FT_NS test_map<ft_uint>()
FT_NAMESPACE_BEGIN
template<typename T>
static int test_map() {
    fr_map<T> map;
    map.insert(3, 3, 1, FC_DEFAULT_USER_DATA);
    map.insert(0, 0, 3, FC_DEFAULT_USER_DATA);
    fr_work<T>::show("test map", "", 1, map, FC_INFO);
    return 0;
}
FT_NAMESPACE_END


#elif defined(FR_TEST_RANDOM)

# include "assert.hh"
# include "log.hh"
# include "util.hh"
#define FR_MAIN(argc, argv) FT_NS test_random(argc, argv)
FT_NAMESPACE_BEGIN
static int test_random(int argc, char ** argv) {
   ft_ull i, n = 1000, max = 7;
   if (argc > 1)
       ff_str2ull(argv[1], & max);
   if (argc > 2)
       ff_str2ull(argv[2], & n);

   
   if (max > 0x10000) {
       for (i = 0; i < n; i++)
           ff_log(FC_INFO, 0, "ff_random(%20"FT_ULL") = %20"FT_ULL, max, ff_random(max));
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
           ff_log(FC_INFO, 0, "histogram[%"FT_ULL"] = %"FT_ULL, i, (ft_ull)hist[i]);
       delete[] hist;
   }
   return 0;
}
FT_NAMESPACE_END

#elif defined(FR_TEST_WRITE_ZEROES)

FT_IO_NAMESPACE_BEGIN
int ff_zero_loop_file_holes(int argc, char ** argv);
FT_IO_NAMESPACE_END
# define FR_MAIN(argc, argv) FT_IO_NS ff_zero_loop_file_holes(argc, argv)


#else /* actual fstranform program */

# include "remap.hh"    // for fr_remap
# define FR_MAIN(argc, argv) FT_NS fr_remap::main(argc, argv)

#endif /* defined(FT_TEST_*) */




int main(int argc, char ** argv) {
    return FR_MAIN(argc, argv);
}
