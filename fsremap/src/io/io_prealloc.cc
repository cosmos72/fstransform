/*q
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
 * io/io_posix.cc
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#include "../first.hh"

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>        // for errno
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>         // for errno
#endif
#if defined(FT_HAVE_STRING_H)
# include <string.h>       // for strlen()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>        // for strlen()
#endif

#ifdef FT_HAVE_SYS_TYPES_H
# include <sys/types.h>    // for open()
#endif
#ifdef FT_HAVE_SYS_STAT_H
# include <sys/stat.h>     // for   "
#endif
#ifdef FT_HAVE_FCNTL_H
# include <fcntl.h>        // for   "
#endif
#ifdef FT_HAVE_UNISTD_H
# include <unistd.h>       // for close()
#endif

#include "../log.hh"       // for ff_log()
#include "../misc.hh"      // for ff_max2(), ff_min2()
#include "../vector.hh"    // for fr_vector<T>
#include "../cache/cache_mem.hh" // for ft_cache_mem<K,V>

#include "util_posix.hh"   // for ff_posix_stat()
#include "extent_posix.hh" // for ff_read_extents_posix()
#include "io_prealloc.hh"  // for fr_io_prealloc


FT_IO_NAMESPACE_BEGIN


const char * const fr_io_prealloc::MP_LABEL[] = { LABEL[FC_DEVICE], LABEL[FC_LOOP_FILE] };

/** constructor. */
fr_io_prealloc::fr_io_prealloc(fr_persist & persist)
: super_type(persist), this_inode_cache(NULL), mount_point(),
  loop_file_path(), loop_dev_path(NULL), cmd_losetup(NULL)
{
    // TODO: command-line option to use ft_cache_symlink_kv<K,V>
    this_inode_cache = new ft_cache_mem<ft_nlink,ft_nlink>();
}

/** destructor. calls close() */
fr_io_prealloc::~fr_io_prealloc()
{
    close();
    delete this_inode_cache;
    this_inode_cache = NULL;
}

/** return true if device and loop file mount points are currently (and correctly) open */
bool fr_io_prealloc::is_open_dirs() const
{
    for (ft_size i = 0; i < FC_MOUNT_POINTS_N; i++)
        if (!mount_point[i].is_open())
            return false;
    return true;
}

/** return true if this fr_io_prealloc is currently (and correctly) open */
bool fr_io_prealloc::is_open() const
{
    return super_type::is_open() && is_open_dirs();
}


/** check for consistency and open DEVICE, LOOP-FILE and ZERO-FILE */
int fr_io_prealloc::open(const fr_args & args)
{
    int err = super_type::open(args);
    if (err != 0)
        return err;

    const char * loop_file_path_cstr = args.io_args[FC_LOOP_FILE];
    if (loop_file_path_cstr == NULL) {
        ff_log(FC_ERROR, 0, "option --io=prealloc requires arguments %s %s [%s]",
                LABEL[FC_DEVICE], LABEL[FC_LOOP_FILE], LABEL[FC_ZERO_FILE]);
        return -EINVAL;
    }

    this_inode_cache->clear();
    loop_file_path = loop_file_path_cstr;
    loop_dev_path = args.loop_dev;
    cmd_losetup = args.cmd_losetup;

    if (!is_replaying()) do {

        const ft_size mp_i = FC_MOUNT_POINT_DEVICE, mp_j = FC_MOUNT_POINT_LOOP_FILE;
        const char * const * mount_point_dir = args.mount_points;

        if (loop_dev_path == NULL || mount_point_dir[mp_i] == NULL || mount_point_dir[mp_j] == NULL) {
            ff_log(FC_ERROR, 0, "option --io=prealloc requires the options --loop-device=LOOP-DEV, --device-mount-point=DIR and --loop-mount-point=DIR");
            err = -EINVAL;
            break;
        }
        
        ft_size i, n = FC_MOUNT_POINTS_N;
        for (i = 0; i < n; i++) {
            ft_string path = mount_point_dir[i];
            if ((err = mount_point[i].open(path)) != 0)
                break;
        }
    } while (0);

    if (err != 0)
        close();

    return err;
}

/**
 * close this I/O, including device and loop file mount points.
 * Obviously calls super_type::close()
 */
void fr_io_prealloc::close()
{
    this_inode_cache->clear();
    ft_size i, n = FC_MOUNT_POINTS_N;
    for (i = 0; i < n; i++) {
        mount_point[i].close();
    }
    loop_file_path.clear();
    loop_dev_path = NULL;
    cmd_losetup = NULL;
    super_type::close();
}

/** call umount(8) on loop_dev_path */
int fr_io_prealloc::cmd_umount_loop_dev()
{
    const char * cmd = cmd_umount(), * dev = loop_dev_path;

    std::vector<const char *> args;

    if (cmd == NULL)
        // posix standard name for umount(8)
        cmd = "/bin/umount";

    args.push_back(cmd);
    // only one argument: device path
    args.push_back(dev);
    args.push_back(NULL); // needed by ff_posix_exec() as end-of-arguments marker

    ff_log(FC_INFO, 0, "unmounting %s '%s'... command: %s %s", label[FC_LOOP_FILE], dev, cmd, dev);

    int err = ff_posix_exec(args[0], & args[0]);

    if (err == 0)
        ff_log(FC_INFO, 0, "successfully unmounted %s '%s'", label[FC_LOOP_FILE], dev);

    return err;
}

/** call losetup(8) on loop_dev_path */
int fr_io_prealloc::cmd_losetup_loop_dev()
{
    const char * cmd = cmd_losetup, * dev = loop_dev_path;

    std::vector<const char *> args;

    if (cmd == NULL)
        // Linux standard name for losetup(8)
        cmd = "/sbin/losetup";

    args.push_back(cmd);
    args.push_back("-d");
    args.push_back(dev);
    args.push_back(NULL); // needed by ff_posix_exec() as end-of-arguments marker

    ff_log(FC_INFO, 0, "disconnecting %s '%s'... command: %s -d %s", label[FC_LOOP_FILE], dev, cmd, dev);

    int err = ff_posix_exec(args[0], & args[0]);

    if (err == 0)
        ff_log(FC_NOTICE, 0, "successfully disconnected %s '%s'", label[FC_LOOP_FILE], dev);

    return err;
}

/** call umount(8) and losetup(8) on loop_dev_path, then call umount(8) on dev_path() */
int fr_io_prealloc::umount_dev()
{
    int err = 0;
    for (ft_size i = 0; i < FC_MOUNT_POINTS_N; i++)
        // we must close mount_point[] to allow unmounting device and loop_device
        if ((err = mount_point[i].close()) != 0)
            break;

    if (err == 0
        && (err = cmd_umount_loop_dev()) == 0
        && (err = cmd_losetup_loop_dev()) == 0
        && (err = super_type::umount_dev()) == 0)
    { }

    return err;
}

/**
 * retrieve LOOP-FILE extents and FREE-SPACE extents and insert them into
 * the vectors loop_file_extents and free_space_extents.
 * the vectors will be ordered by extent ->logical.
 *
 * return 0 for success, else error (and vectors contents will be UNDEFINED).
 *
 * this implementation calls super_type::read_extents() to get the extents for LOOP FILE and (optionally) ZERO FILE,
 * then recursively scans the contents of device and loop file mount points
 * to match the actual extents from files inside device
 * with the preallocated extents from files inside loop file.
 *
 * This trick allows a significant speedup, as fsmove needs only to preallocate files inside loop file, not to actually move them
 */
int fr_io_prealloc::read_extents(fr_vector<ft_uoff> & loop_file_extents,
                                 fr_vector<ft_uoff> & free_space_extents,
                                 fr_vector<ft_uoff> & to_zero_extents,
                                    ft_uoff & ret_block_size_bitmask)
{
    ft_uoff block_size_bitmask = ret_block_size_bitmask;
    int err = 0;
    do {
        if (!is_open_dirs()) {
            ff_log(FC_ERROR, 0, "unexpected call to fr_io_prealloc::read_extents(), I/O is not open");
            err = -ENOTCONN; // not open!
            break;
        }

        err = read_extents_dir(
                mount_point[FC_MOUNT_POINT_DEVICE],
                mount_point[FC_MOUNT_POINT_LOOP_FILE].path(),
                loop_file_extents, to_zero_extents, block_size_bitmask);

        if (err != 0)
            break;

        fr_vector<ft_uoff> super_loop_file_extents;
        err = super_type::read_extents_loop_file(super_loop_file_extents, to_zero_extents, block_size_bitmask);
        if (err != 0)
            break;

        // fr_io_posix::read_extents_loop_file() may add to its loop_file_extents
        // some extents that overlap with the ones computed above.
        // remove any such overlap!
        if (!loop_file_extents.empty() && !super_loop_file_extents.empty()) {

            ff_log(FC_DEBUG, 0, "merging %"FT_ULL" prealloc extents with %"FT_ULL" %s extents",
                    (ft_ull) loop_file_extents.size(), (ft_ull) super_loop_file_extents.size(), LABEL[FC_LOOP_FILE]);

            const ft_uoff eff_block_size_log2 = effective_block_size_log2(block_size_bitmask);
            const ft_uoff eff_block_size = (ft_uoff)1 << eff_block_size_log2;
            loop_file_extents.show("prealloc", " before merge", eff_block_size);
            super_loop_file_extents.show(LABEL[FC_LOOP_FILE], " before merge", eff_block_size);

            // use fr_map<T>::merge_shift() to merge.
            // unluckily it merges based on ->physical, so we must transpose the vectors
            loop_file_extents.transpose();
            super_loop_file_extents.transpose();

            fr_map<ft_uoff> map;
            enum { NO_SHIFT = 0 };
            map.append0_shift(loop_file_extents, NO_SHIFT);

            map.merge_shift(super_loop_file_extents, NO_SHIFT, FC_PHYSICAL1);

            loop_file_extents.assign(map.begin(), map.end());
            loop_file_extents.transpose();
            // loop_file_extents is now sorted by logical,
            // because fr_map<T> is intrinsically sorted by physical

            ff_log(FC_DEBUG, 0, "merge completed, result is %"FT_ULL" extents",
                    (ft_ull) loop_file_extents.size());

            loop_file_extents.show(LABEL[FC_LOOP_FILE], " after merge", eff_block_size);
        }

        // we must call super_type::read_extents_free_space() after preparing loop_file_extents because,
        // if ZERO-FILE is not specified, it will use loop_file_extents
        // (assuming it is sorted by ->logical) and compute free_space_extents as its complement
        err = super_type::read_extents_free_space(loop_file_extents, free_space_extents, to_zero_extents, block_size_bitmask);
        if (err != 0)
            break;

        // no guarantee to_zero_extents is already sorted, so we have to sort it before returning
        to_zero_extents.sort_by_logical();

        ret_block_size_bitmask = block_size_bitmask;

    } while (0);

    return err;
}



/** return true if 'stat' information is about a directory */
FT_INLINE static bool fr_io_posix_is_dir(const ft_stat & stat) {
    return S_ISDIR(stat.st_mode);
}

/** return true if 'stat' information is about a regular file */
FT_INLINE static bool fr_io_posix_is_file(const ft_stat & stat) {
    return S_ISREG(stat.st_mode);
}

/** return file type textual description */
FT_INLINE static const char * fr_io_posix_get_type(const ft_stat & stat) {
    if (S_ISDIR(stat.st_mode))
        return "directory";
    if (S_ISREG(stat.st_mode))
        return "file";
    if (S_ISLNK(stat.st_mode))
        return "symlink";
    if (S_ISCHR(stat.st_mode))
        return "character-device";
    if (S_ISBLK(stat.st_mode))
        return "block-device";
    if (S_ISFIFO(stat.st_mode))
        return "fifo";
    if (S_ISSOCK(stat.st_mode))
        return "socket";
    return "unknown";
}

#define FC_INVALID_FS_STR "invalid preallocated loop file system"

/**
 * check if inode is a hard link to an already examined file or special device,
 * and in case return true, otherwise return EAGAIN.
 *
 * do NOT call this method if inode is a directory!
 *
 * return -errno in case of errors.
 */

int fr_io_prealloc::hard_link(const char * src_path, const ft_stat & src_stat,
                              const char * dst_path, const ft_stat & dst_stat)
{
    if (S_ISDIR(src_stat.st_mode)) {
        ff_log(FC_FATAL, 0, "internal error, io_prealloc::hard_link(%s, ...) invoked on a directory!", src_path);
        return -EISDIR;
    }
    if (S_ISDIR(dst_stat.st_mode)) {
        ff_log(FC_FATAL, 0, "internal error, io_prealloc::hard_link(..., %s) invoked on a directory!", dst_path);
        return -EISDIR;
    }

    const ft_nlink src_nlink = src_stat.st_nlink, dst_nlink = dst_stat.st_nlink;

    if (src_nlink != dst_nlink) {
        ff_log(FC_ERROR, 0, "%s: '%s' has %"FT_ULL" link%s, while '%s' has %"FT_ULL" link%s",
                FC_INVALID_FS_STR,
                src_path, (ft_ull) src_nlink, src_nlink == 1 ? "" : "s",
                dst_path, (ft_ull) dst_nlink, dst_nlink == 1 ? "" : "s");
        return -EINVAL;
    }
    if (dst_nlink <= 1)
        // at most one link, cannot be in inode_cache
        return EAGAIN;

    /*
     * source inode has 2 or more links.
     * check if it is cached already, or add it to detect further links to the same file/device
     */
    const ft_inode dst_inode = dst_stat.st_ino;

    // (dst_nlink - 1) because we just found one link to this inode:
    // we expect to find (dst_nlink - 1) other links
    ft_nlink cached_nlink = dst_nlink - 1;
    
    int err = this_inode_cache->find_or_add(dst_inode, cached_nlink);
    if (err < 0)
        return err;
    
    if (err == 0) {
        // fake error to tell caller that inode was not in inode_cache
        return EAGAIN;
    }
    if (cached_nlink == 1) {
        // we do not expect further links to this inode
        return this_inode_cache->find_and_delete(dst_inode, cached_nlink);
    }
    // we expect further links to this inode
    return this_inode_cache->find_and_update(dst_inode, cached_nlink - 1);
}


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
int fr_io_prealloc::read_extents_dir(ft_io_posix_dir & src, const ft_string & dst,
                                     fr_vector<ft_uoff> & loop_file_extents,
                                     fr_vector<ft_uoff> & to_zero_extents,
                                     ft_uoff & ret_block_size_bitmask)
{
    ft_string src_file = src.path(); src_file += '/';
    ft_string dst_file = dst; dst_file += '/';

    ft_stat src_stat, dst_stat;
    ft_uoff block_size_bitmask = ret_block_size_bitmask;

    ft_io_posix_dirent * entry;
    const char * name;
    ft_size namelen;
    int err;


    /* recurse on directory contents */
    while ((err = src.next(entry)) == 0 && entry != NULL) {
        namelen = strlen(name = entry->d_name);

        // skip "." and ".."
        if ((namelen == 1 || namelen == 2) && !memcmp(name, "..", namelen))
            continue;

        src_file.resize(src.path().size() + 1); // truncate to src + '/'
        src_file.append(name, namelen);
        const char * src_path = src_file.c_str();

        if (src_file == loop_file_path)
            // skip loop file itself!
            continue;

        dst_file.resize(dst.size() + 1); // truncate to dst + '/'
        dst_file.append(name, namelen);
        const char * dst_path = dst_file.c_str();

        if ((err = ff_posix_stat(src_path, & src_stat)) != 0) {
            break;
        }
        if ((err = ff_posix_stat(dst_path, & dst_stat)) != 0) {
            break;
        }

        const char * src_type = fr_io_posix_get_type(src_stat);
        const char * dst_type = fr_io_posix_get_type(dst_stat);
        if (src_type != dst_type) {
            ff_log(FC_ERROR, 0, "%s: '%s' is a %s, while '%s' is a %s",
                    FC_INVALID_FS_STR, src_path, src_type, dst_path, dst_type);
            err = -EINVAL;
            break;
        }

        if (fr_io_posix_is_dir(src_stat)) {
            ft_io_posix_dir src_dir;
            if ((err = src_dir.open(src_file)) == 0) {

                err = read_extents_dir(src_dir, dst_file, loop_file_extents,
                                       to_zero_extents, block_size_bitmask);

                // no need, src_dir destructor calls close()
                // src_dir.close();
            }
            continue;
        }

        if ((err = hard_link(src_path, src_stat, dst_path, dst_stat)) == 0) {
            // no need to examine the file or special device:
            // it is a hard link to an already examined one
            continue;
        } else if (err == EAGAIN) {
            /* no luck with inode_cache, proceed as usual */
            err = 0;
        } else {
            /** hard link() failed */
            break;
        }

        if (fr_io_posix_is_file(src_stat)) {
            err = read_extents_file(src_path, src_stat, dst_path, dst_stat,
                                    loop_file_extents, to_zero_extents, block_size_bitmask);
        }
    }
    if (err == 0)
        ret_block_size_bitmask = block_size_bitmask;
    return err;
}

/**
 * match the actual extents from (src_path) file inside device
 * with the preallocated extents from (dst_path) file inside loop file.
 *
 * any preallocated extents in files inside loop file
 * which do NOT have a correspondence in files inside device
 * are added to to_zero_extents
 */
int fr_io_prealloc::read_extents_file(const char * src_path, const ft_stat & src_stat,
                                      const char * dst_path, const ft_stat & dst_stat,
                                      fr_vector<ft_uoff> & loop_file_extents,
                                      fr_vector<ft_uoff> & to_zero_extents,
                                      ft_uoff & ret_block_size_bitmask)
{
    ft_uoff block_size_bitmask = ret_block_size_bitmask;
    int err = 0;
    do {
        enum { SRC = 0, DST = 1, N = 2 };

        ft_uoff len[N] = { (ft_uoff) src_stat.st_size, (ft_uoff) dst_stat.st_size };
        if (len[SRC] != len[DST]) {
            ff_log(FC_ERROR, 0, "%s: file '%s' is %"FT_ULL" bytes, while file '%s' is %"FT_ULL" bytes",
                    FC_INVALID_FS_STR, src_path, (ft_ull) len[SRC], dst_path, (ft_ull) len[DST]);
            err = -EINVAL;
            break;
        }
        if (len[SRC] == 0)
            break;

        fr_vector<ft_uoff> extent[N];
        const char * const path[N] = { src_path, dst_path };

        // loop on { SRC, DST }
        for (ft_size i = 0; err == 0 && i < N; i++) {
            int fd;
            if ((fd = ::open(path[i], O_RDONLY)) < 0) {
                err = ff_log(FC_ERROR, errno, "error opening file '%s' inside %s", path[i], MP_LABEL[i]);
                break;
            }

            err = ff_read_extents_posix(fd, len[i], extent[i], block_size_bitmask);

            if (::close(fd) < 0)
                ff_log(FC_WARN, errno, "failed to close file '%s' inside %s", path[i], MP_LABEL[i]);
        }
        if (err != 0)
            break;
        fr_vector<ft_uoff> unmapped;

    // PROBLEM: dst file-system may have a smaller blocksize than src file-system
    // Consequence: extent[SRC] may have an 'unnecessary' tail fragment that cannot be mapped to dst
    // SOLUTION: explicitly check for this case and manually remove such tail
    if (!extent[SRC].empty() && !extent[DST].empty()) {
        fr_extent<ft_uoff> & e1 = extent[SRC].back(), & e2 = extent[DST].back();
        ft_uoff end1 = e1.logical() + e1.length();
        ft_uoff end2 = e2.logical() + e2.length();
        if (end1 > end2 && end2 >= len[SRC])
                extent[SRC].truncate_at_logical(end2);
        }
       
        if ((err = loop_file_extents.compose(extent[SRC], extent[DST], block_size_bitmask, unmapped)) != 0)
            break;

        for (ft_size i = 0, n = unmapped.size(); i < n; i++) {
            const fr_extent<ft_uoff> & e = unmapped[i];
            // to_zero_extents must have ->physical == ->logical
            // and in any case the ->logical extents are relative to each file,
            // so they cannot be mixed together
            to_zero_extents.append(e.physical(), e.physical(), e.length(), e.user_data());
        }

        ret_block_size_bitmask = block_size_bitmask;

    } while (0);
    return err;
}


FT_IO_NAMESPACE_END
