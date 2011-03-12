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
#include "arch/mem.hh"    // for ff_arch_mem_system_free()

FT_NAMESPACE_BEGIN

enum {
    FC_DEVICE = FT_IO_NS ft_io::FC_DEVICE,
    FC_LOOP_FILE = FT_IO_NS ft_io::FC_LOOP_FILE
};

char const* const* const label = FT_IO_NS ft_io::label;


template<typename T>
void ft_work<T>::show(const char * label, ft_uoff effective_block_size, const ft_map<T> & map, ft_log_level level)
{
    ft_log_level header_level = level <= FC_TRACE ? FC_DEBUG : level;

    if (!ff_log_is_enabled(header_level) && !ff_log_is_enabled(level))
        return;

    typename ft_map<T>::const_iterator iter = map.begin(), end = map.end();
    ft_size n = map.size();

    if (iter != end) {
        ff_log(header_level, 0, "# %4"FS_ULL" extent%s in %s",
               (FT_ULL) n, (n == 1 ? " " : "s"), label);

        if (ff_log_is_enabled(level)) {
            ff_log(level, 0, "# extent \t\tphysical\t\t logical\t  length\tuser_data");

            for (ft_size i = 0; iter != end; ++iter, ++i) {
                ff_log(level, 0, "%8"FS_ULL"\t%12"FS_ULL"\t%12"FS_ULL"\t%8"FS_ULL"\t(%"FS_ULL")", (FT_ULL)i,
                        (FT_ULL) iter->first.physical,
                        (FT_ULL) iter->second.logical,
                        (FT_ULL) iter->second.length,
                        (FT_ULL) iter->second.user_data);
            }
        }
    } else {
        ff_log(header_level, 0, "#   no extents in %s", label);
    }
    ff_log(level, 0, "");
}



/** default constructor */
template<typename T>
ft_work<T>::ft_work()
    : dev_map(), dev_free_map(), storage_map(), work_count(0)
{ }


/** destructor. calls quit() */
template<typename T>
ft_work<T>::~ft_work()
{
    quit();
}


/** performs cleanup. called by destructor, you can also call it explicitly after (or instead of) run()  */
template<typename T>
void ft_work<T>::quit()
{
    dev_map.clear();
    dev_free_map.clear();
    storage_map.clear();
    work_count = 0;
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
    return worker.run(loop_file_extents, free_space_extents, io);

    // worker.quit() is called automatically by destructor, no need to call explicitly
}

/** full transformation algorithm */
template<typename T>
int ft_work<T>::run(ft_vector<ft_uoff> & loop_file_extents,
                    ft_vector<ft_uoff> & free_space_extents, FT_IO_NS ft_io & io)
{
    int err;
    (err = init(io)) == 0
        && (err = analyze(loop_file_extents, free_space_extents)) == 0
        && (err = create_storage()) == 0
        && (err = relocate()) == 0;
    return err;
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
        err = EOVERFLOW;
    return err;
}

/**
 * call check(io) to ensure that io.dev_length() can be represented by T,
 * then checks that I/O is open.
 * if success, stores a reference to I/O object.
 */
template<typename T>
int ft_work<T>::init(FT_IO_NS ft_io & io)
{
    int err;
    do {
        if ((err = check(io)) != 0)
            break;
        if (!io.is_open()) {
            err = ENOTCONN; // I/O is not open !
            break;
        }
        this->io = & io;
    } while (0);

    return err;
}




static ft_size ff_mem_page_size()
{
    enum {
        FC_PAGE_SIZE_IF_UNKNOWN = 16*1024 // assume 16k as a safe upper bound
    };

    static ft_size page_size = 0;
    if (page_size == 0) {
        if ((page_size = FT_ARCH_NS ff_arch_mem_page_size()) == 0)
            page_size = FC_PAGE_SIZE_IF_UNKNOWN;
    }
    return page_size;
}

/* trim extent on both ends to align it to page_size. return trimmed extent length (can be zero) */
template<typename T>
T ff_extent_align(typename ft_map<T>::value_type & extent, T page_size_blocks_m_1)
{
    T physical = extent.first.physical;
    T end = physical + extent.second.length;
    T new_physical = (physical + page_size_blocks_m_1) & ~page_size_blocks_m_1;
    T new_end = end & ~page_size_blocks_m_1;
    if (new_end <= new_physical)
        return extent.second.length = 0;

    extent.first.physical = new_physical;
    extent.second.logical += new_physical - physical;
    return extent.second.length = new_end - new_physical;
}


/**
 * analysis phase of transformation algorithm,
 * must be executed before create_secondary_storage() and relocate()
 *
 * given LOOP-FILE extents and FREE-SPACE extents as ft_vectors<ft_uoff>,
 * compute LOOP-FILE extents map and DEVICE in-use extents map
 *
 * assumes that vectors are ordered by extent->logical, and modifies them
 * in place: vector contents will be UNDEFINED when this method returns.
 *
 * implementation: to compute this->dev_map, performs in-place the union of specified
 * loop_file_extents and free_space_extents, then sorts in-place and complements such union.
 */
template<typename T>
int ft_work<T>::analyze(ft_vector<ft_uoff> & loop_file_extents,
                        ft_vector<ft_uoff> & free_space_extents)
{
    // cleanup in case dev_map, dev_free_map or storage_map are not empty, or work_count != 0
    quit();

    ft_map<T> loop_map, loop_holes_map, renumbered_map;

    ft_uoff eff_block_size_log2 = io->effective_block_size_log2();
    ft_uoff eff_block_size      = (ft_uoff)1 << eff_block_size_log2;
    ft_uoff dev_length          = io->dev_length();
    /*
     * algorithm: 1) find LOOP-FILE (logical) holes, i.e. LOOP-HOLES,
     * and store them in holes_map
     * note: all complement maps have physical == logical
     */
    loop_holes_map.complement0_logical_shift(loop_file_extents, eff_block_size_log2, dev_length);




    /* algorithm: 0) compute LOOP-FILE extents and store in loop_map, sorted by physical */
    loop_file_extents.sort_by_physical();
    loop_map.append0_shift(loop_file_extents, eff_block_size_log2);
    /* show LOOP-FILE extents sorted by physical */
    show(label[FC_LOOP_FILE], eff_block_size, loop_map);


    /* algorithm: 0) compute FREE-SPACE extents and store in dev_free_map, sorted by physical
     *
     * we must manually set ->logical = ->physical for all free_space_extents:
     * here dev_free_map is just free space, but for I/O that computed it
     * it could have been a ZERO-FILE with its own ->logical,
     *
     * note: changing ->logical may also allow merging extents!
     */
    {
        ft_vector<ft_uoff>::const_iterator iter = free_space_extents.begin(), end = free_space_extents.end();
        T physical, length;
        for (; iter != end; ++iter) {
            physical = iter->first.physical >> eff_block_size_log2;
            length = iter->second.length >> eff_block_size_log2;
            dev_free_map.insert(physical, physical, length);
        }
        show("FREE-SPACE", eff_block_size, dev_free_map);
    }





    /* algorithm: 0) compute DEVICE extents
     *
     * how: compute physical complement of all LOOP-FILE and FREE-SPACE extents
     * and assume they are used by DEVICE for its file-system
     */
    /* compute in-place the union of LOOP-FILE extents and FREE-SPACE extents */
    loop_file_extents.append_all(free_space_extents);
    /* sort the union by physical: needed by dev_map.complement0_physical_shift() immediately below */
    loop_file_extents.sort_by_physical();
    dev_map.complement0_physical_shift(loop_file_extents, eff_block_size_log2, dev_length);
    /* show DEVICE extents sorted by physical */
    show(label[FC_DEVICE], eff_block_size, dev_map);




    /*
     * algorithm: 2), 3) allocate LOOP-HOLES for DEVICE extents logical destination
     * and for LOOP-FILE invariant extents
     */
    /* show LOOP-HOLES extents before allocation, sorted by physical */
    show("initial LOOP-HOLES", eff_block_size, loop_holes_map);

    /* algorithm: 2) re-number used DEVICE blocks, setting ->logical to values
     * from LOOP-HOLES. do not greedily use low hole numbers:
     * a) prefer holes with ->logical numbers equal to DEVICE ->physical block number:
     *    they produce an INVARIANT block, already in its final destination
     *    (marked with @@)
     * b) spread the remaining ->logical across rest of holes (use best-fit allocation)
     */
    /* how: intersect dev_map and loop_holes_map and put result into renumbered_map */
    renumbered_map.intersect_all_all(dev_map, loop_holes_map);
    /* show DEVICE INVARIANT extents (i.e. already in their final destination), sorted by physical */
    show("DEVICE (invariant)", eff_block_size, renumbered_map);
    /* remove from dev_map all the INVARIANT extents in renumbered_map */
    dev_map.remove_all(renumbered_map);
    /*
     * also remove from loop_holes_map all extents in renumbered_map
     * reason: they are no longer free (logical) holes,
     * since we allocated them for DEVICE INVARIANT extents
     */
    loop_holes_map.remove_all(renumbered_map);
    /*
     * then clear renumbered_map: its extents are already in their final destination
     * (they are INVARIANT) -> no work on them
     */
    renumbered_map.clear();
    /* show LOOP-HOLES (sorted by physical) after allocating DEVICE-INVARIANT extents */
    show("LOOP-HOLES after DEVICE (invariant)", eff_block_size, loop_holes_map);



    /*
     * algorithm: 2) b) spread the remaining DEVICE ->logical across rest of LOOP-HOLES
     * (use best-fit allocation)
     */
    /* order loop_holes_map by length */
    ft_pool<T> loop_holes_pool(loop_holes_map);
    /*
     * allocate LOOP-HOLES extents to store DEVICE extents using a best-fit strategy.
     * move allocated extents from dev_map to renumbered_map
     */
    loop_holes_pool.allocate_all(dev_map, renumbered_map);
    /* show DEVICE RENUMBERED extents sorted by physical */
    show("DEVICE (renumbered)", eff_block_size, renumbered_map);
    /* show LOOP-HOLES extents after allocation, sorted by physical */
    show("final LOOP-HOLES", eff_block_size, loop_holes_map);

    /* sanity check */
    if (!dev_map.empty()) {
        ff_log(FC_FATAL, 0, "internal error: there are extents in DEVICE not fitting DEVICE. this is impossible! I give up");
        /* show DEVICE-NOTFITTING extents sorted by physical */
        show("DEVICE-NOTFITTING", eff_block_size, dev_map, FC_NOTICE);
        return ENOSPC;
    }
    /* move DEVICE (RENUMBERED) back into dev_map and clear renumbered_map */
    dev_map.swap(renumbered_map);




    /*
     * 2.1) mark as INVARIANT (with @@) the (logical) extents in LOOP-FILE
     * already in their final destination, and forget them (no work on those).
     * also compute total length of extents remaining in LOOP-FILE and store in work_count.
     */
    map_iterator iter = loop_map.begin(), tmp, end = loop_map.end();
    work_count = 0; /**< number of blocks to be relocated */

    while (iter != end) {
        if (iter->first.physical == iter->second.logical) {
            /* move INVARIANT extents to renumbered_map, to show them later */
            renumbered_map.insert(*iter);
            tmp = iter;
            ++iter;
            /* forget INVARIANT extents (i.e. remove from loop_map) */
            loop_map.remove(tmp);
        } else {
            /* not INVARIANT, compute loop_map length... */
            work_count += iter->second.length;
            /*
             * also prepare for item 3) "merge renumbered DEVICE extents with remaining LOOP-FILE extents"
             * i.e. remember who's who
             */
            iter->second.user_data = FC_LOOP_FILE;
            ++iter;
        }
    }
    /* show LOOP-FILE (INVARIANT) blocks, sorted by physical */
    show("LOOP-FILE (invariant)", eff_block_size, renumbered_map);
    /* then forget them */
    renumbered_map.clear();






    /*
     * algorithm: 3) merge renumbered DEVICE extents with LOOP-FILE blocks (remember who's who)
     * also compute total length of extents remaining in DEVICE and add it to work_count.
     */
    iter = dev_map.begin();
    end = dev_map.end();
    for (; iter != end; ++iter) {
        work_count += iter->second.length;
        iter->second.user_data = FC_DEVICE;
        loop_map.insert0(iter->first, iter->second);
    }
    dev_map.clear();
    /*
     * from now on, we only need one of dev_map or loop_map, not both.
     * we choose dev_map: more intuitive name, and already stored in 'this'
     */
    dev_map.swap(loop_map);
    dev_map.total_count(work_count);
    dev_map.used_count(work_count);
    /* show DEVICE + LOOP-FILE extents after merge, sorted by physical */
    show("DEVICE + LOOP-FILE (merged)", eff_block_size, dev_map);

    double pretty_len = 0.0;
    const char * pretty_unit = ff_pretty_size((ft_uoff) work_count << eff_block_size_log2, & pretty_len);

    ff_log(FC_NOTICE, 0, "analysis completed: %.2f %sbytes must be relocated", pretty_len, pretty_unit);

    /*
     * algorithm: 4) compute (physical) intersection of FREE-SPACE and LOOP-HOLES,
     * and mark it as FREE-SPACE (INVARIANT) (with !!).
     * we can use these extents as partial or total replacement for STORAGE - see 5)
     * if they are relatively large (see below for meaning of "relatively large")
     *
     * forget the rest of LOOP-HOLES extents, we will not need them anymore
     */
    /* how: intersect dev_free_map and loop_holes_map and put result into renumbered_map */
    renumbered_map.intersect_all_all(dev_free_map, loop_holes_map);
    /* then discard extents smaller than either work_count / 1024 or page_size*/

    /* page_size_blocks = number of blocks in one RAM page. will be zero if page_size < block_size */
    const T page_size_blocks = (T) (ff_mem_page_size() >> eff_block_size_log2);

    /* consider for PRIMARY-STORAGE only "relatively large" blocks, i.e.
     * 1) at least 4096 * PAGE_SIZE bytes long, or at least work_count / 1024 blocks long
     * 2) in any case, at least 1 * PAGE_SIZE bytes long
     */
    ft_uoff hole_threshold = ff_min2((ft_uoff) work_count >> 10, (ft_uoff) page_size_blocks << 12);
    T hole_len, hole_count = 0;

    iter = renumbered_map.begin();
    end = renumbered_map.end();
    show("FREE-SPACE (invariant)", eff_block_size, renumbered_map);
    while (iter != end) {
        if ((ft_uoff) iter->second.length >= hole_threshold) {
            /* trim hole on both ends to align it to PAGE_SIZE */
            if (page_size_blocks <= 1 || (ft_uoff) (hole_len = ff_extent_align(*iter, page_size_blocks - 1)) >= hole_threshold) {
                /* hole is large enough to be useful */
                hole_count += hole_len;
                ++iter;
                continue;
            }
        }
        tmp = iter;
        ++iter;
        renumbered_map.remove(tmp);
    }
    /* move FREE-SPACE (INVARIANT) extents into dev_free_map, the latter is stored into 'this' */
    dev_free_map.swap(renumbered_map);
    /* show FREE-SPACE (INVARIANT) extents, sorted by physical */
    show("FREE-SPACE (invariant, contiguous, aligned)", eff_block_size, dev_free_map);


    pretty_len = 0.0;
    pretty_unit = ff_pretty_size((ft_uoff) hole_count << eff_block_size_log2, & pretty_len);
    ft_size dev_free_map_n = dev_free_map.size();

    ff_log(FC_INFO, 0, "located %.2f %sbytes (%"FS_ULL" fragment%s) in %s available as PRIMARY-STORAGE (free, invariant, contiguous and aligned)",
           pretty_len, pretty_unit, (FT_ULL)dev_free_map_n, (dev_free_map_n == 1 ? "" : "s"), label[FC_DEVICE]);

    dev_free_map.total_count(hole_count);

    return 0;
}

/**
 * creates on-disk secondary storage, used as (small) backup area during relocate().
 * must be executed before relocate()
 */
template<typename T>
int ft_work<T>::create_storage()
{
    enum {
        _1M_minus_1 = 1024*1024 - 1,
    };

    int err = 0;
    do {
        ft_uoff len = io->job_storage_size();
        const ft_uoff eff_block_size_log2 = io->effective_block_size_log2();

        if (len == 0) {
            /*
             * auto-detect storage_size: we want it to be the smallest between
             *   33% of free RAM (use 16 MB if free RAM cannot be determined)
             *   10% of bytes to relocate
             */
            ft_uoff free_ram_3 = FT_ARCH_NS ff_arch_mem_system_free() / 3;
            if (free_ram_3 == 0)
                free_ram_3 = (ft_uoff) 16*1024*1024;

            ft_uoff work_length_10 = ((ft_uoff) work_count << eff_block_size_log2) / 10;
            len = ff_min2(free_ram_3, work_length_10);

            /* round up to multiples of 1M */
            len = (len + _1M_minus_1) & ~(ft_uoff)_1M_minus_1;
        }
        /* else try to honor user-specified length */

        const ft_uoff page_size_m_1 = (ft_uoff) ff_mem_page_size() - 1;

        ft_uoff primary_storage_len = (ft_uoff) dev_free_map.total_count() << eff_block_size_log2;
        /* primary_storage_len should be already a multiple of PAGE_SIZE */
        primary_storage_len &= ~page_size_m_1;

        /*
         * truncate primary_storage_len to 1/4 of addressable memory (= 1GB on 32-bit machines)
         * keep PAGE_SIZE alignment!
         */
        const ft_size mem_max = ((ft_size)-1 >> 2) + 1;
        ft_size mem_len = (ft_size) primary_storage_len;
        if (mem_len < 0 || primary_storage_len != (ft_uoff) mem_len || mem_len > mem_max)
            primary_storage_len = (ft_uoff) mem_max & ~page_size_m_1;


        /* fill io->primary_storage() with DEVICE extents actually available as PRIMARY-STORAGE */
        fill_io_primary_storage(primary_storage_len);

        double pretty_len = 0.0;
        const char * pretty_unit = ff_pretty_size(primary_storage_len, & pretty_len);
        ft_size fragment_n = io->primary_storage().size();

        ff_log(FC_INFO, 0, "PRIMARY-STORAGE: actually using %.2f %sbytes (%"FS_ULL" fragment%s) from %s",
               pretty_len, pretty_unit, (FT_ULL)fragment_n, (fragment_n == 1 ? "" : "s"), label[FC_DEVICE]);


        if (len <= primary_storage_len) {
            len = 0;

        } else {
            len -= primary_storage_len;

            /* round up length to a multiple of PAGE_SIZE: required by mmap() inside io_posix::create_storage() */
            len = (len + page_size_m_1) & ~page_size_m_1;
            /*
             * round down length to fit off_t (== ft_off, signed version of ft_uoff)
             * keep PAGE_SIZE alignment!
             */
            if ((ft_off) len < 0)
                len &= (ft_uoff)(ft_off)-1 >> 1;
            /*
             * truncate length to 1/4 of addressable memory (= 1GB on 32-bit machines)
             * keep PAGE_SIZE alignment!
             */
            mem_len = (ft_size) len;
            if (mem_len < 0 || len != (ft_uoff) mem_len || mem_len > mem_max)
                len = (ft_uoff) mem_max & ~page_size_m_1;
        }

        if ((err = io->create_storage(len)) != 0)
            break;

    } while (0);
    return err;
}

/**
 * fill io->primary_storage() with DEVICE extents to be actually used as PRIMARY-STORAGE
 * (already computed into dev_free_map by analyze())
 */
template<typename T>
void ft_work<T>::fill_io_primary_storage(ft_uoff primary_storage_len) const
{
    ft_vector<ft_uoff> & primary_storage = io->primary_storage();
    map_const_iterator iter = dev_free_map.begin(), end = dev_free_map.end();


    ft_uoff physical, length, eff_block_size_log2 = io->effective_block_size_log2();

    for (; primary_storage_len != 0 && iter != end; ++iter) {
        const typename ft_map<T>::value_type & extent = *iter;
        physical = (ft_uoff) extent.first.physical << eff_block_size_log2;
        length   = (ft_uoff) extent.second.length  << eff_block_size_log2;

        /* stop at primary_storage_len bytes */
        if (length > primary_storage_len)
            length = primary_storage_len;
        primary_storage_len -= length;

        primary_storage.append(physical, physical, length, 0/*no user_data*/);
    }
}


/** core of transformation algorithm, actually moves DEVICE blocks */
template<typename T>
int ft_work<T>::relocate()
{
    int err = 0;
    do {

    } while (0);
    return err;
}


FT_NAMESPACE_END
