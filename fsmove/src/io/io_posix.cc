/*
 * io/io_posix.cc
 *
 *  Created on: Sep 22, 2011
 *      Author: max
 */

#include "../first.hh"

#include <cerrno>          // for errno
#include <climits>         // for PATH_MAX
#include <cstdio>          // for rename()
#include <cstring>         // for strcmp(), memset(), memcpy()
#include <dirent.h>        // for opendir(), readdir(), closedir()
#include <fcntl.h>         // for open(), mknod()
#include <sys/stat.h>      // for   "        "    , lstat(), mkdir(), mkfifo(), umask()
#include <sys/types.h>     //  "    "        "        "        "        "         "    , lseek(), ftruncate()
#include <unistd.h>        //  "    "        "        "    , rmdir(), lchown(), close(),    "          "     , readlink(), symlink(), unlink(), read(), write()
#include <sys/time.h>      // for utimes(), utimensat()
#include <utime.h>         //  "    "           "


#define FT_HAVE_UTIMENSAT /* define if utimensat() is supported */
#define FT_HAVE_STRUCT_STAT_XTIM_TV_NSEC /* define if struct stat has fields st_atim.tv_nsec and st_mtim.tv_nsec */


#include "../log.hh"       // for ff_log()

#include "io_posix.hh"     // for fm_io_posix
#include "io_posix_dir.hh" // for fm_io_posix_dir

#ifndef PATH_MAX
# define PATH_MAX 4096
#endif /* PATH_MAX */

FT_IO_NAMESPACE_BEGIN


/** default constructor */
fm_io_posix::fm_io_posix()
: super_type()
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
    return super_type::open(args);
}

/** close this I/O, including file descriptors to DEVICE, LOOP-FILE, ZERO-FILE and SECONDARY-STORAGE */
void fm_io_posix::close()
{
    super_type::close();
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


/** core of recursive move algorithm, actually moves the whole source tree into target */
int fm_io_posix::move()
{
    if (move_rename(source_root().c_str(), target_root().c_str()) == 0)
        return 0;

    /* avoid messing up permissions of created files/directories/special-devices */
    umask(0);

    return move(source_root(), target_root());
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
        fm_io_posix_dir source_dir;
        if ((err = source_dir.open(source_path)))
            break;

        /*
         * we allow target_root() to exist already, but other target directories must NOT exist.
         * option '-f' drops this check, i.e. any target directory can exist already
         */
        if ((err = this->create_dir(target_path, stat)) != 0)
            break;

        ft_string child_source = source_path, child_target = target_path;
        child_source += '/';
        child_target += '/';

        fm_io_posix_dirent * dirent;

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
        err = hard_link(stat, target_path);
        if (err == 0) {
            /** hard link succeeded, no need to create the special-device */
            break;
        } else if (err == EAGAIN) {
            /* no luck with inode_cache, proceed as usual */
            err = 0;
        } else {
            /** hard link failed */
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
        } else if (S_ISLNK(stat.st_mode)) {
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

        if ((err = copy_stat(target, stat)) != 0)
            break;

    } while (0);
   
    if (err == 0 && unlink(source) != 0)
        err = ff_log(FC_ERROR, errno, "failed to remove source special device `%s'", source);

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
    err = hard_link(stat, target_path);
    if (err == 0) {
        /** hard link succeeded, no need to copy the file contents */
        goto move_file_unlink_source;
    } else if (err == EAGAIN) {
        /* no luck with inode_cache, proceed as usual */
        err = 0;
    } else {
        /** hard link failed */
        return err;
    }

    {
        int in_fd = ::open(source, O_RDONLY);
        if (in_fd < 0)
            err = ff_log(FC_ERROR, errno, "failed to open source file `%s'", source);

#ifndef O_EXCL
# define O_EXCL 0
#endif
        int out_fd = ::open(target, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL, 0600);
        if (out_fd < 0)
            err = ff_log(FC_ERROR, errno, "failed to create target file `%s'", target);

        if (err == 0)
            err = copy_stream(in_fd, out_fd, source, target);

        if (in_fd >= 0)
            (void) ::close(in_fd);
        if (out_fd >= 0)
            (void) ::close(out_fd);
    }

    if (err == 0)
        err = copy_stat(target, stat);

move_file_unlink_source:
    if (err == 0) {
        if (unlink(source) != 0)
            err = ff_log(FC_ERROR, errno, "failed to remove source file `%s'", source);
    }
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
        if (rename(source, target) != 0) {
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
 * returns EAGAIN if inode was not in inode_cache
 */
int fm_io_posix::hard_link(const ft_stat & stat, const ft_string & target_path)
{
    const ft_string * cached_link, * to_erase = NULL;

    if (stat.st_nlink > 1)
        /*
         * source path has 2 or more links.
         * check if it is cached already, or add it to detect further links to the same file/device
         */
        cached_link = inode_cache_find_or_add(stat.st_ino, target_path);
    else
        /*
         * source path has only 1 link. it can be either:
         * a) the last link of a file/device which previously had multiple links, but we removed them during fm_io_posix::move()
         * b) a file/device which always had one link
         *
         * so we check for its presence in inode_cache, but we do not add it to inode_cache
         * in any case, if a cached inode is found, we will erase it below with inode_cache_erase()
         * because it is guaranteed that no more links to this inode will ever be found.
         */
        cached_link = to_erase = inode_cache_find(stat.st_ino, target_path);

    int err = 0;
    do {
        if (cached_link == NULL) {
            // fake error to tell caller that inode was not in inode_cache
            err = EAGAIN;
            break;
        }

        const char * link_to = cached_link->c_str(), * link_from = target_path.c_str();
        if (link(link_to, link_from) != 0) {
            err = ff_log(FC_ERROR, errno, "failed to create target hard link `%s'\t-> `%s'", link_from, link_to);
            break;
        }

    } while (0);

    if (to_erase != NULL)
        inode_cache_erase(stat.st_ino);

    return err;
}

/**
 * copy file/stream contents from in_fd to out_fd
 */
int fm_io_posix::copy_stream(int in_fd, int out_fd, const char * source, const char * target)
{
    char buf[65536];

    ft_size present = 0, present_aligned, got;
    ft_size hole_len, nonhole_len, tosend_offset, tosend_left;
    int err = 0;
    for (;;) {
        got = (ft_size) read(in_fd, buf + present, sizeof(buf) - present);
        if (got == (ft_size)-1)
            err = ff_log(FC_ERROR, errno, "error reading from `%s'", source);
        if (got == (ft_size)-1 || got == 0)
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
                if ((err = full_write(out_fd, buf + tosend_offset, nonhole_len, target)) != 0)
                    break;
                tosend_offset += nonhole_len;
                tosend_left -= nonhole_len;
            }
        }
        if (err != 0)
            break;

        if (present > present_aligned)
            // move any remaining unaligned fragment to buffer beginning
            ::memmove(buf, buf + present_aligned, present - present_aligned);
        present -= present_aligned;
        present_aligned = 0;
    }

    if (err == 0) do {
        if (present > present_aligned) {
            // write any remaining unaligned fragment
            err = full_write(out_fd, buf + present_aligned, present - present_aligned, target);
            break;
        }

        // file may end with a hole... handle this case correctly!
        ft_off filelen = ::lseek(out_fd, (ft_off)0, SEEK_CUR);
        if (filelen == (ft_off)-1) {
            err = ff_log(FC_ERROR, errno, "error seeking in file `%s'", target);
            break;
        }
        if (::ftruncate(out_fd, filelen) == -1) {
            err = ff_log(FC_ERROR, errno, "error truncating file `%s' to %"FT_ULL" bytes", target, (ft_ull)filelen);
            break;
        }
    } while (0);

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
    mem_len = (mem_len / APPROX_BLOCK_SIZE) * APPROX_BLOCK_SIZE;
    for (; len < mem_len; len++)
        if (mem[len] != '\0')
            break;
    return (len / APPROX_BLOCK_SIZE) * APPROX_BLOCK_SIZE;
}

/**
 * scan memory for blocksize-length and blocksize-aligned sections NOT full of zeroes
 * return length of NON-zeroed area at the beginning of scanned memory.
 * returned length is rounded UP to block_size
 */
size_t fm_io_posix::nonhole_length(const char * mem, ft_size mem_len)
{
    size_t hole_len, offset = 0;
    while (mem_len >= APPROX_BLOCK_SIZE && (hole_len = hole_length(mem + offset, mem_len)) == 0) {
        offset += APPROX_BLOCK_SIZE;
        mem_len -= APPROX_BLOCK_SIZE;
    }
    return offset;
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
	    while ((chunk = ::write(out_fd, data, len)) == -1 && errno == EINTR)
	        ;
	    if (chunk <= 0) {
	        err = ff_log(FC_ERROR, errno, "error writing to `%s'", target_path);
	        break;
	    }
	    data += chunk;
	    len -= chunk;
	}
	return err;
}

/**
 * copy the permission bits, owner/group and timestamps from 'stat' to 'target'
 */
int fm_io_posix::copy_stat(const char * target, const ft_stat & stat)
{
    int err = 0;
    const char * label = fm_io_posix_is_dir(stat) ? "directory" : fm_io_posix_is_file(stat) ? "file" : "special device";

    /* copy timestamps */
#if defined(FT_HAVE_UTIMENSAT) && defined(AT_FDCWD) && defined(AT_SYMLINK_NOFOLLOW) && defined(FT_HAVE_STRUCT_STAT_XTIM_TV_NSEC)
    do {
        struct timespec time_buf[2];
        time_buf[0].tv_sec = stat.st_atime;
        time_buf[0].tv_nsec = stat.st_atim.tv_nsec;
        time_buf[1].tv_sec = stat.st_mtime;
        time_buf[1].tv_nsec = stat.st_mtim.tv_nsec;

        if (utimensat(AT_FDCWD, target, time_buf, AT_SYMLINK_NOFOLLOW) != 0)
            ff_log(FC_WARN, errno, "warning: cannot change timestamps on %s `%s'", label, target);

    } while (0);
#else
    /* utimes() does not work on symbolic links */
    if (!S_ISLNK(stat.st_mode)) {
        struct timeval time_buf[2];
        time_buf[0].tv_sec = stat.st_atime;
        time_buf[1].tv_sec = stat.st_mtime;
        time_buf[0].tv_usec = time_buf[1].tv_usec = 0;

        if (utimes(target, time_buf) != 0)
            ff_log(FC_WARN, errno, "warning: cannot change timestamps on %s `%s'", label, target);
    }
#endif

    do {
        bool is_error = force_run();
        const char * fail_label = is_error ? "failed to" : "warning: cannot";

        /* copy owner and group. this resets any SUID bits */
        if (lchown(target, stat.st_uid, stat.st_gid) != 0) {
            err = ff_log(FC_ERROR, errno, "%s set owner=%"FT_ULL" and group=%"FT_ULL" on %s `%s'",
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
        if (!S_ISLNK(stat.st_mode) && chmod(target, stat.st_mode) != 0) {
            err = ff_log(FC_ERROR, errno, "% change mode to 0%"FT_OLL" on %s `%s'",
                         fail_label, (ft_ull)stat.st_mode, label, target);
            if (is_error)
                break;
            err = 0;
        }

    } while (0);
    return err;
}

/** create a target directory, copying its mode and other meta-data from 'stat' */
int fm_io_posix::create_dir(const ft_string & path, const ft_stat & stat)
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
            bool is_warn = err == EEXIST && force_run();
            err = ff_log(is_warn ? FC_WARN : FC_ERROR, err, "%sfailed to create target directory `%s'", is_warn ? "warning: " : "", dir);
            if (!is_warn)
                break;
        }
        err = 0;

    } while (0);
    return err;
}

/** remove a source directory */
int fm_io_posix::remove_dir(const ft_string & path)
{
    const char * dir = path.c_str();
    int err = 0;
    ff_log(FC_TRACE, 0, "remove_dir()   `%s'", dir);
    do {
        if (simulate_run())
            break;

        if (rmdir(dir) != 0) {
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
