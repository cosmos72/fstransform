/*
 * vector.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_VECTOR_HH
#define FSTRANSLATE_VECTOR_HH

#include "check.hh"

#include <vector>         // for std::vector<T> */

#include "types.hh"       // for ft_off
#include "fwd.hh"         // for ft_map<T>
#include "extent.hh"      // for ft_extent<T>
#include "extent_list.hh" // for ft_extent_list


FT_NAMESPACE_BEGIN

template<typename T>
class ft_vector : public std::vector<ft_extent<T> >, public ft_extent_list
{
private:
    typedef std::vector<ft_extent<T> > super_type;

public:
    typedef ft_extent_key<T>     key_type;
    typedef ft_extent_payload<T> mapped_type;
    typedef ft_extent<T>         value_type;

    /**
     * append a single extent to this vector.
     *
     * if this vector is not empty
     * and specified extent ->fm_physical is equal to last->fm_physical + last->fm_length
     * and specified extent ->fm_logical  is equal to last->fm_logical  + last->fm_length
     * where 'last' is the last extent in this vector,
     * then merge the two extents
     *
     * otherwise append to this vector a new extent containing specified extent (physical, logical, length)
     */
    void append(T physical, T logical, T length);

    /**
     * append another extent vector to this vector.
     *
     * this method does not merge extents: the two lists of extents will be simply concatenated
     */
    void append(const ft_vector<T> & other);

    /**
     * append an extent map to this vector.
     *
     * this method does not merge extents: the two lists of extents will be simply concatenated
     */
    void append(const ft_map<T> & other);

    /**
     * reorder this vector in-place, sorting by fm_physical
     */
    void sort_by_physical();


    /**
     * append a new extent to this container.
     * container must check if new extent touches the last extent already in the container
     * but is assured (and can assume) than new extent will NEVER touch or intersect other extents.
     *
     * container is required to merge the new extent with the last one it contains, if they touch.
     */
    virtual void extent_append(ft_uoff physical, ft_uoff logical, ft_uoff length);
};

FT_NAMESPACE_END

#ifdef FT_HAVE_EXTERN_TEMPLATE
#  define FT_EXTERN_TEMPLATE_vector(T) class FT_NS ft_vector<T>;
#  define FT_EXTERN_TEMPLATE_vector_hh(prefix, FT_LIST_T_MACRO) FT_LIST_T_MACRO(prefix, FT_EXTERN_TEMPLATE_vector)
   FT_EXTERN_TEMPLATE_DECLARE(FT_EXTERN_TEMPLATE_vector_hh)
#else
#  include "vector.template.hh"
#endif /* FT_EXTERN_TEMPLATE */


#endif /* FSTRANSLATE_VECTOR_HH */
