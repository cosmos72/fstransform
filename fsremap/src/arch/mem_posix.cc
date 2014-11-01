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

    // first attempt: sysconf(_SC_PAGESIZE)
#if defined(FT_HAVE_SYSCONF) && defined(_SC_PAGESIZE)
    long n = sysconf(_SC_PAGESIZE);
    if (n > 0 && n == (long)(ft_size) n)
        return (ft_size) n;
#endif

    // second attempt: getpagesize()
#if defined(FT_HAVE_GETPAGESIZE)
    int n2 = getpagesize();
    if (n2 > 0 && n2 == (int)(ft_size) n2)
        return (ft_size) n2;
#endif
    
    // third attempt: PAGE_SIZE and PAGESIZE
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

