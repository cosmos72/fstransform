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
 * zpool.cc
 *
 *  Created on: Jan 22, 2017
 *      Author: max
 */

#include "first.hh"

#include "log.hh"
#include "zpool.hh"

FT_NAMESPACE_BEGIN


zpool_base::zpool_base() : next_handle(1)
        /* reserve page 0 as "invalid" - we want zpage_handle = 0 to be the invalid pointer to compressed memory */
{ }

/* deallocate all pages */
zpool_base::~zpool_base()
{
    for (ft_size i = 0, n = pool.size(); i < n; i++)
    {
        pool[i].free_page();
    }
}

/* deallocate one page - can be both compressed or decompressed. updates pool[] and next_handle */
bool zpool_base::free_page(zpage_handle handle)
{
    if (handle >= pool.size())
        return false;

    bool success = pool[handle].free_page();
    
    if (success && next_handle > handle)
        next_handle = handle;
    return success;
}


/* allocate one page */
zpage_handle zpool_base::alloc_page(ft_size size)
{
    zpage_handle handle = next_handle;
    if (pool.size() <= handle)
        pool.resize(handle + 1);

    if (pool[handle].alloc_page(size))
        update_next_handle();
    else
        handle = 0;
    
    return handle;
}

void zpool_base::update_next_handle()
{
    ft_size i = next_handle + 1, n = pool.size();
    for (; i < n; i++)
    {
        if (pool[i].address == NULL)
            break;
    }
    next_handle = i;
}


/************* zpool ************/

zpool::zpool() 
{ }

zpool::~zpool()
{ }

zpage_handle zpool::alloc_init_page(ft_size chunk_size)
{
    zpage_handle handle = find_page4(chunk_size);
    if (handle != 0)
        return handle;
    
    handle = next_handle;
    if (pool.size() <= handle)
        pool.resize(handle + 1);

    if (pool[handle].alloc_init_page(chunk_size)) {
        update_next_handle();
        avail_pool[chunk_size] = handle;
    } else
        handle = 0;
    
    return handle;
}

static inline ft_size round_up_chunk_size(ft_size size) {
    if (size <= 16) {
        if (size <= 1)
            size = 1;
        else if (size <= 8)
            size = (size + 1) & ~1;
        else
            size = (size + 3) & ~3;
    } else if (size <= 256) {
        if (size <= 64) {
            if (size <= 32)
                size = (size + 7) & ~7;
            else
                size = (size + 15) & ~15;
        }
        else if (size <= 128)
            size = (size + 31) & ~31;
        else
            size = (size + 63) & ~63;
    }
    else if (size <= 1024) {
        if (size <= 512)
            size = (size + 127) & ~127;
        else
            size = (size + 255) & ~255;
    }
    else if (size <= 4096) {
        if (size <= 2048)
            size = (size + 511) & ~511;
        else
            size = (size + 1023) & ~1023;
    }
    else
        size = (size + 2047) & ~2047;
}

zptr_handle zpool::alloc_ptr(ft_size size)
{
    ft_size chunk_size = round_up_chunk_size(size);
    
    zptr_handle ptr_handle = 0;
    zpage_handle page_handle;
    for (ft_size i = 0; i < 2; i++)
    {
        page_handle = alloc_init_page(chunk_size); 
        if (page_handle == 0)
            break;
        ptr_handle = pool[page_handle].alloc_ptr(page_handle);
        if (ptr_handle != 0)
            break;
        /* page is full */
        avail_pool.erase(chunk_size);
    }
    return ptr_handle;
}

FT_NAMESPACE_END
