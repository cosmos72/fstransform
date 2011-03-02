/*
 * vector.template.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "first.hh"

#include <algorithm>     // for std::sort()

#include "assert.hh"     // for ff_assert()
#include "map.hh"        // for ft_map<T>
#include "vector.hh"     // for ft_vector<T>

FT_NAMESPACE_BEGIN

/**
 * append a single extent to this vector.
 *
 * if this vector is not empty
 * and specified extent ->fm_physical is equal to last->fm_physical + last->fm_length
 * and specified extent ->fm_logical  is equal to last->fm_logical  + last->fm_length
 * where 'last' is the last extent in this vector,
 * then merge the two extents
 *
 * otherwise append to this vector a new extent containing specified extent (physical, logical, length)
 */
template<typename T>
void ft_vector<T>::append(T physical, T logical, T length)
{
    if (!this->empty()) {
        ft_extent<T> & last = this->back();
        T & last_length = last.length();

        if (last.physical() + last_length == physical && last.logical() + last_length == logical) {
            /* merge! */
            last_length += length;
            return;
        }
    }
    /* we could use this->push_back() but creating its parameter with make_pair() is cumbersome */
    this->resize(this->size() + 1);
    ft_extent<T> & extent = this->back();
    extent.physical() = physical;
    extent.logical() = logical;
    extent.length() = length;
}


/**
 * append another extent vector to this vector.
 *
 * this method does not merge extents: the two lists of extents will be simply concatenated
 */
template<typename T>
void ft_vector<T>::append(const ft_vector<T> & other)
{
    if (this != & other) {
        this->insert(this->end(), other.begin(), other.end());
    } else {
        /* should not happen, but it's a recoverable problem */
    }
}


/**
 * append an extent map to this vector.
 *
 * this method does not merge extents: the two lists of extents will be simply concatenated
 */
template<typename T>
void ft_vector<T>::append(const ft_map<T> & other)
{
    this->insert(this->end(), other.begin(), other.end());
}

/**
 * reorder this vector in-place, sorting by fm_physical
 */
template<typename T>
void ft_vector<T>::sort_by_physical()
{
    std::sort(this->begin(), this->end(), typename value_type::comparator_physical());
}




/**
 * append a new extent to this container.
 * container must check if new extent touches the last extent already in the container
 * but is assured (and can assume) than new extent will NEVER touch or intersect other extents.
 *
 * container is required to merge the new extent with the last one it contains, if they touch.
 */
template<typename T>
void ft_vector<T>::extent_append(ft_uoff physical, ft_uoff logical, ft_uoff length)
{
    // we know from ff_io::*_extents_list<T>(ft_map<T> &) that conversion ft_uoff -> T will not overflow
    append((T) physical, (T) logical, (T) length);
}



FT_NAMESPACE_END
