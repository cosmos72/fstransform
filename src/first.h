/*
 * first.h
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_FIRST_H
#define FSTRANSLATE_FIRST_H

#if defined(__USE_ANSI) || defined(_FEATURES_H)
# error "first.h" must be included before any other #include
#endif

/* put here any option affecting system-wide includes */

/*
 * _GNU_SOURCE implies _LARGEFILE_SOURCE,
 * i.e. off_t will be at least 64 bits wide
 */
#define _GNU_SOURCE


#endif /* FSTRANSLATE_FIRST_H */
