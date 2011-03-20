/*
 * extent.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_EXTENT_HH
#define FSTRANSLATE_EXTENT_HH

#include "types.hh" // for ft_size

#include <utility>  // for std_pair<T1,T2>

FT_NAMESPACE_BEGIN


/** possible sources ('from') of a ft_work<T>::move*() or io::ft_io::copy() operation */
enum ft_from { FC_FROM_DEV = 0, FC_FROM_STORAGE = 1 };

/** possible destinations ('to') of a ft_work<T>::move*() or io::ft_io::copy() operation */
enum ft_to   { FC_TO_DEV = 0,   FC_TO_STORAGE = 2 };

/** direction of ft_work<T>::move*() and io::ft_io::copy() operations */
enum ft_dir {
    FC_DEV2STORAGE = FC_FROM_DEV    |FC_TO_STORAGE,
    FC_DEV2DEV     = FC_FROM_DEV    |FC_TO_DEV,
    FC_STORAGE2DEV = FC_FROM_STORAGE|FC_TO_DEV,
    FC_STORAGE2STORAGE = FC_FROM_STORAGE|FC_TO_STORAGE, /* not considered useful, except as 'invalid dir' marker */
};

FT_INLINE ft_from ff_from(ft_dir dir) { return (ft_from)(dir & 1); }
FT_INLINE ft_to   ff_to  (ft_dir dir) { return (ft_to)  (dir & 2); }

FT_INLINE bool ff_is_from_dev(ft_dir dir) { return ff_from(dir) == FC_FROM_DEV; }
FT_INLINE bool ff_is_to_dev  (ft_dir dir) { return ff_to  (dir) == FC_TO_DEV;   }


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
struct ft_extent_key
{
    mutable T physical;  /* physical offset in bytes for the start of the extent from the beginning of the device */
};


// compare two ft_extent_key:s. needed by ft_map<T> to order the keys!
template<typename T>
FT_INLINE bool operator<(const ft_extent_key<T> & key1, const ft_extent_key<T> & key2)
{
    return key1.physical < key2.physical;
}


template<typename T>
struct ft_extent_payload
{
    T logical;   /**< logical offset in bytes for the start of the extent from the beginning of the file */
    T length;    /**< length in bytes for this extent */
    ft_size user_data; /**< caller can store its own data here. used to track whether this extents contains LOOP-FILE blocks or DEVICE blocks */
};

enum { FC_DEFAULT_USER_DATA = 0 }; /**< stored into ft_extent_payload::user_data when caller cannot supply a value */


template<typename T>
class ft_extent : public std::pair<ft_extent_key<T>, ft_extent_payload<T> >
{
private:
    typedef ft_extent_key<T>                   key_type;
    typedef ft_extent_payload<T>               mapped_type;

public:
    typedef std::pair<key_type, mapped_type>   super_type;

    /* default constructor */
    ft_extent() : super_type() { }

    /* copy constructor */
    ft_extent(const ft_extent<T> & other) : super_type(other) { }

    /* copy constructor accepting base class */
    ft_extent(const super_type & other) : super_type(other) { }

    /* assignment operator */
    const super_type & operator=(const ft_extent<T> & other)
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
        FT_INLINE bool operator()(const ft_extent<T> & e1, const ft_extent<T> & e2)
        {
            return e1.physical() < e2.physical();
        }
    };

    class comparator_logical
    {
    public:
        FT_INLINE bool operator()(const ft_extent<T> & e1, const ft_extent<T> & e2)
        {
            return e1.logical() < e2.logical();
        }
    };

    class reverse_comparator_length
    {
    public:
        FT_INLINE bool operator()(const ft_extent<T> & e1, const ft_extent<T> & e2)
        {
            return e1.length() > e2.length();
        }
    };
};


enum ft_extent_relation {
    FC_EXTENT_BEFORE,      /**< first extent is before second, either they do not touch or they touch but their logical or user_data do not match */
    FC_EXTENT_TOUCH_BEFORE,/**< first extent is before second, they touch exactly and their logical and user_data match too (so they can be merged) */
    FC_EXTENT_INTERSECT,   /**< first and second extent intersect each other by at least one (physical) byte */
    FC_EXTENT_TOUCH_AFTER, /**< first extent is after  second, they touch exactly and their logical and user_data match too (so they can be merged) */
    FC_EXTENT_AFTER,       /**< first extent is after  second, either they do not touch or they touch but their logical or user_data do not match */
};


FT_NAMESPACE_END

#endif /* FSTRANSLATE_EXTENT_HH */
