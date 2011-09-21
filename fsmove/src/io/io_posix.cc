/*
 * io/io_posix.cc
 *
 *  Created on: Sep 22, 2011
 *      Author: max
 */

#include "../first.hh"

#include <cerrno>          // for errno
#include <cstring>         // for strcmp
#include <sys/types.h>     // for lstat(), mkdir()
#include <sys/stat.h>      //  "    "        "
#include <unistd.h>        //  "    "    , rmdir(), chown()
#include <dirent.h>        // for opendir(), readdir(), closedir()



#include "../log.hh"       // for ff_log()

#include "io_posix.hh"     // for fm_io_posix
#include "io_posix_dir.hh" // for fm_io_posix_dir


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

/** core of recursive move algorithm, actually moves the whole source tree into target */
int fm_io_posix::move()
{
    return move(source_root(), target_root());
}


/**
 * return true if 'stat' information is about a directory
 */
FT_INLINE static bool fm_io_posix_is_dir(const ft_stat & stat)
{
    return S_ISDIR(stat.st_mode);
}

/**
 * move a single file/socket/device or a whole directory tree
 */
int fm_io_posix::move(const ft_string & source_path, const ft_string & target_path)
{
    ft_stat stat;
    int err = 0;

    ff_log(FC_DEBUG, 0, "move()        %s\t-> %s", source_path.c_str(), target_path.c_str());

    do {
        if ((err = this->stat(source_path, stat)) != 0)
            break;
        
        if (fm_io_posix_is_dir(stat)) {
            fm_io_posix_dir source_dir;
            if ((err = source_dir.open(source_path)))
                break;
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
            if ((err = this->remove_dir(source_path)) != 0)
                break;

        } else if ((err = this->move_single(source_path, stat, target_path)) != 0) {
            break;
        }
    } while (0);
    return err;
}

/**
 * fill 'stat' with information about the file/directory/special-device 'path'
 */
int fm_io_posix::stat(const ft_string & path, ft_stat & stat)
{
    int err = 0;
    if (lstat(path.c_str(), & stat) != 0) {
        err = errno;
        // TODO log if err != 0
    }
    return err;
}

/**
 * move the single file or special-device 'source_path' to 'target_path'.
 */
int fm_io_posix::move_single(const ft_string & source_path, const ft_stat & source_stat, const ft_string & target_path)
{
    const char * source = source_path.c_str(), * target = target_path.c_str();
    int err = 0;
    ff_log(FC_TRACE, 0, "move_single() %s\t-> %s", source, target);

    // TODO
    return err;
}

/** create a target directory, copying its mode and other meta-data from 'stat' */
int fm_io_posix::create_dir(const ft_string & path, const ft_stat & stat)
{
    const char * dir = path.c_str();
    int err;
    ff_log(FC_TRACE, 0, "create_dir()  %s", dir);

    if ((err = mkdir(dir, stat.st_mode)) == 0 || (err = errno) == EEXIST)
        err = chown(dir, stat.st_uid, stat.st_gid);

    // TODO log if err != 0
    return err;
}

/** remove a source directory */
int fm_io_posix::remove_dir(const ft_string & path)
{
    const char * dir = path.c_str();
    ff_log(FC_TRACE, 0, "remove_dir()  %s", dir);

    int err = 0;
    if (rmdir(dir) != 0) {
        err = errno;
        // TODO log if err != 0
    }
    return err;
}

FT_IO_NAMESPACE_END
