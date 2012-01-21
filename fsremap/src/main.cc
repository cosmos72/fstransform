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
#undef FR_TEST_PRETTY_TIME




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


#elif defined(FR_TEST_PRETTY_TIME)


#include "util.hh"
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
    		ff_log(log_level, 0, "%sprogress: %4.1f%% done, %.2f %sbytes still to relocate, estimated %"FT_ULL" %s%s and %"FT_ULL" %s%s left",
        			simul_msg, percentage, pretty_len, pretty_label,
        			time_left1, time_left_label1, (time_left1 != 1 ? "s": ""),
        			time_left2, time_left_label2, (time_left2 != 1 ? "s": ""));
    	} else {
    		ff_log(log_level, 0, "%sprogress: %4.1f%% done, %.2f %sbytes still to relocate, estimated %"FT_ULL" %s%s left",
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
