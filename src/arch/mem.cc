/*
 * arch/mem.cc
 *
 *  Created on: Mar 10, 2011
 *      Author: max
 */
#include "../first.hh"

#include "mem.hh"    // for ff_arch_free_system_memory()
#include "mem_linux.hh"


FT_ARCH_NAMESPACE_BEGIN

/**
 * return an approximation of free system memory in bytes,
 * or 0 if cannot be determined
 */
ft_uoff ff_arch_free_system_memory() {
#if defined(__linux__)
    return ff_arch_linux_free_system_memory();
#else
    return 0;
#endif
}

FT_ARCH_NAMESPACE_END
