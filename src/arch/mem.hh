/*
 * arch/mem.hh
 *
 *  Created on: Mar 10, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_ARCH_MEM_HH
#define FSTRANSFORM_ARCH_MEM_HH

#include "../types.hh"

FT_ARCH_NAMESPACE_BEGIN

/**
 * return an approximation of free system memory in bytes,
 * or 0 if cannot be determined
 */
ft_uoff ff_arch_free_system_memory();

FT_ARCH_NAMESPACE_END

#endif /* FSTRANSFORM_ARCH_MEM_HH */
