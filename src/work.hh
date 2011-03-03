/*
 * transform.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_WORK_HH
#define FSTRANSFORM_WORK_HH

#include "types.hh"   // for ft_off
#include "io/io.hh"   // for ft_io
#include "map.hh"     // for ft_map<T>


FT_NAMESPACE_BEGIN

/**
 * instantiate and run ft_work<T>::main(io) with the smallest T that can represent device blocks count.
 * return 0 if success, else error.
 *
 * implementation:
 * iterates on all known types T and, if ft_work<T>::init(io) succeeds,
 * calls ff_work<T>::run(), then ff_work<T>::quit()
 */
int ff_work_dispatch(FT_IO_NS ft_io & io);



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
    static int main(FT_IO_NS ft_io & io);


    /** return true if this ft_work is currently (and correctly) initialized */
    bool is_initialized();

    /**
     * checks if device blocks count can be represented by T (else immediately returns EFBIG),
     * then read extents from LOOP-FILE and ZERO-FILE and use them to fill ft_work<T>
     * return 0 if success, else error
     */
    int init(FT_IO_NS ft_io & io);

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
