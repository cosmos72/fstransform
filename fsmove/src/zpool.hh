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
 * zmem.hh
 *
 *  Created on: Jan 22, 2017
 *      Author: max
 */

#ifndef FSTRANSFORM_ZPOOL_HH
#define FSTRANSFORM_ZPOOL_HH

#include "zmem.hh"

#include <vector>

FT_NAMESPACE_BEGIN

typedef ft_size zpool_handle;


class zpool {
private:
    std::vector<zmem> pool;
    zpool_handle next_handle;
    
    void update_next_handle();

public:
    zpool();
    ~zpool();
    
    zpool_handle alloc(ft_size size);
    void free(zpool_handle handle);

    /* return address of uncompressed page */
    inline void * uncompress(zpool_handle handle)
    {
        if (handle >= pool.size())
            return NULL;
        return pool[handle].uncompress();
    }
    
    inline bool compress(zpool_handle handle)
    {
        if (handle >= pool.size())
            return false;
        return pool[handle].compress();
    }
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_ZPOOL_HH */