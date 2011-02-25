/*
 * fiemap.h
 *
 *  Created on: Feb 14, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_TRANSLATE_H
#define FSTRANSLATE_TRANSLATE_H

#include "map.h" /* for ft_map */

/**
 * reorder a ft_map in-place, sorting by fc_physical
 */
void ff_map_sort_by_physical(ft_map * map);


/**
 * makes the complement of a ft_map in-place,
 * i.e. calculates the device extents NOT used by the file described by ft_map
 *
 * since the file(s) contained in such extents are not known,
 * all returned extents will have fc_logical == 0.
 *
 * internally calls ff_map_sort_by_physical() on the map,
 * so do not sort it beforehand.
 */
ft_map * ff_map_complement(ft_map * map, ft_off device_size);


#endif /* FSTRANSLATE_TRANSLATE_H */
