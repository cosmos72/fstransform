/*
 * fiemap.h
 *
 *  Created on: Feb 14, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_MAP_H
#define FSTRANSLATE_MAP_H

#include "types.h" /* for ft_off */

typedef struct {
    ft_off fc_logical;   /* logical offset in bytes for the start of the extent from the beginning of the file */
    ft_off fc_physical;  /* physical offset in bytes for the start of the extent from the beginning of the device */
    ft_off fc_length;    /* length in bytes for this extent */
} ft_extent;


typedef struct {
    ft_size     fc_size;        /* size of fc_extents array */
    ft_size     fc_capacity;    /* capacity of fc_extents array */
    ft_extent * fc_extents;     /* array of mapped extents */
} ft_map;

enum { ft_map_default_capacity = (65536 - sizeof(ft_map)) / sizeof(ft_extent) };


/**
 * allocate and initialize ft_map with default capacity.
 * return 0 if success, else error
 */
ft_map * ff_map_alloc(ft_size capacity);

/**
 * grow (or shrink) ft_map to make room for more extents
 * and return reallocated ft_map.
 * if reallocation fails, return NULL and leave original map untouched.
 */
ft_map * ff_map_realloc(ft_map * map, ft_size new_capacity);

/**
 * free ft_map
 */
void ff_map_free(ft_map * map);

/**
 * duplicate a ft_map, i.e. allocate and return a copy
 */
ft_map * ff_map_dup(const ft_map * map);

/**
 * add an extent to the ft_map,
 * merging with existing extents if possible,
 * and reallocating ft_map if necessary.
 * if reallocation fails, return NULL and leave original map untouched.
 */
ft_map * ff_map_append(ft_map * map, ft_off logical, ft_off physical, ft_off length);


#endif /* FSTRANSLATE_MAP_H */
