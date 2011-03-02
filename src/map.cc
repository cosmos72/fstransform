/*
 * map.cc
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "first.hh"      /* for FT_*EXTERN_TEMPLATE* macros */

#ifdef FT_HAVE_EXTERN_TEMPLATE
#  include "map.template.hh"
   FT_EXTERN_TEMPLATE_INSTANTIATE(FT_EXTERN_TEMPLATE_map_hh)
#endif /* FT_EXTERN_TEMPLATE */
