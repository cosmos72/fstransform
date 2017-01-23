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


zpool::zpool() : next_handle(1)
        /* reserve page 0 as "invalid" - we want zmem(handle = 0) to be the invalid pointer to compressed memory */
{ }

/* deallocate all pages */
zpool::~zpool()
{
    for (ft_size i = 0, n = pool.size(); i < n; i++)
    {
        pool[i].free();
    }
}

/* deallocate one page - can be both compressed or decompressed. updates pool[] and next_handle */
void zpool::free(zpool_handle handle)
{
    if (handle >= pool.size())
        return;

    pool[handle].free();

    if (next_handle > handle)
        next_handle = handle;
}


/* allocate one page */
zpool_handle zpool::alloc(ft_size size)
{
    zpool_handle handle = next_handle;
    if (pool.size() <= handle)
        pool.resize(handle + 1);

    if (pool[handle].alloc(size))
        update_next_handle();
    else
        handle = 0;
    
    return handle;
}

void zpool::update_next_handle()
{
    ft_size i = next_handle + 1, n = pool.size();
    for (; i < n; i++)
    {
        if (pool[i].address == NULL)
            break;
    }
    next_handle = i;
}

FT_NAMESPACE_END
