/*
 * stl_map_max_size.cpp
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */
#include <stddef.h>    // for size_t */
#include <sys/types.h> // for off_t */

#include <cstdio>              // for fprintf() */
#include <map>                // for std::map<K,V> */

typedef size_t ft_size; /* type used to represent number of items in memory */
typedef off_t  ft_off; /* type used to represent length of a file */

struct ft_map_value {
	ft_off logical;
	ft_off length;
};

typedef std::map<ft_off, ft_map_value> ft_map;

int main()
{
	ft_map map;

	std::fprintf(stdout,
			"MAXOF(size_t)                     = 0x%lx\n"
			"MAXOF(size_t)/sizeof(net payload) = 0x%lx\n"
			"MAXOF(size_t)/sizeof(map node)    = 0x%lx\n"
			"std::map.max_size()               = 0x%lx\n",
			(unsigned long)(ft_size)-1,
			(unsigned long)((ft_size)-1 / (sizeof(ft_off) + sizeof(ft_map_value))),
			(unsigned long)((ft_size)-1 / (4 * sizeof(void *) + sizeof(ft_off) + sizeof(ft_map_value))),
			(unsigned long)map.max_size());
	return 0;
}
