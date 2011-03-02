/*
 * map.template.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#include "first.hh"

#include <utility>       // for std::make_pair() */

#include "assert.hh"     // for ff_assert macro */
#include "map.hh"        // for ft_map<T> */
#include "vector.hh"     // for ft_vector<T> */

FT_NAMESPACE_BEGIN

#define ff_map_assert ff_assert

// construct empty ft_map
template<typename T>
ft_map<T>::ft_map() : super_type() { }


// duplicate a ft_map, i.e. initialize this ft_map as a copy of other.
template<typename T>
ft_map<T>::ft_map(const ft_map<T> & other) : super_type(other)
{ }

// copy ft_map, i.e. set this ft_map contents as a copy of other's contents.
template<typename T>
const ft_map<T> & ft_map<T>::operator=(const ft_map<T> & other)
{
    super_type::operator=(other);
    return * this;
}

// destroy ft_map
template<typename T>
ft_map<T>::~ft_map()
{ }



/** compare two key+value extents and find relative position */
template<typename T>
ft_extent_relation ft_map<T>::compare(const_iterator pos1,
                                      const_iterator pos2)
{
    return compare(pos1->first, pos1->second, pos2->first, pos2->second);
}

/** compare two key+value extents and find relative position */
template<typename T>
ft_extent_relation ft_map<T>::compare(const_iterator pos1,
                                      const key_type & key2, const mapped_type & value2)
{
    return compare(pos1->first, pos1->second, key2, value2);
}

/** compare two key+value extents and find relative position */
template<typename T>
ft_extent_relation ft_map<T>::compare(const key_type & key1, const mapped_type & value1,
                                      const key_type & key2, const mapped_type & value2)
{
    T physical1 = key1.fm_physical, logical1 = value1.fm_logical, length1 = value1.fm_length;
    T physical2 = key2.fm_physical, logical2 = value2.fm_logical, length2 = value2.fm_length;
    ft_extent_relation rel;

    if (physical1 < physical2) {
        if (physical1 + length1 == physical2 && logical1 + length1 == logical2)
            rel = FC_EXTENT_TOUCH_BEFORE;
        else if (physical1 + length1 <= physical2)
            rel = FC_EXTENT_BEFORE;
        else
            rel = FC_EXTENT_INTERSECT;
    } else if (physical1 == physical2) {
        rel = FC_EXTENT_INTERSECT;
    } else /* physical1 > physical2 */ {
        if (physical2 + length2 == physical1 && logical2 + length2 == logical1)
            rel = FC_EXTENT_TOUCH_BEFORE;
        else if (physical2 + length2 <= physical1)
            rel = FC_EXTENT_BEFORE;
        else
            rel = FC_EXTENT_INTERSECT;
    }
    return rel;
}


/**
 * merge extent (which can belong to this ft_map) into specified position.
 * the two extents MUST exactly touch!
 * i.e. their ft_extent_relation MUST be either FC_EXTENT_TOUCH_BEFORE or FC_EXTENT_TOUCH_AFTER
 *
 * return iterator to merged position.
 *
 * WARNING: this is an internal method and should ONLY be invoked by merge(),
 *          as it does not handle chains of merges, as merge() does instead.
 *          Again: call merge(), not this method.
 */
template<typename T>
typename ft_map<T>::iterator ft_map<T>::merge0(iterator pos1, const key_type & key2, const mapped_type & value2)
{
    ft_extent_relation rel = compare(pos1, key2, value2);
    mapped_type & value1 = pos1->second;
    T length = value1.fm_length + value2.fm_length;

    if (rel == FC_EXTENT_TOUCH_BEFORE) {
        // modify extent in-place
        value1.fm_length = length;

    } else if (rel == FC_EXTENT_TOUCH_AFTER) {
        /*
         * we cannot modify std::map keys in-place!
         * so we need to erase pos1 and reinsert it with updated key
         *
         * implementation:
         * go one place forward before erasing, in worst case we will land into end()
         * which is still a valid iterator (ok, it cannot be dereferenced)
         * then erase original position,
         * finally reinsert merged extent, giving hint as where to insert it
         */
        iterator tmp = pos1;
        ++pos1;
        super_type::erase(tmp);

        mapped_type value_to_insert = { value2.fm_logical, length };

        pos1 = super_type::insert(pos1, std::make_pair(key2, value_to_insert));

    } else {
        /* must not happen! trigger an assertion failure. */
        ff_map_assert(rel == FC_EXTENT_TOUCH_BEFORE || rel == FC_EXTENT_TOUCH_AFTER);
    }
    return pos1;
}

/**
 * merge together two extents (which must belong to this ft_map).
 * the two extents MUST exactly touch!
 * i.e. their ft_extent_relation MUST be either FC_EXTENT_TOUCH_BEFORE or FC_EXTENT_TOUCH_AFTER
 *
 * return iterator to merged position.
 *
 * WARNING: this is an internal method and should ONLY be invoked by merge(),
 *          as it does not handle chains of merges, as merge() does instead.
 *          Again: call merge(), not this method.
 *
 * this method exists because it is simpler to implement than
 * merge0(iterator, const key_type &, const mapped_type &),
 * as it does not need to work around the limitation that std::map keys
 * cannot be modified in-place
 */
template<typename T>
typename ft_map<T>::iterator ft_map<T>::merge0(iterator pos1, iterator pos2)
{
    ft_extent_relation rel = compare(pos1, pos2);
    mapped_type & value1 = pos1->second;
    mapped_type & value2 = pos2->second;

    T length = value1.fm_length + value2.fm_length;

    if (rel == FC_EXTENT_TOUCH_BEFORE) {
        // modify first extent in-place, erase second
        value1.fm_length = length;
        super_type::erase(pos2);
        return pos1;

    } else if (rel == FC_EXTENT_TOUCH_AFTER) {
        // modify second extent in-place, erase first
        value2.fm_length = length;
        super_type::erase(pos1);
        return pos2;

    } else {
        /* must not happen! trigger an assertion failure. */
        ff_map_assert(rel == FC_EXTENT_TOUCH_BEFORE || rel == FC_EXTENT_TOUCH_AFTER);
    }
    return pos1;
}


/**
 * merge extent (which must NOT belong to this ft_map) into specified ft_map position.
 * the two extents MUST exactly touch!
 * i.e. their ft_extent_relation MUST be either FC_EXTENT_TOUCH_BEFORE or FC_EXTENT_TOUCH_AFTER
 *
 * return iterator to merged position.
 *
 * this method automatically performs chain of merges if needed:
 * for example, if extent 2 is merged in a ft_map containing 0..1 and 3..5,
 * this method will first merge 0..1 with 2, obtaining 0..2,
 * then it will realize that result can be merged with 3..5
 * and it will also perform this second merging, obtaining 0..5
 */
template<typename T>
typename ft_map<T>::iterator ft_map<T>::merge(iterator pos1, const key_type & key2, const mapped_type & value2)
{
    ft_extent_relation rel = compare(pos1, key2, value2);

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

/**
 * add a single extent to the ft_map,
 * merging with existing extents where possible,
 */
template<typename T>
typename ft_map<T>::iterator ft_map<T>::insert(const key_type & key, const mapped_type & value)
{
    /*
     * pos = "next" extent, i.e.
     * first extent greater than or equal to this key,
     * or end() if no such extent exists
     */
    iterator pos = super_type::lower_bound(key);
    ft_extent_relation rel;

    if (pos != end()) {
        // check if extent to be added intersects or touches "next" extent
        rel = compare(pos, key, value);
        if (rel == FC_EXTENT_TOUCH_BEFORE || rel == FC_EXTENT_TOUCH_AFTER)
            return merge(pos, key, value);
    }
    if (pos != begin()) {
        // check if extent to be added intersects or touches "previous" extent
        --pos;
        rel = compare(pos, key, value);
        if (rel == FC_EXTENT_TOUCH_BEFORE || rel == FC_EXTENT_TOUCH_AFTER)
            return merge(pos, key, value);
        ++pos;
    }
    // just insert the key/value pair
    return super_type::insert(pos, std::make_pair(key, value));
}


/**
 * insert the whole other map into this map.
 * extents will be merged where possible
 */
template<typename T>
void ft_map<T>::insert(const ft_map<T> & other)
{
    if (this == & other)
        return; // WARNING: should not happen, but it's a recoverable error

    const_iterator o_iter = other.begin(), o_end = other.end();
    for (; o_iter != o_end; ++o_iter)
        insert(o_iter->first, o_iter->second);
}


/**
 * insert the whole other vector into this map.
 * extents will be merged where possible
 */
template<typename T>
void ft_map<T>::insert(const ft_vector<T> & other)
{
    typename ft_vector<T>::const_iterator o_iter = other.begin(), o_end = other.end();
    for (; o_iter != o_end; ++o_iter)
        insert(o_iter->first, o_iter->second);
}


/**
 * insert the whole other vector into this map, hinting that insertion is at map end.
 * optimized assuming that 'other' is sorted by physical.
 *
 * WARNING: does not merge and does not check for merges
 */
template<typename T>
void ft_map<T>::append0(const ft_vector<T> & other)
{
    typename ft_vector<T>::const_iterator o_iter = other.begin(), o_end = other.end();
    for (; o_iter != o_end; ++o_iter)
        super_type::insert(end(), * o_iter);
}

/**
 * insert a single extent the ft_map, hinting that insertion is at map end
 *
 * WARNING: does not merge and does not check for merges
 */
template<typename T>
void ft_map<T>::append0(T physical, T logical, T length)
{
    key_type key = { physical };
    mapped_type value = { logical, length };
    super_type::insert(end(), std::make_pair(key, value));
}


/**
 * makes the complement of 'other' vector,
 * i.e. calculates the extents NOT used by the extents in 'other' vector
 * and inserts it in this map.
 *
 * since the file(s) contained in such complementary extents are not known,
 * all calculated extents will have fm_logical == fm_physical.
 *
 * WARNING: 'other' must be already sorted by physical!
 * WARNING: does not merge and does not check for merges
 */
template<typename T>
void ft_map<T>::complement0(const ft_vector<T> & other, T device_length)
{
    T physical, last = 0;
    ft_size i, n = other.size();

    /* loop on 'other' extents */
    for (i = 0; i < n; i++) {
        physical = other[i].physical();

        if (physical == last) {
            /* nothing to do */
        } else if (physical > last) {
            /* add "hole" with fm_logical == fm_physical */
            append0(last, last, physical - last);
        } else {
            /* oops.. some programmer really screwed up */
            ff_assert("internal error! somebody programmed a call to complement0() without sorting the vector first!" == 0);
        }

        last = physical + other[i].length();
    }
    if (last < device_length) {
        /* add last "hole" with fm_logical == fm_physical */
        append0(last, last, device_length - last);
    }
}

FT_NAMESPACE_END
