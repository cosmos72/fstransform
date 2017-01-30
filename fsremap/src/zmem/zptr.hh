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

#include "zpool.hh"

FT_NAMESPACE_BEGIN

class zptr_void
{
private:
    zptr_handle handle;

public:
    explicit inline zptr_void(zptr_handle new_handle = 0)
        : handle(new_handle)
    { }
    
    inline void * get()
    {
        return default_zpool.decompress_ptr(handle);
    }
    
    inline const void * get() const
    {
        return default_zpool.decompress_ptr(handle);
    }
    
    inline bool compress()
    {
        return default_zpool.compress_ptr(handle);
    }
    
    inline bool alloc(ft_size size)
    {
        handle = default_zpool.alloc_ptr(size);
        return handle != 0;
    }
    
    inline bool free()
    {
        return default_zpool.free_ptr(handle);
    }
};



template<class T>
    class zptr : private zptr_void
{
public:
    explicit inline zptr(zptr_handle new_handle = 0)
        : zptr_void(new_handle)
    { }
    
    inline T * get()
    {
        return reinterpret_cast<T *>(zptr_void::get());
    }
    
    inline const T * get() const
    {
        return reinterpret_cast<const T *>(zptr_void::get());
    }
    
    inline T * operator->()
    {
        return get();
    }

    inline const T * operator->() const
    {
        return get();
    }
    
    inline bool alloc(ft_size element_count)
    {
        return zptr_void::alloc(element_count * sizeof(T));
    }
    
    using zptr_void::free;
    using zptr_void::compress;
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_ZPTR_HH */
