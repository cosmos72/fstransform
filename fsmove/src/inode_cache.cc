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

const ft_string * fm_inode_cache::find_or_add(ft_inode inode, const ft_string & name)
{
    ff_assert(!name.empty());

    ft_string & value = map[inode];
    if (value.empty()) {
        value = name;
        return 0;
    }
    return & value;
}

FT_NAMESPACE_END
