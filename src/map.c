/*
 * map.c
 *
 *  Created on: Feb 22, 2011
 *      Author: max
 */

#include "first.h"

#include <errno.h>         /* for errno, ENOMEM, EINVAL, EFBIG */
#include <stdlib.h>        /* for malloc(), free() */
#include <string.h>        /* for memset() */

#include "map.h"           /* for ft_extent, ft_map, ff_map_*() */


/**
 * allocate and initialize ft_map with default capacity.
 * return 0 if success, else error
 */
ft_map * ff_map_alloc(ft_size capacity)
{
    ft_size bytes_n = sizeof(ft_map) + capacity * sizeof(ft_extent);
	ft_map * map;
    
    if ((map = (ft_map *) malloc(bytes_n)) != NULL) {
    	map->fc_size = 0;
    	map->fc_capacity = capacity;
    	map->fc_extents = (ft_extent *)(void *)(map + 1);
    }
    return map;
}

/**
 * grow (or shrink) ft_map to make room for more extents.
 * if reallocation fails, return NULL and leave original map untouched.
 */
ft_map * ff_map_realloc(ft_map * map, ft_size new_capacity)
{
	if (map != NULL) {
		ft_size new_bytes_n = sizeof(ft_map) + new_capacity * sizeof(ft_extent);
		ft_map * new_map;
		if ((new_map = (ft_map *) realloc(map, new_bytes_n)) == NULL)
			return new_map;

		if (new_map->fc_size > new_capacity)
			new_map->fc_size = new_capacity;
		new_map->fc_capacity = new_capacity;
		new_map->fc_extents = (ft_extent *)(void *)(map + 1);

		return new_map;
	}
	return ff_map_alloc(new_capacity);
}

/**
 * free ft_map
 */
void ff_map_free(ft_map * map) {
	free(map);
}

/**
 * duplicate a ft_map, i.e. allocate and return a copy
 */
ft_map * ff_map_dup(const ft_map * map) {
	size_t n = map->fc_size;
	ft_map * new_map;
	if ((new_map = ff_map_alloc(n)) != NULL) {
		memcpy(new_map, map, sizeof(ft_map) + n * sizeof(ft_extent));
		new_map->fc_capacity = n;
		new_map->fc_extents = (ft_extent *)(void *)(new_map + 1);
	}
	return new_map;
}

/**
 * add an extent to the ft_map,
 * merging with existing extents if possible,
 * and reallocating ft_map if necessary.
 * if reallocation fails, return NULL and leave original map untouched.
 */
ft_map * ff_map_append(ft_map * map, ft_off logical, ft_off physical, ft_off length)
{
    ft_extent * extent;
    ft_size map_size;
    ft_map * new_map;

    do {
        map_size = map->fc_size;
        
        if (map_size != 0) {
            /* check if block to add is contiguous to last one in the map */
            extent = & map->fc_extents[map_size - 1];
            if (extent->fc_logical + extent->fc_length == logical
                && extent->fc_physical + extent->fc_length == physical)
            {
                extent->fc_length += length;
                break; /* done. */
            }
        }
    
        /* add a new extent */
        if (map_size == map->fc_capacity) {
            /* we must grow ft_map first */
            if ((new_map = ff_map_realloc(map, 2 * map_size)) != NULL)
            	map = new_map;
            else
                break;
        }
        extent = & map->fc_extents[map_size];
        extent->fc_logical = logical;
        extent->fc_physical = physical;
        extent->fc_length = length;

        map->fc_size++;
    
    } while (0);
    return map;
}

