/*
 * vector.cc
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "first.hh"      /* for FT_*EXTERN_TEMPLATE* macros */

#ifdef FT_HAVE_EXTERN_TEMPLATE
#  include "vector.template.hh"
   FT_EXTERN_TEMPLATE_INSTANTIATE(FT_EXTERN_TEMPLATE_vector_hh)
#endif /* FT_EXTERN_TEMPLATE */
