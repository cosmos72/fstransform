/*
 * arch/mem.hh
 *
 *  Created on: Mar 10, 2011
 *      Author: max
 */

#ifndef FSREMAP_ARCH_MEM_HH
#define FSREMAP_ARCH_MEM_HH

#include "../types.hh"

FT_ARCH_NAMESPACE_BEGIN

/**
 * return an approximation of free system memory in bytes,
 * or 0 if cannot be determined
 */
ft_uoff ff_arch_mem_system_free();

/**
 * return RAM page size, or 0 if cannot be determined
 */
ft_size ff_arch_mem_page_size();

FT_ARCH_NAMESPACE_END

#endif /* FSREMAP_ARCH_MEM_HH */
