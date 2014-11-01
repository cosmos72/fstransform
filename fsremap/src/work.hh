/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 * 
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * remap.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSREMAP_WORK_HH
#define FSREMAP_WORK_HH

#include "types.hh"     // for ft_uoff
#include "map_stat.hh"  // for fr_map_stat<T>
#include "eta.hh"       // for ft_eta
#include "log.hh"       // for ft_log_level
#include "io/io.hh"     // for fr_io


FT_NAMESPACE_BEGIN

/**
 * class doing the core of remapping work.
 *
 * contains the algorithm to move LOOP-FILE around
 * until its physical layout matches its logical layout.
 * at that point, DEVICE will have been fully remaped.
 */
template<typename T>
class fr_work
{
private:
    typedef fr_map_stat<T>                  map_stat_type;
    typedef fr_map<T>                       map_type;

    typedef typename fr_map<T>::iterator    map_iterator;
    typedef typename fr_map<T>::const_iterator map_const_iterator;
    typedef typename fr_map<T>::key_type    map_key_type;
    typedef typename fr_map<T>::mapped_type map_mapped_type;
    typedef typename fr_map<T>::value_type  map_value_type;


    map_stat_type dev_map, storage_map;
    map_type dev_free, dev_transpose;
    map_type storage_free, storage_transpose;
    map_type toclear_map;

    FT_IO_NS fr_io * io;

    ft_eta eta;
    T work_total;

    /** cannot call copy constructor */
    fr_work(const fr_work<T> &);

    /** cannot call assignment operator */
    const fr_work<T> & operator=(const fr_work<T> &);

    /**
     * call check(io) to ensure that io.dev_length() can be represented by T,
     * then checks that I/O is open.
     * if success, stores a reference to I/O object.
     */
    int init(FT_IO_NS fr_io & io);

    /**
     * analysis phase of remapping algorithm,
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
    int analyze(fr_vector<ft_uoff> & loop_file_extents,
                fr_vector<ft_uoff> & free_space_extents,
                fr_vector<ft_uoff> & to_zero_extents);

    /**
     * fill io->primary_storage() with DEVICE extents to be actually used as PRIMARY-STORAGE
     * (already computed into storage_map by analyze())
     *
     * if only a fraction of available PRIMARY-STORAGE will be actually used,
     * exploit a fr_pool<T> to select the largest contiguous extents.
     *
     * updates storage_map to contain the PRIMARY-STORAGE extents actually used.
     */
    void fill_io_primary_storage_extents(ft_size primary_size);

    /**
     * creates on-disk secondary storage, used as (small) backup area during relocate().
     * must be executed before relocate()
     */
    int create_storage();

    /** start UI, passing I/O object to it. requires I/O to know device length and storage size */
    int start_ui();

    /** core of remapping algorithm, actually moves DEVICE blocks */
    int relocate();

    /**
     * called by run() after relocate(). depending on job_clear, it will:
     * 1) if job_clear == FC_CLEAR_ALL, fill with zeroes all free space
     * 2) if job_clear == FC_CLEAR_MINIMAL, fill with zeroes PRIMARY-STORAGE, DEVICE-RENUMBERED and LOOP-FILE "unwritten" extents
     * 3) if job_clear == FC_CLEAR_NONE, only fill with zeroes LOOP-FILE "unwritten" extents
     */
    int clear_free_space();

    /** called after relocate() and clear_free_space(). closes storage */
    int close_storage_after_success();


    /**
     * called once by relocate() immediately before starting the remapping phase.
     *
     * 1) check that last device block to be written is actually writable.
     *    Reason: at least on Linux, if a filesystems is smaller than its containing device,
     *    it often limits to its length the writable blocks in the device.
     *
     * 2) check for corner care where we have an odd-sized (i.e. smaller than effective block size) last device block,
     *    which does not appear in any extent map: its length is zero in 1-block units !
     *
     *    by itself it is not a problem and we could just ignore it,
     *    but it is likely that an equally odd-sized (or slightly smaller) last loop-file block will be present,
     *    and since its length is instead rounded UP to one block by the various io->read_extents() functions,
     *    the normal algorithm in relocate() would not find its final destination and enter an infinite loop (ouch)
     */
    int check_last_block();


    /** called by relocate(). move as many extents as possible from DEVICE to STORAGE */
    int fill_storage();

    /** called by relocate(). move as many extents as possible from DEVICE or STORAGE directly to their final destination */
    int move_to_target(fr_from from);

    /**
     * called by fill_storage().
     * move as much as possible of a single extent from DEVICE to FREE-STORAGE or from STORAGE to FREE-DEVICE.
     * invalidates from_iter.
     * note: the extent can be fragmented in the process.
     * on return, 'ret_moved' will be increased by the number of blocks actually moved
     * note: some blocks may be moved even in case of errors!
     */
    int move(ft_size counter, map_iterator from_iter, fr_dir dir, T & ret_moved);

    /**
     * called by move().
     * move a single fragment from DEVICE to FREE STORAGE, or from STORAGE to FREE-DEVICE or from DEVICE to FREE DEVICE.
     * the moved amount is the largest between (from_length = from_iter->length) and (to_length = to_free_iter->length).
     *
     * updates dev_* and storage_* maps.
     *
     * if from_length <= to_length, invalidates from_iter.
     * if from_length >= to_length, invalidates to_iter.
     *
     * on return, 'ret_moved' will be increased by the number of blocks actually moved
     * note: some blocks may be moved even in case of errors!
     */
    int move_fragment(map_iterator from_iter, map_iterator to_free_iter, fr_dir dir, T & ret_moved);

    /** read or write next step from persistence file */
    int update_persistence();

    /** show progress status and E.T.A. */
    void show_progress(ft_log_level log_level);

public:
    /** default constructor */
    fr_work();

    /** destructor. calls cleanup() */
    ~fr_work();

    /**
     * high-level do-everything method. calls in sequence run() and cleanup().
     * return 0 if success, else error.
     */
    static int main(fr_vector<ft_uoff> & loop_file_extents,
                    fr_vector<ft_uoff> & free_space_extents,
                    fr_vector<ft_uoff> & to_zero_extents,
                    FT_IO_NS fr_io & io);


    /**
     *  check if LOOP-FILE and DEVICE in-use extents can be represented
     *  by fr_map<T>. takes into account the fact that all extents
     *  physical, logical and length will be divided by effective block size
     *  before storing them into fr_map<T>.
     *
     *  return 0 for check passes, else error (usually EFBIG)
     */
    static int check(const FT_IO_NS fr_io & io);

    /**
     * main remapping algorithm.
     * calls in sequence init(), analyze(), create_secondary_storage() and relocate()
     */
    int run(fr_vector<ft_uoff> & loop_file_extents,
            fr_vector<ft_uoff> & free_space_extents,
            fr_vector<ft_uoff> & to_zero_extents,
            FT_IO_NS fr_io & io);

    /** performs cleanup. called by destructor, you can also call it explicitly after (or instead of) run()  */
    void cleanup();
};


FT_NAMESPACE_END


#ifdef FT_HAVE_EXTERN_TEMPLATE
#  define FT_TEMPLATE_work_hh(ft_prefix, T)     ft_prefix class FT_NS fr_work< T >;
   FT_TEMPLATE_DECLARE(FT_TEMPLATE_work_hh)
#else
#  include "work.t.hh"
#endif /* FT_HAVE_EXTERN_TEMPLATE */



#endif /* FSREMAP_WORK_HH */
