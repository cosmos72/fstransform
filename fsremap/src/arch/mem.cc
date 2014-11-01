/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 * 
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * arch/mem.cc
 *
 *  Created on: Mar 10, 2011
 *      Author: max
 */
#include "../first.hh"

#include "mem.hh"        // for ff_arch_mem_system_free(), ff_arch_mem_page_size()
#include "mem_posix.hh"  // for ff_arch_posix_mem_page_size()
#include "mem_linux.hh"  // for ff_arch_linux_mem_system_free()


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
