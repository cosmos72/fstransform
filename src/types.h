/*
 * types.h
 *
 *  Created on: Feb 14, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_TYPES_H
#define FSTRANSLATE_TYPES_H

#include "check.h"

#include <stddef.h>    /* for size_t */
#include <sys/types.h> /* for off_t */
#include <sys/stat.h>  /* for struct stat */
#include <unistd.h>    /*  "    "     "   */

typedef size_t ft_size; /* type used to represent number of items in memory */
typedef off_t  ft_off; /* type used to represent length of a file */
typedef dev_t  ft_dev;

typedef struct stat ft_stat;

#endif /* FSTRANSLATE_TYPES_H */
