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
 * map.t.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "first.hh"

#include "map.hh"        // for fr_map<T>
#include "misc.hh"       // for ff_max2(), ff_min2()
#include "vector.hh"     // for fr_vector<T>

FT_NAMESPACE_BEGIN

#define ff_map_assert ff_assert

// construct empty fr_map
template<typename T>
fr_map<T>::fr_map() : super_type() { }


// duplicate a fr_map, i.e. initialize this fr_map as a copy of other.
template<typename T>
fr_map<T>::fr_map(const fr_map<T> & other) : super_type(other)
{ }

// destroy fr_map
template<typename T>
fr_map<T>::~fr_map()
{ }



/** compare two key+value extents and find relative position */
template<typename T>
fr_extent_relation fr_map<T>::compare(const_iterator pos1,
                                      const_iterator pos2)
{
    return compare(pos1->first, pos1->second, pos2->first, pos2->second);
}

/** compare two key+value extents and find relative position */
template<typename T>
fr_extent_relation fr_map<T>::compare(const_iterator pos1,
                                      const key_type & key2, const mapped_type & value2)
{
    return compare(pos1->first, pos1->second, key2, value2);
}

/** compare two key+value extents and find relative position */
template<typename T>
fr_extent_relation fr_map<T>::compare(const key_type & key1, const mapped_type & value1,
                                      const key_type & key2, const mapped_type & value2)
{
    T physical1 = key1.physical, logical1 = value1.logical, length1 = value1.length;
    T physical2 = key2.physical, logical2 = value2.logical, length2 = value2.length;
    ft_size user_data1 = value1.user_data;
    ft_size user_data2 = value2.user_data;
    fr_extent_relation rel;

    if (physical1 < physical2) {
        if (physical1 + length1 == physical2 && logical1 + length1 == logical2 && user_data1 == user_data2)
            rel = FC_EXTENT_TOUCH_BEFORE;
        else if (physical1 + length1 <= physical2)
            rel = FC_EXTENT_BEFORE;
        else
            rel = FC_EXTENT_INTERSECT;
    } else if (physical1 == physical2) {
        rel = FC_EXTENT_INTERSECT;
    } else /* physical1 > physical2 */ {
        if (physical1 == physical2 + length2 && logical1 == logical2 + length2 && user_data1 == user_data2)
            rel = FC_EXTENT_TOUCH_AFTER;
        else if (physical2 + length2 <= physical1)
            rel = FC_EXTENT_BEFORE;
        else
            rel = FC_EXTENT_INTERSECT;
    }
    return rel;
}

#define FR_MAP_VALIDATE(iter) do { \
	ff_assert((iter).this_map == this); \
	ff_assert((iter) != end()); \
	/* iter->first also calls iter->validate_deref() */ \
	iterator __match = find((iter)->first); \
	ff_assert((iter) == __match); \
} while (false)

/**
 * validate iterator
 */
template<typename T>
void fr_map<T>::validate(iterator iter) {
	FR_MAP_VALIDATE(iter);
}

/**
 * merge extent (which can belong to this fr_map) into specified position.
 * the two extents MUST exactly touch!
 * i.e. their fr_extent_relation MUST be either FC_EXTENT_TOUCH_BEFORE or FC_EXTENT_TOUCH_AFTER
 *
 * return iterator to merged position.
 *
 * this is an internal method and should ONLY be invoked by merge(),
 *          as it does not handle chains of merges, as merge() does instead.
 *          Again: call merge(), not this method.
 */
template<typename T>
typename fr_map<T>::iterator fr_map<T>::merge0(iterator pos1, const key_type & key2, const mapped_type & value2)
{
	FR_MAP_VALIDATE(pos1);
    fr_extent_relation rel = compare(pos1, key2, value2);

    if (rel == FC_EXTENT_TOUCH_BEFORE) {
        // modify extent in-place
        mapped_type & value1 = pos1->second;
        value1.length += value2.length;

    } else if (rel == FC_EXTENT_TOUCH_AFTER) {
        // for safety, remove and reinsert pos1 entry instead of modifying it in place:
    	// we do not want to risk creating out-of-order entries in map!
    	mapped_type value = pos1->second;
    	value.logical = value2.logical;
    	value.length += value2.length;

        remove(pos1);
        pos1 = insert0(key2, value);

    } else {
        /* must not happen! trigger an assertion failure. */
        ff_map_assert(rel == FC_EXTENT_TOUCH_BEFORE || rel == FC_EXTENT_TOUCH_AFTER);
    }
    return pos1;
}

/**
 * merge together two extents (which must belong to this fr_map).
 * the two extents MUST exactly touch!
 * i.e. their fr_extent_relation MUST be either FC_EXTENT_TOUCH_BEFORE or FC_EXTENT_TOUCH_AFTER
 *
 * return iterator to merged position.
 *
 * this is an internal method and should ONLY be invoked by merge(),
 *          as it does not handle chains of merges, as merge() does instead.
 *          Again: call merge(), not this method.
 *
 * this method exists because it is simpler to implement than
 * merge0(iterator, const key_type &, const mapped_type &)
 */
template<typename T>
typename fr_map<T>::iterator fr_map<T>::merge0(iterator pos1, iterator pos2)
{
	FR_MAP_VALIDATE(pos1);
	FR_MAP_VALIDATE(pos2);
	ff_assert(pos1 != pos2);

    fr_extent_relation rel = compare(pos1, pos2);
    mapped_type & value1 = pos1->second;
    mapped_type & value2 = pos2->second;

    const T length = value1.length + value2.length;

    if (rel == FC_EXTENT_TOUCH_BEFORE) {
        // modify first extent in-place, erase second
        value1.length = length;
        remove(pos2);
        return pos1;

    } else if (rel == FC_EXTENT_TOUCH_AFTER) {
        // modify second extent in-place, erase first
        value2.length = length;
        remove(pos1);
        return pos2;

    } else {
        /* must not happen! trigger an assertion failure. */
        ff_map_assert(rel == FC_EXTENT_TOUCH_BEFORE || rel == FC_EXTENT_TOUCH_AFTER);
    }
    return pos1;
}


/**
 * merge extent (which must NOT belong to this fr_map) into specified fr_map position.
 * the two extents MUST exactly touch!
 * i.e. their fr_extent_relation MUST be either FC_EXTENT_TOUCH_BEFORE or FC_EXTENT_TOUCH_AFTER
 *
 * return iterator to merged position.
 *
 * this method automatically performs chain of merges if needed:
 * for example, if extent 2 is merged in a fr_map containing 0..1 and 3..5,
 * this method will first merge 0..1 with 2, obtaining 0..2,
 * then it will realize that result can be merged with 3..5
 * and it will also perform this second merging, obtaining 0..5
 */
template<typename T>
typename fr_map<T>::iterator fr_map<T>::merge(iterator pos1, const key_type & key2, const mapped_type & value2)
{
	FR_MAP_VALIDATE(pos1);

	fr_extent_relation rel = compare(pos1, key2, value2);

    if (rel == FC_EXTENT_TOUCH_BEFORE) {
        pos1 = merge0(pos1, key2, value2);

        /* check for further possible merges! */
        if (pos1 != begin()) {
            iterator prev = pos1;
            --prev;
            if (compare(prev, pos1) == FC_EXTENT_TOUCH_BEFORE)
                pos1 = merge0(prev, pos1);
        }
    } else if (rel == FC_EXTENT_TOUCH_AFTER) {
        pos1 = merge0(pos1, key2, value2);

        /* check for further possible merges! */
        iterator next = pos1;
        ++next;
        if (next != end()) {
            if (compare(pos1, next) == FC_EXTENT_TOUCH_BEFORE)
                pos1 = merge0(pos1, next);
        }
    } else {
        /* must not happen! trigger an assertion failure. */
        ff_assert(rel == FC_EXTENT_TOUCH_BEFORE || rel == FC_EXTENT_TOUCH_AFTER);
    }
    return pos1;
}



// copy fr_map, i.e. set this fr_map contents as a copy of other's contents.
template<typename T>
const fr_map<T> & fr_map<T>::operator=(const fr_map<T> & other)
{
    super_type::operator=(other);
    return * this;
}

// swap this map contents with other map
template<typename T>
void fr_map<T>::swap(fr_map<T> & other)
{
    super_type::swap(other);
}


/**
 * returns the minimum physical and the maximum physical+length in this map.
 * if this map is empty, return {0,0}
 */
template<typename T>
void fr_map<T>::bounds(key_type & min_key, key_type & max_key) const
{
    const_iterator b = begin(), e = end();
    if (b != e) {
        min_key.physical = b->first.physical;
        --e;
        max_key.physical = e->first.physical + e->second.length;
    } else
        min_key.physical = max_key.physical = 0;
}

#if 0 // unused
/**
 * find the intersection (matching physical)
 * between the specified single block and this map, and store the intersection in ret_extent.
 * if no intersections, return false and ret_extent will be unchanged.
 */
template<typename T>
bool fr_map<T>::find_physical_block(T key_physical, value_type & ret_extent) const
{
	if (empty())
		return false;

	key_type key;
	key.physical = key_physical;

	/* iterator to first element >= key */
	const_iterator iter = this->lower_bound(key), e = end();
	if (iter == e)
		--iter;

	const value_type & extent1 = *iter;
    T physical1 = extent1.first.physical;
    T logical1 = extent1.second.logical;
    T end1 = physical1 + extent1.second.length;
    if (key_physical < physical1 || key_physical >= end1)
    	return false;

    ret_extent.first.physical = key_physical;
    ret_extent.second.logical = logical1 + (key_physical - physical1);
    ret_extent.second.length = 1;
    ret_extent.second.user_data = extent1.second.user_data;
    return true;
}
#endif // 0

/**
 * find the intersection (matching physical, or both physical and logical) between the two specified extents,
 * insert it into 'result' (with user_data = FC_DEFAULT_USER_DATA) and return true.
 * if no intersections, return false and 'result' will be unchanged.
 *
 * note: if the intersection is only physical,
 * the intersection will contain the appropriate subrange of extent[which] -> logical
 */
template<typename T>
bool fr_map<T>::intersect(const value_type & extent1, const value_type & extent2, ft_match match)
{
    const key_type & key1 = extent1.first;
    const mapped_type & value1 = extent1.second;

    T physical1 = key1.physical;
    T logical1 = value1.logical;
    T end1 = physical1 + value1.length;

    const key_type & key2 = extent2.first;
    const mapped_type & value2 = extent2.second;

    T physical2 = key2.physical;
    T logical2 = value2.logical;
    T end2 = physical2 + value2.length;

    key_type key = { 0 };
    mapped_type value = { 0, 0, (match == FC_PHYSICAL2 ? extent2 : extent1).second.user_data };

    switch (match) {
        case FC_PHYSICAL1:
        case FC_PHYSICAL2:
            if (end1 > physical2 && physical1 < end2)
            {
                key.physical = ff_max2(physical1, physical2);
                value.logical = match == FC_PHYSICAL1 ? logical1 + (key.physical - physical1) : logical2 + (key.physical - physical2);
                value.length = ff_min2(end1, end2) - key.physical;
            } else
                return false;
            break;
        case FC_BOTH:
            if (end1 > physical2 && physical1 < end2
                && logical2 - logical1 == physical2 - physical1)
            {
                key.physical = ff_max2(physical1, physical2);
                value.logical = ff_max2(logical1, logical2);
                value.length = ff_min2(end1, end2) - key.physical;
            } else
                return false;
            break;
        default:
            return false;
    }
    /*
     * insert the intersection in this map, but do NOT try to merge it.
     * merges must NOT be performed because this->remove() and this->remove1()
     * expect intersections to be subsets of extents they come from
     */
    this->insert0(key, value);
    return true;
}

/**
 * find the intersections (matching physical, or both physical and logical) between specified map and extent.
 * insert list of intersections into this and return true.
 * if no intersections, return false and this will be unchanged
 *
 * note: if the intersection is only physical,
 * the intersection will contain the appropriate subrange of {map,extent}[which] -> logical
 */
template<typename T>
bool fr_map<T>::intersect_all(const fr_map<T> & map, const value_type & extent, ft_match match)
{
    const key_type & key1 = extent.first;
    super_const_iterator
		pos   = map.super_type::upper_bound(key1),
		begin = map.super_type::begin(),
		end   = map.super_type::end();
    bool ret = false;

    if (pos != begin) {
        --pos;
        /* pos is now last extent starting before key */
        ret |= intersect(*pos, extent, match);
        ++pos;
    }
    for (; pos != end; ++pos) {
        if (intersect(*pos, extent, match))
            ret = true;
        else
            break;
    }
    return ret;
}


/**
 * find the intersections (matching physical, or both physical and logical) between specified map1 and map2.
 * insert list of intersections into this map and return true.
 * if no intersections, return false and this map will be unchanged
 *
 * note: if the intersection is only physical,
 * the intersection will contain the appropriate subrange of map[match] -> logical
 */
template<typename T>
bool fr_map<T>::intersect_all_all(const fr_map<T> & map1, const fr_map<T> & map2, ft_match match)
{
    ft_size size1 = map1.size(), size2 = map2.size();
    if (size1 == 0 || size2 == 0)
        return false;

    const fr_map<T> & map_iterate = size1 < size2 ? map1 : map2;
    const fr_map<T> & map_other   = size1 < size2 ? map2 : map1;
    if (size1 < size2)
        match = ff_match_transpose(match);

    key_type bound_lo, bound_hi;
    map_other.bounds(bound_lo, bound_hi);

    super_const_iterator
		iter = map_iterate.super_type::upper_bound(bound_lo),
		end  = map_iterate.super_type::lower_bound(bound_hi);
    if (iter != map_iterate.super_type::begin())
        /* iter is now last position less than bound_lo */
        --iter;

    bool ret = false;

    for (; iter != end; ++iter)
        ret |= intersect_all(map_other, *iter, match);
    return ret;
}

/**
 * log fatal error during insert() then terminate the program
 */
template<typename T>
void fr_map<T>::log_fatal_terminate(iterator iter, const key_type & key, const mapped_type & value) {
	ff_log(FC_FATAL, 0,
			"cannot insert extent {phys=%" FT_ULL ", log=%" FT_ULL ", len=%" FT_ULL "} "
			"in %" FT_ULL "-elements map, it already contains "
			"{phys=%" FT_ULL ", log=%" FT_ULL ", len=%" FT_ULL "}",
			(ft_ull)key.physical, (ft_ull)value.logical, (ft_ull)value.length,
			(ft_ull)size(),
			(ft_ull)iter->first.physical, (ft_ull)iter->second.logical, (ft_ull)iter->second.length);
	ft_log_appender::flush_all(FC_FATAL);
	exit(1);
}

/**
 * add a single extent to the fr_map,
 * merging with existing extents where possible
 */
template<typename T>
typename fr_map<T>::iterator fr_map<T>::insert(const key_type & key, const mapped_type & value)
{
    /*
     * pos = "next" extent, i.e.
     * first extent greater than or equal to this key,
     * or end() if no such extent exists
     */
    iterator pos(super_type::lower_bound(key), this);
    fr_extent_relation rel;

    if (pos != end()) {
        // check if extent to be added intersects or touches "next" extent
        rel = compare(pos, key, value);
        if (rel == FC_EXTENT_INTERSECT) {
        	log_fatal_terminate(pos, key, value);
        }
        if (rel == FC_EXTENT_TOUCH_BEFORE || rel == FC_EXTENT_TOUCH_AFTER)
            return merge(pos, key, value);
    }
    if (pos != begin()) {
        // check if extent to be added intersects or touches "previous" extent
        --pos;
        rel = compare(pos, key, value);
        if (rel == FC_EXTENT_INTERSECT) {
        	log_fatal_terminate(pos, key, value);
        }
        if (rel == FC_EXTENT_TOUCH_BEFORE || rel == FC_EXTENT_TOUCH_AFTER)
            return merge(pos, key, value);
        ++pos;
    }
    // just insert the key/value pair
    return insert0(key, value);
}

/**
 * insert a single extent into this fr_map,
 * merging with existing extents where possible.
 * return iterator to inserted/merged extent
 */
template<typename T>
typename fr_map<T>::iterator fr_map<T>::insert(T physical, T logical, T length, ft_size user_data)
{
    key_type key = { physical };
    mapped_type value = { logical, length, user_data };
    return insert(key, value);
}


/** insert another extents map into this fr_map, merging where possible. */
template<typename T>
void fr_map<T>::insert_all(const fr_map<T> & map)
{
    insert_all(map.begin(), map.end());
}

/**
 * remove a part of an existing extent (or a whole existing extent)
 * from this fr_map, splitting the existing extent if needed.
 * throws an assertion failure if extent to remove is not part of existing extents.
 *
 * does not support removing an extent that is part of TWO OR MORE existing extents.
 */
template<typename T>
void fr_map<T>::remove1(const value_type & extent, ft_match match)
{
    ff_assert(!empty());
    const key_type & key = extent.first;
    const mapped_type & value = extent.second;
    /*
     * pos = "next" extent, i.e. first extent greater than key to remove,
     * or end() if no such extent exists
     */
    super_iterator pos = super_type::upper_bound(key);
    ff_assert(pos != super_type::begin());
    /*
     * go back one place. pos will now be "prev",
     * i.e. the last extent lesser than or equal to key to remove
     */
    --pos;
    const key_type & last_key = pos->first;
    mapped_type & last_payload = pos->second;

    T last_physical = last_key.physical;
    T last_logical = last_payload.logical;
    T last_length = last_payload.length;
    ft_size user_data = last_payload.user_data;

    T physical = key.physical;
    T logical = value.logical;
    T length = value.length;

    ff_assert(last_physical <= physical);
    if (match == FC_BOTH) {
        ff_assert(last_logical  <= logical);
    	/* also logical to remove must match */
    	ff_assert(physical - last_physical == logical - last_logical);
    }
    /* last must finish together or after extent to remove */
    ff_assert(last_physical + last_length >= physical + length);

    /* let's consider extents start points */
    if (last_physical < physical) {
        /* first case:
         * "last" existing extent starts before extent to remove
         *    +------------
         *    | to_remove
         *  +-+------------
         *  | last extent
         *  +--------------
         */
        last_payload.length = physical - last_physical;
    } else {
        /* second case:
         * "last" existing extent starts together with extent to remove
         *  +--------------
         *  | to_remove
         *  +--------------
         *  | last extent
         *  +--------------
         */
        super_type::erase(pos);
    }

    /* 2) let's consider extents end points */
    if (last_physical + last_length > physical + length) {
        /* if "last" existing extent ends after extent to remove
         * then we need to insert the remainder of last extent
         *  -----------+
         *   to_remove |
         *  -----------+-+
         *   last extent |
         *  -------------+
         */
        T new_physical = physical + length;
        T new_logical = logical + length;
        T new_length = last_physical + last_length - new_physical;

        insert0(new_physical, new_logical, new_length, user_data);
    } else {
        /* nothing to do */
    }
}


/**
 * remove an existing extent from this fr_map.
 * no need to check for splitting in this method, as it cannot happen:
 * the extent to remove is specified by its iterator,
 * so it must be exactly one of the extents of this map
 */
template<typename T>
void fr_map<T>::remove(iterator iter)
{
	FR_MAP_VALIDATE(iter);
    super_type::erase(iter.super());
}


/**
 * remove a part of an existing extent (or one or more existing extents)
 * from this fr_map, splitting the existing extents if needed.
 */
template<typename T>
void fr_map<T>::remove(const value_type & extent, ft_match match)
{
    fr_map<T> intersect_list;
    intersect_list.intersect_all(*this, extent, match);
    const_iterator iter = intersect_list.begin(), end = intersect_list.end();
    for (; iter != end; ++iter)
        remove1(*iter, match);
}

/**
 * remove a part of an existing extent (or one or more existing extents)
 * from this fr_map, splitting the existing extents if needed.
 */
template<typename T>
void fr_map<T>::remove(T physical, T logical, T length, ft_match match)
{
    key_type key = { physical };
    mapped_type value = { logical, length, FC_DEFAULT_USER_DATA };
    value_type extent(key, value);
    remove(extent, match);
}


/**
 * remove any (partial or full) intersection with existing extents from this fr_map,
 * splitting the existing extents if needed.
 */
template<typename T>
void fr_map<T>::remove_all(const fr_map<T> & map, ft_match match)
{
    if (this == & map) {
        clear();
        return;
    }
    key_type bound_lo, bound_hi;
    bounds(bound_lo, bound_hi);
    super_const_iterator
		iter = map.super_type::upper_bound(bound_lo),
		end = map.super_type::lower_bound(bound_hi);
    if (iter != map.super_type::begin())
        --iter;
    remove_all(iter, end, match);
}


/**
 * remove an initial part of an existing extent from this fr_map.
 * returns iterator to new, smaller extent, or end() if the whole extent was removed
 */
template<typename T>
typename fr_map<T>::iterator fr_map<T>::remove_front(iterator iter, T shrink_length)
{
	FR_MAP_VALIDATE(iter);

    key_type key = iter->first;
    mapped_type value = iter->second;
    const T length = value.length;

    ff_assert(length >= shrink_length);

    /** do not modify the extent in-place: for safety, remove and reinsert it if needed */
    remove(iter);

    if (length == shrink_length) {
        return end();
    }

    key.physical += shrink_length;
    value.logical += shrink_length;
    value.length -= shrink_length;

    return insert0(key, value);
}

/**
 * set this map to a transposed copy of other map,
 * i.e. to a copy where all ->physical and ->logical are swapped
 */
template<typename T>
void fr_map<T>::transpose(const fr_map<T> & other)
{
    clear();
    const_iterator iter = other.begin(), end = other.end();
    for (; iter != end; ++iter) {
        const value_type & extent = *iter;
        insert0(extent.second.logical, extent.first.physical, extent.second.length, extent.second.user_data);
    }
}

/**
 * add a single extent the fr_map
 *
 * does not merge and does not check for merges
 */
template<typename T>
void fr_map<T>::insert0(T physical, T logical, T length, ft_size user_data)
{
    key_type key = { physical };
    mapped_type & value = (*this)[key];
    value.logical = logical;
    value.length = length;
    value.user_data = user_data;
}

/**
 * add a single extent the fr_map
 *
 * does not merge and does not check for merges
 */
template<typename T>
typename fr_map<T>::iterator fr_map<T>::insert0(const key_type & key, const mapped_type & value)
{
    std::pair<super_iterator,bool> inserted = super_type::insert(value_type(key, value));
    ff_map_assert(inserted.second == true);
    return iterator(inserted.first, this);
}

/**
 * insert a single extent the fr_map, hinting that insertion is at map end
 *
 * does not merge and does not check for merges
 */
template<typename T>
void fr_map<T>::append0(T physical, T logical, T length, ft_size user_data)
{
    key_type key = { physical };
    mapped_type value = { logical, length, user_data };
    value_type extent(key, value);

    super_type::insert(super_type::end(), extent);
}



/**
 * insert the whole other vector into this map,
 * shifting extents by effective_block_size_log2,
 * and hinting that insertion is at map end.
 * optimized assuming that 'other' is sorted by physical.
 *
 * does not merge and does not check for merges
 * does not check for overflows
 */
template<typename T>
void fr_map<T>::append0_shift(const fr_vector<ft_uoff> & other, ft_uoff effective_block_size_log2)
{
    typename fr_vector<ft_uoff>::const_iterator iter = other.begin(), end = other.end();
    for (; iter != end; ++iter) {
        const fr_extent<ft_uoff> & extent = * iter;
        append0(extent.physical() >> effective_block_size_log2,
                extent.logical()  >> effective_block_size_log2,
                extent.length()   >> effective_block_size_log2,
                extent.user_data());
    }
}


/**
 * shift and merge specified extent vector
 * into this map, skipping any intersection.
 */
template<typename T>
void fr_map<T>::merge_shift(const fr_vector<ft_uoff> & other, ft_uoff effective_block_size_log2, ft_match match)
{
	if (other.empty()) {
		// nothing to do
	} else if (this->empty()) {
		// easy
		this->append0_shift(other, effective_block_size_log2);
	} else {
		fr_map<T> other_map;

		other_map.append0_shift(other, effective_block_size_log2);

		// delete the intersection between this and other
		if (match == FC_PHYSICAL1)
			other_map.remove_all(* this, FC_PHYSICAL2);
		else
			this->remove_all(other_map, FC_PHYSICAL2);

		// insert the remainder into this
		this->insert_all(other_map);
	}
}


/**
 * makes the physical complement of 'other' vector,
 * i.e. calculates the physical extents NOT used in 'other' vector,
 * shifts them by effective_block_size_log2,
 * and inserts it in this map (with user_data = FC_DEFAULT_USER_DATA)
 *
 * since the file(s) contained in such complementary extents are not known,
 * all calculated extents will have ->logical == ->physical.
 *
 * 'other' must be already sorted by physical!
 * does not merge and does not check for merges
 * does not check for overflows
 */
template<typename T>
void fr_map<T>::complement0_physical_shift(const fr_vector<ft_uoff> & other,
                                  ft_uoff effective_block_size_log2, ft_uoff device_length)
{
    T physical, last;
    ft_size i, n = other.size();

    if (empty())
        last = 0;
    else {
        const value_type & back = *--this->end();
        last = back.first.physical + back.second.length;
    }
    /* loop on 'other' extents */
    for (i = 0; i < n; i++) {
    	const fr_extent<ft_uoff> & curr = other[i];
        physical = curr.physical() >> effective_block_size_log2;

        if (physical == last) {
            /* nothing to do */
        } else if (physical > last) {
            /* add "hole" with logical == physical */
            append0(last, last, physical - last, FC_DEFAULT_USER_DATA);
        } else {
            /* oops.. some programmer really screwed up */
        	const fr_extent<ft_uoff> & prev = other[i-1];
        	ff_log(FC_FATAL, 0, "internal error in ft_map<T>::complement0_physical_shift():");
        	ff_log(FC_FATAL, 0, "\textent[%" FT_ULL "] = {physical = %" FT_ULL ", logical = %" FT_ULL ", length = %" FT_ULL " /* physical end = %" FT_ULL " */} does not end before",
        			(ft_ull) (i-1), (ft_ull) prev.physical(), (ft_ull) prev.logical(), (ft_ull) prev.length(), (ft_ull) (prev.physical() + prev.length()));
        	ff_log(FC_FATAL, 0, "\textent[%" FT_ULL "] = {physical = %" FT_ULL ", logical = %" FT_ULL ", length = %" FT_ULL " /* physical end = %" FT_ULL " */}",
        			(ft_ull) i,     (ft_ull) curr.physical(), (ft_ull) curr.logical(), (ft_ull) curr.length(), (ft_ull) (curr.physical() + curr.length()));
            ff_assert_fail("internal error in ft_map<T>::complement0_physical_shift(): map is not sorted by ->physical()");
        }

        last = physical + (curr.length() >> effective_block_size_log2);
    }
    device_length >>= effective_block_size_log2;
    if (last < device_length) {
        /* add last "hole" with logical == physical */
        append0(last, last, device_length - last, FC_DEFAULT_USER_DATA);
    }
}


/**
 * makes the logical complement of 'other' vector,
 * i.e. calculates the logical extents NOT used in 'other' vector,
 * shifts them by effective_block_size_log2,
 * and inserts it in this map (with user_data = FC_DEFAULT_USER_DATA).
 *
 * since the file(s) contained in such complementary extents are not known,
 * all calculated extents will have ->logical == ->physical.
 *
 * 'other' must be already sorted by logical!
 * does not merge and does not check for merges
 * does not check for overflows
 */
template<typename T>
void fr_map<T>::complement0_logical_shift(const fr_vector<ft_uoff> & other, ft_uoff effective_block_size_log2, ft_uoff device_length)
{
    T logical, last;
    ft_size i, n = other.size();

    if (empty())
        last = 0;
    else {
        const mapped_type & back = (--this->end())->second;
        last = back.logical + back.length;
    }
    /* loop on 'other' extents */
    for (i = 0; i < n; i++) {
    	const fr_extent<ft_uoff> & curr = other[i];
        logical = curr.logical() >> effective_block_size_log2;

        if (logical == last) {
            /* nothing to do */
        } else if (logical > last) {
            /* add "hole" with logical == logical */
            append0(last, last, logical - last, FC_DEFAULT_USER_DATA);
        } else {
            /* oops.. some programmer really screwed up */
        	const fr_extent<ft_uoff> & prev = other[i-1];
        	ff_log(FC_FATAL, 0, "internal error in ft_map<T>::complement0_logical_shift():");
        	ff_log(FC_FATAL, 0, "\textent[%" FT_ULL "] = {physical = %" FT_ULL ", logical = %" FT_ULL ", length = %" FT_ULL " /* logical end = %" FT_ULL " */} does not end before",
        			(ft_ull) (i-1), (ft_ull) prev.physical(), (ft_ull) prev.logical(), (ft_ull) prev.length(), (ft_ull) (prev.logical() + prev.length()));
        	ff_log(FC_FATAL, 0, "\textent[%" FT_ULL "] = {physical = %" FT_ULL ", logical = %" FT_ULL ", length = %" FT_ULL " /* logical end = %" FT_ULL " */}",
        			(ft_ull) i,     (ft_ull) curr.physical(), (ft_ull) curr.logical(), (ft_ull) curr.length(), (ft_ull) (curr.logical() + curr.length()));
            ff_assert_fail("somebody programmed a call to ft_map<T>::complement0_logical_shift() with an argument not sorted by ->logical() !");
        }

        last = logical + (curr.length() >> effective_block_size_log2);
    }
    /*
     * NOTE: right-shifting device_length by effective_block_size_log2
     * forgets any odd-sized last device block
     */
    device_length >>= effective_block_size_log2;
    if (last < device_length) {
        /* add last "hole" with logical == logical */
        append0(last, last, device_length - last, FC_DEFAULT_USER_DATA);
    }
}


/** print map contents to log */
template<typename T>
void fr_map<T>::show(const char * label1, const char * label2, ft_uoff effective_block_size, ft_log_level level) const
{
	fr_extent<T>::show(this->begin(), this->end(), this->size(), label1, label2, effective_block_size, level);
}



FT_NAMESPACE_END
