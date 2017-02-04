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
 * zmem.cc
 *
 *  Created on: Jan 22, 2017
 *      Author: max
 */

#include "first.hh"
#include "../assert.hh"
#include "ztree.hh"

#if defined(FT_HAVE_STRING_H)
# include <string.h>       // for memset()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>        // for memset()
#endif

FT_NAMESPACE_BEGIN

enum { ZTREE_INNER_N = 256 };

ztree_void::ztree_void(ft_size values_inline_size) : this_values_inline_size(values_inline_size)
{
}

ztree_void::~ztree_void()
{
    clear();
}


ft_size ztree_void::key_to_depth(ft_ull key)
{
    ft_size depth = 0;
    while (key > 0xFFFFFFFFu)
        depth += 4, key >>= 32;
    if (key > 0xFFFF)
        depth += 2, key >>= 16;
    if (key > 0xFF)
        depth++;
    return depth;
}

/****************************************************************/

const void * ztree_void::get(ft_ull key) const
{
    ft_size depth = key_to_depth(key);
    ff_assert(depth < ZTREE_TOP_N);
    
    const zptr_void * ptr = &this_tree[depth];
    while (depth)
    {
        const zptr_void & ref = *ptr;
        const zptr_void * vec = reinterpret_cast<const zptr_void *>(ref.get());
        if (!vec)
            return NULL;
        ptr = vec + (key & 0xFF);
        key >>= 8;
        depth--;
    }
    return get_leaf(*ptr, key & 0xFF);
}

const void * ztree_void::get_leaf(const zptr_void & ref, ft_u8 offset) const
{
    const char * mem = reinterpret_cast<const char *>(ref.get());
    if (!mem)
        return NULL;
    
    ft_size element_size = (this_values_inline_size ? this_values_inline_size : sizeof(zptr_void));
    mem += offset * element_size;
    if (this_values_inline_size == 0) {
        const zptr_void & value_ref = * reinterpret_cast<const zptr_void *>(mem);
        mem = reinterpret_cast<const char *>(value_ref.get());
    }
    return mem;
}

/****************************************************************/

bool ztree_void::put(ft_ull key, const void * value, ft_size size)
{
    ft_size depth = key_to_depth(key);
    
    ff_assert(this_values_inline_size == 0 || size <= this_values_inline_size);
    ff_assert(depth < ZTREE_TOP_N);
    
    zptr_void * ptr = &this_tree[depth];
    while (depth)
    {
        zptr_void & ref = *ptr;
        if (ref || alloc_inner(ref))
        {
            zptr_void * vec = reinterpret_cast<zptr_void *>(ref.get());
            if (vec)
            {
                ptr = vec + (key & 0xFF);
                key >>= 8;
                depth--;
                continue;
            }
        }
        return false;
    }
    return put_leaf(*ptr, key & 0xFF, value, size);
}

bool ztree_void::put_leaf(zptr_void & ref, ft_u8 offset, const void * value, ft_size size)
{
    if (ref || alloc_leaf(ref))
    {
        char * mem = reinterpret_cast<char *>(ref.get());
        if (mem)
        {
            ft_size element_size = (this_values_inline_size ? this_values_inline_size : sizeof(zptr_void));
            mem += offset * element_size;
            if (this_values_inline_size != 0)
            {
                memcpy(mem, value, size);
                if (size < this_values_inline_size)
                    memset(mem + size, 0, this_values_inline_size - size);
                return true;
            } else {
                zptr_void & value_ref = * reinterpret_cast<zptr_void *>(mem);
                if (value_ref.realloc(size))
                {
                    void * new_value = value_ref.get();
                    if (new_value) {
                        memcpy(new_value, value, size);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/****************************************************************/

bool ztree_void::del(ft_ull key)
{
    ft_size depth = key_to_depth(key);
    ff_assert(depth < ZTREE_TOP_N);
    
    zptr_void * ptr = &this_tree[depth];
    zptr_void * stack[ZTREE_TOP_N] = { };

    while (depth)
    {
        stack[depth] = ptr;
        zptr_void & ref = *ptr;
        zptr_void * vec = reinterpret_cast<zptr_void *>(ref.get());
        if (!vec)
            return false;
        ptr = vec + (key & 0xFF);
        key >>= 8;
        depth--;
    }
    stack[depth] = ptr;
    if (del_leaf(*ptr, key & 0xFF)) {
        trim_leaf(*ptr);
        trim_inner(stack);
        return true;
    }
    return true;
}

bool ztree_void::del_leaf(zptr_void & ref, ft_u8 offset)
{
    char * mem = reinterpret_cast<char *>(ref.get());
    if (!mem)
        return false;
    
    ft_size element_size = (this_values_inline_size ? this_values_inline_size : sizeof(zptr_void));
    mem += offset * element_size;
    if (this_values_inline_size != 0) {
        memset(mem, 0, this_values_inline_size);
    } else {
        zptr_void & value_ref = * reinterpret_cast<zptr_void *>(mem);
        if (!value_ref.free())
            return false;
    }
    return true;
}

/****************************************************************/

void ztree_void::trim_inner(zptr_void * stack[ZTREE_TOP_N])
{
    for (ft_size i = 0; i < ZTREE_TOP_N; i++)
    {
        zptr_void * ptr = stack[i];
        if (!ptr || !*ptr)
            continue;
        zptr_void * vec = reinterpret_cast<zptr_void *>(ptr->get());
        if (!vec)
            break; // failed to decompress (argh)
        for (ft_size j = 0; j < ZTREE_INNER_N; j++) {
            if (vec[j])
                return;
        }
        if (!ptr->free())
            break;
    }
}

void ztree_void::trim_leaf(zptr_void & ref)
{
    ft_size * mem = reinterpret_cast<ft_size *>(ref.get());
    if (!mem)
        return;
    ft_size count = ZTREE_INNER_N  / sizeof(ft_size) * (this_values_inline_size ? this_values_inline_size : sizeof(zptr_void));
    for (ft_size i = 0; i < count; i++) {
        if (mem[i])
            return;
    }
    ref.free();
}

/****************************************************************/

bool ztree_void::alloc_inner(zptr_void & ref)
{
    if (ref.alloc(ZTREE_INNER_N * sizeof(zptr_void)))
    {
        zptr_void * vec = reinterpret_cast<zptr_void *>(ref.get());
        if (vec)
        {
            for (ft_size i = 0; i < ZTREE_INNER_N; i++)
            {
                new (vec + i) zptr_void(); // placement new
            }
            return true;
        }
    }
    return false;
}

bool ztree_void::alloc_leaf(zptr_void & ref)
{
    ft_size size = ZTREE_INNER_N * (this_values_inline_size ? this_values_inline_size : sizeof(zptr_void));
    void * mem;
    if (ref.alloc(size) && (mem = ref.get()))
    {
        if (this_values_inline_size)
            memset(mem, 0, size);
        else {
            zptr_void * vec = reinterpret_cast<zptr_void *>(mem);
            for (ft_size i = 0; i < ZTREE_INNER_N; i++)
            {
                new (vec + i) zptr_void(); // placement new
            }
        }
        return true;
    }
    return false;
}

/****************************************************************/

void ztree_void::clear()
{
    for (ft_size i = 0; i < ZTREE_TOP_N; i++)
    {
        free_tree_recursive(this_tree[i], i);
    }
}

void ztree_void::free_tree_recursive(zptr_void & ref, ft_size depth)
{
    if (depth > 0)
    {
        zptr_void * vec = reinterpret_cast<zptr_void *>(ref.get());
        if (vec)
        {
            for (ft_size i = 0; i < ZTREE_INNER_N; i++)
            {
                free_tree_recursive(vec[i], depth - 1);
            }
        }
    }
    else
        free_leaf(ref);
    ref.free();
}

void ztree_void::free_leaf(zptr_void & ref)
{
    if (this_values_inline_size == 0)
    {
        zptr_void * vec = reinterpret_cast<zptr_void *>(ref.get());
        if (vec)
        {
            for (ft_size i = 0; i < ZTREE_INNER_N; i++)
                vec[i].free();
        }
    }
}

FT_NAMESPACE_END
