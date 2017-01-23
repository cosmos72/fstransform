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
 * zptr.hh
 *
 *  Created on: Jan 22, 2017
 *      Author: max
 */

#ifndef FSTRANSFORM_ZPTR_HH
#define FSTRANSFORM_ZPTR_HH

#include "types.hh"     // for ft_size

FT_NAMESPACE_BEGIN

typedef ft_size zptr_handle;

zptr_handle zptr_alloc(ft_size size);
void zptr_free(zptr_handle handle, ft_size size);

void zptr_compress(zptr_handle handle);
void * zptr_decompress(zptr_handle handle);

template<class T>
    class zptr
{
private:
    zptr_handle handle;
    
public:
    explicit inline zptr(zptr_handle new_handle = 0)
        : handle(new_handle)
    { }
    
    inline T * get()
    {
        return reinterpret_cast<T *>(zptr_decompress(handle));
    }
    
    inline const T * get() const
    {
        return reinterpret_cast<const T *>(zptr_decompress(handle));
    }
    
    inline T * operator->()
    {
        return get();
    }

    inline const T * operator->() const
    {
        return get();
    }
    
    static inline zptr<T> alloc(ft_size size)
    {
        return zptr<T>(zptr_alloc(size));
    }

    inline void free(ft_size size)
    {
        return zptr_free(handle, size);
    }
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_ZPTR_HH */
