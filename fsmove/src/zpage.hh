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
 * zpage.hh
 *
 *  Created on: Jan 22, 2017
 *      Author: max
 */

#ifndef FSTRANSFORM_ZPAGE_HH
#define FSTRANSFORM_ZPAGE_HH

#include "zmem.hh"

FT_NAMESPACE_BEGIN

struct zpage_content
{
    uint8_t next, count;
    uint16_t chunk_size;
    uint8_t bitmap[12];
};

class zpage : private zmem
{
private:
    friend class zpool_base;

public:
    enum { PTR_BITS = CHAR_BIT, PTR_MASK = UCHAR_MAX, MAX_ALIGN = 16, MAX_N = 12 * PTR_BITS };

    using zmem::compress_page;
    using zmem::decompress_page;
    using zmem::compressed;

    /* allocate and initialize internal data required for alloc_ptr() and free_ptr() */
    bool alloc_init_page(ft_size chunk_size);
    
    zptr_handle alloc_ptr(zpage_handle page_handle);
    bool free_ptr(zptr_handle handle);

    inline bool compress_ptr(zptr_handle handle)
    {
        return compress_page();
    }

    inline void * decompress_ptr(zptr_handle handle)
    {
        void * addr = decompress_page();
        if (addr == NULL)
            return addr;
        zpage_content * content = reinterpret_cast<zpage_content *>(addr);
        uint8_t i = (handle & PTR_MASK), n = content->count;
        if (i >= n)
            return NULL;
        return reinterpret_cast<char *>(addr) + MAX_ALIGN + i * content->chunk_size;
    }
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_ZPAGE_HH */