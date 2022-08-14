/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 2 of the License, or
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
 * arch/mem_linux.hh
 *
 *  Created on: Mar 10, 2011
 *      Author: max
 */

#ifndef FSREMAP_ARCH_MEM_LINUX_HH
#define FSREMAP_ARCH_MEM_LINUX_HH

#include "../types.hh"

#ifdef __linux__

FT_ARCH_NAMESPACE_BEGIN

/**
 * return an approximation of free system memory in bytes,
 * or 0 if cannot be determined
 */
ft_uoff ff_arch_linux_mem_system_free();

FT_ARCH_NAMESPACE_END

#endif /* __linux__ */

#endif /* FSREMAP_ARCH_MEM_LINUX_HH */
