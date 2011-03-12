/*
 * arch/mem_posix.hh
 *
 *  Created on: Mar 11, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_ARCH_mem_posix_HH
#define FSTRANSFORM_ARCH_mem_posix_HH

#include "../types.hh"

FT_ARCH_NAMESPACE_BEGIN

/**
 * return RAM page size, or 0 if cannot be determined
 */
ft_size ff_arch_posix_mem_page_size();

FT_ARCH_NAMESPACE_END

#endif /* FSTRANSFORM_ARCH_mem_posix_HH */
