/*
 * first.hh
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_FIRST_HH
#define FSTRANSLATE_FIRST_HH

#if defined(__USE_ANSI) || defined(_FEATURES_H) || defined(FSTRANSLATE_AUTOCONF_HH)
#  error "first.hh" must be included before any other #include
#endif

/* put here any option/define/... affecting system-wide includes */

/*
 * _GNU_SOURCE implies _LARGEFILE_SOURCE,
 * i.e. off_t will be at least 64 bits wide
 */
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#ifndef _LARGEFILE_SOURCE
#  define _LARGEFILE_SOURCE
#endif


/* put here any option/define/... affecting the whole program (headers and source files) */

#include "features.hh"



#endif /* FSTRANSLATE_FIRST_HH */
