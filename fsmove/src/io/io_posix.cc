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
 * io/io_posix.cc
 *
 *  Created on: Sep 22, 2011
 *      Author: max
 */

#include "../first.hh"


#if defined(FT_HAVE_CERRNO)
# include <cerrno>         // for errno and error codes
#elif defined(FT_HAVE_ERRNO_H)
# include <errno.h>
#endif
#if defined(FT_HAVE_CLIMITS)
# include <climits>        // for PATH_MAX
#elif defined(FT_HAVE_LIMITS_H)
# include <limits.h>       // for PATH_MAX
#endif
#if defined(FT_HAVE_CSTDIO)
# include <cstdio>         // for rename()
#elif defined(FT_HAVE_STDIO_H)
# include <stdio.h>        // for rename()
#endif
#if defined(FT_HAVE_CSTRING)
# include <cstring>        // for strcmp(), memset(), memcpy()
#elif defined(FT_HAVE_STRING_H)
# include <string.h>       // for strcmp(), memset(), memcpy()
#endif

#ifdef FT_HAVE_DIRENT_H
# include <dirent.h>       // for opendir(), readdir(), closedir()
#endif
#ifdef FT_HAVE_FCNTL_H
# include <fcntl.h>        // for open(), mknod()
#endif
#ifdef FT_HAVE_SYS_STAT_H
# include <sys/stat.h>     // for   "        "    , lstat(), mkdir(), mkfifo(), umask()
#endif
#ifdef FT_HAVE_SYS_TYPES_H
# include <sys/types.h>    //  "    "        "        "        "        "         "    , lseek(), ftruncate()
#endif
#ifdef FT_HAVE_UNISTD_H
# include <unistd.h>       //  "    "        "        "   ,symlink(),lchown(), close(),    "          "     , readlink(), read(), write()
#endif
#ifdef FT_HAVE_SYS_STATVFS_H
# include <sys/statvfs.h>  // for statvfs(), fsblkcnt_t
#endif
#ifdef FT_HAVE_SYS_TIME_H
# include <sys/time.h>     // for utimes(), utimensat()
#endif
#ifdef FT_HAVE_UTIME_H
# include <utime.h>        //  "    "           "
#endif



#include "../assert.hh"    // for ff_assert()
#include "../log.hh"       // for ff_log()
#include "../misc.hh"      // for ff_min2()

#include "disk_stat.hh"    // for fm_disk_stat::THRESHOLD_MIN
#include "io_posix.hh"     // for fm_io_posix
#include "io_posix_dir.hh" // for ft_io_posix_dir
#include "util_posix.hh"   // for ff_posix_exec_silent()

#ifndef PATH_MAX
# define PATH_MAX 4096
#endif /* PATH_MAX */

FT_IO_NAMESPACE_BEGIN


/** default constructor */
fm_io_posix::fm_io_posix()
: super_type(), bytes_copied_since_last_check(0)
{ }

/** destructor. calls close() */
fm_io_posix::~fm_io_posix()
{
    close();
}

/** return true if this fr_io_posix is currently (and correctly) open */
bool fm_io_posix::is_open() const
{
    return !source_root().empty() && !target_root().empty();
}

/** check for consistency and open SOURCE_ROOT, TARGET_ROOT */
int fm_io_posix::open(const fm_args & args)
{
    int err;
    do {
        if ((err = super_type::open(args)) != 0)
            break;
        bytes_copied_since_last_check = 0;
        err = check_free_space();
    } while (0);
    return err;
}

/** close this I/O, including file descriptors */
void fm_io_posix::close()
{
    super_type::close();
    bytes_copied_since_last_check = 0;
}


/**
 * return true if estimated free space is enough to write 'bytes_to_write'
 * if first_check is true, does a more conservative estimation, requiring twice more free space than normal
 */
bool fm_io_posix::enough_free_space(ft_uoff bytes_to_write, bool first_time)
{
    ft_uoff half_free_space = ff_min2(source_stat().get_free(), target_stat().get_free()) >> 1;
    if (first_time)
        half_free_space >>= 1;

    return half_free_space > bytes_to_write
        && half_free_space - bytes_to_write > bytes_copied_since_last_check;
}

/**
 * add bytes_just_written to bytes_copied_since_last_check.
 *
 * if bytes_copied_since_last_check >= PERIODIC_CHECK_FREE_SPACE or >= 50% of free space,
 * reset bytes_copied_since_last_check to zero and call check_free_space()
 */
int fm_io_posix::periodic_check_free_space(ft_uoff bytes_just_written, ft_uoff bytes_to_write)
{
    add_work_done(bytes_just_written);

    bytes_copied_since_last_check += bytes_just_written;

    int err = 0;

    if (!enough_free_space(bytes_to_write)) {
        bytes_copied_since_last_check = 0;
        err = check_free_space();
    }
    return err;
}


/**
 * sync(), then call disk_stat() twice: one time on source_root() and another on target_root().
 * return error if statvfs() fails or if free disk space becomes critically low
 */
int fm_io_posix::check_free_space()
{
    sync(); // slow, but needed to get accurate disk stats when loop devices are involved
    int err = disk_stat(source_root().c_str(), source_stat());
    if (err == 0)
        err = disk_stat(target_root().c_str(), target_stat());
    return err;
}

/** call ::sync(). slow, but needed to get accurate disk stats when loop devices are involved */
void fm_io_posix::sync()
{
	::sync();
}

/**
 * fill 'disk_stat' with information about the file-system containing 'path'.
 * return error if statvfs() fails or if free disk space becomes critically low
 */
int fm_io_posix::disk_stat(const char * path, fm_disk_stat & disk_stat)
{
    struct statvfs buf;
    int err = 0;

    for (int i = 0; i < 2; i++) {
        if (::statvfs(path, & buf) != 0)
            return ff_log(FC_ERROR, errno, "failed to statvfs() `%s'", path);
        
        ft_uoff disk_total = (ft_uoff) buf.f_bsize * (ft_uoff) buf.f_blocks;
        ft_uoff disk_free =  (ft_uoff) buf.f_bsize * (ft_uoff) buf.f_bfree;
        disk_stat.set_total(disk_total);

        if (i == 0 && disk_stat.is_too_low_free_space(disk_free)) {
            try_to_make_free_space(path);
            continue;
        }
        err = disk_stat.set_free(disk_free);
        break;
    }
    return err;
}


/**
 * use some file-system specific trickery and try to free some space.
 */
void fm_io_posix::try_to_make_free_space(const char * path)
{
#if 0
    /* 
     * we COULD run 'xfs_fsr <path>' and try to free some space on 'xfs' file-systems,
     * but at least on linux with an almost-full source device
     * xfs_fsr can WORSEN the problem by triggering 'loop write error' kernel errors,
     * which mean the source device has not enough space to accommodate the loop file contents.
     * typically this CORRUPTS the file system inside target (loop) device!
     */
    const char * cmd = "xfs_fsr";
    const char * const args[] = { cmd, path, NULL };
    
    if (ff_posix_exec_silent(cmd, args) == 0)
        ff_log(FC_INFO, 0, "successfully executed '%s %s' to free some disk space", args[0], args[1]);
#endif
}


/**
 * return true if 'stat' information is about a directory
 */
FT_INLINE static bool fm_io_posix_is_dir(const ft_stat & stat)
{
    return S_ISDIR(stat.st_mode);
}

/**
 * return true if 'stat' information is about a regular file
 */
FT_INLINE static bool fm_io_posix_is_file(const ft_stat & stat)
{
    return S_ISREG(stat.st_mode);
}

/**
 * return true if 'stat' information is about a symbolic link
 */
FT_INLINE static bool fm_io_posix_is_symlink(const ft_stat & stat)
{
    return S_ISLNK(stat.st_mode);
}


/** core of recursive move algorithm, actually moves the whole source tree into target */
int fm_io_posix::move()
{
    if (move_rename(source_root().c_str(), target_root().c_str()) == 0)
        return 0;

    /* avoid messing up permissions of created files/directories/special-devices */
    umask(0);

    int err = init_work();
    if (err == 0)
        err = move(source_root(), target_root());
    if (err == 0)
    	ff_log(FC_NOTICE, 0, "job completed.");
    return err;
}


/**
 * move a single file/socket/special-device or a whole directory tree
 */
int fm_io_posix::move(const ft_string & source_path, const ft_string & target_path)
{
    ft_stat stat;
    const std::set<ft_string> & exclude_set = this->exclude_set();
    int err = 0;

    ff_log(FC_DEBUG, 0, "move()         `%s'\t-> `%s'", source_path.c_str(), target_path.c_str());

    do {
        if (exclude_set.count(source_path) != 0) {
            ff_log(FC_INFO, 0, "move() skipped `%s', matches exclude list", source_path.c_str());
            break;
        }

        if ((err = this->stat(source_path, stat)) != 0)
            break;

        if (fm_io_posix_is_file(stat)) {
            err = this->move_file(source_path, stat, target_path);
            break;
        } else if (!fm_io_posix_is_dir(stat)) {
            err = this->move_special(source_path, stat, target_path);
            break;
        }
        ft_io_posix_dir source_dir;
        if ((err = source_dir.open(source_path)))
            break;

        /*
         * we allow target_root() to exist already, but other target directories must NOT exist.
         * option '-f' drops this check, i.e. any target directory can exist already
         *
         * Exception: we allow a 'lost+found' directory to exist inside target_root()
         */
        if ((err = this->create_dir(target_path)) != 0)
            break;


        if ((err = this->periodic_check_free_space()) != 0)
            break;

        ft_string child_source = source_path, child_target = target_path;
        child_source += '/';
        child_target += '/';

        ft_io_posix_dirent * dirent;

        /* recurse on directory contents */
        while ((err = source_dir.next(dirent)) == 0 && dirent != NULL) {
            /* skip "." and ".." */
            if (!strcmp(".", dirent->d_name) || !strcmp("..", dirent->d_name))
                continue;

            child_source.resize(1 + source_path.size()); // faster than child_source = source_path + '/'
            child_source += dirent->d_name;

            child_target.resize(1 + target_path.size()); // faster than child_target = target_path + '/'
            child_target += dirent->d_name;

            if ((err = this->move(child_source, child_target)) != 0)
                break;
        }
        if (err != 0)
            break;
        if ((err = this->copy_stat(target_path.c_str(), stat)) != 0)
            break;
        /*
         * we do not delete 'lost+found' directory inside source_root()
         */
        if ((err = this->remove_dir(source_path)) != 0)
            break;

    } while (0);
    return err;
}


/**
 * fill 'stat' with information about the file/directory/special-device 'path'
 */
int fm_io_posix::stat(const ft_string & path, ft_stat & stat)
{
    const char * str = path.c_str();
    int err = 0;
    if (lstat(str, & stat) != 0)
        err = ff_log(FC_ERROR, errno, "failed to lstat() `%s'", str);
    return err;
}

/**
 * move the special-device 'source_path' to 'target_path'.
 */
int fm_io_posix::move_special(const ft_string & source_path, const ft_stat & stat, const ft_string & target_path)
{
    const char * source = source_path.c_str(), * target = target_path.c_str();
    int err = 0;
    ff_log(FC_TRACE, 0, "move_special() `%s'\t-> `%s'", source, target);

    if (simulate_run())
        return err;

    do {
        /* check inode_cache for hard links and recreate them */
        err = this->hard_link(stat, target_path);
        if (err == 0) {
            /** hard link succeeded, no need to create the special-device */
            err = this->periodic_check_free_space();
            break;
        } else if (err == EAGAIN) {
            /* no luck with inode_cache, proceed as usual */
            err = 0;
        } else {
            /** hard link() failed */
            return err;
        }

        /* found a special device */
        if (S_ISCHR(stat.st_mode) || S_ISBLK(stat.st_mode) || S_ISSOCK(stat.st_mode)) {
            if (mknod(target, (stat.st_mode | 0600) & ~0077, stat.st_rdev) != 0) {
                if (!S_ISSOCK(stat.st_mode)) {
                    err = ff_log(FC_ERROR, errno, "failed to create target special device `%s'", target);
                    break;
                }
                ff_log(FC_WARN, errno, "failed to create target UNIX socket `%s'", target);
            }
        } else if (S_ISFIFO(stat.st_mode)) {
            if (mkfifo(target, 0600) != 0) {
                err = ff_log(FC_ERROR, errno, "failed to create target named pipe `%s'", target);
                break;
            }
        } else if (fm_io_posix_is_symlink(stat)) {
            char link_to[PATH_MAX+1];
            ssize_t link_len = readlink(source, link_to, PATH_MAX);
            if (link_len == -1) {
                err = ff_log(FC_ERROR, errno, "failed to read source symbolic link `%s'", source);
                break;
            }
            link_to[link_len] = '\0';
            if (symlink(link_to, target) != 0) {
                err = ff_log(FC_ERROR, errno, "failed to create target symbolic link `%s'\t-> `%s'", target, link_to);
                break;
            }

        } else {
            ff_log(FC_ERROR, 0, "special device %s has unknown type 0%"FT_OLL", cannot create it",
                    source, (ft_ull)(stat.st_mode & ~07777));
            err = -EOPNOTSUPP;
            break;
        }

        if ((err = this->copy_stat(target, stat)) != 0)
            break;

        if ((err = this->periodic_check_free_space()) != 0)
            break;

    } while (0);

    if (err == 0)
        err = remove_special(source);

    return err;
}

/**
 * remove the special file 'source_path'
 */
int fm_io_posix::remove_special(const char * source_path)
{
    int err = 0;
    if (::remove(source_path) != 0)
        err = ff_log(FC_ERROR, errno, "failed to remove source special device `%s'", source_path);
    return err;
}

/**
 * remove the regular file 'source_path'
 */
int fm_io_posix::remove_file(const char * source_path)
{
    int err = 0;
    if (::remove(source_path) != 0)
        err = ff_log(FC_ERROR, errno, "failed to remove source file `%s'", source_path);
    return err;
}

/**
 * move the regular file 'source_path' to 'target_path'.
 */
int fm_io_posix::move_file(const ft_string & source_path, const ft_stat & stat, const ft_string & target_path)
{
    const char * source = source_path.c_str(), * target = target_path.c_str();
    int err = 0;
    ff_log(FC_TRACE, 0, "move_file()    `%s'\t-> `%s'", source, target);

    if (simulate_run())
        return err;

    /* check inode_cache for hard links and recreate them */
    err = this->hard_link(stat, target_path);
    if (err == 0) {
        /** hard link succeeded, no need to copy the file contents */
        err = this->periodic_check_free_space();
        goto move_file_remove_source;
    } else if (err != EAGAIN) {
        /** hard link failed */
        return err;
    }
    
    /* no luck with inode_cache, proceed as usual */
    err = copy_file_contents(source_path, stat, target_path);
    
move_file_remove_source:
    if (err == 0)
        err = remove_file(source);
    return err;
}


/**
 * copy the contents of regular file 'source_path' to 'target_path'.
 */
int fm_io_posix::copy_file_contents(const ft_string & source_path, const ft_stat & stat, const ft_string & target_path)
{
    const char * source = source_path.c_str(), * target = target_path.c_str();
    int err = 0;
    
    int in_fd = ::open(source, O_RDWR);
    if (in_fd < 0)
        err = ff_log(FC_ERROR, errno, "failed to open source file `%s'", source);

#ifndef O_EXCL
# define O_EXCL 0
#endif
    int out_fd = ::open(target, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL, 0600);
    if (out_fd < 0)
        err = ff_log(FC_ERROR, errno, "failed to create target file `%s'", target);

    if (err == 0) {
        err = this->periodic_check_free_space();
        if (err == 0)
            err = this->copy_stream(in_fd, out_fd, stat, source, target);
    }
    if (in_fd >= 0)
        (void) ::close(in_fd);
    if (out_fd >= 0)
        (void) ::close(out_fd);

    if (err == 0)
        err = this->copy_stat(target, stat);

    return err;
}

/**
 * try to rename a file, directory or special-device from 'source_path' to 'target_path'.
 */
int fm_io_posix::move_rename(const char * source, const char * target)
{
    int err = 0;
    do {
        if (simulate_run()) {
            err = EXDEV;
            break;
        }
        if (::rename(source, target) != 0) {
            err = errno;
            break;
        }
        ff_log(FC_TRACE, 0, "move_rename()  `%s'\t-> `%s': success", source, target);

    } while (0);
    return err;
}

/**
 * check inode_cache for hard links and recreate them.
 * must be called if and only if stat.st_nlink > 1
 *
 * returns EAGAIN if inode *was* not in inode_cache
 */
int fm_io_posix::hard_link(const ft_stat & stat, const ft_string & target_path)
{
    ft_string cached_link = target_path;
    int err;

    if (stat.st_nlink > 1) {
        /*
         * source path has 2 or more links.
         * check if it is cached already, or add it to detect further links to the same file/device
         */
        err = inode_cache_find_or_add(stat.st_ino, cached_link);
    } else {
        /*
         * source path has only 1 link. it can be either:
         * a) the last link of a file/device which previously had multiple links,
         *    but we all other links during fm_io_posix::move()
         * b) a file/device which always had one link
         *
         * so we check for its presence in inode_cache, but we do not add it to inode_cache
         * in any case, if a cached inode is found, we erase it
         * because it is guaranteed that no more links to this inode will ever be found.
         */
    	err = inode_cache_find_and_delete(stat.st_ino, cached_link);
    }

    if (err == 0) {
    	// fake error to tell caller that inode was not in cache
    	err = EAGAIN;
    }
    else if (err == 1) {
    	// inode found in cache
        const char * link_to = cached_link.c_str(), * link_from = target_path.c_str();
        if (::link(link_to, link_from) != 0)
            err = ff_log(FC_ERROR, errno, "failed to create target hard link `%s'\t-> `%s'", link_from, link_to);
        else
        	err = 0;
    }
    return err;
}

enum {
    FT_LOG_BUFSIZE = 16, //< log2(FT_BUFSIZE)

    // FT_BUFSIZE must be a power of 2 (currently 64k),
    // and must be somewhat smaller than fm_disk_stat::THRESHOLD_MIN (currently 96k)
    FT_BUFSIZE = (ft_size)1 << FT_LOG_BUFSIZE,

    FT_BUFSIZE_m1 = FT_BUFSIZE - 1,

    FT_BUFSIZE_SANITY_CHECK = sizeof(char[FT_BUFSIZE * 3 / 2 >= fm_disk_stat::THRESHOLD_MIN ? 1 : -1])
};

/**
 * forward or backward copy file/stream contents from in_fd to out_fd.
 *
 * if disk space is low, we copy backward and progressively truncate in_fd to conserve space:
 * results in heavy fragmentation on target file, but at least we can continue
 */
int fm_io_posix::copy_stream(int in_fd, int out_fd, const ft_stat & stat, const char * source, const char * target)
{
    ft_off file_size = stat.st_size;
    int err;

    if ((err = periodic_check_free_space(0, file_size)) != 0)
        return err;

    if (enough_free_space(file_size)) {
        /* enough free space, use normal forward copy */
        return copy_stream_forward(in_fd, out_fd, source, target);
    }

    /* not enough free space, use backward copy + progressively truncate source file */
    double pretty_size = 0.0;
    const char * pretty_label = ff_pretty_size((ft_uoff) file_size, & pretty_size);
    ff_log(FC_INFO, 0, "using backward copy and truncate for file `%s': less than %.2f %sbytes free space left",
           target, pretty_size, pretty_label);

    if (::lseek(in_fd, 0, SEEK_END) != file_size)
        return ff_log(FC_ERROR, errno, "error seeking to end of file `%s'", source);

    ft_off offset_high = file_size, offset_low;

    if ((err = fd_truncate(out_fd, offset_high, target)) != 0)
        return err;

    // slow, but on Linux not doing it is worse:
    // you can get inaccurate disk usage statistics
    // and (if loop device becomes full) silent I/O errors!
    sync();

    char buf[FT_BUFSIZE];

    ft_size expected, got;
    ft_size hole_len, nonhole_len, tosend_offset, tosend_left;
    while (offset_high > 0) {
        /** truncate in_fd, discarding any data that we already copied */
        if ((err = fd_truncate(in_fd, offset_high, source)) != 0)
            break;

        offset_low = (offset_high - 1) & ~(ft_off)FT_BUFSIZE_m1;
        ff_assert(offset_high - offset_low <= FT_BUFSIZE);
        got = expected = (ft_size)(offset_high - offset_low);

        if ((err = fd_seek2(in_fd, out_fd, offset_low, source, target)) != 0)
            break;

        if ((err = this->full_read(in_fd, buf, got, source)) != 0 || got != expected) {
            if (err == 0) {
                ff_log(FC_ERROR, 0, "error reading from `%s': expected %"FT_ULL" bytes, got %"FT_ULL" bytes",
                        source, (ft_ull)expected, (ft_ull)got);
                err = -EIO;
            }
            break;
        }

        tosend_offset = 0;
        for (tosend_left = got; tosend_left != 0; ) {
            /* detect hole */
            if ((hole_len = hole_length(buf + tosend_offset, tosend_left)) != 0) {
                /* re-create hole in target file */
                if (::lseek(out_fd, (ft_off)hole_len, SEEK_CUR) == (ft_off)-1) {
                    err = ff_log(FC_ERROR, errno, "error seeking %"FT_ULL" bytes forward in file `%s'", (ft_ull)hole_len, target);
                    break;
                }
                tosend_offset += hole_len;
                tosend_left -= hole_len;
            }

            if ((nonhole_len = nonhole_length(buf + tosend_offset, tosend_left)) != 0) {
                // copy the non-hole data
                if ((err = this->full_write(out_fd, buf + tosend_offset, nonhole_len, target)) != 0)
                    break;
                tosend_offset += nonhole_len;
                tosend_left -= nonhole_len;
            }
        }
        if (err != 0)
            break;
        offset_high = offset_low;
    }
    if (err != 0 && offset_high != 0 && offset_high != file_size) {
        ff_log(FC_ERROR, 0, "DANGER! due to previous error, copying `%s' -> `%s' was aborted", source, target);
        ff_log(FC_ERROR, 0, "        and BOTH copies of this file are now incomplete.");
        ff_log(FC_ERROR, 0, "        To recover this file, execute the following command");
        ff_log(FC_ERROR, 0, "        AFTER freeing enough space in the source device:");

        offset_high >>= FT_LOG_BUFSIZE;
        ff_log(FC_ERROR, 0, "          /bin/dd bs=%"FT_ULL" skip=%"FT_ULL" seek=%"FT_ULL" conv=notrunc if=\"%s\" of=\"%s\"",
                (ft_ull)FT_BUFSIZE, (ft_ull)offset_high, (ft_ull)offset_high, target, source);
    }
    return err;
}


/**
 * forward copy file/stream contents from in_fd to out_fd.
 */
int fm_io_posix::copy_stream_forward(int in_fd, int out_fd, const char * source, const char * target)
{
    char buf[FT_BUFSIZE];

    ft_size present = 0, present_aligned, got;
    ft_size hole_len, nonhole_len, tosend_offset, tosend_left;
    int err = 0;
    for (;;) {
        got = FT_BUFSIZE - present;
        if ((err = this->full_read(in_fd, buf + present, got, source)) != 0 || got == 0)
            break;

        tosend_left = present_aligned = (present += got) / APPROX_BLOCK_SIZE * APPROX_BLOCK_SIZE;

        for (tosend_offset = 0; tosend_left != 0;) {
            /* detect hole */
            if ((hole_len = hole_length(buf + tosend_offset, tosend_left)) != 0) {
                /* re-create hole in target file */
                if (::lseek(out_fd, (ft_off)hole_len, SEEK_CUR) == (ft_off)-1) {
                    err = ff_log(FC_ERROR, errno, "error seeking in file `%s'", target);
                    break;
                }
                tosend_offset += hole_len;
                tosend_left -= hole_len;
            }

            if ((nonhole_len = nonhole_length(buf + tosend_offset, tosend_left)) != 0) {
                // copy the non-hole data
                if ((err = this->full_write(out_fd, buf + tosend_offset, nonhole_len, target)) != 0)
                    break;
                tosend_offset += nonhole_len;
                tosend_left -= nonhole_len;
            }
        }
        if (err != 0)
            break;

        if (present_aligned != 0 && present > present_aligned)
            // move any remaining unaligned fragment to buffer beginning
            ::memmove(buf, buf + present_aligned, present - present_aligned);
        present -= present_aligned;
        present_aligned = 0;
    }

    if (err == 0) do {
        if (present > present_aligned) {
            // write any remaining unaligned fragment
            err = this->full_write(out_fd, buf + present_aligned, present - present_aligned, target);
            break;
        }

        // file may end with a hole... handle this case correctly!
        ft_off filelen = ::lseek(out_fd, (ft_off)0, SEEK_CUR);
        if (filelen == (ft_off)-1) {
            err = ff_log(FC_ERROR, errno, "error seeking in file `%s'", target);
            break;
        }
        if ((err = fd_truncate(out_fd, filelen, target)) != 0)
            break;

    } while (0);

    return err;
}


/**
 * truncate file pointed by descriptor to specified length
 */
int fm_io_posix::fd_truncate(int fd, ft_off length, const char * path)
{
    int err = 0;
    if (::ftruncate(fd, length) == -1)
        err = ff_log(FC_ERROR, errno, "error truncating file `%s' to %"FT_ULL" bytes", path, (ft_ull)length);
    return err;
}

/**
 * seek to specified position of *both* fd1 and fd2
 */
int fm_io_posix::fd_seek2(int fd1, int fd2, ft_off offset, const char * path1, const char * path2)
{
    int err = fd_seek(fd1, offset, path1);
    if (err == 0)
        err = fd_seek(fd2, offset, path2);
    return err;
}

/**
 * seek to specified position of file descriptor
 */
int fm_io_posix::fd_seek(int fd, ft_off offset, const char * path)
{
    int err = 0;
    if (::lseek(fd, offset, SEEK_SET) != offset)
        err = ff_log(FC_ERROR, errno, "error seeking to position %"FT_ULL" of file `%s'", (ft_ull)offset, path);
    return err;
}

/**
 * scan memory for blocksize-length and blocksize-aligned sections full of zeroes
 * return length of zeroed area at the beginning of scanned memory.
 * returned length is rounded down to block_size
 */
size_t fm_io_posix::hole_length(const char * mem, ft_size mem_len)
{
    size_t len = 0;
    /* blocks smaller than APPROX_BLOCK_SIZE are always considered non-hole */
    mem_len = (mem_len / APPROX_BLOCK_SIZE) * APPROX_BLOCK_SIZE;
    for (; len < mem_len; len++)
        if (mem[len] != '\0')
            break;
    return (len / APPROX_BLOCK_SIZE) * APPROX_BLOCK_SIZE;
}

/**
 * scan memory for blocksize-length and blocksize-aligned sections NOT full of zeroes
 * return length of NON-zeroed area at the beginning of scanned memory.
 * returned length is rounded UP to block_size if it fits mem_len
 */
size_t fm_io_posix::nonhole_length(const char * mem, ft_size mem_len)
{
    size_t hole_len, offset = 0;

    while (mem_len >= APPROX_BLOCK_SIZE && (hole_len = hole_length(mem + offset, APPROX_BLOCK_SIZE)) == 0) {
        offset += APPROX_BLOCK_SIZE;
        mem_len -= APPROX_BLOCK_SIZE;
    }
    /*
     * blocks smaller than APPROX_BLOCK_SIZE,
     * or final fragments smaller than APPROX_BLOCK_SIZE,
     * are always considered non-hole
     */
    if (mem_len < APPROX_BLOCK_SIZE)
        offset += mem_len;
    return offset;
}

/**
 * read bytes from in_fd, retrying in case of short reads or interrupted system calls.
 * returns 0 for success, else error.
 * on return, len will contain the number of bytes actually read
 */
int fm_io_posix::full_read(int in_fd, char * data, ft_size & len, const char * source_path)
{
    ft_size got, left = len;
    int err = 0;
    while (left) {
        while ((got = ::read(in_fd, data, len)) == (ft_size)-1 && errno == EINTR)
            ;
        if (got == 0 || got == (ft_size)-1) {
            if (got != 0)
                err = ff_log(FC_ERROR, errno, "error reading from `%s'", source_path);
            // else got == 0: end-of-file
            break;
        }
        left -= got;
        data += got;
    }
    len -= left;
    return err;
}



/**
 * write bytes to out_fd, retrying in case of short writes or interrupted system calls.
 * returns 0 for success, else error
 */
int fm_io_posix::full_write(int out_fd, const char * data, ft_size len, const char * target_path)
{
    ft_size chunk;
    int err = 0;
    while (len) {
        while ((chunk = ::write(out_fd, data, len)) == (ft_size)-1 && errno == EINTR)
            ;
        if (chunk == 0 || chunk == (ft_size)-1) {
            err = ff_log(FC_ERROR, errno, "error writing to `%s'", target_path);
            break;
        }
        data += chunk;
        len -= chunk;

        if ((err = this->periodic_check_free_space(chunk)) != 0)
            break;
    }
    return err;
}

/**
 * copy the permission bits, owner/group and timestamps from 'stat' to 'target'
 */
int fm_io_posix::copy_stat(const char * target, const ft_stat & stat)
{
    int err = 0;
    if (simulate_run())
        return err;

    const char * label = fm_io_posix_is_dir(stat) ? "directory" : fm_io_posix_is_file(stat) ? "file" : "special device";

    /* copy timestamps */
#if defined(FT_HAVE_UTIMENSAT) && defined(AT_FDCWD) && defined(AT_SYMLINK_NOFOLLOW)
    do {
        struct timespec time_buf[2];
        time_buf[0].tv_sec = stat.st_atime;
        time_buf[1].tv_sec = stat.st_mtime;
# if defined(FT_HAVE_STRUCT_STAT_ST_ATIM_TV_NSEC)
        time_buf[0].tv_nsec = stat.st_atim.tv_nsec;
# elif defined(FT_HAVE_STRUCT_STAT_ST_ATIMENSEC)
        time_buf[0].tv_nsec = stat.st_atimensec;
# endif

# if defined(FT_HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC)
        time_buf[1].tv_nsec = stat.st_mtim.tv_nsec;
# elif defined(FT_HAVE_STRUCT_STAT_ST_MTIMENSEC)
        time_buf[1].tv_nsec = stat.st_mtimensec;
# endif
        if (utimensat(AT_FDCWD, target, time_buf, AT_SYMLINK_NOFOLLOW) != 0)
            ff_log(FC_WARN, errno, "cannot change timestamps on %s `%s'", label, target);

    } while (0);
#elif defined(FT_HAVE_UTIMES)
    /* utimes() does not work on symbolic links */
    if (!fm_io_posix_is_symlink(stat)) {
        struct timeval time_buf[2];
        time_buf[0].tv_sec = stat.st_atime;
        time_buf[1].tv_sec = stat.st_mtime;
        time_buf[0].tv_usec = time_buf[1].tv_usec = 0;

        if (utimes(target, time_buf) != 0)
            ff_log(FC_WARN, errno, "cannot change timestamps on %s `%s'", label, target);
    }
#else
# warning utimensat() and utimes() are both missing. fsmove will not be able to set timestamps of copied files/directories
#endif

    do {
        bool is_error = !force_run();
        bool is_symlink = fm_io_posix_is_symlink(stat);
        const char * fail_label = is_error ? "failed to" : "cannot";

        /* copy owner and group. this resets any SUID bits */

#if defined(FT_HAVE_LCHOWN)
        if (lchown(target, stat.st_uid, stat.st_gid) != 0)
#else
        if (!is_symlink && chown(target, stat.st_uid, stat.st_gid) != 0)
#endif
        {
            err = ff_log(is_error ? FC_ERROR : FC_WARN, errno,
                    "%s set owner=%"FT_ULL" and group=%"FT_ULL" on %s `%s'",
                    fail_label, (ft_ull)stat.st_uid, (ft_ull)stat.st_gid, label, target);
            if (is_error)
                break;
            err = 0;
        }
        /*
         * copy permission bits
         * 1. chmod() on a symbolic link has no sense, don't to it
         * 2. chmod() must be performed AFTER lchown(), because lchown() resets any SUID bits
         */
        if (!is_symlink && chmod(target, stat.st_mode) != 0) {
            err = ff_log(is_error ? FC_ERROR : FC_WARN, errno,
                    "%s change mode to 0%"FT_OLL" on %s `%s'",
                    fail_label, (ft_ull)stat.st_mode, label, target);
            if (is_error)
                break;
            err = 0;
        }

    } while (0);
    return err;
}

/**
 * return true if path is the target directory lost+found.
 * Treated specially because it is emptied but not removed.
 */
bool fm_io_posix::is_source_lost_found(const ft_string & path) const
{
    return path == source_root() + "/lost+found";
}


/**
 * return true if path is the target directory lost+found.
 * Treated specially because it is allowed to exist already.
 */
bool fm_io_posix::is_target_lost_found(const ft_string & path) const
{
    return path == target_root() + "/lost+found";
}


/** create a target directory, copying its mode and other meta-data from 'stat' */
int fm_io_posix::create_dir(const ft_string & path)
{
    const char * dir = path.c_str();
    int err = 0;
    ff_log(FC_TRACE, 0, "create_dir()   `%s'", dir);

    do {
        if (simulate_run())
            break;
        if (::mkdir(dir, 0700) == 0)
            break;

        /* if creating target root, ignore EEXIST error: target root is allowed to exist already */
        if ((err = errno) != EEXIST || path != target_root()) {
            /* if force_run(), always ignore EEXIST error: any target directory is allowed to exist already */
            /* in any case, we also allow target directory lost+found to exist already */
            bool is_warn = err == EEXIST && (force_run() || is_target_lost_found(path));
            err = ff_log(is_warn ? FC_WARN : FC_ERROR, err, "failed to create target directory `%s'", dir);
            if (!is_warn)
                break;
        }
        err = 0;

    } while (0);
    return err;
}

/**
 * remove a source directory.
 * exception: we do not delete 'lost+found' directory inside source_root()
 */
int fm_io_posix::remove_dir(const ft_string & path)
{
    const char * dir = path.c_str();
    int err = 0;
    ff_log(FC_TRACE, 0, "remove_dir()   `%s'", dir);
    do {
        if (simulate_run() || is_source_lost_found(path))
            break;

        if (::remove(dir) != 0) {
            /* ignore error if we are removing source root: it is allowed to be in use */
            if (path != source_root()) {
                /* if force_run(), failure to remove a source directory is just a warning */
                bool is_warn = force_run();
                err = ff_log(is_warn ? FC_WARN : FC_ERROR, errno, "%sfailed to remove source directory `%s'", is_warn ? "warning: " : "", dir);
                if (!is_warn)
                    break;
                err = 0;
            }
            break;
        }
    } while (0);
    return err;
}

FT_IO_NAMESPACE_END
