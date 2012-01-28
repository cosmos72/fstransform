/*
 * arch/mem_posix.cc
 *
 *  Created on: Mar 11, 2011
 *      Author: max
 */
#include "../first.hh"

#ifdef __unix__

#ifdef FT_HAVE_UNISTD_H
# include <unistd.h>  // for sysconf(), _SC_PAGESIZE
#endif

#include "mem_posix.hh"


FT_ARCH_NAMESPACE_BEGIN

/**
 * return RAM page size, or 0 if cannot be determined
 */
ft_size ff_arch_posix_mem_page_size() {
#if defined(_SC_PAGESIZE)
    long n = sysconf(_SC_PAGESIZE);
    if (n > 0)
        return (ft_size) n;
#endif
#if defined(PAGE_SIZE)
    return PAGE_SIZE;
#elif defined(PAGESIZE)
    return PAGESIZE;
#else
    return 0;
#endif
}

FT_ARCH_NAMESPACE_END


#endif /* __unix__ */

