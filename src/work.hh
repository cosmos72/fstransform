/*
 * transform.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_WORK_HH
#define FSTRANSFORM_WORK_HH

#include "types.hh"   /* for ft_off                     */
#include "ctx.hh"     /* for ft_ctx                     */
#include "map.hh"     /* for ft_map<T>                  */

/**
 * instantiate and run ft_work<T>::work() with the smallest T that can represent device length.
 * return 0 if success, else error.
 *
 * implementation:
 * iterates on all known types T and, if ff_check<T>() succeeds,
 * calls ff_work<T>() passing to it device length and file descriptors
 */
int ff_work_dispatch(const ft_ctx & ctx);



template<typename T>
class ft_work_ctx
{
public:
    enum { FC_FILE_COUNT = ft_ctx::FC_FILE_COUNT };

protected:
    T fm_dev_length;
    const int * fm_file_fd;

public:
    /* constructor. takes a reference to ctx.file_fds(), does NOT copy them! */
    ft_work_ctx(T dev_length, const ft_ctx & ctx);

    /** destructor. does nothing */
    ~ft_work_ctx();

    /** return true if this ft_ctx is currently (and correctly) open */
    bool is_open() const;

    /** close file descriptors */
    void close();

    /** return device length */
    FT_INLINE const T dev_length() const { return fm_dev_length; }

    /** return file descriptors */
    FT_INLINE const int * file_fds() const { return fm_file_fd; }
};






/**
 * class doing the bulk of transformation work.
 *
 * contains the algorithm to move LOOP-FILE around
 * until its physical layout matches its logical layout.
 * at that point, DEVICE will have been fully transformed.
 */
template<typename T>
class ft_work
{
private:
    ft_map<T> loop_map, loop_holes_map, dev_map;

    /** cannot call copy constructor */
    ft_work(const ft_work &);

    /** cannot call assignment operator */
    const ft_work & operator=(const ft_work &);

    /**
     * read extents from LOOP-FILE and ZERO-FILE and use them to fill ft_work<T>
     * return 0 if success, else error
     */
    int init(const ft_work_ctx<T> & ctx);

    /** core of transformation algorithm */
    int run();

public:
    /**
     * default constructor.
     *
     * implementation note: compiler-generated default constructor would be OK,
     * but our (intentionally uncallable) copy constructor declaration inhibits its generation
     */
    ft_work();

    /**
     * destructor
     *
     * implementation note: compiler-generated destructor would be OK,
     * we implement it only for symmetry with default constructor
     */
    ~ft_work();


    /**
     * call in sequence init() and run().
     * return 0 if success, else error.
     *
     * in case of error in init() it does not call run()
     */
    int work(const ft_work_ctx<T> & ctx);
};


#ifdef FT_HAVE_EXTERN_TEMPLATE

#  define FT_EXTERN_TEMPLATE_work_ctx(T) class ft_work_ctx<T>;
#  define FT_EXTERN_TEMPLATE_work(T)     class ft_work<T>;

#  define FT_EXTERN_TEMPLATE_work_hh(ft_prefix, ft_list_t) \
        ft_list_t(ft_prefix, FT_EXTERN_TEMPLATE_work_ctx) \
        ft_list_t(ft_prefix, FT_EXTERN_TEMPLATE_work)

   FT_EXTERN_TEMPLATE_DECLARE(FT_EXTERN_TEMPLATE_work_hh)
#else
#  include "work.template.hh"
#endif /* FT_EXTERN_TEMPLATE */


#endif /* FSTRANSFORM_WORK_HH */
