/*
 * arch/mem_linux.hh
 *
 *  Created on: Mar 10, 2011
 *      Author: max
 */

#ifndef FSREMAP_ARCH_MEM_LINUX_HH
#define FSREMAP_ARCH_MEM_LINUX_HH

#include "../types.hh"

FT_ARCH_NAMESPACE_BEGIN

/**
 * return an approximation of free system memory in bytes,
 * or 0 if cannot be determined
 */
ft_uoff ff_arch_linux_mem_system_free();

FT_ARCH_NAMESPACE_END

#endif /* FSREMAP_ARCH_MEM_LINUX_HH */
