/*
 * map_stat.t.hh
 *
 *  Created on: Mar 12, 2011
 *      Author: max
 */

#include "first.hh"

#include "map_stat.hh"   // for ft_map_stat<T>

FT_NAMESPACE_BEGIN

// construct empty ft_map_stat
template<typename T>
ft_map_stat<T>::ft_map_stat()
    : super_type(), fm_total_count(0), fm_used_count(0)
{ }

// destroy ft_map_stat
template<typename T>
ft_map_stat<T>::~ft_map_stat()
{ }



// duplicate a ft_map_stat, i.e. initialize this ft_map_stat as a copy of other.
template<typename T>
ft_map_stat<T>::ft_map_stat(const ft_map<T> & other)
    : super_type(other), fm_total_count(0), fm_used_count(0)
{ }

// duplicate a ft_map_stat, i.e. initialize this ft_map_stat as a copy of other.
template<typename T>
ft_map_stat<T>::ft_map_stat(const ft_map_stat<T> & other)
    : super_type(other), fm_total_count(other.fm_total_count), fm_used_count(other.fm_used_count)
{ }




// copy ft_map_stat, i.e. set this ft_map_stat contents as a copy of other's contents.
template<typename T>
const ft_map_stat<T> & ft_map_stat<T>::operator=(const ft_map<T> & other)
{
    super_type::operator=(other);
    fm_total_count = fm_used_count = 0;
    return * this;
}

// copy ft_map_stat, i.e. set this ft_map_stat contents as a copy of other's contents.
template<typename T>
const ft_map_stat<T> & ft_map_stat<T>::operator=(const ft_map_stat<T> & other)
{
    super_type::operator=(other);
    fm_total_count = other.fm_total_count;
    fm_used_count = other.fm_used_count;
    return * this;
}

FT_NAMESPACE_END
