/*
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
    std::map<ft_inode, ft_string> map;
    
public:
    /** default constructor */
    fm_inode_cache();
    
    /** copy constructor */
    fm_inode_cache(const fm_inode_cache &);
    
    /** assignment operator */
    const fm_inode_cache & operator=(const fm_inode_cache &);
    
    /** destructor */
    ~fm_inode_cache();
    
    const ft_string * find_or_add(ft_inode inode, const ft_string & name);
};

FT_NAMESPACE_END

#endif /* FSMOVE_INODE_CACHE_HH */
