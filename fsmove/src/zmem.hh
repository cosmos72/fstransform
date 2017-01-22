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

#ifndef FSTRANSFORM_ZMEM_HH
#define FSTRANSFORM_ZMEM_HH

#include "types.hh"     // for ft_size

FT_NAMESPACE_BEGIN

typedef ft_size zhandle;

zhandle zmem_alloc(ft_size size);
void zmem_free(zhandle handle, ft_size size);

void zmem_compress(zhandle handle);
void * zmem_decompress(zhandle handle);

template<class T>
    class zmem
{
private:
    zhandle handle;
    
public:
    explicit inline zmem(zhandle new_handle = 0)
        : handle(new_handle)
    { }
    
    inline T * operator->()
    {
        return reinterpret_cast<T *>(zmem_decompress(handle));
    }

    inline const T * operator->() const
    {
        return reinterpret_cast<const T *>(zmem_decompress(handle));
    }
    
    static inline zmem<T> alloc(ft_size size)
    {
        return zmem<T>(zmem_alloc(size));
    }

    inline void free(ft_size size)
    {
        return zmem_free(handle, size);
    }
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_ZMEM_HH */
