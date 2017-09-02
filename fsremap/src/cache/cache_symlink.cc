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
 * io/extent_file.cc
 *
 *  Created on: Mar 3, 2011
 *      Author: max
 */

#include "../first.hh"

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>        // for errno
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>         // for errno
#endif

#ifdef FT_HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "log.hh"
#include "cache_symlink.hh"
#include "../assert.hh"       // for ff_assert()
#include "../copy.hh"         // for ff_cat()
#include "../io/util_dir.hh"  // for ff_mkdir_recursive(), ff_remove_recursive()



FT_NAMESPACE_BEGIN

/** one-arg constructor */
ft_cache_symlink::ft_cache_symlink()
{ }

/** copy constructor */
ft_cache_symlink::ft_cache_symlink(const ft_cache_symlink & other) : path(other.path)
{ }


/** assignment operator */
const ft_cache_symlink & ft_cache_symlink::operator=(const ft_cache_symlink & other)
{
    if (this != &other)
        path = other.path;
    return *this;
}

/** destructor */
ft_cache_symlink::~ft_cache_symlink()
{
    ft_cache_symlink::clear();
}

/* initialize the inode-cache. return 0 on success, else return error */
int ft_cache_symlink::init(const ft_string & init_path)
{
    int err = FT_IO_NS ff_mkdir_recursive(init_path);
    if (err != 0)
    {
        // EEXIST is a warning, everything else is an error
        bool exists = err == EEXIST;
        err = ff_log(exists ? FC_WARN : FC_ERROR, err, "failed to create cache directory `%s'", init_path.c_str());
        if (exists)
            err = 0;
    }
    if (err == 0)
    {
        path = init_path;
        /*
         * comply with get_path() promise:
         * remove trailing '/' unless it's exactly the path "/"
         */
        size_t len = path.length();
        if (len > 1 && path[len-1] == '/')
            path.resize(len - 1);
    }
    return err;
}


int ft_cache_symlink::build_path(const ft_string & rel, ft_string & abs, FT_ICP_OPTIONS options) const
{
    size_t len = rel.length();
    ff_assert(len != 0);
    size_t len_1 = len - 1, depth = len_1 / 3, mod_3p1 = (len_1 % 3) + 1;
    int err = 0;

    abs = path;
    // add trailing '/' if needed
    if (!abs.empty() && *abs.rbegin() != '/')
        abs += '/';

    // add prefix 'L' + depth +'/'
    abs += 'L';
    ff_cat(abs, depth);
    if (options == FT_ICP_READWRITE)
        err = FT_IO_NS ff_mkdir_or_warn(abs.c_str());
    abs += '/';
    // add 1-3 chars prefix taken from beginning of rel
    abs += rel.substr(0, mod_3p1);

    // add rest of rel in 3-chars steps
    for (size_t i = 0; i < depth; i++)
    {
        if (options == FT_ICP_READWRITE)
        {
            err = FT_IO_NS ff_mkdir_or_warn(abs.c_str());
        }
        abs += '/';
        abs += rel.substr(mod_3p1 + i * 3, 3);
    }
    return err == EEXIST ? 0 : err;
}

int ft_cache_symlink::readlink(const ft_string & src, ft_string & dst)
{
    size_t orig_len = dst.length(), len = orig_len < 256 ? 256 : orig_len;
    ssize_t got;
again:
    dst.resize(len);
    got = ::readlink(src.c_str(), &dst[0], len);
    if (got > 0 && (size_t)got == len)
    {
        len *= 2;
        goto again;
    }
    if (got < 0)
    {
        // not found, or other error: restore dst.
        dst.resize(orig_len);
        int err = errno;
        if (err != ENOENT)
            err = ff_log(FC_ERROR, err, "failed to read cache symlink `%s'", src.c_str());
        return err;
    }
    dst.resize(got);
    return 0;
}

/**
 * if cached inode found, set payload and return 1.
 * Otherwise add it to cache and return 0.
 * On error, return < 0.
 * if returns 0, find_and_delete() must be called on the same inode when done with payload!
 */
int ft_cache_symlink::find_or_add(const ft_string & inode, ft_string & payload)
{
    ft_string link_from;
    build_path(inode, link_from, FT_ICP_READWRITE);

    int err = readlink(link_from, payload);
    if (err == 0)
        // found
        return 1;

    if (::symlink(payload.c_str(), link_from.c_str()) != 0)
        return ff_log(FC_ERROR, errno, "failed to create cache symlink `%s' -> `%s'", link_from.c_str(), payload.c_str());

    return 0;
}

/**
 * if cached inode found, set payload, remove cached inode and return 1.
 * Otherwise return 0. On error, return < 0.
 */
int ft_cache_symlink::find_and_delete(const ft_string & inode, ft_string & result_payload)
{
    ft_string link_from;
    build_path(inode, link_from, FT_ICP_READONLY);

    int err = readlink(link_from, result_payload);
    if (err == ENOENT)
        // not found
        return 0;

    if (err != 0)
        ff_log(FC_WARN, errno, "failed to read cache symlink `%s'", link_from.c_str());

        // either found, or error
    if (::unlink(link_from.c_str()) != 0)
        ff_log(FC_WARN, errno, "failed to remove cache symlink `%s'", link_from.c_str());

        return err == 0 ? 1 : err;
}


/**
 * if cached inode found, change its payload and return 1.
 * Otherwise return 0. On error, return < 0.
 */
int ft_cache_symlink::find_and_update(const ft_string inode, const ft_string & new_payload)
{
    ft_string link_from;
    build_path(inode, link_from, FT_ICP_READONLY);

    int err = ::unlink(link_from.c_str());
    if (err == ENOENT)
            // not found
            return 0;

    if (err != 0)
        ff_log(FC_WARN, errno, "failed to remove cache symlink `%s'", link_from.c_str());

        // either found, or warning
    if (::symlink(new_payload.c_str(), link_from.c_str()) != 0)
        return ff_log(FC_ERROR, errno, "failed to create cache symlink `%s' -> `%s'", link_from.c_str(), new_payload.c_str());

    return 1;
}

void ft_cache_symlink::clear()
{
    FT_IO_NS ff_remove_recursive(path);
}



FT_NAMESPACE_END
