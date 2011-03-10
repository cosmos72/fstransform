/*
 * vector.cc
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "first.hh"      // for FT_*TEMPLATE* macros */

#ifdef FT_HAVE_EXTERN_TEMPLATE
#  include "vector.t.hh"
   FT_TEMPLATE_INSTANTIATE(FT_TEMPLATE_vector_hh)
#else
#endif /* FT_HAVE_EXTERN_TEMPLATE */
