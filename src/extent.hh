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
    T fm_physical;  /* physical offset in bytes for the start of the extent from the beginning of the device */
};


// compare two ft_extent_key:s. needed by ft_map to order the keys!
template<typename T>
FT_INLINE bool operator<(const ft_extent_key<T> & key1, const ft_extent_key<T> & key2)
{
    return key1.fm_physical < key2.fm_physical;
}


template<typename T>
struct ft_extent_payload
{
    T fm_logical;   /**< logical offset in bytes for the start of the extent from the beginning of the file */
    T fm_length;    /**< length in bytes for this extent */
};



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

    FT_INLINE T & physical() { return this->first.fm_physical; }
    FT_INLINE T & logical()  { return this->second.fm_logical; }
    FT_INLINE T & length()   { return this->second.fm_length;  }

    FT_INLINE T physical() const { return this->first.fm_physical; }
    FT_INLINE T logical() const  { return this->second.fm_logical; }
    FT_INLINE T length() const   { return this->second.fm_length;  }


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
    FC_EXTENT_BEFORE,      /**< first key+extent is before second, either they do not touch or they touch but their fm_logical does not match */
    FC_EXTENT_TOUCH_BEFORE,/**< first key+extent is before second, they touch exactly and their fm_logical matches (so they can be merged) */
    FC_EXTENT_INTERSECT,   /**< first and second key+extent intersect each other by at least one byte */
    FC_EXTENT_TOUCH_AFTER, /**< first key+extent is after  second, they touch exactly and their fm_logical matches (so they can be merged) */
    FC_EXTENT_AFTER,       /**< first key+extent is after  second, either they do not touch or they touch but their fm_logical does not match */
};


FT_NAMESPACE_END

#endif /* FSTRANSLATE_EXTENT_HH */
