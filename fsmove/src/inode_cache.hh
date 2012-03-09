/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
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
 * inode_cache.hh
 *
 *  Created on: Aug 18, 2011
 *      Author: max
 */

#ifndef FSMOVE_INODE_CACHE_HH
#define FSMOVE_INODE_CACHE_HH

#include "types.hh"     // for ft_inode

#include <map>          // for std::map
#include <string>       // for ft_string


FT_NAMESPACE_BEGIN

class fm_inode_cache
{
private:
    typedef std::map<ft_inode, ft_string> fm_inode_cache_map_type;

    fm_inode_cache_map_type map;
    
public:
    /** default constructor */
    fm_inode_cache();
    
    /** copy constructor */
    fm_inode_cache(const fm_inode_cache &);
    
    /** assignment operator */
    const fm_inode_cache & operator=(const fm_inode_cache &);
    
    /** destructor */
    ~fm_inode_cache();
    
    /** return path of cached inode if found, else add it to cache and return NULL */
    const ft_string * find_or_add(ft_inode inode, const ft_string & name);

    /**
     * return path of cached inode if found, else NULL
     * if path is returned, erase() must be called on the same inode when done with path!
     */
    const ft_string * find(ft_inode inode, const ft_string & name) const;

    /**
     * must be called if and only if find(inode) returned != NULL
     */
    void erase(ft_inode inode);

    void clear();
};

FT_NAMESPACE_END

#endif /* FSMOVE_INODE_CACHE_HH */
