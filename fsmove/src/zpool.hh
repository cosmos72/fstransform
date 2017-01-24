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
 * zpool.hh
 *
 *  Created on: Jan 22, 2017
 *      Author: max
 */

#ifndef FSTRANSFORM_ZPOOL_HH
#define FSTRANSFORM_ZPOOL_HH

#include "zpage.hh"

#include <vector>
#include <map>

FT_NAMESPACE_BEGIN

class zpool_base {
protected:
    std::vector<zpage> pool;
    zpage_handle next_handle;
    
    void update_next_handle();

public:
    zpool_base();
    ~zpool_base();

    zpage_handle alloc_page(ft_size size);
    bool free_page(zpage_handle handle);

    /* return address of decompressed page */
    inline void * decompress_page(zpage_handle page_handle)
    {
        if (page_handle >= pool.size())
            return NULL;
        return pool[page_handle].decompress_page();
    }
    
    inline bool compress_page(zpage_handle page_handle)
    {
        if (page_handle >= pool.size())
            return false;
        return pool[page_handle].compress_page();
    }
};

class zpool : public zpool_base {
private:
    std::map<ft_size, zpage_handle> avail_pool;

    inline zpage_handle find_page4(ft_size chunk_size)
    {
        std::map<ft_size, zpage_handle>::iterator iter = avail_pool.find(chunk_size);
        return iter != avail_pool.end() ? iter->second : 0;
    }
    
public:
    zpool();
    ~zpool();

    zpage_handle alloc_init_page(ft_size chunk_size);
    
    inline void * decompress_ptr(zptr_handle ptr_handle)
    {
        zpage_handle page_handle = ptr_handle >> zpage::PTR_BITS;
        if (page_handle >= pool.size())
            return NULL;
        return pool[page_handle].decompress_ptr(ptr_handle);
    }
    
    inline bool compress_ptr(zptr_handle ptr_handle)
    {
        zpage_handle page_handle = ptr_handle >> zpage::PTR_BITS;
        if (page_handle >= pool.size())
            return false;
        return pool[page_handle].compress_ptr(ptr_handle);
    }

    zptr_handle alloc_ptr(ft_size size);
    
    inline bool free_ptr(zptr_handle ptr_handle)
    {
        zpage_handle page_handle = ptr_handle >> zpage::PTR_BITS;
        if (page_handle >= pool.size())
            return false;
        return pool[page_handle].free_ptr(ptr_handle);
    }
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_ZPOOL_HH */