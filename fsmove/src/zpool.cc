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

zpool default_zpool;

zpool::zpool()
{ }

zpool::~zpool()
{ }

zpool::iter_type zpool::do_alloc_init_page(ft_size chunk_size)
{
    zpage_handle handle = next_handle;
    if (pool.size() <= handle)
        pool.resize(handle + 1);

    iter_type iter;
    if (pool[handle].alloc_init_page(chunk_size)) {
        update_next_handle();
        iter = avail_pool.insert(std::make_pair(chunk_size, handle));
    } else
        iter = avail_pool.end();

    return iter;
}

ft_size zpool::round_up_chunk_size(ft_size size) {
   if ((size & (size - 1)) != 0) {
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
      } else if (size <= 1024) {
	 if (size <= 512)
	   size = (size + 127) & ~127;
	 else
	   size = (size + 255) & ~255;
      } else if (size <= 4096) {
	 if (size <= 2048)
	   size = (size + 511) & ~511;
	 else
	   size = (size + 1023) & ~1023;
      } else
	size = (size + 2047) & ~2047;
   }
   else if (size == 0)
     size = 1;
   return size;
}

zptr_handle zpool::alloc_ptr(ft_size size)
{
    ft_size chunk_size = round_up_chunk_size(size);

    zptr_handle ptr_handle = 0;
    zpage_handle page_handle;

    iter_type iter = avail_pool.lower_bound(chunk_size), end = avail_pool.end();
    while (iter != end && iter->first == chunk_size)
    {
        page_handle = iter->second;
        zpage & page = pool[page_handle];
        ptr_handle = page.alloc_ptr(page_handle);

        if (ptr_handle == 0 || page.is_full_page()) {
            /* page is full... try another one. */
            /* C++11 allows the elegant "iter = avail_pool.erase(iter)" */
            avail_pool.erase(iter++);
        }

        if (ptr_handle != 0)
            return ptr_handle;
    }

    iter = do_alloc_init_page(chunk_size);
    if (iter != end) {
        page_handle = iter->second;
        zpage & page = pool[page_handle];
        ptr_handle = page.alloc_ptr(page_handle);
        if (ptr_handle == 0 || page.is_full_page()) {
            avail_pool.erase(iter);
        }
    }
    return ptr_handle;
}

bool zpool::free_ptr(zptr_handle ptr_handle)
{
    zpage_handle page_handle = ptr_handle >> zpage::PTR_BITS;
    if (page_handle >= pool.size())
        return false;

    zpage & page = pool[page_handle];
    bool was_full = page.is_full_page();
    bool success = page.free_ptr(ptr_handle);
    if (success) {
        if (page.is_empty_page()) {
            /* free this page, i.e. return it to the OS */
            ft_size chunk_size = page.get_page_chunk_size();
            if (free_page(page_handle)) {
                iter_type iter = avail_pool.lower_bound(chunk_size), end = avail_pool.end();
                for (; iter != end && iter->first == chunk_size; ++iter) {
                    if (iter->second == page_handle) {
                        avail_pool.erase(iter);
                        break;
                    }
                }
            }
        } else if (was_full) {
            /* reinsert page among available ones */
            ft_size chunk_size = page.get_page_chunk_size();
            avail_pool.insert(std::make_pair(chunk_size, page_handle));
        }
    }
    return success;
}

FT_NAMESPACE_END
