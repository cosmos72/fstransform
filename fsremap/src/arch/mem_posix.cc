/*
 * arch/mem_posix.cc
 *
 *  Created on: Mar 11, 2011
 *      Author: max
 */
#include "../first.hh"

#include "mem_posix.hh"


#ifdef __unix__

#include <unistd.h>  // for sysconf(), _SC_PAGESIZE

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

