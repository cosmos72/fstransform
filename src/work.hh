/*
 * transform.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_WORK_HH
#define FSTRANSFORM_WORK_HH

#include "types.hh"     // for ft_uoff
#include "io/io.hh"     // for ft_io
#include "map.hh"       // for ft_map<T>


FT_NAMESPACE_BEGIN

/**
 * class doing the core of transformation work.
 *
 * contains the algorithm to move LOOP-FILE around
 * until its physical layout matches its logical layout.
 * at that point, DEVICE will have been fully transformed.
 */
template<typename T>
class ft_work
{
private:
    ft_map<T> dev_map, loop_map;
    FT_IO_NS ft_io * io;

    /** cannot call copy constructor */
    ft_work(const ft_work<T> &);

    /** cannot call assignment operator */
    const ft_work<T> & operator=(const ft_work<T> &);

public:
    /** default constructor */
    ft_work();

    /** destructor. calls quit() */
    ~ft_work();

    /**
     * high-level do-everything method. calls in sequence init(), run() and quit().
     * return 0 if success, else error.
     */
    static int main(ft_vector<ft_uoff> & loop_file_extents,
                    ft_vector<ft_uoff> & free_space_extents, FT_IO_NS ft_io & io);


    /** return true if this ft_work is currently (and correctly) initialized */
    bool is_initialized();

    /**
     *  check if LOOP-FILE and DEVICE in-use extents can be represented
     *  by ft_map<T>. takes into account the fact that all extents
     *  physical, logical and length will be divided by effective block size
     *  before storing them into ft_map<T>.
     *
     *  return 0 for check passes, else error (usually EFBIG)
     */
    static int check(const FT_IO_NS ft_io & io);

    /**
     * given LOOP-FILE extents and FREE-SPACE extents as ft_vectors<ft_uoff>,
     * compute LOOP-FILE extents and DEVICE in-use extents maps and insert them
     * into the maps this->dev_map and this->loop_map.
     *
     * assumes that vectors are ordered by extent->logical, and modifies them
     * in place: vector contents will be UNDEFINED when this method returns.
     *
     * also calls check(io) to be sure that allextents can be represented by ft_map<T>
     *
     * implementation: to compute this->dev_map, performs in-place the union of specified
     * loop_file_extents and free_space_extents, then sorts in-place and complements such union.
     */
    int init(ft_vector<ft_uoff> & loop_file_extents,
             ft_vector<ft_uoff> & free_space_extents, FT_IO_NS ft_io & io);


    /** core of transformation algorithm */
    int run();

    /** performs cleanup. called by destructor, you can also call it explicitly after (or instead of) run()  */
    void quit();
};


FT_NAMESPACE_END


#ifdef FT_HAVE_EXTERN_TEMPLATE
#  define FT_TEMPLATE_work_hh(ft_prefix, T)     ft_prefix class FT_NS ft_work< T >;
   FT_TEMPLATE_DECLARE(FT_TEMPLATE_work_hh)
#else
#  include "work.template.hh"
#endif /* FT_HAVE_EXTERN_TEMPLATE */



#endif /* FSTRANSFORM_WORK_HH */
