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


FT_NAMESPACE_BEGIN

template<typename T>
class ft_vector : public std::vector<ft_extent<T> >
{
private:
    typedef std::vector<ft_extent<T> > super_type;

public:
    typedef ft_extent_key<T>      key_type;
    typedef ft_extent_payload<T>  mapped_type;

    typedef typename super_type::value_type value_type;

    /**
     * append a single extent to this vector.
     *
     * if this vector is not empty
     * and specified extent ->physical is equal to last->physical + last->length
     * and specified extent ->logical  is equal to last->logical  + last->length
     * where 'last' is the last extent in this vector,
     * then merge the two extents
     *
     * otherwise append to this vector a new extent containing specified extent (physical, logical, length)
     */
    void append(T physical, T logical, T length, ft_size user_data);

    /**
     * append a single extent to this vector.
     *
     * if this vector is not empty
     * and specified extent ->physical is equal to last->physical + last->length
     * and specified extent ->logical  is equal to last->logical  + last->length
     * where 'last' is the last extent in this vector,
     * then merge the two extents
     *
     * otherwise append to this vector a new extent containing specified extent (physical, logical, length, user_data)
     */
    FT_INLINE void append(const typename value_type::super_type & extent)
    {
        append(extent.first.physical, extent.second.logical, extent.second.length, extent.second.user_data);
    }

    /**
     * append another extent vector to this vector.
     *
     * this method does not merge extents: the two lists of extents will be simply concatenated
     */
    void append_all(const ft_vector<T> & other);

    /**
     * reorder this vector in-place, sorting by physical
     */
    void sort_by_physical();
};

FT_NAMESPACE_END


#ifdef FT_HAVE_EXTERN_TEMPLATE
#  define FT_TEMPLATE_vector_hh(ft_prefix, T) ft_prefix class FT_NS ft_vector< T >;
   FT_TEMPLATE_DECLARE(FT_TEMPLATE_vector_hh)
#else
#  include "vector.t.hh"
#endif /* FT_HAVE_EXTERN_TEMPLATE */



#endif /* FSTRANSLATE_VECTOR_HH */
