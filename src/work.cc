/*
 * work.cc
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#include "first.hh"      // for FT_*EXTERN_TEMPLATE* macros */

#ifdef FT_HAVE_EXTERN_TEMPLATE
#  include "work.template.hh"
   FT_EXTERN_TEMPLATE_INSTANTIATE(FT_EXTERN_TEMPLATE_work_hh)
#endif /* FT_EXTERN_TEMPLATE */
