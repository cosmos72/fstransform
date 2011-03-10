/*
 * work.t.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */
#include "first.hh"

#include <cerrno>         // for errno, EFBIG
#include <cstdio>         // for fprintf(), stdout, stderr

#include "assert.hh"      // for ff_assert()
#include "traits.hh"      // for FT_TYPE_TO_UNSIGNED(T) macro
#include "log.hh"         // for ff_log()
#include "vector.hh"      // for ft_vector<T>
#include "map.hh"         // for ft_map<T>
#include "pool.hh"        // for ft_pool<T>
#include "util.hh"        // for ff_pretty_size()
#include "work.hh"        // for ff_dispatch(), ft_work<T>
#include "io/io.hh"       // for ft_io

FT_NAMESPACE_BEGIN


template<typename T>
void ft_work<T>::show(const char * label, ft_uoff effective_block_size, const ft_map<T> & map, ft_level level)
{
    if (!ff_log_is_enabled(level))
        return;

    typename ft_map<T>::const_iterator iter = map.begin(), end = map.end();
    ft_size n = map.size();

    if (iter != end) {
        ff_log(level, 0, "# %"FS_ULL" extent%s in %s, effective block size = %"FS_ULL,
               (FT_ULL) n, (n == 1 ? "" : "s"), label, (FT_ULL) effective_block_size);
        ff_log(level, 0, "# extent \t\tphysical\t\t logical\t  length\tuser_data");

        for (ft_size i = 0; iter != end; ++iter, ++i) {
            ff_log(level, 0, "%8"FS_ULL"\t%12"FS_ULL"\t%12"FS_ULL"\t%8"FS_ULL"\t(%"FS_ULL")", (FT_ULL)i,
                    (FT_ULL) iter->first.physical,
                    (FT_ULL) iter->second.logical,
                    (FT_ULL) iter->second.length,
                    (FT_ULL) iter->second.user_data);
        }
    } else {
        ff_log(level, 0, "# no extents in %s", label);
    }
    ff_log(level, 0, "");
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
    show(FT_IO_NS ft_io::label[FT_IO_NS ft_io::FC_LOOP_FILE], eff_block_size, loop_map);


    /* compute in-place the union of LOOP-FILE extents and FREE-SPACE extents */
    loop_file_extents.append_all(free_space_extents);
    /*
     * sort the extents union by physical.
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
    show(FT_IO_NS ft_io::label[FT_IO_NS ft_io::FC_DEVICE], eff_block_size, dev_map);

    if (err == 0)
        this->io = & io;

    return err;
}





enum {
    FC_DEVICE = FT_IO_NS ft_io::FC_DEVICE,
    FC_LOOP_FILE = FT_IO_NS ft_io::FC_LOOP_FILE
};

/** core of transformation algorithm */
template<typename T>
int ft_work<T>::run()
{
    typedef typename ft_map<T>::iterator map_iterator;
    typedef typename ft_map<T>::const_iterator map_const_iterator;

    int err = 0;

    do {
        ft_uoff eff_block_size_log2 = io->effective_block_size_log2();
        ft_uoff eff_block_size = (ft_uoff)1 << eff_block_size_log2;

        /* show LOOP-HOLES extents before allocation, sorted by physical */
        show("LOOP-HOLES (INITIAL)", eff_block_size, loop_holes);

        /* algorithm: 2) re-number used device blocks, setting logical to numbers from loop-file holes
         * do not greedily use low hole numbers:
         * a) prefer hole numbers equal to device block number: they produce a block
         *    already in its final destination (marked with @@)
         * b) spread the remaining numbers (uniformly?) across rest of holes
         */
        ft_map<T> dev_renumbered;
        /* how: intersect dev_map and loop_holes and put result into dev_renumbered */
        dev_renumbered.intersect_all_all(dev_map, loop_holes);

        /* show DEVICE extents already in their final destination, sorted by physical */
        show("DEVICE (INVARIANT)", eff_block_size, dev_renumbered);

        /* remove from dev_map all the extents moved to dev_renumbered */
        dev_map.remove_all(dev_renumbered);
        /*
         * remove from loop_holes all extents in dev_renumbered, then clear the latter:
         * its extents are already in their final destination (they are INVARIANT)
         * -> no work on them
         */
        loop_holes.remove_all(dev_renumbered);
        dev_renumbered.clear();

        /*
         * 3) mark as INVARIANT the (logical) blocks in loop-file already in their final destination
         * also compute loop_map length...
         */
        map_iterator iter = loop_map.begin(), tmp, end = loop_map.end();
        ft_uoff work_length = 0;
        while (iter != end) {
            if (iter->first.physical == iter->second.logical) {
                dev_renumbered.insert(*iter);
                tmp = iter;
                ++iter;
                loop_map.remove(tmp);
            } else {
                /* also compute loop_map length... */
                work_length += iter->second.length;
                /*
                 * also prepare for item 4) "merge renumbered device blocks with loop-file blocks"
                 * i.e. remember who's who
                 */
                iter->second.user_data = FC_LOOP_FILE;
                ++iter;
            }
        }
        /* show LOOP-FILE (INVARIANT) blocks, sorted by physical */
        show("LOOP-FILE (INVARIANT)", eff_block_size, dev_renumbered);
        dev_renumbered.clear();

        /* show LOOP-HOLES after allocating invariant DEVICE extents, sorted by physical */
        show("LOOP-HOLES (AFTER INVARIANT)", eff_block_size, loop_holes);

        /* order loop_holes by length */
        ft_pool<T> loop_holes_pool(loop_holes);

        /* allocate loop_holes extents to store dev_map extents using a best-fit strategy */
        loop_holes_pool.allocate_all(dev_map, dev_renumbered);

        /* show DEVICE-RENUMBERED extents sorted by physical */
        show("DEVICE (RENUMBERED)", eff_block_size, dev_renumbered);
        /* show LOOP-HOLES extents after allocation, sorted by physical */
        show("LOOP-HOLES (FINAL)", eff_block_size, loop_holes);

        if (!dev_map.empty()) {
            ff_log(FC_FATAL, 0, "internal error: there are extents in DEVICE not fitting DEVICE. this is impossible! I give up");
            /* show DEVICE-NOTFITTING extents sorted by physical */
            show("DEVICE (NOTFITTING)", eff_block_size, dev_map, FC_NOTICE);
            err = ENOSPC;
            break;
        }
        /*
         * 4) merge renumbered device blocks with loop-file blocks (remember who's who)
         */
        iter = dev_renumbered.begin();
        end = dev_renumbered.end();
        for (; iter != end; ++iter) {
            work_length += iter->second.length;
            iter->second.user_data = FC_DEVICE;
            loop_map.insert0(iter->first, iter->second);
        }
        dev_renumbered.clear();

        /* show DEVICE + LOOP-FILE extents after merge, sorted by physical */
        show("DEVICE + LOOP-FILE (MERGED)", eff_block_size, loop_map);

        double pretty_len = 0.0;
        const char * pretty_unit = ff_pretty_size(work_length << eff_block_size_log2, & pretty_len);

        ff_log(FC_NOTICE, 0, "analysis completed: %.2f %sbytes must be relocated", pretty_len, pretty_unit);

    } while (0);

    return err;
}


FT_NAMESPACE_END
