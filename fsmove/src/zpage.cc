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
 * zmem.cc
 *
 *  Created on: Jan 22, 2017
 *      Author: max
 */

#include "first.hh"

#if defined(FT_HAVE_STRING_H)
# include <string.h>       // for memset()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>        // for memset()
#endif

#include "log.hh"
#include "zpage.hh"

FT_NAMESPACE_BEGIN

/* initialize internal data required for alloc_ptr() and free_ptr() */
bool zpage::alloc_init_page(ft_size chunk_size)
{
    ft_size n;
    if (chunk_size >= 8192)
        n = 1;
    else if (chunk_size > 256)
        n = MAX_N * 256 / chunk_size;
    else
        n = MAX_N;

    ft_size new_size = MAX_ALIGN + n * chunk_size;
    if (!alloc_page(new_size))
        return false;
    
    zpage_content * content = reinterpret_cast<zpage_content *>(address);
    content->next = 0;
    content->count = n;
    content->chunk_size = n > 1 ? chunk_size : 0;
    memset(content->bitmap, '\0', sizeof(content->bitmap));
    return true;
}

bool zpage::is_full_page()
{
    if (!decompress_page())
        return false;
    
    zpage_content * content = reinterpret_cast<zpage_content *>(address);
    return content->next >= content->count;
}

bool zpage::is_empty_page()
{
    if (!decompress_page())
        return false;
    
    zpage_content * content = reinterpret_cast<zpage_content *>(address);
    if (content->next != 0)
        return false;
    
    static const uint8_t zero[sizeof(content->bitmap)] = { }; /* zero-initialize */
    return !memcmp(content->bitmap, zero, sizeof(content->bitmap));
}

ft_size zpage::get_page_chunk_size()
{
    if (!decompress_page())
        return 0;
    
    zpage_content * content = reinterpret_cast<zpage_content *>(address);
    ft_size chunk_size;
    if (content->count > 1)
        chunk_size = content->chunk_size;
    else
        chunk_size = size - MAX_ALIGN;
    return chunk_size;
}

zptr_handle zpage::alloc_ptr(zpage_handle page_handle)
{
    if (!decompress_page())
        return 0;
    
    zpage_content * content = reinterpret_cast<zpage_content *>(address);
    
    ft_size n = content->count;
    ft_size i = content->next;
    ft_size mask = (ft_size)1 << (i % PTR_BITS);
    uint8_t * bitmap = content->bitmap + (i / PTR_BITS);
    
    for (; i < n; i++, mask <<= 1)
    {
        if (mask > PTR_MASK) {
            mask = 1;
            bitmap++;
        }
        if ((bitmap[0] & mask) == 0) {
            bitmap[0] |= mask;
            content->next = i + 1;
            return (page_handle << PTR_BITS) | i;
        }
    }
    return 0;
}

bool zpage::free_ptr(zptr_handle handle)
{
    if (!decompress_page())
        return false;
    
    zpage_content * content = reinterpret_cast<zpage_content *>(address);
    uint8_t i = (handle & PTR_MASK), n = content->count;
    
    if (i >= n)
        return false;

    uint8_t * bitmap = content->bitmap + i / PTR_BITS;
    uint8_t mask = (uint8_t)1 << (i % PTR_BITS);
    if ((bitmap[0] & mask) == 0)
        return false;
    bitmap[0] &= ~mask;
    if (content->next > i)
        content->next = i;
    return true;
}

FT_NAMESPACE_END
