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
 * vector.t.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "first.hh"

#include <algorithm>     // for std::sort(), std::swap()

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>        // for EINVAL
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>         // for EINVAL
#endif

#include "log.hh"        // for ff_log()
#include "misc.hh"       // for ff_can_sum(), ff_min2()
#include "vector.hh"     // for fr_vector<T>

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
void fr_vector<T>::append(T physical, T logical, T length, ft_size user_data)
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
void fr_vector<T>::append_all(const fr_vector<T> & other)
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
void fr_vector<T>::sort_by_physical()
{
    std::sort(this->begin(), this->end(), typename value_type::comparator_physical());
}

/**
 * reorder this vector in-place, sorting by physical
 */
template<typename T>
void fr_vector<T>::sort_by_physical(iterator from, iterator to)
{
    std::sort(from, to, typename value_type::comparator_physical());
}


/**
 * reorder this vector in-place, sorting by logical
 */
template<typename T>
void fr_vector<T>::sort_by_logical()
{
    std::sort(this->begin(), this->end(), typename value_type::comparator_logical());
}

/**
 * reorder this vector in-place, sorting by logical
 */
template<typename T>
void fr_vector<T>::sort_by_logical(iterator from, iterator to)
{
    std::sort(from, to, typename value_type::comparator_logical());
}


/**
 * reorder this vector in-place, sorting by reverse length (largest extents will be first)
 */
template<typename T>
void fr_vector<T>::sort_by_reverse_length()
{
    std::sort(this->begin(), this->end(), typename value_type::reverse_comparator_length());
}

/**
 * reorder this vector in-place, sorting by reverse length (largest extents will be first)
 */
template<typename T>
void fr_vector<T>::sort_by_reverse_length(iterator from, iterator to)
{
    std::sort(from, to, typename value_type::reverse_comparator_length());
}

/**
 * swap ->physical with ->logical in each extent of this vector.
 * Note: does NOT sort after swapping!
 */
template<typename T>
void fr_vector<T>::transpose()
{
	iterator iter = this->begin(), end_iter = this->end();
	for (; iter != end_iter; ++iter) {
		value_type & e = *iter;
		std::swap(e.physical(), e.logical());
	}
}


/**
 * used by ft_io_prealloc.
 *
 * truncate at specified logical value
 */
template<typename T>
void fr_vector<T>::truncate_at_logical(T logical_end)
{
    while (!this->empty()) {
       value_type & e = this->back();
       T logical = e.logical();
       if (logical >= logical_end) {
    	   this->pop_back();
    	   continue;
       }
       T & length = e.length();
       T delta = logical_end - logical;
       if (length > delta)
    	   length = delta;
       break;
    }
}


/**
 * used by ft_io_prealloc.
 *
 * given a vector mapping a<->c (v1, 'a' is stored in ->physical and 'c' in ->logical)
 * and a vector mapping b<->c (v2, 'a' is stored in ->physical and 'c' in ->logical),
 * compute the vector mapping a<->b (v2, 'a' is stored in ->physical and 'b' in ->logical)
 * and append it to this vector.
 *
 * user_data will be copied from v2.
 * all extents in b not mapped to c will be added to 'unmapped' (if unmapped is not NULL)
 *
 * Prerequisite: a<->c and b<->c must be sorted by ->logical (i.e. by 'c')
 *
 * Returns error if a<->c codomain (range in 'c') is smaller than b->c codomain (range in 'c')
 * and in particular if a<->c has holes in 'c' where b->c does not.
 */
template<typename T>
int fr_vector<T>::compose0(const fr_vector<T> & v1, const fr_vector<T> & v2, T & ret_block_size_bitmask, fr_vector<T> * unmapped)
{
	T block_size_bitmask = ret_block_size_bitmask;
	ft_size n1 = v1.size(), n2 = v2.size();
	int err = 0;

	if (n1 == 0) {
		if (n2 != 0) {
			ff_log(FC_ERROR, 0, "compose(): a<->c mapping is empty while b<->c mapping is not");
			err = -EINVAL;
		}
		return err;
	}
	ft_size i1 = 0, i2 = 0;
	T delta1 = 0, delta2 = 0;
	while (i1 < n1 && i2 < n2) {
		const value_type & e1 = v1[i1], & e2 = v2[i2];

		const T phys1 = e1.physical(), log1 = e1.logical(), len1 = e1.length();
		const T phys2 = e2.physical(), log2 = e2.logical(), len2 = e2.length();

		if (!ff_can_sum(phys1, len1) || !ff_can_sum(log1, len1)) {
			ff_log(FC_ERROR, 0, "compose(): a<->c extent {physical = %" FT_ULL ", logical = %" FT_ULL ", length = %" FT_ULL "} overflows type T",
					(ft_ull) phys1, (ft_ull) log1, (ft_ull) len1);
			err = -EFBIG;
		}
		else if (!ff_can_sum(phys2, len2) || !ff_can_sum(log2, len2)) {
			ff_log(FC_ERROR, 0, "compose(): b<->c extent {physical = %" FT_ULL ", logical = %" FT_ULL ", length = %" FT_ULL "} overflows type T",
					(ft_ull) phys2, (ft_ull) log2, (ft_ull) len2);
			err = -EFBIG;
		}
		else if (delta1 >= len1) {
			ff_log(FC_FATAL, 0, "compose(): internal error, length offset = %" FT_ULL " is not strictly inside a<->c extent"
					" {physical = %" FT_ULL ", logical = %" FT_ULL ", length = %" FT_ULL "}",
					(ft_ull) delta1, (ft_ull) log1, (ft_ull) phys1, (ft_ull) len1);
			err = -EINVAL;
		}
		else if (delta2 >= len2) {
			ff_log(FC_FATAL, 0, "compose(): internal error, length offset = %" FT_ULL " is not strictly inside b<->c extent"
					" {physical = %" FT_ULL ", length = %" FT_ULL "}",
					(ft_ull) delta2, (ft_ull) log2, (ft_ull) phys2, (ft_ull) len2);
			err = -EINVAL;
		}
		else if (log1 + delta1 < log2 + delta2) {
			ff_log(FC_ERROR, 0, "compose(): unexpected hole: a<->c mapping {physical = %" FT_ULL ", logical = %" FT_ULL ", length = %" FT_ULL "} with offset = %" FT_ULL ,
					(ft_ull) phys1, (ft_ull) log1, (ft_ull) len1, (ft_ull) delta1);
			ff_log(FC_ERROR, 0, "           cannot be covered by b<->c mapping {physical = %" FT_ULL ", logical = %" FT_ULL ", length = %" FT_ULL "} with offset = %" FT_ULL ,
					(ft_ull) phys2, (ft_ull) log2, (ft_ull) len2, (ft_ull) delta2);
			err = -EINVAL;
		}
		if (err != 0)
			break;

		const ft_size user_data = e2.user_data();

		if (log1 + delta1 > log2 + delta2) {
			// a<->c (v1) has a hole: insert the unmapped b<->c fragment in 'unmapped'
			T hole_len = ff_min2(len2 - delta2, (log1 + delta1) - (log2 + delta2));
			if (unmapped != 0) {
				unmapped->append(phys2 + delta2, log2 + delta2, hole_len, user_data);
			}
			delta2 += hole_len;
		}

		const T len = ff_min2(len1 - delta1, len2 - delta2);
		if (len > 0) {
			append(phys1 + delta1, phys2 + delta2, len, user_data);

			block_size_bitmask |= (phys1 + delta1) | (phys2 + delta2) | len;
		}


		if (len != len1 - delta1)
			delta1 += len;
		else
			delta1 = 0, i1++;

		if (len != len2 - delta2)
			delta2 += len;
		else
			delta2 = 0, i2++;
	}
	if (err == 0) {
		const value_type & e1 = v1[i1 < n1 ? i1 : n1 - 1], & e2 = v2[i2 < n2 ? i2 : n2 - 1];

		const T phys1 = e1.physical(), log1 = e1.logical(), len1 = e1.length();
		const T phys2 = e2.physical(), log2 = e2.logical(), len2 = e2.length();

		if (i1 < n1 && i2 == n2) {
			ff_log(FC_ERROR, 0, "compose() error: a<->c current extent {physical = %" FT_ULL ", logical = %" FT_ULL ", length = %" FT_ULL "}"
					" cannot be covered by b<->c last extent {physical = %" FT_ULL ", logical = %" FT_ULL ", length = %" FT_ULL "}",
					(ft_ull) phys1, (ft_ull) log1, (ft_ull) len1,
					(ft_ull) phys2, (ft_ull) log2, (ft_ull) len2);
			err = -EINVAL;
		} else if (i1 == n1 && i2 < n2) {
			// a<->c (v1) has a hole: insert the unmapped b<->c fragment in 'unmapped'
			if (unmapped != 0) {
				T hole_len = len2 - delta2;
				unmapped->append(phys2 + delta2, log2 + delta2, hole_len, e2.user_data());
			}
		}
	}
	if (err == 0)
		ret_block_size_bitmask = block_size_bitmask;
	return err;
}


/** print vector contents to log */
template<typename T>
void fr_vector<T>::show(const char * label1, const char * label2, ft_uoff effective_block_size, ft_log_level level) const
{
	fr_extent<T>::show(this->begin(), this->end(), this->size(), label1, label2, effective_block_size, level);
}

FT_NAMESPACE_END
