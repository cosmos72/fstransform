/*
 * work.t.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */
#include "first.hh"

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>       // for errno, EOVERFLOW
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>        // for errno, EOVERFLOW
#endif
#if defined(FT_HAVE_STRING_H)
# include <string.h>       // for strerror()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>        // for strerror()
#endif

#include "assert.hh"      // for ff_assert()
#include "log.hh"         // for ff_log()
#include "vector.hh"      // for fr_vector<T>
#include "map.hh"         // for fr_map<T>
#include "pool.hh"        // for fr_pool<T>
#include "misc.hh"        // for ff_pretty_size()
#include "work.hh"        // for ff_dispatch(), fr_work<T>
#include "arch/mem.hh"    // for ff_arch_mem_system_free()
#include "io/io.hh"       // for fr_io
#include "io/io_posix.hh" // for fr_io_posix
#include "ui/ui.hh"       // for fr_ui

FT_NAMESPACE_BEGIN

enum {
    FC_DEVICE = FT_IO_NS fr_io::FC_DEVICE,
    FC_LOOP_FILE = FT_IO_NS fr_io::FC_LOOP_FILE,
    FC_FREE_SPACE = FT_IO_NS fr_io_posix::FC_FREE_SPACE,
    FC_STORAGE = FT_IO_NS fr_io_posix::FC_STORAGE,
    FC_PRIMARY_STORAGE = FT_IO_NS fr_io_posix::FC_PRIMARY_STORAGE,
    FC_SECONDARY_STORAGE = FT_IO_NS fr_io_posix::FC_SECONDARY_STORAGE,
};

char const* const* const label = FT_IO_NS fr_io_posix::label;

char const* const label_LOOP_HOLES = "loop-holes";



template<typename T>
void fr_work<T>::show(const char * label1, const char * label2, ft_uoff effective_block_size,
                      const fr_map<T> & map, ft_log_level level)
{
    ft_log_level header_level = level >= FC_DEBUG ? level : (ft_log_level)(level + 1);

    if (!ff_log_is_enabled(header_level) && !ff_log_is_enabled(level))
        return;

    map_const_iterator iter = map.begin(), end = map.end();
    ft_size n = map.size();

    if (iter != end) {
        ff_log(header_level, 0, "# %4"FT_ULL" extent%s in %s%s",
               (ft_ull) n, (n == 1 ? " " : "s"), label1, label2);

        if (ff_log_is_enabled(level)) {
            ff_log(level, 0, "# effective block size = %"FT_ULL, (ft_ull) effective_block_size);
            show(level);

            for (ft_size i = 0; iter != end; ++iter, ++i)
                show(i, *iter, level);
        }
    } else {
        ff_log(header_level, 0, "#   no extents in %s%s", label1, label2);
    }
    ff_log(level, 0, "");
}

/** print extents header to log */
template<typename T>
void fr_work<T>::show(ft_log_level level)
{
    ff_log(level, 0, "#  extent        physical         logical      length  user_data");
}

/** print extent contents to log */
template<typename T>
void fr_work<T>::show(ft_size i, T physical, T logical, T length, ft_size user_data, ft_log_level level)
{
    ff_log(level, 0, "#%8"FT_ULL"\t%12"FT_ULL"\t%12"FT_ULL"\t%8"FT_ULL"\t(%"FT_ULL")", (ft_ull)i,
           (ft_ull) physical, (ft_ull) logical, (ft_ull) length, (ft_ull) user_data);
}


/** default constructor */
template<typename T>
fr_work<T>::fr_work()
    : dev_map(), storage_map(), dev_free(), dev_transpose(),
      storage_free(), storage_transpose(), toclear_map(),
      eta(), work_total(0)
{ }


#ifdef FT_HAVE_EXTERNAL_TEMPLATE
/**
 * copy constructor CANNOT be invoked,
 * but some C++ compilers need it to be defined
 * if explicit template instantiation is used
 */
template<typename T>
fr_work<T>::fr_work(const fr_work<T> & other)
{
    ff_assert_fail("fr_work<T> copy constructor CANNOT be invoked");
}

/** assignment operator CANNOT be invoked */
template<typename T>
    const fr_work<T> & fr_work<T>::operator=(const fr_work<T> & other)
{
    ff_assert_fail("fr_work<T> assignment operator CANNOT be invoked");
}
#endif /* FT_HAVE_EXTERNAL_TEMPLATE */


/** destructor. calls cleanup() */
template<typename T>
fr_work<T>::~fr_work()
{
    cleanup();
}


/** performs cleanup. called by destructor, you can also call it explicitly after (or instead of) run()  */
template<typename T>
void fr_work<T>::cleanup()
{
    dev_map.clear();
    storage_map.clear();
    dev_free.clear();
    dev_transpose.clear();
    storage_free.clear();
    storage_transpose.clear();
    toclear_map.clear();
    eta.clear();
    work_total = 0;
}



/**
 * high-level do-everything method. calls in sequence init(), run() and cleanup().
 * return 0 if success, else error.
 */
template<typename T>
int fr_work<T>::main(fr_vector<ft_uoff> & loop_file_extents,
                     fr_vector<ft_uoff> & free_space_extents, FT_IO_NS fr_io & io)
{
    fr_work<T> worker;
    return worker.run(loop_file_extents, free_space_extents, io);

    // worker.cleanup() is called automatically by destructor, no need to call explicitly
}

/** full remapping algorithm */
template<typename T>
int fr_work<T>::run(fr_vector<ft_uoff> & loop_file_extents,
                    fr_vector<ft_uoff> & free_space_extents, FT_IO_NS fr_io & io)
{
    int err;
    (err = init(io)) == 0
        && (err = analyze(loop_file_extents, free_space_extents)) == 0
        && (err = create_storage()) == 0
        && (err = start_ui()) == 0
        && (err = relocate()) == 0
        && (err = clear_free_space()) == 0
        && (err = close_storage()) == 0;

    if (err == 0) {
        ff_log(FC_NOTICE, 0, "%sjob completed.", io.simulate_run() ? "(simulated) " : "");

    } else if (!ff_log_is_reported(err)) {
        /*
         * note 1.2.2) fr_remap.main() and other high-level *.main() methods
         * must check for unreported errors and log them them with message
         * "failed with unreported error"
         */
        err = ff_log(FC_ERROR, err, "failed with unreported error");
    }
    return err;
}

/**
 *  check if LOOP-FILE and DEVICE in-use extents can be represented
 *  by fr_map<T>. takes into account the fact that all extents
 *  physical, logical and length will be divided by effective block size
 *  before storing them into fr_map<T>.
 *
 *  return 0 for check passes, else error (usually EOVERFLOW)
 */
template<typename T>
int fr_work<T>::check(const FT_IO_NS fr_io & io)
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
int fr_work<T>::init(FT_IO_NS fr_io & io)
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
        FC_PAGE_SIZE_IF_UNKNOWN = 4096 // assume 4k (most common value) if cannot be detected
    };

    static ft_size page_size = 0;
    if (page_size == 0) {
        if ((page_size = FT_ARCH_NS ff_arch_mem_page_size()) == 0) {
            ff_log(FC_WARN, 0, "cannot detect system PAGE_SIZE. assuming 4 kilobytes and continuing, but troubles (mmap() errors) are very likely");
            page_size = FC_PAGE_SIZE_IF_UNKNOWN;
        }
    }
    return page_size;
}


template<typename T>
static T ff_round_up(T n, T power_of_2_minus_1)
{
    if (n & power_of_2_minus_1)
        n = (n | power_of_2_minus_1) + 1;
    return n;
}

/* trim extent on both ends to align it to page_size. return trimmed extent length (can be zero) */
template<typename T>
T ff_extent_align(typename fr_map<T>::value_type & extent, T page_size_blocks_m_1)
{
    T physical = extent.first.physical;
    T end = physical + extent.second.length;
    T new_physical = ff_round_up<T>(physical, page_size_blocks_m_1);
    T new_end = end & ~page_size_blocks_m_1;
    if (new_end <= new_physical)
        return extent.second.length = 0;

    extent.first.physical = new_physical;
    extent.second.logical += new_physical - physical;
    return extent.second.length = new_end - new_physical;
}


/**
 * analysis phase of remapping algorithm,
 * must be executed before create_storage() and relocate()
 *
 * given LOOP-FILE extents and FREE-SPACE extents as ft_vectors<ft_uoff>,
 * compute LOOP-FILE extents map and DEVICE in-use extents map
 *
 * assumes that vectors are ordered by extent->logical, and modifies them
 * in place: vector contents will be UNDEFINED when this method returns.
 *
 * basic implementation idea: to compute this->dev_map, performs in-place the union of specified
 * loop_file_extents and free_space_extents, then sorts in-place and complements such union.
 *
 * detailed implementation is quite complicated... see the comments and the documentation
 */
template<typename T>
int fr_work<T>::analyze(fr_vector<ft_uoff> & loop_file_extents,
                        fr_vector<ft_uoff> & free_space_extents)
{
    // cleanup in case dev_map, storage_map or storage_map are not empty, or work_count != 0
    cleanup();

    map_type loop_map, loop_holes_map, renumbered_map;

    ft_uoff eff_block_size_log2 = io->effective_block_size_log2();
    ft_uoff eff_block_size      = (ft_uoff)1 << eff_block_size_log2;
    ft_uoff dev_length          = io->dev_length();
    /*
     * algorithm: 1) find LOOP-FILE (logical) holes, i.e. LOOP-HOLES,
     * and store them in loop_holes_map
     * note: all complement maps have physical == logical
     */
    loop_holes_map.complement0_logical_shift(loop_file_extents, eff_block_size_log2, dev_length);
    if (io->job_clear() == FC_CLEAR_ALL)
        toclear_map = loop_holes_map;




    /* algorithm: 0) compute LOOP-FILE extents and store in loop_map, sorted by physical */
    loop_file_extents.sort_by_physical();
    loop_map.append0_shift(loop_file_extents, eff_block_size_log2);
    /* show LOOP-FILE extents sorted by physical */
    show(label[FC_LOOP_FILE], "", eff_block_size, loop_map);


    /* algorithm: 0) compute FREE-SPACE extents and store in dev_free, sorted by physical
     *
     * we must manually set ->logical = ->physical for all free_space_extents:
     * here dev_free is just free space, but for I/O that computed it
     * it could have been a ZERO-FILE with its own ->logical,
     *
     * note: changing ->logical may also allow merging extents!
     */
    {
        fr_vector<ft_uoff>::const_iterator iter = free_space_extents.begin(), end = free_space_extents.end();
        T physical, logical, length;
        for (; iter != end; ++iter) {
            physical = iter->first.physical >> eff_block_size_log2;
            logical = iter->second.logical >> eff_block_size_log2;
            length = iter->second.length >> eff_block_size_log2;
            dev_free.insert(physical, physical, length, FC_DEFAULT_USER_DATA);
        }
    }

    /* sanity check: LOOP-FILE and FREE-SPACE extents ->physical must NOT intersect */
    renumbered_map.intersect_all_all(loop_map, dev_free, FC_PHYSICAL1);
    if (!renumbered_map.empty()) {
        ff_log(FC_FATAL, 0, "inconsistent %s and %s: they share common blocks on %s !", label[FC_LOOP_FILE], label[FC_FREE_SPACE], label[FC_DEVICE]);
        show(label[FC_LOOP_FILE], " intersection with free-space", eff_block_size, renumbered_map, FC_DEBUG);
        return -EFAULT;
    }

    /*
     * move from LOOP-FILE to DEV-FREE and to TO-CLEAR-MAP
     * the (physical) extents in LOOP-FILE that are full with zeros (UNWRITTEN).
     *
     * this converts FC_EXTENT_ZEROED extents into free extents,
     * and will also mark them to be cleared after relocate() finishes.
     */
    map_iterator iter = loop_map.begin(), tmp, end = loop_map.end();
    {
        T physical, length;
        while (iter != end) {
            map_value_type & extent = *iter;
            if (extent.second.user_data == FC_EXTENT_ZEROED) {
                physical = extent.first.physical;
                length = extent.second.length;
                /* extent must be inserted in toclear_map even if io->job_clear() == FC_CLEAR_ALL */
                toclear_map.insert(physical, physical, length, FC_DEFAULT_USER_DATA);
                dev_free.insert(physical, physical, length, FC_DEFAULT_USER_DATA);
                tmp = iter;
                ++iter;
                loop_map.remove(tmp);
            } else
                ++iter;
        }
    }
    show("to-clear", " (initial)", eff_block_size, toclear_map, FC_DEBUG);
    show(label[FC_FREE_SPACE], " (after to-clear)", eff_block_size, dev_free);



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
    show(label[FC_DEVICE], "", eff_block_size, dev_map);




    /*
     * algorithm: 2), 3) allocate LOOP-HOLES for DEVICE extents logical destination
     * and for LOOP-FILE invariant extents
     */
    /* show LOOP-HOLES extents before allocation, sorted by physical */
    show(label_LOOP_HOLES, " (initial)", eff_block_size, loop_holes_map);

    /* algorithm: 2) re-number used DEVICE blocks, setting ->logical to values
     * from LOOP-HOLES. do not greedily use low hole numbers:
     * a) prefer holes with ->logical numbers equal to DEVICE ->physical block number:
     *    they produce an INVARIANT block, already in its final destination
     *    (marked with @@)
     * b) spread the remaining ->logical across rest of holes (use best-fit allocation)
     */
    /* how: intersect dev_map and loop_holes_map and put result into renumbered_map */
    renumbered_map.intersect_all_all(dev_map, loop_holes_map, FC_BOTH);
    /* show DEVICE INVARIANT extents (i.e. already in their final destination), sorted by physical */
    show(label[FC_DEVICE], " (invariant)", eff_block_size, renumbered_map);
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
    show(label_LOOP_HOLES, " after device (invariant)", eff_block_size, loop_holes_map);



    /*
     * algorithm: 2) b) spread the remaining DEVICE ->logical across rest of LOOP-HOLES
     * (use best-fit allocation)
     */
    /* order loop_holes_map by length */
    fr_pool<T> loop_holes_pool(loop_holes_map);
    /*
     * allocate LOOP-HOLES extents to store DEVICE extents using a best-fit strategy.
     * move allocated extents from dev_map to renumbered_map
     */
    loop_holes_pool.allocate_all(dev_map, renumbered_map);
    /* show DEVICE RENUMBERED extents sorted by physical */
    show(label[FC_DEVICE], " (renumbered)", eff_block_size, renumbered_map);
    /* show LOOP-HOLES extents after allocation, sorted by physical */
    show(label_LOOP_HOLES, " (final)", eff_block_size, loop_holes_map);

    /* sanity check */
    if (!dev_map.empty()) {
        ff_log(FC_FATAL, 0, "internal error: there are extents in DEVICE not fitting DEVICE. this is impossible! I give up");
        /* show DEVICE-NOTFITTING extents sorted by physical */
        show(label[FC_DEVICE], " (not fitting)", eff_block_size, dev_map, FC_NOTICE);
        return ENOSPC;
    }
    /* move DEVICE (RENUMBERED) back into dev_map and clear renumbered_map */
    dev_map.swap(renumbered_map);



    if (io->job_clear() == FC_CLEAR_MINIMAL) {
        /*
         * also add DEVICE (RENUMBERED) to TO-CLEAR-MAP:
         * we MUST clear it once remapping is finished,
         * since it contains data from the original device, NOT from the loop-file
         * WARNING: beware of intersections,
         * and be careful to use dev_map->logical instead of dev_map->physical!
         */
        dev_transpose.transpose(dev_map);
        renumbered_map.clear();
        renumbered_map.intersect_all_all(dev_transpose, toclear_map, FC_PHYSICAL2);
        toclear_map.remove_all(renumbered_map);
        iter = dev_transpose.begin();
        end = dev_transpose.end();
        for (; iter != end; ++iter) {
            const map_value_type & extent = *iter;
            toclear_map.insert(extent.first.physical, extent.first.physical, extent.second.length, FC_DEFAULT_USER_DATA);
        }
        dev_transpose.clear();
        show("to-clear", " (final)", eff_block_size, toclear_map, FC_DEBUG);
    }





    /*
     * 2.1) mark as INVARIANT (with @@) the (logical) extents in LOOP-FILE
     * already in their final destination, and forget them (no work on those).
     * also compute total length of extents remaining in LOOP-FILE and store in work_count.
     */
    iter = loop_map.begin();
    end = loop_map.end();
    T work_count = 0; /**< number of blocks to be relocated */

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
    show(label[FC_LOOP_FILE], " (invariant)", eff_block_size, renumbered_map);
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
    show("device + loop-file", " (merged)", eff_block_size, dev_map);

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
    /*
     * how: intersect storage_map (hosted in dev_free variable)
     * and loop_holes_map and put result into renumbered_map
     */
    renumbered_map.clear();
    renumbered_map.intersect_all_all(dev_free, loop_holes_map, FC_BOTH);
    /* then discard extents smaller than either work_count / 1024 or page_size*/

    /* page_size_blocks = number of blocks in one RAM page. will be zero if page_size < block_size */
    const ft_size page_size = ff_mem_page_size();
    const T page_size_blocks = (T) (page_size >> eff_block_size_log2);

    /*
     * consider for PRIMARY-STORAGE only "relatively large" blocks, i.e.
     * 1) at least 1024 * PAGE_SIZE bytes long, or at least work_count / 1024 blocks long
     * 2) in any case, at least 1 * PAGE_SIZE bytes long
     */
    ft_uoff hole_threshold = ff_max2((ft_uoff)page_size_blocks, ff_min2((ft_uoff) work_count >> 10, (ft_uoff) page_size << 10 >> eff_block_size_log2));
    T hole_len, hole_total_len = 0;

    iter = renumbered_map.begin();
    end = renumbered_map.end();
    show(label[FC_FREE_SPACE], " (invariant)", eff_block_size, renumbered_map);
    while (iter != end) {
        map_value_type & extent = *iter;
        /*
         * whether this hole (extent from dev_free) is large enough to be useful or not,
         * it is invariant free space. the current remapping algorithm will never use it,
         * so remove it from free space to get accurate calculation of usable free space.
         */
        dev_free.remove(extent);

        if ((ft_uoff) (hole_len = iter->second.length) >= hole_threshold) {
            /* trim hole on both ends to align it to PAGE_SIZE */
            if (page_size_blocks <= 1 || (ft_uoff) (hole_len = ff_extent_align(extent, page_size_blocks - 1)) >= hole_threshold) {
                hole_total_len += hole_len;
                ++iter;
                continue;
            }
        }
        /* extent is small, do not use for PRIMARY-STORAGE */
        tmp = iter;
        ++iter;
        renumbered_map.remove(tmp);
    }
    /*
     * move FREE-SPACE (INVARIANT) extents into storage_map (i.e. PRIMARY-STORAGE),
     * as the latter is stored into 'this'
     */
    storage_map.swap(renumbered_map);
    /* show PRIMARY-STORAGE extents, sorted by physical */
    show(label[FC_PRIMARY_STORAGE], " (= free-space, invariant, contiguous, aligned)", eff_block_size, storage_map);


    pretty_len = 0.0;
    pretty_unit = ff_pretty_size((ft_uoff) hole_total_len << eff_block_size_log2, & pretty_len);
    ft_size storage_map_n = storage_map.size();

    ff_log(FC_INFO, 0, "%s: located %.2f %sbytes (%"FT_ULL" fragment%s) usable in %s (free, invariant, contiguous and aligned)",
           label[FC_PRIMARY_STORAGE], pretty_len, pretty_unit, (ft_ull)storage_map_n, (storage_map_n == 1 ? "" : "s"), label[FC_DEVICE]);

    storage_map.total_count(hole_total_len);

    /* all done */

    return 0;
}


static int unusable_storage_size(const char * label, ft_uoff requested_len, const char * type_descr, ft_ull type_bytes)
{
    ff_log(FC_FATAL, 0, "fatal error: cannot use job %s length = %"FT_ULL" bytes, it is incompatible with %s = %"FT_ULL" bytes,"
            " original job was probably created on a platform with %s",
            label, (ft_ull) requested_len, type_descr, type_bytes);
    /* mark error as reported */
    return -EOVERFLOW;
}


/**
 * creates on-disk secondary storage, used as (hopefully small) backup area during relocate().
 * must be executed before relocate()
 */
template<typename T>
int fr_work<T>::create_storage()
{
    enum {
        _1M_minus_1 = 1024*1024 - 1,
        _64k_minus_1 = 64*1024 - 1,
    };

    const ft_uoff eff_block_size_log2 = io->effective_block_size_log2();
    const ft_uoff eff_block_size_minus_1 = ((ft_uoff)1 << eff_block_size_log2) - 1;

    const ft_uoff free_ram_or_0 = FT_ARCH_NS ff_arch_mem_system_free();
    const ft_uoff free_ram_or_min = free_ram_or_0 != 0 ? free_ram_or_0
        : sizeof(ft_size) <= 4 ? (ft_uoff) 48*1024*1024 : (ft_uoff) 768*1024*1024;

    ft_uoff avail_primary_len = (ft_uoff) storage_map.total_count() << eff_block_size_log2;

    ft_size avail_primary_size = (ft_size) ff_min2<ft_uoff>(avail_primary_len, (ft_size)-1);
    ft_size auto_total_size = 0, mem_buffer_size;

    const ft_size page_size_minus_1 = ff_mem_page_size() - 1;

    ft_size req_mem_buffer_size = io->job_storage_size(FC_MEM_BUFFER_SIZE);
    ft_size req_secondary_size = io->job_storage_size(FC_SECONDARY_STORAGE_SIZE);
    const ft_size req_primary_size_exact = io->job_storage_size(FC_PRIMARY_STORAGE_EXACT_SIZE);
    const ft_size req_secondary_size_exact = io->job_storage_size(FC_SECONDARY_STORAGE_EXACT_SIZE);
    const ft_size req_total_size_exact = req_primary_size_exact + req_secondary_size_exact;

    double free_pretty_len = 0.0;
    const char * free_pretty_unit = ff_pretty_size(free_ram_or_min, & free_pretty_len);

    if (req_primary_size_exact != 0 && req_secondary_size_exact != 0 &&
        req_secondary_size_exact > (ft_size)-1 - req_primary_size_exact)
    {
        double req_p_pretty_len = 0.0, req_s_pretty_len = 0.0;
        const char * req_p_pretty_unit = ff_pretty_size(req_primary_size_exact, & req_p_pretty_len);
        const char * req_s_pretty_unit = ff_pretty_size(req_secondary_size_exact, & req_s_pretty_len);

        return ff_log(FC_ERROR, EOVERFLOW, "requested %s + %s exact size (%.2f %sbytes + %.2f %sbytes) overflow addressable memory",
                      label[FC_PRIMARY_STORAGE], label[FC_SECONDARY_STORAGE],
                      req_p_pretty_len, req_p_pretty_unit, req_s_pretty_len, req_s_pretty_unit);
    }

    if (free_ram_or_0 == 0)
        ff_log(FC_WARN, 0, "cannot detect free RAM amount");

    if (req_total_size_exact != 0 || req_secondary_size != 0) {
        /* honor requested storage size, but warn if it may exhaust free RAM */

        const ft_size req_len = req_total_size_exact != 0 ? req_total_size_exact : req_secondary_size;
        const char * req_label = label[req_total_size_exact != 0 ? FC_STORAGE : FC_SECONDARY_STORAGE];

        double req_pretty_len = 0.0;
        const char * req_pretty_unit = ff_pretty_size(req_len, & req_pretty_len);

        if (free_ram_or_0 == 0) {
            ff_log(FC_WARN, 0, "no idea if the %.2f %sbytes requested for %s%s will fit into free RAM",
                    req_pretty_len, req_pretty_unit, "mmapped() ", req_label);
            ff_log(FC_WARN, 0, "continuing, but troubles (memory exhaustion) are possible");
        } else if ((ft_uoff) req_len >= free_ram_or_0 / 2) {
            ff_log(FC_WARN, 0, "using %.2f %sbytes as requested for %s, but only %.2f %sbytes RAM are free",
                    req_pretty_len, req_pretty_unit, req_label, free_pretty_len, free_pretty_unit);
            ff_log(FC_WARN, 0, "honoring the request, but expect troubles (memory exhaustion)");
        }
    }
    if (req_total_size_exact == 0) {
        /*
         * auto-detect total storage size to use:
         * we want it to be the smallest between
         *   50% of free RAM (if free RAM cannot be determined, use 16 MB on 32bit platforms, and 256MB on 64bit+ platforms)
         *   12.5% of bytes to relocate
         */
        if (req_secondary_size == 0 && free_ram_or_0 == 0) {
            ff_log(FC_WARN, 0, "assuming at least %.2f %sbytes RAM are free", free_pretty_len, free_pretty_unit);
            ff_log(FC_WARN, 0, "expect troubles (memory exhaustion) if not true");
        }
        T work_count = dev_map.used_count();
        ft_uoff work_length_8 = (((ft_uoff) work_count << eff_block_size_log2) + 7) / 8;
        ft_uoff total_len = ff_min2(free_ram_or_min / 2, work_length_8);

        /* round up to multiples of 1M */
        total_len = ff_round_up<ft_uoff>(total_len, _1M_minus_1);
        /* truncate to ft_size */
        auto_total_size = (ft_size) ff_min2<ft_uoff>(total_len, (ft_uoff)(ft_size)-1);
    }


    if (req_mem_buffer_size != 0) {
        const ft_size req_len = req_mem_buffer_size;

        double req_pretty_len = 0.0;
        const char * req_pretty_unit = ff_pretty_size(req_len, & req_pretty_len);

        if (free_ram_or_0 == 0) {
            ff_log(FC_WARN, 0, "no idea if the %.2f %sbytes requested for %s%s will fit into free RAM",
                    req_pretty_len, req_pretty_unit, "memory ", "buffer");
            ff_log(FC_WARN, 0, "continuing, but troubles (memory exhaustion) are possible");
        } else if ((ft_uoff) req_len >= free_ram_or_0 / 2) {
            ff_log(FC_WARN, 0, "using %.2f %sbytes as requested for %s%s, but only %.2f %sbytes RAM are free",
                    req_pretty_len, req_pretty_unit, "memory ", "buffer", free_pretty_len, free_pretty_unit);
            ff_log(FC_WARN, 0, "honoring the request, but expect troubles (memory exhaustion)");
        }
        mem_buffer_size = req_mem_buffer_size;
    } else {
        /*
         * autodect RAM buffer size:
         * it will be the smallest between 25% free RAM and number of bytes to relocate
         * (and truncated to fit addressable RAM)
         */
        ft_uoff work_bytes = (ft_uoff)dev_map.used_count() << eff_block_size_log2;
        mem_buffer_size = (ft_size) ff_min2((ft_uoff)(ft_size)-1, ff_min2(free_ram_or_min / 4, work_bytes));
    }

    bool flag;

    /* round down all parameters to a multiple of PAGE_SIZE */
    mem_buffer_size &= ~page_size_minus_1;
    auto_total_size &= ~page_size_minus_1;
    avail_primary_size &= ~page_size_minus_1;
    req_secondary_size &= ~page_size_minus_1;
    if ((flag = ((req_primary_size_exact & page_size_minus_1) != 0)) || (req_secondary_size_exact & page_size_minus_1))
        return unusable_storage_size(label[flag ? FC_PRIMARY_STORAGE : FC_SECONDARY_STORAGE],
                                     flag ? req_primary_size_exact : req_secondary_size_exact, "system PAGE_SIZE",
            (ft_ull)(page_size_minus_1 + 1));

    /* round down all parameters to a multiple of effective block size */
    mem_buffer_size &= ~eff_block_size_minus_1;
    auto_total_size &= ~eff_block_size_minus_1;
    avail_primary_size &= ~eff_block_size_minus_1;
    req_secondary_size &= ~eff_block_size_minus_1;
    if ((flag = ((req_primary_size_exact & eff_block_size_minus_1) != 0)) || (req_secondary_size_exact & eff_block_size_minus_1))
        return unusable_storage_size(label[flag ? FC_PRIMARY_STORAGE : FC_SECONDARY_STORAGE],
                                     flag ? req_primary_size_exact : req_secondary_size_exact, "device effective block size",
            (ft_ull)(eff_block_size_minus_1 + 1));

    /*
     * truncate mem_buffer_size to 1/4 of addressable RAM (= 1GB on 32-bit machines)
     * truncate all non-mandatory storage sizes (auto_total_size, avail_primary_size, req_secondary_size)
     *    to 1/2 of addressable RAM (= 2GB on 32-bit machines)
     * keep alignment to PAGE_SIZE and effective block size!
     */
    const ft_size mem_4 = (((ft_size)-1 >> 2) + 1) & ~page_size_minus_1 & ~eff_block_size_minus_1;
    const ft_size mem_2 = mem_4 << 1;
    mem_buffer_size = ff_min2(mem_buffer_size, mem_4);
    auto_total_size = ff_min2(auto_total_size, mem_2);
    avail_primary_size = ff_min2(avail_primary_size, mem_2);
    req_secondary_size = ff_min2(req_secondary_size, mem_2);


    if (req_total_size_exact == 0 && req_secondary_size == 0 && auto_total_size == 0) {
        auto_total_size = (page_size_minus_1 | eff_block_size_minus_1) + 1;

        double total_pretty_len = 0.0;
        const char * total_pretty_unit = ff_pretty_size(auto_total_size, & total_pretty_len);

        ff_log(FC_WARN, 0, "%s size to use would be 0 bytes, increasing to %.2f %sbytes",
                label[FC_STORAGE], total_pretty_len, total_pretty_unit);
    }
    if (mem_buffer_size == 0) {
        mem_buffer_size = (page_size_minus_1 | eff_block_size_minus_1) + 1;

        double mem_pretty_len = 0.0;
        const char * mem_pretty_unit = ff_pretty_size(mem_buffer_size, & mem_pretty_len);

        ff_log(FC_WARN, 0, "%s size to use would be 0 bytes, increasing to %.2f %sbytes",
                "memory buffer", mem_pretty_len, mem_pretty_unit);
    }

    ft_size primary_size;
    if (req_primary_size_exact > avail_primary_size) {
        double avail_pretty_len = 0.0, req_pretty_len = 0.0;
        const char * avail_pretty_unit = ff_pretty_size(avail_primary_size, & avail_pretty_len);
        const char * req_pretty_unit = ff_pretty_size(req_primary_size_exact, & req_pretty_len);

        ff_log(FC_ERROR, 0, "available %s is only %"FT_ULL" bytes (%.2f %sbytes),"
               " too small for requested %"FT_ULL" bytes (%.2f %sbytes)", label[FC_PRIMARY_STORAGE],
               (ft_ull)avail_primary_size, avail_pretty_len, avail_pretty_unit,
               (ft_ull) req_primary_size_exact, req_pretty_len, req_pretty_unit);
        /* mark error as reported */
        return -ENOSPC;
    } else if (req_primary_size_exact != 0)
        primary_size = req_primary_size_exact;
    else
        primary_size = ff_min2(avail_primary_size, auto_total_size);


    ft_size secondary_size;
    if (req_secondary_size_exact != 0)
        secondary_size = req_secondary_size_exact;
    else if (req_secondary_size != 0)
        secondary_size = req_secondary_size;
    else if (auto_total_size > primary_size)
        secondary_size = auto_total_size - primary_size;
    else
        secondary_size = 0;

    /* remember storage sizes */
    io->job_storage_size(FC_MEM_BUFFER_SIZE, mem_buffer_size);
    io->job_storage_size(FC_PRIMARY_STORAGE_EXACT_SIZE, primary_size);
    io->job_storage_size(FC_SECONDARY_STORAGE_EXACT_SIZE, secondary_size);

    /* fill io->primary_storage() with PRIMARY-STORAGE extents actually used */
    fill_io_primary_storage(primary_size);

    return io->create_storage(secondary_size, mem_buffer_size);
}

/**
 * fill io->primary_storage() with DEVICE extents to be actually used as PRIMARY-STORAGE
 * (already computed into storage_map by analyze()).
 *
 * if only a fraction of available PRIMARY-STORAGE will be actually used,
 * exploit a fr_pool<T> to select the largest contiguous extents.
 *
 * updates storage_map to contain the PRIMARY-STORAGE extents actually used.
 */
template<typename T>
void fr_work<T>::fill_io_primary_storage(ft_size primary_size)
{
    const ft_uoff primary_len = (ft_uoff) primary_size;
    const ft_uoff eff_block_size_log2 = io->effective_block_size_log2();
    const ft_uoff eff_block_size_minus_1 = ((ft_uoff)1 << eff_block_size_log2) - 1;

    ff_assert((primary_len & eff_block_size_minus_1) == 0);

    /* first, copy all extents from storage_map to primary_storage */
    fr_vector<ft_uoff> & primary_storage = io->primary_storage();
    ft_uoff physical, length;
    map_iterator map_iter = storage_map.begin(), map_end = storage_map.end();
    for (; map_iter != map_end; ++map_iter) {
        typename fr_map<T>::value_type & extent = *map_iter;
        physical = (ft_uoff) extent.first.physical << eff_block_size_log2;
        length   = (ft_uoff) extent.second.length  << eff_block_size_log2;

        primary_storage.append(physical, physical, length, extent.second.user_data);
    }

    /* then check: if not all extents will be actually used, drop the smallest ones */
    const ft_uoff available_len = (ft_uoff) storage_map.total_count() << eff_block_size_log2;

    if (available_len > primary_len) {
        ft_uoff extra_len = available_len - primary_len;

        /* sort by reverse length */
        primary_storage.sort_by_reverse_length();

        /*
         * iterate dropping the last (smallest) extents until we exactly reach primary_len.
         * (one final extent may be shrank instead of dropped)
         */
        while (extra_len != 0 && !primary_storage.empty()) {
            fr_extent<ft_uoff> & extent = primary_storage.back();
            length   = extent.length();
            if (length <= extra_len) {
                // completely drop this extent
                extra_len -= length;
                primary_storage.pop_back();
            } else {
                // shrink this extent and break
                extent.length() -= extra_len;
                extra_len = 0;
            }
        }
        primary_storage.sort_by_physical();

        /* update storage_map. needed by show() below */
        storage_map.clear();
        storage_map.append0_shift(primary_storage, eff_block_size_log2);
    }

    storage_map.total_count((T)(primary_len >> eff_block_size_log2));

    double pretty_len = 0.0;
    const char * pretty_unit = ff_pretty_size(primary_len, & pretty_len);
    ft_size fragment_n = primary_storage.size();

    ff_log(FC_INFO, 0, "%s: actually using %.2f %sbytes (%"FT_ULL" fragment%s) from %s",
           label[FC_PRIMARY_STORAGE], pretty_len, pretty_unit,
           (ft_ull)fragment_n, (fragment_n == 1 ? "" : "s"), label[FC_DEVICE]);

    show(label[FC_PRIMARY_STORAGE], " (actually used)", (ft_uoff) 1 << eff_block_size_log2, storage_map);
}

/** start UI, passing I/O object to it. requires I/O to know device length and storage size */
template<typename T>
int fr_work<T>::start_ui()
{
    FT_UI_NS fr_ui * ui = io->ui();
    int err = 0;
    if (ui != NULL)
        err = ui->start(io);
    return err;
}


/** core of remapping algorithm, actually moves DEVICE blocks */
template<typename T>
int fr_work<T>::relocate()
{
    const char * dev_path = io->dev_path();
    int err = 0;
    const bool simulated = io->simulate_run();
    const bool ask_questions = io->ask_questions();
    const char * simul_msg = simulated ? "(simulated) " : "";

    if (!simulated) {
        err = io->umount_dev();
            
        if (err == 0) {
            if (ask_questions) {
                ff_log(FC_NOTICE, 0, "everything ready for in-place remapping, this is your LAST chance to quit.");
                ff_log(FC_WARN, 0, "press ENTER to proceed, or CTRL+C to quit");
            }
        } else {
            if (ask_questions) {
                ff_log(FC_WARN, 0, "please manually unmount %s '%s' before continuing.", label[FC_DEVICE], dev_path);
                ff_log(FC_WARN, 0, "press ENTER when done, or CTRL+C to quit");
                err = 0;
            } else {
                err = ff_log(FC_ERROR, err, "unmount %s '%s' failed", label[FC_DEVICE], dev_path);
                ff_log(FC_ERROR, 0, "    you could unmount it yourself and continue");
                ff_log(FC_ERROR, 0, "    but this is a non-interactive run, so fsremap will exit now");
                ff_log(FC_ERROR, 0, "exiting.");
            }
        }

        if (ask_questions) {
            char ch;
            if (::read(0, &ch, 1) < 0)
                err = ff_log(FC_ERROR, errno, "read(stdin) failed");
        }
        if (err)
            return err;
    }

    ff_log(FC_NOTICE, 0, "%sstarting in-place remapping. this may take a LONG time ...", simul_msg);

    ft_uoff eff_block_size_log2 = io->effective_block_size_log2();

    /* storage_count = number of storage blocks */
    T storage_count = (T) ((io->job_storage_size(FC_PRIMARY_STORAGE_EXACT_SIZE) +
                            io->job_storage_size(FC_SECONDARY_STORAGE_EXACT_SIZE)) >> eff_block_size_log2);

    /* storage starts free */
    storage_map.clear();
    storage_map.total_count(storage_count);
    storage_transpose.clear();
    storage_free.insert0(0, 0, storage_count, FC_DEFAULT_USER_DATA);

    work_total = dev_map.used_count();

    /* device starts (almost) full */
    T dev_free_count = 0;
    map_const_iterator iter = dev_free.begin(), end = dev_free.end();
    for (; iter != end; ++iter)
        dev_free_count += iter->second.length;
    dev_map.total_count(work_total + dev_free_count);
    dev_transpose.transpose(dev_map);


    /*
     * do we have an odd-sized (i.e. smaller than effective block size) last loop-file block?
     * it must be treated specially, see the next function for details.
     */
    err = check_last_odd_sized_block();

    /* initialize progress report */
    eta.clear();
    eta.add(0.0);

    while (err == 0 && !(dev_map.empty() && storage_map.empty())) {

        if (!dev_map.empty() && !storage_free.empty())
            err = fill_storage();

        if (err == 0)
            show_progress(FC_NOTICE, simul_msg);

        if (err == 0 && !dev_map.empty())
            err = move_to_target(FC_FROM_DEV);

        if (err == 0 && !storage_map.empty())
            err = move_to_target(FC_FROM_STORAGE);
    }
    if (err == 0)
        ff_log(FC_INFO, 0, "%sblocks remapping completed.", simul_msg);

    return err;
}

/** show progress status and E.T.A. */
template<typename T>
void fr_work<T>::show_progress(ft_log_level log_level, const char * simul_msg)
{
    const ft_uoff eff_block_size_log2 = io->effective_block_size_log2();

    T dev_used = dev_map.used_count(), storage_used = storage_map.used_count();
    ft_uoff total_len = ((ft_uoff)dev_used + (ft_uoff)storage_used) << eff_block_size_log2;

    double percentage = 0.0, time_left = -1.0;

    if (work_total != 0) {
        percentage = 1.0 - ((double)dev_used + 0.5 * (double)storage_used) / (double)work_total;
        time_left = eta.add(percentage);
        percentage *= 100.0;
    }

    if (io->simulate_run())
        time_left = -1.0;

    ff_show_progress(log_level, simul_msg, percentage, total_len, " still to relocate", time_left);

    const ft_uoff eff_block_size = (ft_uoff)1 << eff_block_size_log2;

    show(label[FC_DEVICE], "", eff_block_size, dev_map, FC_DUMP);
    show(label[FC_DEVICE], " free space", eff_block_size, dev_free, FC_DUMP);

    show(label[FC_STORAGE], "", eff_block_size, storage_map, FC_DUMP);
    show(label[FC_STORAGE], " free space", eff_block_size, storage_free, FC_DUMP);
}


/*
 * called once by relocate() during the remapping phase.
 *
 * do we have an odd-sized (i.e. smaller than effective block size) last device block?
 * it does not appear in any extent map: its length is zero in 1-block units !
 *
 * by itself it is not a problem and we could just ignore it,
 * but it is likely that an equally odd-sized (or slightly smaller) last loop-file block will be present,
 * and since its length is instead rounded UP to one block by the various io->read_extents() functions,
 * the normal algorithm in relocate() would not find its final destination and enter an infinite loop (ouch)
 */
template<typename T>
int fr_work<T>::check_last_odd_sized_block()
{
    ft_uoff eff_block_size_log2 = io->effective_block_size_log2();
    ft_uoff eff_block_size = (ft_uoff) 1 << eff_block_size_log2;

    ft_uoff dev_len = io->dev_length(), loop_file_len = io->loop_file_length();
    dev_len &= ~(eff_block_size - 1);

    if (loop_file_len <= dev_len)
        return 0;

    ft_uoff odd_block_len = loop_file_len & (eff_block_size - 1);

    const char * simul_msg = io->simulate_run() ? "(simulated) " : "";

    ff_log(FC_ERROR, 0, "%s%s has an odd-sized last block (%"FT_ULL" bytes long) that exceeds device rounded length.",
            simul_msg, label[FC_LOOP_FILE], (ft_ull) odd_block_len);
    ff_log(FC_ERROR, 0, "%sExiting, please shrink %s to %"FT_ULL" bytes or less before running fsremap again.",
            simul_msg, label[FC_LOOP_FILE], (ft_ull) dev_len);
    return -EFBIG;
}






/** called by relocate(). move as many extents as possible from DEVICE to STORAGE */
template<typename T>
int fr_work<T>::fill_storage()
{
    map_iterator from_iter = dev_map.begin(), from_pos, from_end = dev_map.end();
    T moved = 0, from_used_count = dev_map.used_count(), to_free_count = storage_map.free_count();
    const bool simulated = io->simulate_run();
    const char * simul_msg = simulated ? "(simulated) " : "";

    double pretty_len = 0.0;
    const char * pretty_label = ff_pretty_size((ft_uoff)ff_min2<T>(from_used_count, to_free_count)
                                               << io->effective_block_size_log2(), & pretty_len);
    ff_log(FC_INFO, 0, "%sfilling %s by moving %.2f %sbytes from %s ...",
           simul_msg, label[FC_STORAGE], pretty_len, pretty_label, label[FC_DEVICE]);
    show(); /* show extents header */

    ft_size counter = 0;
    int err = 0;
    while (err == 0 && moved < to_free_count && from_iter != from_end) {
        /* fully or partially move this extent to STORAGE */
        from_pos = from_iter;
        ++from_iter;
        /* note: some blocks may have been moved even in case of errors! */
        err = move(counter++, from_pos, FC_DEV2STORAGE, moved);
    }
    if (err == 0) {
        if ((err = io->flush()) == 0)
            ff_log(FC_INFO, 0, "%sstorage filled", simul_msg);
        else {
            /* error should has been reported by io->flush() */
            if (!ff_log_is_reported(err))
                err = ff_log(FC_ERROR, err, "%sio->flush() failed with unreported error", simul_msg);
        }
    }
    return err;
}


/**
 * called by fill_storage().
 * move as much as possible of a single extent from DEVICE to FREE-STORAGE or from STORAGE to FREE-DEVICE.
 * invalidates from_iter.
 * note: the extent could be fragmented in the process
 */
template<typename T>
int fr_work<T>::move(ft_size counter, map_iterator from_iter, fr_dir dir, T & ret_moved)
{
    T moved, length = from_iter->second.length;
    const bool is_to_dev = ff_is_to_dev(dir);
    map_stat_type & to_map = is_to_dev ? dev_map : storage_map;
    map_type & to_free_map = is_to_dev ? dev_free : storage_free;

    map_iterator to_free_iter = to_free_map.begin(), to_free_pos, to_free_end = to_free_map.end();
    int err = 0;

    if (ff_log_is_enabled((ft_log_level)FC_SHOW_DEFAULT_LEVEL)) {
        const map_value_type & extent = *from_iter;
        show(counter, extent.first.physical, extent.second.logical,
             ff_min2(extent.second.length, to_map.free_count()),
             extent.second.user_data);
    }

    while (err == 0 && length != 0 && to_free_iter != to_free_end) {
        to_free_pos = to_free_iter;
        ++to_free_iter;
        moved = 0;
        err = move_fragment(from_iter, to_free_pos, dir, moved);
        length -= moved;
        ret_moved += moved;
    }
    if (err == 0)
        ff_assert(length == 0 || to_map.free_count() == 0);
    return err;
}


/**
 * called by move().
 * move a single extent or a fragment of it from DEVICE to FREE STORAGE or from STORAGE to FREE-DEVICE.
 * the moved amount is the smaller between (from_length = from_iter->length) and (to_length = to_free_iter->length).
 *
 * updates dev_* and storage_* maps.
 *
 * if from_length <= to_length, invalidates from_iter.
 * if from_length >= to_length, invalidates to_iter.
 */
template<typename T>
int fr_work<T>::move_fragment(map_iterator from_iter, map_iterator to_free_iter, fr_dir dir, T & ret_queued)
{
    map_value_type & from_extent = *from_iter, & to_free_extent = *to_free_iter;
    map_mapped_type & from_value = from_extent.second;

    T from_length = from_value.length, to_free_length = to_free_extent.second.length;
    T length = ff_min2<T>(from_length, to_free_length);

    T from_physical = from_extent.first.physical;
    T to_physical = to_free_extent.first.physical;

    int err = io->copy(dir, from_physical, to_physical, length);
    if (err != 0)
        return err;
    /** io->copy() returned success. trust that it copied exactly 'length' blocks */
    ret_queued += length;
    /*
     * some blocks were copied (or queued for copying).
     * update 'from' and 'to' maps also in case of errors,
     * since we know how many blocks were actually copied
     */
    T logical = from_value.logical;
    ft_size user_data = from_extent.second.user_data;

    /* update the 'to' maps */
    {
        const bool is_to_dev = ff_is_to_dev(dir);

        map_stat_type & to_map = is_to_dev ? dev_map : storage_map;
        to_map.stat_insert(to_physical, logical, length, user_data);

        map_type & to_transpose = is_to_dev ? dev_transpose : storage_transpose;
        to_transpose.insert(logical, to_physical, length, user_data);

        map_type & to_free = is_to_dev ? dev_free : storage_free;
        /*
         * erase to_free_iter completely (if moved == to_free_length),
         * or shrink it (if moved < to_free_length)
         */
        to_free.remove_front(to_free_iter, length);
    }

    /* update the 'from' maps */
    {
        const bool is_from_dev = ff_is_from_dev(dir);

        map_stat_type & from_map = is_from_dev ? dev_map : storage_map;
        /* beware: this could be a _partial_ remove! */
        from_map.stat_remove_front(from_iter, length); /* can invalidate from_iter, from_extent, from_value */

        map_type & from_transpose = is_from_dev ? dev_transpose : storage_transpose;
        from_transpose.remove(logical, from_physical, length);

        map_type & from_free = is_from_dev ? dev_free : storage_free;
        from_free.insert(from_physical, from_physical, length, FC_DEFAULT_USER_DATA);
    }

    return err;
}




/** called by relocate(). move as many extents as possible from DEVICE or STORAGE directly to their final destination */
template<typename T>
int fr_work<T>::move_to_target(fr_from from)
{
    map_type movable;

    map_stat_type & from_map = from == FC_FROM_DEV ? dev_map: storage_map;
    map_type & from_free = from == FC_FROM_DEV ? dev_free: storage_free;
    map_type & from_transpose = from == FC_FROM_DEV ? dev_transpose : storage_transpose;

    const char * label_from = label[from == FC_FROM_DEV ? FC_DEVICE : FC_STORAGE];
    const fr_dir dir = from == FC_FROM_DEV ? FC_DEV2DEV : FC_STORAGE2DEV;
    int err = 0;
    const bool simulated = io->simulate_run();
    const char * simul_msg = simulated ? "(simulated) " : "";

    /* find all DEVICE or STORAGE extents that can be moved to their final destination into DEVICE free space */
    movable.intersect_all_all(from_transpose, dev_free, FC_PHYSICAL1);

    if (movable.empty()) {
        ff_log(FC_INFO, 0, "%smoved 0 bytes from %s to target (not so useful)", simul_msg, label_from);
        ft_uoff eff_block_size = (ft_uoff)1 << io->effective_block_size_log2();
        show(label_from, " transposed", eff_block_size, from_transpose, FC_DEBUG);
        show(label[FC_DEVICE], " free space", eff_block_size, dev_free, FC_DEBUG);
        return err;
    }

    if (ff_log_is_enabled(FC_INFO)) {
        map_const_iterator iter = movable.begin(), end = movable.end();
        ft_uoff movable_length = 0;
        for (; iter != end; ++iter)
            movable_length += iter->second.length;
        movable_length <<= io->effective_block_size_log2();

        double pretty_len = 0.0;
        const char * pretty_label = ff_pretty_size(movable_length, & pretty_len);
        ff_log(FC_INFO, 0, "%smoving %.2f %sbytes from %s to target ...",
               simul_msg, pretty_len, pretty_label, label_from);
        show(); /* show extents header */
    }

    /* move them */
    map_const_iterator iter = movable.begin(), end = movable.end();
    T from_physical, to_physical, length;
    ft_size counter = 0;
    for (; iter != end; ++iter) {
        const map_value_type & extent = *iter;
        from_physical = extent.second.logical;
        to_physical = extent.first.physical;
        length = extent.second.length;

        /* sequential disk access: consecutive calls to io->copy() are sorted by to_physical, i.e. device to_offset */
        err = io->copy(dir, from_physical, to_physical, length);
        show(counter++, from_physical, to_physical, length, extent.second.user_data);
        if (err != 0)
            return err;
        from_transpose.remove(extent);
        from_map.stat_remove(from_physical, to_physical, length);
        from_free.insert(from_physical, from_physical, length, FC_DEFAULT_USER_DATA);
        /*
         * forget final destination extent: it's NOT free anymore, but nothing to do there.
         * actually, if it is DEVICE-RENUMBERED, it will likely be cleared after relocate() finishes,
         * but in such case it is supposed to be ALREADY in toclear_map
         */
        dev_free.remove(to_physical, to_physical, length);
        dev_map.total_count(dev_map.total_count() - length);
    }

    if ((err = io->flush()) == 0)
        ff_log(FC_INFO, 0, "%sfinished moving from %s to target", simul_msg, label_from);
    else {
        /* error should has been reported by io->flush() */
        if (!ff_log_is_reported(err))
            err = ff_log(FC_ERROR, err, "%s%s move_to_target(): io->flush() failed with unreported error", simul_msg, label_from);
    }
    return err;
}

/**
 * called by run() after relocate(). depending on job_clear, it will:
 * 1) if job_clear == FC_CLEAR_ALL, fill with zeroes all free space
 * 2) if job_clear == FC_CLEAR_MINIMAL, fill with zeroes PRIMARY-STORAGE, DEVICE-RENUMBERED and LOOP-FILE "unwritten" extents
 * 3) if job_clear == FC_CLEAR_NONE, only fill with zeroes LOOP-FILE "unwritten" extents
 */
template<typename T>
int fr_work<T>::clear_free_space()
{
    const char * sim_msg = io->simulate_run() ? "(simulated) " : "";
    int err = 0;
    fr_clear_free_space job_clear = io->job_clear();

    do {
        ft_uoff toclear_len = 0;
        if (job_clear == FC_CLEAR_MINIMAL)
            toclear_len += (ft_uoff) io->job_storage_size(FC_PRIMARY_STORAGE_EXACT_SIZE);

        map_const_iterator iter = toclear_map.begin(), end = toclear_map.end();
        for (; iter != end; ++iter)
            toclear_len += iter->second.length << io->effective_block_size_log2();

        double pretty_len = 0.0;
        const char * pretty_label = ff_pretty_size(toclear_len, & pretty_len);
        switch (job_clear) {
            case FC_CLEAR_MINIMAL:
                ff_log(FC_INFO, 0, "%sclearing %.2f %sbytes free space from %s to remove temporary data (%s and device backup)...",
                       sim_msg, pretty_len, pretty_label, label[FC_DEVICE], label[FC_PRIMARY_STORAGE]);
                err = io->zero_primary_storage();
                break;
            default:
            case FC_CLEAR_ALL:
            case FC_CLEAR_NONE:
                ff_log(FC_INFO, 0, "%sclearing %.2f %sbytes %s from %s ...",
                       sim_msg, pretty_len, pretty_label, job_clear == FC_CLEAR_NONE ? "UNWRITTEN blocks" : label[FC_FREE_SPACE], label[FC_DEVICE]);
                break;
        }
        if (err != 0)
            break;

        iter = toclear_map.begin();
        end = toclear_map.end();
        for (; iter != end; ++iter) {
            const map_value_type & extent = *iter;
            if (extent.first.physical == 0) {
                ff_log(FC_FATAL, 0, "PANIC! this is a BUG in fsremap! tried to clear first block of remapped device!");
                ff_log(FC_FATAL, 0, "\tABORTED clearing %s free space to prevent filesystem corruption.", label[FC_DEVICE]);
                ff_log(FC_FATAL, 0, "\tSuspending process to allow debugging... press ENTER or CTRL+C to quit.");
                char ch;
                if (::read(0, &ch, 1) < 0) {
                	ff_log(FC_WARN, errno, "read(stdin) failed");
                }
                err = -EFAULT;
                break;

            } else if ((err = io->zero(FC_TO_DEV, extent.first.physical, extent.second.length)) != 0)
                break;
        }
        int err2;
        if ((err2 = io->flush()) != 0) {
            if (err == 0)
                err = err2;
            break;
        }

        ff_log(FC_INFO, 0, "%s%s %s cleared", sim_msg, label[FC_DEVICE],
               job_clear == FC_CLEAR_NONE ? "UNWRITTEN blocks" : job_clear == FC_CLEAR_MINIMAL ? "temporary data" : label[FC_FREE_SPACE]);

    } while (0);
    return err;
}


/** called after relocate() and clear_free_space(). closes storage */
template<typename T>
int fr_work<T>::close_storage()
{
    return io->close_storage();
}

FT_NAMESPACE_END
