/*
 * map.cc
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "first.hh"      // for FT_*TEMPLATE* macros */

#ifdef FT_HAVE_EXTERN_TEMPLATE
#  include "map.t.hh"
   FT_TEMPLATE_INSTANTIATE(FT_TEMPLATE_map_hh)
#endif /* FT_HAVE_EXTERN_TEMPLATE */
