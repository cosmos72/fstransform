/*
 * vector.t.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "first.hh"

#include <algorithm>     // for std::sort()
#include <cerrno>        // for EFBIG

#include "vector.hh"     // for ft_vector<T>

FT_NAMESPACE_BEGIN


/**
 * append a single extent to this vector.
 *
 * if this vector is not empty
 * and specified extent ->physical is equal to last->physical + last->length
 * and specified extent ->logical  is equal to last->logical  + last->length
 * where 'last' is the last extent in this vector,
 * then merge the two extents
 *
 * otherwise append to this vector a new extent containing specified extent (physical, logical, length)
 */
template<typename T>
void ft_vector<T>::append(T physical, T logical, T length, ft_size user_data)
{
    if (!this->empty()) {
        value_type & last = this->back();
        T & last_length = last.length();

        if (last.physical() + last_length == physical && last.logical() + last_length == logical) {
            /* merge! */
            last_length += length;
            return;
        }
    }
    /* we could use this->push_back() but creating its parameter with make_pair() is cumbersome */
    this->resize(this->size() + 1);
    value_type & extent = this->back();
    extent.physical() = physical;
    extent.logical() = logical;
    extent.length() = length;
    extent.user_data() = user_data;
}


/**
 * append another extent vector to this vector.
 *
 * this method does not merge extents: the two lists of extents will be simply concatenated
 */
template<typename T>
void ft_vector<T>::append_all(const ft_vector<T> & other)
{
    if (this != & other) {
        this->insert(this->end(), other.begin(), other.end());
    } else {
        /* should not happen, but it's a recoverable problem */
    }
}


/**
 * reorder this vector in-place, sorting by physical
 */
template<typename T>
void ft_vector<T>::sort_by_physical()
{
    std::sort(this->begin(), this->end(), typename value_type::comparator_physical());
}

/**
 * reorder this vector in-place, sorting by physical
 */
template<typename T>
void ft_vector<T>::sort_by_physical(iterator from, iterator to)
{
    std::sort(from, to, typename value_type::comparator_physical());
}


/**
 * reorder this vector in-place, sorting by logical
 */
template<typename T>
void ft_vector<T>::sort_by_logical()
{
    std::sort(this->begin(), this->end(), typename value_type::comparator_logical());
}

/**
 * reorder this vector in-place, sorting by logical
 */
template<typename T>
void ft_vector<T>::sort_by_logical(iterator from, iterator to)
{
    std::sort(from, to, typename value_type::comparator_logical());
}


/**
 * reorder this vector in-place, sorting by reverse length (largest extents will be first)
 */
template<typename T>
void ft_vector<T>::sort_by_reverse_length()
{
    std::sort(this->begin(), this->end(), typename value_type::reverse_comparator_length());
}

/**
 * reorder this vector in-place, sorting by reverse length (largest extents will be first)
 */
template<typename T>
void ft_vector<T>::sort_by_reverse_length(iterator from, iterator to)
{
    std::sort(from, to, typename value_type::reverse_comparator_length());
}

FT_NAMESPACE_END
