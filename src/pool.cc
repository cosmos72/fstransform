/*
 * pool.cc
 *
 *  Created on: Mar 8, 2011
 *      Author: max
 */

#include "first.hh"      // for FT_*TEMPLATE* macros */

#include "types.hh"      // for ft_uint, ft_uoff

#ifdef FT_HAVE_EXTERN_TEMPLATE
#  include "pool.template.hh"
   FT_TEMPLATE_INSTANTIATE(FT_TEMPLATE_pool_hh)
#endif /* FT_HAVE_EXTERN_TEMPLATE */
