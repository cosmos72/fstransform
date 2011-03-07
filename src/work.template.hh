/*
 * work.template.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */
#include "first.hh"

#include <cerrno>         // for errno, EFBIG              */
#include <cstdio>         // for fprintf(), stdout, stderr */
#include <limits>         // for std::numeric_limits<T>    */

#include "traits.hh"      // for FT_TYPE_TO_UNSIGNED(T) macro
#include "vector.hh"      // for ft_vector<T>
#include "map.hh"         // for ft_map<T>
#include "fail.hh"        // for ff_fail()
#include "io/io.hh"       // for ft_io
#include "work.hh"        // for ff_work_dispatch(), ft_work<T>

FT_NAMESPACE_BEGIN


template<typename const_iter>
static void ff_map_show(const char * label, ft_uoff effective_block_size, const_iter iter, const_iter end)
{
    if (iter != end) {
        fprintf(stdout, "# extents in %s, effective block size = %lu\n# extent \t\t logical\t\tphysical\t      length\n",
                label, (unsigned long) effective_block_size);

        for (ft_size i = 0; iter != end; ++iter, ++i) {
#ifdef FT_HAVE_LONG_LONG
            fprintf(stdout, "%8lu\t%16llu\t%16llu\t%12llu\n", (unsigned long)i,
                    (unsigned long long) iter->second.fm_logical,
                    (unsigned long long) iter->first.fm_physical,
                    (unsigned long long) iter->second.fm_length);
#else
            fprintf(stdout, "%8lu\t%16lu\t%16lu\t%12lu\n", (unsigned long)i,
                    (unsigned long) iter->second.fm_logical,
                    (unsigned long) iter->first.fm_physical,
                    (unsigned long) iter->second.fm_length);
#endif /* FT_HAVE_LONG_LONG */
        }
        fprintf(stdout, "\n");
    } else {
        fprintf(stdout, "# no extents in %s\n", label);
    }
}


template<typename T>
static void ff_map_show(const char * label, ft_uoff effective_block_size, const ft_vector<T> & vector)
{
    ff_map_show(label, effective_block_size, vector.begin(), vector.end());
}

template<typename T>
static void ff_map_show(const char * label, ft_uoff effective_block_size, const ft_map<T> & map)
{
    ff_map_show(label, effective_block_size, map.begin(), map.end());
}



/** default constructor */
template<typename T>
ft_work<T>::ft_work()
    : dev_map(), loop_map(), loop_holes(), io(NULL)
{ }


/** destructor. calls quit() */
template<typename T>
ft_work<T>::~ft_work()
{
    quit();
}


/**
 * high-level do-everything method. calls in sequence init(), run() and quit().
 * return 0 if success, else error.
 */
template<typename T>
int ft_work<T>::main(ft_vector<ft_uoff> & loop_file_extents,
                     ft_vector<ft_uoff> & free_space_extents, FT_IO_NS ft_io & io)
{
    ft_work<T> worker;
    int err = worker.init(loop_file_extents, free_space_extents, io);
    if (err == 0)
        err = worker.run();

    // worker.quit() is called automatically by destructor, no need to call explicitly

    return err;
}

/** return true if this ft_work is currently (and correctly) initialized */
template<typename T>
bool ft_work<T>::is_initialized()
{
    return io != NULL && io->is_open();
}


/** performs cleanup. called by destructor, you can also call it explicitly after (or instead of) run()  */
template<typename T>
void ft_work<T>::quit()
{
    dev_map.clear();
    loop_map.clear();
    loop_holes.clear();
    io = NULL; // do not delete or close ft_io, we did not create it!
}


/**
 *  check if LOOP-FILE and DEVICE in-use extents can be represented
 *  by ft_map<T>. takes into account the fact that all extents
 *  physical, logical and length will be divided by effective block size
 *  before storing them into ft_map<T>.
 *
 *  return 0 for check passes, else error (usually EFBIG)
 */
template<typename T>
int ft_work<T>::check(const FT_IO_NS ft_io & io)
{
    ft_uoff eff_block_size_log2 = io.effective_block_size_log2();
    ft_uoff dev_length = io.dev_length();

    ft_uoff block_count = dev_length >> eff_block_size_log2;
    // possibly narrowing cast, let's check for overflow
    T n = (T) block_count;
    int err = 0;
    if (n < 0 || block_count != (ft_uoff) n)
        /* overflow! */
        err = EFBIG;
    return err;
}


/**
 * given LOOP-FILE extents and FREE-SPACE extents as ft_vectors<ft_uoff>,
 * compute LOOP-FILE extents and DEVICE in-use extents maps and insert them
 * into the maps this->dev_map and this->loop_map.
 *
 * assumes that vectors are ordered by extent->logical, and modifies them
 * in place: vector contents will be UNDEFINED when this method returns.
 *
 * WARNING: this method does not check for overflows, you must call
 *  check_extents_map() to check if extents can be represented by ft_map<T>
 *
 * implementation: to compute this->dev_map, performs in-place the union of specified
 * loop_file_extents and free_space_extents, then sorts in-place and complements such union.
 */
template<typename T>
int ft_work<T>::init(ft_vector<ft_uoff> & loop_file_extents,
                     ft_vector<ft_uoff> & free_space_extents, FT_IO_NS ft_io & io)
{
    int err = 0;
    if (is_initialized())
        err = EISCONN; // already initialized !
    else if ((err = check(io)) != 0)
        ; // extents cannot be represented by ft_map<T> !
    else if (!io.is_open())
        err = ENOTCONN; // I/O is not open !

    if (err != 0)
        return err;

    // cleanup in case io != NULL or dev_map, loop_map are not empty
    quit();

    ft_uoff eff_block_size_log2 = io.effective_block_size_log2();
    ft_uoff eff_block_size      = (ft_uoff)1 << eff_block_size_log2;
    ft_uoff dev_length          = io.dev_length();

    /* algorithm: 1) find loop-file (logical) holes and store them in loop_holes */
    loop_holes.complement0_logical_shift(loop_file_extents, eff_block_size_log2, dev_length);

    loop_file_extents.sort_by_physical();
    loop_map.append0_shift(loop_file_extents, eff_block_size_log2);

    /* show LOOP-FILE extents sorted by physical */
    ff_map_show(FT_IO_NS ft_io::label[FT_IO_NS ft_io::FC_LOOP_FILE], eff_block_size, loop_map);


    /* compute in-place the union of LOOP-FILE extents and FREE-SPACE extents */
    loop_file_extents.append_all(free_space_extents);
    /*
     * sort the extents union by fm_physical.
     * needed by dev_map.complement0_physical_shift() immediately below
     */
    loop_file_extents.sort_by_physical();
    /*
     * algorithm: 0) compute DEVICE extents
     *
     * how: compute physical complement of all LOOP-FILE and FREE_SPACE extents
     * and assume they are used by DEVICE for its file-system
     */
    dev_map.complement0_physical_shift(loop_file_extents, eff_block_size_log2, dev_length);

    /* show DEVICE extents sorted by physical */
    ff_map_show(FT_IO_NS ft_io::label[FT_IO_NS ft_io::FC_DEVICE], eff_block_size, dev_map);

    if (err == 0)
        this->io = & io;

    return err;
}



/** core of transformation algorithm */
template<typename T>
int ft_work<T>::run()
{
    typedef typename ft_map<T>::iterator ft_map_iterator;
    typedef typename ft_map<T>::const_iterator ft_map_const_iterator;

    {
        /* algorithm: 2) re-number used device blocks with numbers from loop-file holes
         * do not greedily use low hole numbers:
         * a) prefer hole numbers equal to device block number: they produce a block
         *    already in its final destination (marked with @@)
         * b) spread the remaining numbers (uniformly?) across rest of holes
         */
        ft_map<T> dev_renumbered;
        /* how: intersect dev_map and loop_holes and put result into dev_renumbered */
        dev_renumbered.intersect_all_all(dev_map, loop_holes);

        ft_uoff eff_block_size = (ft_uoff)1 << io->effective_block_size_log2();

        /* show LOOP-HOLES extents sorted by physical */
        ff_map_show("LOOP-HOLES", eff_block_size, loop_holes);
        /* show DEVICE-RENUMBERED extents sorted by physical */
        ff_map_show("DEVICE-RENUMBERED", eff_block_size, dev_renumbered);
    }

    return 0;
}


FT_NAMESPACE_END
