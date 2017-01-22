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
 * move.cc
 *
 *  Created on: Jan 22, 2017
 *      Author: max
 */

#include "first.hh"

#include "log.hh"
#include "zpage.hh"

#include <zlib.h>

#if defined(FT_HAVE_SYS_MMAN_H)
# include <sys/mman.h>     // for mmap(), munmap()
#endif

#if defined(FT_HAVE_STDLIB_H)
# include <stdlib.h>       // for malloc(), realloc(), free()
#elif defined(FT_HAVE_CSTDLIB)
# include <cstdlib>        // for malloc(), realloc(), free()
#endif

FT_NAMESPACE_BEGIN

zpage_pool::zpage_pool() : next_page_handle(1)
        /* reserve page 0 as "invalid" - we want zmem(handle = 0) to be the invalid pointer to compressed memory */
{ }

/* deallocate all pages */
zpage_pool::~zpage_pool()
{
    for (ft_size i = 0, n = pages.size(); i < n; i++)
    {
        del_page(i);
    }
}

/* deallocate one page - can be both compressed or uncompressed. updates pages[] and next_page_handle */
void zpage_pool::free_page(page_handle handle)
{
    if (del_page(handle)) {
        pages[handle].address = NULL;
        if (next_page_handle > handle)
            next_page_handle = handle;
    }
}

/* deallocate one page - can be both compressed or uncompressed. does not update next_page_handle */
bool zpage_pool::del_page(page_handle handle) const
{
    if (handle >= pages.size())
        return false;
    const zpage & page = pages[handle];
    void * address = page.address;
    if (address == NULL)
        return false;
    return page.size != 0 ? del_compressed_page(address) : del_uncompressed_page(address);
}

bool zpage_pool::del_uncompressed_page(void * address)
{
    int err = EINVAL;
    if (address != NULL) {
        err = munmap(address, Z_UNCOMPRESSED_PAGE_SIZE);
        if (err != 0)
            ff_log(FC_ERROR, errno, "munmap(0x%"FT_XLL", %"FT_ULL") failed", (ft_ull)(size_t)address, (ft_ull)Z_UNCOMPRESSED_PAGE_SIZE);
    }
    return err == 0;
}

bool zpage_pool::del_compressed_page(void * address)
{
    free(address);
    return address != NULL;
}

/* allocate one page */
page_handle zpage_pool::alloc_page()
{
    void * address = new_uncompressed_page();
    page_handle handle = insert_new_page(address, 0);
    update_next_page_handle();
    
    return handle;
}

void * zpage_pool::new_uncompressed_page()
{
    void * address = mmap(NULL, Z_UNCOMPRESSED_PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (address == MAP_FAILED)
    {
        ff_log(FC_ERROR, errno, "mmap(%"FT_ULL") failed", (ft_ull)Z_UNCOMPRESSED_PAGE_SIZE);
        address = NULL;
    }
    return address;
}


void zpage_pool::insert_page(page_handle handle, void * address, size_t size)
{
    if (pages.size() <= handle)
        pages.resize(handle + 1);
    zpage & page = pages[handle];
    page.address = address;
    page.size = size;
}

void zpage_pool::update_next_page_handle()
{
    size_t i = next_page_handle + 1, n = pages.size();
    for (; i < n; i++)
    {
        if (pages[i].address == NULL)
            break;
    }
    next_page_handle = i;
}

void zpage_pool::do_compress_page(page_handle handle)
{
    z_stream z = { }; /* zero-initialize */
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    
    int err = deflateInit(&z, 3);
    if (err != Z_OK) {
        ff_log(FC_ERROR, 0, "zlib deflateInit() failed: %s",
               z.msg != NULL ? z.msg
               : err == Z_MEM_ERROR ? "out of memory!"
               : err == Z_STREAM_ERROR ? "invalid compression level"
               : err == Z_VERSION_ERROR ? "incompatible zlib version"
               : "unknown error");
        return;
    }
    
    zpage & page = pages[handle];
    size_t size = deflateBound(&z, Z_UNCOMPRESSED_PAGE_SIZE);
    void * address = malloc(size);
    if (address == NULL) {
        ff_log(FC_ERROR, errno, "malloc(%"FT_ULL") failed", (ft_ull)size);
        err = Z_MEM_ERROR;
        goto cleanup;
    }
    z.next_in = (unsigned char *)page.address;
    z.avail_in = Z_UNCOMPRESSED_PAGE_SIZE;
    z.next_out = (unsigned char *)address;
    z.avail_out = size;
    err = deflate(&z, Z_FINISH);
    if (err != Z_STREAM_END) {
        ff_log(FC_ERROR, 0, "zlib deflate(Z_FINISH) returned %d instead of Z_STREAM_END", err);
        goto cleanup;
    }
    if (z.avail_out > 0 && z.avail_out < size) {
        size -= z.avail_out;
        void * new_address = realloc(address, size);
        if (new_address != NULL)
            address = new_address;
    }
    
cleanup:
    deflateEnd(&z);
    if (err == Z_STREAM_END && del_uncompressed_page(page.address)) {
        page.address = address;
        page.size = size;
    } else {
        free(address);
        address = NULL;
    }
}

void * zpage_pool::do_uncompress_page(page_handle handle)
{
    z_stream z = { }; /* zero-initialize */
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    
    int err = inflateInit(&z);
    if (err != Z_OK) {
        ff_log(FC_ERROR, 0, "zlib inflateInit() failed: %s",
               z.msg != NULL ? z.msg
               : err == Z_MEM_ERROR ? "out of memory!"
               : err == Z_STREAM_ERROR ? "invalid compression level"
               : err == Z_VERSION_ERROR ? "incompatible zlib version"
               : "unknown error");
        return NULL;
    }
    
    void * address = new_uncompressed_page();
    if (address == NULL) {
        inflateEnd(&z);
        return NULL;
    }
    zpage & page = pages[handle];
    size_t size = page.size;
    z.next_in = (unsigned char *)page.address;
    z.avail_in = size;
    z.next_out = (unsigned char *)address;
    z.avail_out = Z_UNCOMPRESSED_PAGE_SIZE;
    err = inflate(&z, Z_FINISH);
    if (err != Z_STREAM_END) {
        ff_log(FC_ERROR, 0, "zlib inflate(Z_FINISH) returned %d instead of Z_STREAM_END", err);
    }
    else if (z.avail_out != 0) {
        ff_log(FC_ERROR, 0, "zlib inflate(Z_FINISH) produced %"FT_ULL" bytes instead of %"FT_ULL, (ft_ull)(Z_UNCOMPRESSED_PAGE_SIZE - z.avail_out), (ft_ull)Z_UNCOMPRESSED_PAGE_SIZE);
        err = Z_DATA_ERROR;
    }
    
    inflateEnd(&z);
    if (err == Z_STREAM_END && del_compressed_page(page.address)) {
        page.address = address;
        page.size = 0;
    } else {
        del_uncompressed_page(address);
        address = NULL;
    }
    return address;
}


FT_NAMESPACE_END
