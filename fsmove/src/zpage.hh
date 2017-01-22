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

#include "types.hh"     // for ft_size

#include <vector>

FT_NAMESPACE_BEGIN

enum { Z_UNCOMPRESSED_PAGE_SIZE = 65536 };

typedef ft_size page_handle;

class zpage
{
public:
    void * address;
    ft_size size; /* if zero, page is uncompressed */
    
    inline zpage() : address(NULL), size(0)
    {
    }
    
    inline zpage(void * addr, ft_size sz) : address(addr), size(sz)
    {
    }
    
    inline void clear()
    {
        address = NULL;
        size = 0;
    }
};

class zpage_pool {
private:
    std::vector<zpage> pages;
    page_handle next_page_handle;
    
    void * new_uncompressed_page();
    
    bool del_page(page_handle handle) const;
    static bool del_uncompressed_page(void * address);
    static bool del_compressed_page(void * address);
    
    inline page_handle insert_new_page(void * address, size_t size)
    {
        insert_page(next_page_handle, address, size);
        return next_page_handle;
    }
    
    void insert_page(page_handle handle, void * address, size_t size);
    
    void update_next_page_handle();

    void * do_uncompress_page(page_handle handle);
    void do_compress_page(page_handle handle);
        
public:
    zpage_pool();
    ~zpage_pool();
    
    page_handle alloc_page();
    void free_page(page_handle handle);
    
    /* return address of uncompressed page */
    inline void * uncompress_page(page_handle handle)
    {
        if (handle >= pages.size())
            return NULL;
        const zpage & page = pages[handle];
        if (page.address != NULL && page.size != 0)
            return do_uncompress_page(handle);
        return page.address; /* already uncompressed (or does not exist) */
    }
    
    inline void compress_page(page_handle handle)
    {
        if (handle >= pages.size())
            return;
        const zpage & page = pages[handle];
        if (page.address != NULL && page.size == 0)
            do_compress_page(handle);
    }
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_ZPAGE_HH */