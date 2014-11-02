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
 * io/io_prealloc.hh
 *
 *  Created on: Apr 18, 2012
 *      Author: max
 */

#ifndef FSREMAP_IO_IO_PREALLOC_HH
#define FSREMAP_IO_IO_PREALLOC_HH

#include "../args.hh"        // for FC_MOUNT_POINT*
#include "../types.hh"       // for ft_uoff
#include "../inode/inode_cache_mem.hh" // for ft_cache_mem<V>
#include "io_posix.hh"       // for fr_io_posix
#include "io_posix_dir.hh"   // for ft_io_posix_dir

FT_IO_NAMESPACE_BEGIN

/**
 * class performing I/O on POSIX systems with preallocation
 */
class fr_io_prealloc: public fr_io_posix
{
private:
    typedef fr_io_posix super_type;

    static const char * const MP_LABEL[FC_MOUNT_POINTS_N];

    // inode-cache. used to examine only once multiple links to the same file
    ft_cache_mem<ft_nlink> this_inode_cache;

    // device and loop mount points
    ft_io_posix_dir mount_point[FC_MOUNT_POINTS_N];

    // loop device path
    ft_string loop_file_path;

    // loop device path
    const char * loop_dev_path;

    // 'losetup' command to disconnect loop device
    const char * cmd_losetup;

    /** call umount(8) on loop_dev_path */
    int cmd_umount_loop_dev();

    /** call losetup(8) on loop_dev_path */
    int cmd_losetup_loop_dev();

    /**
     * check if inode is a hard link to an already examined file or special device,
     * and in case return true, otherwise return EAGAIN.
     *
	 * do NOT call this method if inode is a directory!
	 *
     * return -errno in case of errors.
     */
    int hard_link(const char * src_path, const ft_stat & src_stat,
                  const char * dst_path, const ft_stat & dst_stat);

    /**
     * recursively scan the contents of src and dst directories
     * to match the actual extents from files inside device (src)
     * with the preallocated extents from files inside loop file (dst).
     *
     * any preallocated extents in files inside loop file
     * which do NOT have a correspondence in files inside device
     * are added to to_zero_extents.
     *
     * NOTE: loop_file_extents will be filled with UNSORTED data
     */
    int read_extents_dir(ft_io_posix_dir & src, const ft_string & dst,
                         fr_vector<ft_uoff> & loop_file_extents,
                         fr_vector<ft_uoff> & to_zero_extents,
                         ft_uoff & ret_block_size_bitmask);

    /**
     * match the actual extents from (src_path) file inside device
     * with the preallocated extents from (dst_path) file inside loop file.
     *
     * any preallocated extents in files inside loop file
     * which do NOT have a correspondence in files inside device
     * are added to to_zero_extents
     */
    int read_extents_file(const char * src_path, const ft_stat & src_stat,
                          const char * dst_path, const ft_stat & dst_stat,
                          fr_vector<ft_uoff> & loop_file_extents,
                          fr_vector<ft_uoff> & to_zero_extents,
                          ft_uoff & ret_block_size_bitmask);

protected:

    /** return true if device and loop file mount points are currently (and correctly) open */
    bool is_open_dirs() const;

    /**
     * retrieve LOOP-FILE extents and FREE-SPACE extents and insert them into
     * the vectors loop_file_extents and free_space_extents.
     * the vectors will be ordered by extent ->logical.
     *
     * return 0 for success, else error (and vectors contents will be UNDEFINED).
     *
     * if success, also returns in ret_effective_block_size_log2 the log2()
     * of device effective block size.
     * the device effective block size is defined as follows:
     * it is the largest power of 2 that exactly divides all physical,
     * logical and lengths in all returned extents (both for LOOP-FILE
     * and for FREE-SPACE) and that also exactly exactly divides device length.
     *
     * the trick fr_io_posix uses to implement this method
     * is to fill the device's free space with a ZERO-FILE,
     * and actually retrieve the extents used by ZERO-FILE.
     */
    virtual int read_extents(fr_vector<ft_uoff> & loop_file_extents,
                             fr_vector<ft_uoff> & free_space_extents,
                             fr_vector<ft_uoff> & to_zero_extents,
                             ft_uoff & ret_block_size_bitmask);


public:
    /** constructor. */
    fr_io_prealloc(fr_persist & persist);

    /** destructor. calls close() */
    virtual ~fr_io_prealloc();

    /** check for consistency and open DEVICE, LOOP-FILE and ZERO-FILE */
    virtual int open(const fr_args & args);

    /** return true if this fr_io_prealloc is currently (and correctly) open */
    virtual bool is_open() const;

    /** call umount(8) and losetup(8) on loop_dev_path, then call umount(8) on dev_path() */
    virtual int umount_dev();

    /**
     * close this I/O, including device and loop file mount points.
     * Obviously calls super_type::close()
     */
    virtual void close();
};

FT_IO_NAMESPACE_END

#endif /* FSREMAP_IO_IO_PREALLOC_HH */
