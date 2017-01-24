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
 * ztest.cc
 *
 *  Created on: Jan 23, 2017
 *      Author: max
 */

#include "first.hh"

#include "log.hh"
#include "zpool.hh"

FT_NAMESPACE_BEGIN

void ztest()
{
    zpool pool;
    enum { size = 65536, chunk_size = 256 };
    zpage_handle h = pool.alloc_page(size);
    ft_size * address = reinterpret_cast<ft_size *>(pool.decompress_page(h));
    if (!address) {
        ff_log(FC_ERROR, 0, "compressed memory test failed! pool.decompress_page() returned NULL");
        return;
    }

    for (ft_size i = 0; i < size / sizeof(ft_size); i++)
    {
        address[i] = (ft_size)address + i;
    }
    if (!pool.compress_page(h)) {
        ff_log(FC_ERROR, 0, "compressed memory test failed! pool.compress_page() returned false");
        return;
    }
    
    ft_size * new_address = reinterpret_cast<ft_size *>(pool.decompress_page(h));
    if (!new_address) {
        ff_log(FC_ERROR, 0, "compressed memory test failed! pool.decompress_page() returned NULL");
        return;
    }
        
    for (ft_size i = 0; i < size / sizeof(ft_size); i++)
    {
        if (new_address[i] != (ft_size)address + i)
            ff_log(FC_ERROR, 0, "compressed memory test failed! wrote 0x%"FT_XLL", read 0x%"FT_XLL, (ft_ull)address + i, (ft_ull)new_address[i]);
    }
    pool.free_page(h);
    
    h = pool.alloc_init_page(chunk_size);
    if (h == 0) {
        ff_log(FC_ERROR, 0, "compressed memory test failed! pool.alloc_init_page(%"FT_ULL") returned NULL", (ft_ull)chunk_size);
        return;
    }
    
    zptr_handle zh = pool.alloc_ptr(chunk_size);
    if (zh == 0) {
        ff_log(FC_ERROR, 0, "compressed memory test failed! pool.alloc_ptr(%"FT_ULL") returned NULL", (ft_ull)chunk_size);
        return;
    }

    new_address = reinterpret_cast<ft_size *>(pool.decompress_ptr(zh));
    if (!new_address) {
        ff_log(FC_ERROR, 0, "compressed memory test failed! pool.decompress_ptr() returned NULL");
        return;
    }
}

FT_NAMESPACE_END
