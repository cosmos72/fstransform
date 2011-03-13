/*
 * map.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_MAP_HH
#define FSTRANSLATE_MAP_HH

#include "check.hh"

#include <map>       // for std::map<K,V> */

#include "types.hh"  // for ft_uoff */
#include "fwd.hh"    // for ft_map<T> and ft_vector<T> forward declarations */
#include "extent.hh" // for ft_extent_key<T>, ft_extent_payload<T> */

FT_NAMESPACE_BEGIN

template<typename T>
class ft_map : private std::map<ft_extent_key<T>, ft_extent_payload<T> >
{
private:
    typedef std::map<ft_extent_key<T>, ft_extent_payload<T> > super_type;

public:
    typedef typename super_type::key_type       key_type;
    typedef typename super_type::mapped_type    mapped_type;
    typedef typename super_type::value_type     value_type;
    typedef typename super_type::iterator       iterator;
    typedef typename super_type::const_iterator const_iterator;

private:

    // report an assertion failure (and terminate the program)
    void assert_fail(const_iterator where, const key_type & problem_key, const mapped_type & problem_value,
                     const char * fmt, ...) const;

    /** compare two key+value extents and find relative position */
    static ft_extent_relation compare(const_iterator pos1,
                                      const_iterator pos2);

    /** compare two key+value extents and find relative position */
    static ft_extent_relation compare(const_iterator pos1,
                                      const key_type & key2, const mapped_type & value2);

    /** compare two key+value extents and find relative position */
    static ft_extent_relation compare(const key_type & key1, const mapped_type & value1,
                                      const key_type & key2, const mapped_type & value2);

    /**
     * add a single extent the ft_map
     *
     * WARNING: does not merge and does not check for merges
     */
    void insert0(T physical, T logical, T length, ft_size user_data);

    /**
     * add a single extent the ft_map, hinting that insertion is at map end
     *
     * WARNING: does not merge and does not check for merges
     */
    void append0(T physical, T logical, T length, ft_size user_data);

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
    iterator merge0(iterator pos1, const key_type & key2, const mapped_type & value2);

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
     * merge0(iterator, const key_type &, const mapped_type &)
     */
    iterator merge0(iterator pos1, iterator pos2);

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
    iterator merge(iterator pos1, const key_type & key2, const mapped_type & value2);

    /**
     * remove a part of an existing extent (or a whole existing extent)
     * from this ft_map, splitting the existing extent if needed.
     * throws an assertion failure if extent to remove is not part of existing extents.
     *
     * WARNING: does not support removing an extent that is part of TWO OR MORE existing extents.
     */
    void remove1(const value_type & extent);

public:

    // construct empty ft_map
    ft_map();

    // duplicate a ft_map, i.e. initialize this ft_map as a copy of other.
    ft_map(const ft_map<T> & other);

    // destroy ft_map
    ~ft_map();

    // return iterator to beginning of this map
    super_type::begin;

    // return iterator one past the end of this map
    super_type::end;

    // return true is this map is empty, i.e. if it size() == 0
    super_type::empty;

    // return number of extents in this map
    super_type::size;

    // clear this map, i.e. erase all elements
    super_type::clear;

    // copy ft_map, i.e. set this ft_map contents as a copy of other's contents.
    const ft_map<T> & operator=(const ft_map<T> & other);

    // swap this map contents with other map
    void swap(ft_map<T> & other);

    /**
     * returns the minimum physical and the maximum physical+length in this map.
     * if this map is empty, return {0,0}
     */
    void bounds(key_type & min_key, key_type & max_key) const;

    /**
     * find the intersection (matching physical and logical) between the two specified extents,
     * insert it into this map and return true.
     * if no intersections, return false and this will be unchanged
     */
    bool intersect(const value_type & extent1, const value_type & extent2);

    /**
     * find the intersections (matching physical and logical) between specified map and extent.
     * insert list of intersections into this map and return true.
     * if no intersections, return false and this will be unchanged
     */
    bool intersect_all(const ft_map<T> & map, const value_type & extent);

    /**
     * find the intersections (matching physical and logical) between specified map1 and map2.
     * insert list of intersections into this map and return true.
     * if no intersections, return false and this map will be unchanged
     */
    bool intersect_all_all(const ft_map<T> & map1, const ft_map<T> & map2);

    /**
     * add a single extent the ft_map
     *
     * WARNING: does not merge and does not check for merges
     */
    void insert0(const key_type & key, const mapped_type & value);

    /**
     * insert a single extent into this ft_map,
     * merging with existing extents where possible.
     * return iterator to inserted/merged extent
     */
    iterator insert(const key_type & key, const mapped_type & value);

    /**
     * insert a single extent into this ft_map,
     * merging with existing extents where possible.
     * return iterator to inserted/merged extent
     *
     * simply calls insert(key_type, value_type)
     */
    iterator insert(const value_type & extent) { return insert(extent.first, extent.second); }

    /**
     * insert a single extent into this ft_map,
     * merging with existing extents where possible.
     * return iterator to inserted/merged extent.
     *
     * simply calls insert(key_type, value_type)
     */
    iterator insert(T physical, T logical, T length, ft_size user_data);

    /**
     * remove an existing extent from this ft_map.
     * no need to check for splitting in this method, as it cannot happen:
     * the extent to remove is specified by its iterator,
     * so it must be exactly one of the extents of this map
     */
    void remove(iterator iter);

    /**
     * remove a part of an existing extent (or one or more existing extents)
     * from this ft_map, splitting the existing extents if needed.
     */
    void remove(const value_type & extent);

    /**
     * remove any (partial or full) intersection with existing extents from this ft_map,
     * splitting the existing extents if needed.
     */
    template<typename const_iter>
    void remove_all(const_iter iter, const_iter end);


    /**
     * remove any (partial or full) intersection with existing extents from this ft_map,
     * splitting the existing extents if needed.
     */
    void remove_all(const ft_map<T> & map);



    /**
     * remove an initial part of an existing extent from this ft_map.
     * returns iterator to new, smaller extent, or end() if the whole extent was removed
     */
    iterator shrink_front(iterator iter, T shrink_length);








    /**
     * insert the whole other vector into this map,
     * shifting extents by effective_block_size_log2,
     * and hinting that insertion is at map end.
     * optimized assuming that 'other' is sorted by physical.
     *
     * WARNING: does not merge and does not check for merges
     * WARNING: does not check for overflows
     */
    void append0_shift(const ft_vector<ft_uoff> & other, ft_uoff effective_block_size_log2);

    /**
     * makes the physical complement of 'other' vector,
     * i.e. calculates the physical extents NOT used in 'other' vector,
     * shifts them by effective_block_size_log2,
     * and inserts it in this map.
     *
     * since the file(s) contained in such complementary extents are not known,
     * all calculated extents will have ->logical == ->physical.
     *
     * WARNING: 'other' must be already sorted by physical!
     * WARNING: does not merge and does not check for merges
     * WARNING: does not check for overflows
     */
    void complement0_physical_shift(const ft_vector<ft_uoff> & other, ft_uoff effective_block_size_log2, ft_uoff device_length);

    /**
     * makes the logical complement of 'other' vector,
     * i.e. calculates the logical extents NOT used in 'other' vector,
     * shifts them by effective_block_size_log2,
     * and inserts it in this map.
     *
     * since the file(s) contained in such complementary extents are not known,
     * all calculated extents will have ->logical == ->physical.
     *
     * WARNING: 'other' must be already sorted by physical!
     * WARNING: does not merge and does not check for merges
     * WARNING: does not check for overflows
     */
    void complement0_logical_shift(const ft_vector<ft_uoff> & other, ft_uoff effective_block_size_log2, ft_uoff device_length);
};


FT_NAMESPACE_END


#ifdef FT_HAVE_EXTERN_TEMPLATE
#  define FT_TEMPLATE_map_hh(ft_prefix, T) ft_prefix class FT_NS ft_map< T >;
   FT_TEMPLATE_DECLARE(FT_TEMPLATE_map_hh)
#else
#  include "map.t.hh"
#endif /* FT_HAVE_EXTERN_TEMPLATE */



#endif /* FSTRANSLATE_MAP_HH */
