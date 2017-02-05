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
 * cache_zmem.hh
 *
 *  Created on: Feb 5, 2017
 *      Author: max
 */

#include "first.hh"
#include "cache_zmem.hh"

FT_NAMESPACE_BEGIN

    
/** default constructor */
ft_cache_zmem_is::ft_cache_zmem_is()
{ }
   
/** destructor */
ft_cache_zmem_is::~ft_cache_zmem_is()
{ }
   

/**
 * if cached key found, set inout_payload and return 1.
 * Otherwise add (key,inout_payload) to cache and return 0.
 * On error, return < 0.
 * if returns 0, erase() must be called on the same key when done with payload!
 */
int ft_cache_zmem_is::find_or_add(const ft_inode key, ft_string & inout_payload)
{
   enum { verify_ft_inode_fits_ft_ull = sizeof(char[sizeof(ft_inode) <= sizeof(ft_ull) ? 1 : -1]) };
   
   ft_ull ukey = (ft_ull)key;
   const char * value = tree.get(ukey);
   if (value != NULL) {
      inout_payload = value;
      return 1;
   }
   if (tree.put(ukey, inout_payload.c_str(), 1 + inout_payload.size())) // also copy final '\0'
     return 0;
   
   return ff_log(FC_ERROR, 0, "ztree<char *>::put(%"FT_ULL", \"%s\") failed", ukey, inout_payload.c_str());
}
   
    
/**
 * if cached key found, set result_payload, remove cached key and return 1.
 * Otherwise return 0. On error, return < 0.
 */
int ft_cache_zmem_is::find_and_delete(const ft_inode key, ft_string & result_payload)
{
   ft_ull ukey = (ft_ull)key;
   const char * value = tree.get(ukey);
   if (value == NULL)
     return 0;
   
   result_payload = value;
   if (tree.del(ukey))
     return 1;
   
   return ff_log(FC_ERROR, 0, "ztree<char *>::del(%"FT_ULL") failed", ukey);
}
   
   
/**
 * if cached inode found, change its payload and return 1.
 * Otherwise return 0. On error, return < 0.
 */
int ft_cache_zmem_is::find_and_update(const ft_inode key, const ft_string & new_payload)
{    
   ft_ull ukey = (ft_ull)key;
   const char * value = tree.get(ukey);
   if (value == NULL)
     return 0;
   
   if (tree.put(ukey, new_payload.c_str(), 1 + new_payload.size())) // also copy final '\0'
     return 1;
   
   return ff_log(FC_ERROR, 0, "ztree<char *>::put(%"FT_ULL", \"%s\") failed", ukey, new_payload.c_str());
}


void ft_cache_zmem_is::clear()
{
   tree.clear();
}

   
FT_NAMESPACE_END

