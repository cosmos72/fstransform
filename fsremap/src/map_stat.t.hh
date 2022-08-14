/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 2 of the License, or
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
 * map_stat.t.hh
 *
 *  Created on: Mar 12, 2011
 *      Author: max
 */

#include "first.hh"

#include "map_stat.hh"   // for fr_map_stat<T>

FT_NAMESPACE_BEGIN

// construct empty fr_map_stat
template<typename T>
fr_map_stat<T>::fr_map_stat()
    : super_type(), this_total_count(0), this_used_count(0)
{ }

// destroy fr_map_stat
template<typename T>
fr_map_stat<T>::~fr_map_stat()
{ }



// duplicate a fr_map_stat, i.e. initialize this fr_map_stat as a copy of other.
template<typename T>
fr_map_stat<T>::fr_map_stat(const fr_map<T> & other)
    : super_type(other), this_total_count(0), this_used_count(0)
{ }

// duplicate a fr_map_stat, i.e. initialize this fr_map_stat as a copy of other.
template<typename T>
fr_map_stat<T>::fr_map_stat(const fr_map_stat<T> & other)
    : super_type(other), this_total_count(other.this_total_count), this_used_count(other.this_used_count)
{ }




// copy fr_map_stat, i.e. set this fr_map_stat contents as a copy of other's contents.
template<typename T>
const fr_map_stat<T> & fr_map_stat<T>::operator=(const fr_map<T> & other)
{
    super_type::operator=(other);
    this_total_count = this_used_count = 0;
    return * this;
}

// copy fr_map_stat, i.e. set this fr_map_stat contents as a copy of other's contents.
template<typename T>
const fr_map_stat<T> & fr_map_stat<T>::operator=(const fr_map_stat<T> & other)
{
    super_type::operator=(other);
    this_total_count = other.this_total_count;
    this_used_count = other.this_used_count;
    return * this;
}


/** clear this fr_map_stat. also sets total_count, used_count and free_count to zero */
template<typename T>
void fr_map_stat<T>::clear()
{
    super_type::clear();
    this_total_count = this_used_count = 0;
}

FT_NAMESPACE_END
