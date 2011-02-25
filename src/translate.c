/*
 * translate.c
 *
 *  Created on: Feb 25, 2011
 *      Author: max
 */
#include "first.h"

#include <stdlib.h> /* for qsort()            */

#include "map.h"    /* for ft_extent, ft_map, fc_map_*() */



static int ff_map_compare_physical(const void * p1, const void * p2) {
	const ft_extent * extent1 = (const ft_extent *) p1;
	const ft_extent * extent2 = (const ft_extent *) p2;
	if (extent1->fc_physical > extent2->fc_physical)
		return 1;
	else if (extent1->fc_physical == extent2->fc_physical)
		return 0;
	else
		return -1;
}

/**
 * reorder a ft_map in-place, sorting by fc_physical
 */
void ff_map_sort_by_physical(ft_map * map) {
	qsort(map->fc_extents, map->fc_size, sizeof(ft_extent), ff_map_compare_physical);
}

/**
 * makes the complement of a ft_map in-place,
 * i.e. calculates the device extents NOT used by the file described by ft_map
 *
 * since the file(s) contained in such extents are not known,
 * all returned extents will have fc_logical == 0.
 *
 * internally calls ff_map_sort_by_physical() on the map,
 * so do not sort it beforehand.
 *
 * return reallocated ft_map.
 * if reallocation fails, return NULL and leave original map untouched.
 */
ft_map * ff_map_complement(ft_map * map, ft_off device_length) {
	ft_off physical_start, physical_end, last = device_length;
	ft_size i, j, n = map->fc_size;
	ft_extent * extents;

	if (map->fc_capacity == n)
		/* ensure we have enough capacity for complement _before_ sorting
		 * n + 1 is always enough */
		if ((map = ff_map_realloc(map, n + 1)) == NULL)
			return map;

	ff_map_sort_by_physical(map);

	/* loop backward: easier to code */
	extents = map->fc_extents;
	for (i = j = n; i != 0; i--) {
		// assert(j >= i);
		physical_start = extents[i-1].fc_physical;
		physical_end = physical_start + extents[i-1].fc_length;

		if (physical_end < last) {
			extents[j].fc_logical = 0;
			extents[j].fc_physical = physical_end;
			extents[j].fc_length = last - physical_end;
			j--;
		}
		last = physical_start;
	}
	// assert(j >= i);
	if (last != 0) {
		extents[j].fc_logical = 0;
		extents[j].fc_physical = 0;
		extents[j].fc_length = last;
	} else
		j++;

	if (j <= n) {
		map->fc_size = n - j + 1;
		map->fc_capacity -= j;
		map->fc_extents += j; /* this trick is the reason why map->fc_extents is a pointer and not an array */
	} else
		// no free extents!
		map->fc_size = 0;

	return map;
}




