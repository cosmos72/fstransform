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
#include "zptr.hh"
#include "ztest.hh"

FT_NAMESPACE_BEGIN

enum { chunk_size = 256, size = zpage::MAX_N * chunk_size + zpage::MAX_ALIGN };

void ztest_page_handle(zpool & pool, zpage_handle h);
void ztest_ptr_handle(zpool & pool, zptr_handle zh);

void ztest()
{
    zpool pool;

    zpage_handle h = pool.alloc_page(size); /* may (or may not) be used as the backing page for zh, zh2 */
    ztest_page_handle(pool, h);
    
    zptr_handle zh = pool.alloc_ptr(chunk_size);
    zptr_handle zh2 = pool.alloc_ptr(chunk_size);
    ztest_ptr_handle(pool, zh2);
    ztest_ptr_handle(pool, zh);

    pool.free_ptr(zh2);
    pool.free_ptr(zh);
    pool.free_page(h);
}

void ztest_page_handle(zpool & pool, zpage_handle h)
{
    ft_size * address = reinterpret_cast<ft_size *>(pool.decompress_page(h));
    if (address == NULL) {
        ff_log(FC_ERROR, 0, "zpage_handle test failed! pool.decompress_page() returned NULL");
        return;
    }

    for (ft_size i = 0; i < size / sizeof(ft_size); i++)
        address[i] = (ft_size)address + i;

    if (!pool.compress_page(h)) {
        ff_log(FC_ERROR, 0, "zpage_handle test failed! pool.compress_page() returned false");
        return;
    }
    
    ft_size * new_address = reinterpret_cast<ft_size *>(pool.decompress_page(h));
    if (new_address == NULL) {
        ff_log(FC_ERROR, 0, "zpage_handle test failed! pool.decompress_page() returned NULL");
        return;
    }
        
    for (ft_size i = 0; i < size / sizeof(ft_size); i++)
    {
        if (new_address[i] != (ft_size)address + i)
            ff_log(FC_ERROR, 0, "zpage_handle test failed! wrote 0x%"FT_XLL", read 0x%"FT_XLL, (ft_ull)address + i, (ft_ull)new_address[i]);
    }
}

void ztest_ptr_handle(zpool & pool, zptr_handle zh)
{
    if (zh == 0) {
        ff_log(FC_ERROR, 0, "zptr_handle test failed! pool.alloc_ptr(%"FT_ULL") returned NULL", (ft_ull)chunk_size);
        return;
    }

    ft_size * address = reinterpret_cast<ft_size *>(pool.decompress_ptr(zh));
    if (address == NULL) {
        ff_log(FC_ERROR, 0, "zptr_handle test failed! pool.decompress_ptr() returned NULL");
        return;
    }
    
    for (ft_size i = 0; i < chunk_size / sizeof(ft_size); i++)
        address[i] = (ft_size)address + i;
    
    if (!pool.compress_ptr(zh)) {
        ff_log(FC_ERROR, 0, "zptr_handle test failed! pool.compress_ptr() returned false");
        return;
    }

    ft_size * new_address = reinterpret_cast<ft_size *>(pool.decompress_ptr(zh));
    if (new_address == NULL) {
        ff_log(FC_ERROR, 0, "zptr_handle test failed! pool.decompress_ptr() returned NULL");
        return;
    }    
    
    for (ft_size i = 0; i < chunk_size / sizeof(ft_size); i++)
    {
        if (new_address[i] != (ft_size)address + i)
            ff_log(FC_ERROR, 0, "zptr_handle test failed! wrote 0x%"FT_XLL", read 0x%"FT_XLL, (ft_ull)address + i, (ft_ull)new_address[i]);
    }
}

void ztest_ptr()
{
    enum { N = chunk_size };
    
    zptr<ft_size> p;
    if (!p.alloc(N)) {
        ff_log(FC_ERROR, 0, "zptr test failed! zptr.alloc() returned false");
        return;
    }
    
    ft_size * address = p.get();
    if (address == NULL) {
        ff_log(FC_ERROR, 0, "zptr test failed! zptr.get() returned NULL");
        return;
    }
    
    for (ft_size i = 0; i < N; i++)
        address[i] = (ft_size)address + i;
    
    if (!p.compress()) {
        ff_log(FC_ERROR, 0, "zptr test failed! zptr.compress() returned false");
        return;
    }
    
    ft_size * new_address = p.get();
    if (new_address == NULL) {
        ff_log(FC_ERROR, 0, "zptr test failed! zptr.get() returned NULL");
        return;
    }    
    
    for (ft_size i = 0; i < N; i++)
    {
        if (new_address[i] != (ft_size)address + i)
            ff_log(FC_ERROR, 0, "zptr test failed! wrote 0x%"FT_XLL", read 0x%"FT_XLL, (ft_ull)address + i, (ft_ull)new_address[i]);
    }
    
    if (!p.free()) {
        ff_log(FC_ERROR, 0, "zptr test failed! zptr.free() returned false"); 
        return;
    }
}

FT_NAMESPACE_END
