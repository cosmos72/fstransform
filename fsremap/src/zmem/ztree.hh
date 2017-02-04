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
 * ztree.hh
 *
 *  Created on: Jan 30, 2017
 *      Author: max
 */

#ifndef FSTRANSFORM_ZTREE_HH
#define FSTRANSFORM_ZTREE_HH

#include "zptr.hh"

FT_NAMESPACE_BEGIN

class ztree_void
{
private:
    enum { ZTREE_TOP_N = sizeof(ft_ull) };
    
    zptr_void this_tree[ZTREE_TOP_N];
    ft_size this_values_inline_size;

    static ft_size key_to_depth(ft_ull key);
    
    const void * get_leaf(const zptr_void & ref, ft_u8 offset) const;
    bool put_leaf(zptr_void & ref, ft_u8 offset, const void * value, ft_size size);
    
    bool alloc_inner(zptr_void & ref);
    bool alloc_leaf(zptr_void & ref);

    void free_tree_recursive(zptr_void & ptr, ft_size depth);
    void free_leaf(zptr_void & ref);


public:
    explicit ztree_void(ft_size values_inline_size); /* 0 if values are not inline, i.e. they must be allocated one by one */
    ~ztree_void();
    
    const void * get(ft_ull key) const;
    bool put(ft_ull key, const void * value, ft_size size);
    
    void free();
};


template<class T>
    class ztree : private ztree_void
{
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_ZTREE_HH */
