/*
 * vector.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSLATE_VECTOR_HH
#define FSTRANSLATE_VECTOR_HH

#include "check.hh"

#include "types.hh"  /* for ft_off */
#include "fwd.hh"    /* for ft_extent */
#include "extent.hh" /* for ft_extent */

#include <vector>    /* for std::vector<T> */

template<typename T>
class ft_vector : public std::vector<ft_extent<T> >
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
     * reorder this vector in-place, sorting by fm_physical
     */
    void sort_by_physical();
};

#ifdef FT_HAVE_EXTERN_TEMPLATE
#  define FT_EXTERN_TEMPLATE_vector(T) class ft_vector<T>;
#  define FT_EXTERN_TEMPLATE_vector_hh(prefix, FT_LIST_T_MACRO) FT_LIST_T_MACRO(prefix, FT_EXTERN_TEMPLATE_vector)
   FT_EXTERN_TEMPLATE_DECLARE(FT_EXTERN_TEMPLATE_vector_hh)
#else
#  include "vector.template.hh"
#endif /* FT_EXTERN_TEMPLATE */


#endif /* FSTRANSLATE_VECTOR_HH */
