/*
 * extent.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSREMAP_EXTENT_HH
#define FSREMAP_EXTENT_HH

#include "types.hh" // for ft_size

#include <utility>  // for std_pair<T1,T2>

FT_NAMESPACE_BEGIN


/** possible sources ('from') of a fr_work<T>::move*() or io::fr_io::copy() operation */
enum fr_from { FC_FROM_DEV = 0, FC_FROM_STORAGE = 1 };

/** possible destinations ('to') of a fr_work<T>::move*() or io::fr_io::copy() operation */
enum fr_to   { FC_TO_DEV = 0,   FC_TO_STORAGE = 2 };

/** direction of fr_work<T>::move*() and io::fr_io::copy() operations */
enum fr_dir {
    FC_DEV2DEV     = FC_FROM_DEV    |FC_TO_DEV,
    FC_STORAGE2DEV = FC_FROM_STORAGE|FC_TO_DEV,
    FC_DEV2STORAGE = FC_FROM_DEV    |FC_TO_STORAGE,
    FC_INVALID2INVALID = FC_FROM_STORAGE|FC_TO_STORAGE, /* 'invalid dir' marker */
};

FT_INLINE fr_from ff_from(fr_dir dir) { return (fr_from)(dir & 1); }
FT_INLINE fr_to   ff_to  (fr_dir dir) { return (fr_to)  (dir & 2); }

FT_INLINE bool ff_is_from_dev(fr_dir dir) { return ff_from(dir) == FC_FROM_DEV; }
FT_INLINE bool ff_is_to_dev  (fr_dir dir) { return ff_to  (dir) == FC_TO_DEV;   }


/**
 * ft_match enumerates the kinds of matches/intersections between extents or maps:
 * FC_PHYSICAL1: match/intersection of extents ->physical ranges. select subrange of extent1 -> logical
 * FC_PHYSICAL2: match/intersection of extents ->physical ranges. select subrange of extent2 -> logical
 * FC_BOTH: simultaneous match/intersection of extents ->physical and ->logical ranges
 */
enum ft_match { FC_BOTH = 0, FC_PHYSICAL1 = 1, FC_PHYSICAL2 = -1, };

/** swap FC_PHYSICAL1 <-> FC_PHYSICAL2 and FC_LOGICAL1 <-> FC_LOGICAL2 */
FT_INLINE ft_match ff_match_transpose(ft_match match) { return (ft_match)-match; }


template<typename T>
struct fr_extent_key
{
    mutable T physical;  /* physical offset in bytes for the start of the extent from the beginning of the device */
};


// compare two fr_extent_key:s. needed by fr_map<T> to order the keys!
template<typename T>
FT_INLINE bool operator<(const fr_extent_key<T> & key1, const fr_extent_key<T> & key2)
{
    return key1.physical < key2.physical;
}


template<typename T>
struct fr_extent_payload
{
    T logical;   /**< logical offset in bytes for the start of the extent from the beginning of the file */
    T length;    /**< length in bytes for this extent */
    ft_size user_data; /**< caller can store its own data here. used to track whether this extents contains LOOP-FILE blocks or DEVICE blocks */
};

enum {
    FC_DEFAULT_USER_DATA = 0, /**< stored into fr_extent_payload::user_data when caller cannot supply a value */
    FC_EXTENT_ZEROED = 1,     /**< stored into fr_extent_payload::user_data to indicate that on-disk extent only contains zeroes */
};


template<typename T>
class fr_extent : public std::pair<fr_extent_key<T>, fr_extent_payload<T> >
{
private:
    typedef fr_extent_key<T>                   key_type;
    typedef fr_extent_payload<T>               mapped_type;

public:
    typedef std::pair<key_type, mapped_type>   super_type;

    /* default constructor */
    fr_extent() : super_type() { }

    /* copy constructor */
    fr_extent(const fr_extent<T> & other) : super_type(other) { }

    /* copy constructor accepting base class */
    fr_extent(const super_type & other) : super_type(other) { }

    /* assignment operator */
    const super_type & operator=(const fr_extent<T> & other)
    {
        super_type::operator=(other);
        return *this;
    }

    /* assignment operator accepting base class */
    const super_type & operator=(const super_type & other)
    {
        super_type::operator=(other);
        return *this;
    }

    FT_INLINE T & physical() { return this->first.physical; }
    FT_INLINE T & logical()  { return this->second.logical; }
    FT_INLINE T & length()   { return this->second.length;  }

    FT_INLINE T physical() const { return this->first.physical; }
    FT_INLINE T logical()  const { return this->second.logical; }
    FT_INLINE T length()   const { return this->second.length;  }

    FT_INLINE ft_size & user_data()       { return this->second.user_data;  }
    FT_INLINE ft_size   user_data() const { return this->second.user_data;  }

    void clear() {
        physical() = logical() = length() = 0;
        user_data() = 0;
    }

    class comparator_physical
    {
    public:
        FT_INLINE bool operator()(const fr_extent<T> & e1, const fr_extent<T> & e2)
        {
            return e1.physical() < e2.physical();
        }
    };

    class comparator_logical
    {
    public:
        FT_INLINE bool operator()(const fr_extent<T> & e1, const fr_extent<T> & e2)
        {
            return e1.logical() < e2.logical();
        }
    };

    class reverse_comparator_length
    {
    public:
        FT_INLINE bool operator()(const fr_extent<T> & e1, const fr_extent<T> & e2)
        {
            return e1.length() > e2.length();
        }
    };
};


enum fr_extent_relation {
    FC_EXTENT_BEFORE,      /**< first extent is before second, either they do not touch or they touch but their logical or user_data do not match */
    FC_EXTENT_TOUCH_BEFORE,/**< first extent is before second, they touch exactly and their logical and user_data match too (so they can be merged) */
    FC_EXTENT_INTERSECT,   /**< first and second extent intersect each other by at least one (physical) byte */
    FC_EXTENT_TOUCH_AFTER, /**< first extent is after  second, they touch exactly and their logical and user_data match too (so they can be merged) */
    FC_EXTENT_AFTER,       /**< first extent is after  second, either they do not touch or they touch but their logical or user_data do not match */
};


FT_NAMESPACE_END

#endif /* FSREMAP_EXTENT_HH */
