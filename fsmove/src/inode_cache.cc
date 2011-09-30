/*
 * inode_cache.cc
 *
 *  Created on: Aug 18, 2011
 *      Author: max
 */

#include "first.hh"


#include "assert.hh"          // for ft_assert
#include "inode_cache.hh"     // for fm_inode_cache

FT_NAMESPACE_BEGIN


/** default constructor */
fm_inode_cache::fm_inode_cache() : map()
{ }

/** copy constructor */
fm_inode_cache::fm_inode_cache(const fm_inode_cache & other) : map(other.map)
{ }
    
/** assignment operator */
const fm_inode_cache & fm_inode_cache::operator=(const fm_inode_cache & other)
{
    if (this != &other)
        map = other.map;
    return *this;
}
    
/** destructor */
fm_inode_cache::~fm_inode_cache()
{ }

/** return path of cached inode if found, else add it to cache and return NULL */
const ft_string * fm_inode_cache::find_or_add(ft_inode inode, const ft_string & name)
{
    ff_assert(!name.empty());

    ft_string & value = map[inode];
    if (value.empty()) {
        value = name;
        return NULL;
    }
    return & value;
}

/**
 * return path of cached inode if found, else NULL.
 * if path is returned, erase() must be called on the same inode when done with path!
 */
const ft_string * fm_inode_cache::find(ft_inode inode, const ft_string & name) const
{
    ff_assert(!name.empty());

    fm_inode_cache_map_type::const_iterator iter = map.find(inode);
    if (iter == map.end())
        return NULL;

    return & iter->second;
}

/**
 * must be called if and only if find(inode) returned != NULL
 */
void fm_inode_cache::erase(ft_inode inode)
{
    ft_size erased = map.erase(inode);

    ff_assert(erased == 1);
}

void fm_inode_cache::clear()
{
    map.clear();
}

FT_NAMESPACE_END
