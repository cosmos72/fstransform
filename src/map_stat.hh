/*
 * map_stat.hh
 *
 *  Created on: Mar 12, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_MAP_STAT_HH
#define FSTRANSFORM_MAP_STAT_HH

#include "map.hh"    // for ft_map<T>


FT_NAMESPACE_BEGIN

template<typename T>
class ft_map_stat : public ft_map<T>
{
private:
    typedef ft_map<T> super_type;
    typedef typename ft_map<T>::iterator iterator;

    T fm_total_count; /**< total length (number of blocks) in this map extents */
    T fm_used_count;  /**< used  length (number of blocks) in this map extents */

public:
    // construct empty ft_map_stat
    ft_map_stat();

    // destructor
    ~ft_map_stat();

    // duplicate a ft_map_stat, i.e. initialize this ft_map as a copy of other.
    ft_map_stat(const ft_map<T> & other);
    // duplicate a ft_map_stat, i.e. initialize this ft_map as a copy of other.
    ft_map_stat(const ft_map_stat<T> & other);

    // copy ft_map_stat, i.e. set ft_map_stat ft_map contents as a copy of other's contents.
    const ft_map_stat<T> & operator=(const ft_map<T> & other);
    // copy ft_map_stat, i.e. set ft_map_stat ft_map contents as a copy of other's contents.
    const ft_map_stat<T> & operator=(const ft_map_stat<T> & other);

    /** clear this ft_map_stat. also sets total_count, used_count and free_count to zero */
    void clear();

    /** same as super_type::insert(T,T,T,ft_size), but also updates used_count() */
    FT_INLINE iterator stat_insert(T physical, T logical, T length, ft_size user_data)
    {
        fm_used_count += length;
        return super_type::insert(physical, logical, length, user_data);
    }

    /** same as super_type::remove(iterator), but also updates used_count() */
    FT_INLINE void stat_remove(iterator iter)
    {
        fm_used_count -= iter->second.length;
        super_type::remove(iter);
    }

    FT_INLINE T total_count() const { return fm_total_count; }
    FT_INLINE T used_count() const { return fm_used_count; }
    FT_INLINE T free_count() const { return fm_total_count - fm_used_count; }

    FT_INLINE void total_count(T n) { fm_total_count = n; }

    FT_INLINE void used_count(T n) { fm_used_count = n; }
    FT_INLINE void used_count_add(T n) { fm_used_count += n; }
    FT_INLINE void used_count_sub(T n) { fm_used_count -= n; }
};

FT_NAMESPACE_END


#ifdef FT_HAVE_EXTERN_TEMPLATE
#  define FT_TEMPLATE_map_stat_hh(ft_prefix, T) ft_prefix class FT_NS ft_map_stat< T >;
   FT_TEMPLATE_DECLARE(FT_TEMPLATE_map_stat_hh)
#else
#  include "map_stat.t.hh"
#endif /* FT_HAVE_EXTERN_TEMPLATE */

#endif /* FSTRANSFORM_MAP_STAT_HH */
