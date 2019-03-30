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
 * map_iterator.hh
 *
 *  Created on: Mar 30, 2019
 *      Author: max
 */

#ifndef FSREMAP_MAP_ITERATOR_HH
#define FSREMAP_MAP_ITERATOR_HH

#include "check.hh"

#include <map>       // for std::map<K,V>

#include "assert.hh" // for ff_assert macro
#include "extent.hh" // for fr_extent_key<T>, fr_extent_payload<T>

FT_NAMESPACE_BEGIN

template<typename T>
class fr_map_iterator {
private:
	template<typename U> friend class fr_map;
	template<typename U> friend class fr_map_const_iterator;

	typedef fr_map_iterator<T> iterator;
	typedef typename std::map<fr_extent_key<T>, fr_extent_payload<T> >::iterator base;

	base        this_iter;
	fr_map<T> * this_map;

	fr_map_iterator(base iter, fr_map<T> * map)
		: this_iter(iter), this_map(map) {
	}

	base super() {
		return this_iter;
	}

public:
	typedef typename base::value_type        value_type;
	typedef typename base::difference_type   difference_type;
	typedef typename base::reference         reference;
	typedef typename base::pointer           pointer;
	typedef typename base::iterator_category iterator_category;

	fr_map_iterator() : this_iter(), this_map() {
	}

	~fr_map_iterator() {
		this_map = 0;
	}

	bool operator==(iterator other) const {
		ff_assert(this_map == other.this_map);
		return this_iter == other.this_iter;
	}

	bool operator!=(iterator other) const {
		ff_assert(this_map == other.this_map);
		return this_iter != other.this_iter;
	}

	difference_type operator-(iterator other) {
		ff_assert(this_map != 0);
		ff_assert(this_map == other.this_map);
		return this_iter - other.this_iter;
	}

	value_type & operator*() {
		validate_deref();
		return *this_iter;
	}

	value_type * operator->() {
		validate_deref();
		return this_iter.operator->();
	}

	iterator & operator++() {
		validate_increment();
		++this_iter;
		return *this;
	}

	iterator operator++(int) {
		validate_increment();
		base copy = this_iter;
		return iterator(++copy, this_map);
	}

	iterator & operator--() {
		validate_decrement();
		--this_iter;
		return *this;
	}

	iterator operator--(int) {
		validate_decrement();
		base copy = this_iter;
		return iterator(--copy, this_map);
	}

	void validate_deref() {
		ff_assert(this_map != 0);
		ff_assert(this_iter != this_map->end().this_iter);
		iterator match = this_map->find(this_iter->first);
		ff_assert(this_iter == match.this_iter);
	}

	void validate_increment() {
		ff_assert(this_map != 0);
		ff_assert(this_iter != this_map->end().this_iter);
	}

	void validate_decrement() {
		ff_assert(this_map != 0);
		ff_assert(this_iter != this_map->begin().this_iter);
	}
};


template<typename T>
class fr_map_const_iterator {
private:
	template<typename U> friend class fr_map;

	typedef fr_map_const_iterator<T> const_iterator;
	typedef typename std::map<fr_extent_key<T>, fr_extent_payload<T> >::const_iterator base;

	base              this_iter;
	const fr_map<T> * this_map;

	fr_map_const_iterator(base iter, const fr_map<T> * map)
		: this_iter(iter), this_map(map) {
	}

	base super() const {
		return this_iter;
	}

public:
	typedef typename base::value_type        value_type;
	typedef typename base::difference_type   difference_type;
	typedef typename base::reference         reference;
	typedef typename base::pointer           pointer;
	typedef typename base::iterator_category iterator_category;

	fr_map_const_iterator() : this_iter(), this_map() {
	}

	/*implicit*/ fr_map_const_iterator(fr_map_iterator<T> other)
		: this_iter(other.this_iter), this_map(other.this_map) {
	}

	~fr_map_const_iterator() {
		this_map = 0;
	}

	bool operator==(const_iterator other) const {
		ff_assert(this_map == other.this_map);
		return this_iter == other.this_iter;
	}

	bool operator!=(const_iterator other) const {
		ff_assert(this_map == other.this_map);
		return this_iter != other.this_iter;
	}

	difference_type operator-(const_iterator other) const {
		ff_assert(this_map == other.this_map);
		return this_iter - other.this_iter;
	}

	const value_type & operator*() const {
		validate_deref();
		return *this_iter;
	}

	const value_type * operator->() const {
		validate_deref();
		return this_iter.operator->();
	}

	const_iterator & operator++() {
		validate_increment();
		++this_iter;
		return *this;
	}

	const_iterator operator++(int) {
		validate_increment();
		base copy = this_iter;
		return const_iterator(++copy, this_map);
	}

	const_iterator & operator--() {
		validate_decrement();
		--this_iter;
		return *this;
	}

	const_iterator operator--(int) {
		validate_decrement();
		base copy = this_iter;
		return const_iterator(--copy, this_map);
	}

	void validate_deref() const {
		ff_assert(this_map != 0);
		ff_assert(this_iter != this_map->end().this_iter);
		const_iterator match = this_map->find(this_iter->first);
		ff_assert(this_iter == match.this_iter);
	}

	void validate_increment() const {
		ff_assert(this_map != 0);
		ff_assert(this_iter != this_map->end().this_iter);
	}

	void validate_decrement() const {
		ff_assert(this_map != 0);
		ff_assert(this_iter != this_map->begin().this_iter);
	}
};


FT_NAMESPACE_END

#endif /* FSREMAP_MAP_ITERATOR_HH */
