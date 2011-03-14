/*
 * transform.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_WORK_HH
#define FSTRANSFORM_WORK_HH

#include "types.hh"     // for ft_uoff
#include "map_stat.hh"  // for ft_map_stat<T>
#include "log.hh"       // for ft_log_level
#include "io/io.hh"     // for ft_io


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
    ft_map_stat<T> dev_map, dev_free_map, storage_map;
    T work_count; /**< number of blocks to be relocated */

    FT_IO_NS ft_io * io;

    /** cannot call copy constructor */
    ft_work(const ft_work<T> &);

    /** cannot call assignment operator */
    const ft_work<T> & operator=(const ft_work<T> &);

    void show(const char * label1, const char * label2, ft_uoff effective_block_size, const ft_map<T> & map, ft_log_level level = FC_TRACE);

    /**
     * call check(io) to ensure that io.dev_length() can be represented by T,
     * then checks that I/O is open.
     * if success, stores a reference to I/O object.
     */
    int init(FT_IO_NS ft_io & io);

    /**
     * analysis phase of transformation algorithm,
     * must be executed before create_secondary_storage() and relocate()
     *
     * given LOOP-FILE extents and FREE-SPACE extents as ft_vectors<ft_uoff>,
     * compute LOOP-FILE extents map and DEVICE in-use extents map
     * and stores them into this->loop_map and this->dev_map.
     *
     * assumes that vectors are ordered by extent->logical, and modifies them
     * in place: vector contents will be UNDEFINED when this method returns.
     *
     * implementation: to compute this->dev_map, performs in-place the union of specified
     * loop_file_extents and free_space_extents, then sorts in-place and complements such union.
     */
    int analyze(ft_vector<ft_uoff> & loop_file_extents,
                ft_vector<ft_uoff> & free_space_extents);

    /**
     * fill io->primary_storage() with DEVICE extents to be actually used as PRIMARY-STORAGE
     * (already computed into dev_free_map by analyze())
     *
     * if only a fraction of available PRIMARY-STORAGE will be actually used,
     * exploit a ft_pool<T> to select the largest contiguous extents.
     *
     * updates dev_free_map to contain the PRIMARY-STORAGE extents actually used.
     */
    void fill_io_primary_storage(ft_uoff primary_len);

    /**
     * creates on-disk secondary storage, used as (small) backup area during relocate().
     * must be executed before relocate()
     */
    int create_storage();

    /** core of transformation algorithm, actually moves DEVICE blocks */
    int relocate();

    typedef typename ft_map<T>::iterator map_iterator;
    typedef typename ft_map<T>::const_iterator map_const_iterator;

public:
    /** default constructor */
    ft_work();

    /** destructor. calls cleanup() */
    ~ft_work();

    /**
     * high-level do-everything method. calls in sequence run() and cleanup().
     * return 0 if success, else error.
     */
    static int main(ft_vector<ft_uoff> & loop_file_extents,
                    ft_vector<ft_uoff> & free_space_extents, FT_IO_NS ft_io & io);


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
     * main transformation algorithm.
     * calls in sequence init(), analyze(), create_secondary_storage() and relocate()
     */
    int run(ft_vector<ft_uoff> & loop_file_extents,
            ft_vector<ft_uoff> & free_space_extents, FT_IO_NS ft_io & io);

    /** performs cleanup. called by destructor, you can also call it explicitly after (or instead of) run()  */
    void cleanup();
};


FT_NAMESPACE_END


#ifdef FT_HAVE_EXTERN_TEMPLATE
#  define FT_TEMPLATE_work_hh(ft_prefix, T)     ft_prefix class FT_NS ft_work< T >;
   FT_TEMPLATE_DECLARE(FT_TEMPLATE_work_hh)
#else
#  include "work.t.hh"
#endif /* FT_HAVE_EXTERN_TEMPLATE */



#endif /* FSTRANSFORM_WORK_HH */
