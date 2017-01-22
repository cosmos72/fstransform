/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2017 Massimiliano Ghilardi
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
 * zmem.hh
 *
 *  Created on: Jan 22, 2017
 *      Author: max
 */

#include "first.hh"

#include "zmem.hh"

#if defined(FT_HAVE_SYS_MMAN_H)
# include <sys/mman.h>      /* for mmap(), munmap() */
#endif

#include <vector>

FT_NAMESPACE_BEGIN

enum { Z_UNCOMPRESSED_PAGE_SIZE = 65536 };

class zpage
{
public:
    void * address;
    ft_size size;
    
    inline zpage() : address(NULL), size(0)
    {
    }
    
    inline zpage(void * addr, ft_size sz) : address(addr), size(sz)
    {
    }
};

class zpage_pool {
private:
    std::vector<void *> uncompressed_pages;
    ft_size next_uncompressed_page;
    
    std::vector<zpage> compressed_pages;
    
public:
    zpage_pool();
    ~zpage_pool();
};



zhandle zmem_alloc(ft_size size);
void zmem_free(zhandle handle, ft_size size);

void zmem_deflate(zhandle handle);
void * zmem_inflate(zhandle handle);

FT_NAMESPACE_END


