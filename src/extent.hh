/*
 * extent.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_EXTENT_HH
#define FSTRANSLATE_EXTENT_HH

#include "check.hh"

#include <utility>  // for std_pair<T1,T2> */

FT_NAMESPACE_BEGIN

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
