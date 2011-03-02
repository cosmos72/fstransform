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
 * instantiate and run ft_work<T>::work(io) with the smallest T that can represent device length.
 * return 0 if success, else error.
 *
 * implementation:
 * iterates on all known types T and, if ff_check<T>() succeeds,
 * calls ff_work<T>() passing to it device length and file descriptors
 */
int ff_work_dispatch(FT_IO_NS ft_io & io);



template<typename T>
class ft_work_ctx
{
private:
    T fm_dev_length;
    FT_IO_NS ft_io & fm_io;

public:
    /** constructor. stores a reference to ft_io in this ft_work_ctx */
    ft_work_ctx(T dev_length, FT_IO_NS ft_io & io);

    /** destructor does nothing */
    // ~ft_work_ctx();

    /** return true if this ft_work_ctx is currently (and correctly) open */
    FT_INLINE bool is_open() const { return fm_dev_length != 0 && fm_io.is_open(); }

    /** close file descriptors */
    void close();

    /** return device length */
    FT_INLINE T dev_length() const { return fm_dev_length; }

    /** return I/O implementation */
    FT_INLINE FT_IO_NS ft_io & io() const { return fm_io; }
};






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
    ft_work_ctx<T> * ctx;

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
    static int main(ft_work_ctx<T> & work_ctx);


    /** return true if this ft_work is currently (and correctly) initialized */
    bool is_initialized();

    /**
     * read extents from LOOP-FILE and ZERO-FILE and use them to fill ft_work<T>
     * return 0 if success, else error
     */
    int init(ft_work_ctx<T> & work_ctx);

    /** core of transformation algorithm */
    int run();

    /** performs cleanup. called by destructor, you can also call it explicitly after (or instead of) run()  */
    void quit();
};


FT_NAMESPACE_END


#ifdef FT_HAVE_EXTERN_TEMPLATE

#  define FT_EXTERN_TEMPLATE_work_ctx(T) class FT_NS ft_work_ctx<T>;
#  define FT_EXTERN_TEMPLATE_work(T)     class FT_NS ft_work<T>;

#  define FT_EXTERN_TEMPLATE_work_hh(ft_prefix, ft_list_t) \
        ft_list_t(ft_prefix, FT_EXTERN_TEMPLATE_work_ctx) \
        ft_list_t(ft_prefix, FT_EXTERN_TEMPLATE_work)

   FT_EXTERN_TEMPLATE_DECLARE(FT_EXTERN_TEMPLATE_work_hh)
#else
#  include "work.template.hh"
#endif /* FT_EXTERN_TEMPLATE */



#endif /* FSTRANSFORM_WORK_HH */
