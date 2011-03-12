/*
 * arch/mem.cc
 *
 *  Created on: Mar 10, 2011
 *      Author: max
 */
#include "../first.hh"

#include "mem.hh"    // for ff_arch_mem_system_free()
#include "mem_posix.hh"
#include "mem_linux.hh"


FT_ARCH_NAMESPACE_BEGIN

/**
 * return an approximation of free system memory in bytes,
 * or 0 if cannot be determined
 */
ft_uoff ff_arch_mem_system_free() {
#if defined(__linux__)
    return ff_arch_linux_mem_system_free();
#else
    return 0;
#endif
}

/**
 * return RAM page size, or 0 if cannot be determined
 */
ft_size ff_arch_mem_page_size() {
#if defined(__unix__)
	return ff_arch_posix_mem_page_size();
#else
    return 0;
#endif
}

FT_ARCH_NAMESPACE_END
