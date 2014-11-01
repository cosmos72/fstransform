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

#ifdef FT_HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "log.hh"
#include "inode_cache_posix.hh"
#include "../io/util_posix_dir.hh"  // for ff_mkdir_recursive(), ff_remove_recursive()



FT_NAMESPACE_BEGIN

/** one-arg constructor */
ft_inode_cache_posix::ft_inode_cache_posix(const ft_string & init_path)
	: path(init_path)
{
	FT_IO_NS ff_posix_mkdir_recursive(path);
}

/** copy constructor */
ft_inode_cache_posix::ft_inode_cache_posix(const ft_inode_cache_posix & other) : path(other.path)
{ }


/** assignment operator */
const ft_inode_cache_posix & ft_inode_cache_posix::operator=(const ft_inode_cache_posix & other)
{
	if (this != &other)
		path = other.path;
	return *this;
}

/** destructor */
ft_inode_cache_posix::~ft_inode_cache_posix()
{
	ft_inode_cache_posix::clear();
}

void ft_inode_cache_posix::build_path(const ft_string & rel, ft_string & abs) const
{
	abs = path;
	abs += rel;
}

bool ft_inode_cache_posix::readlink(const ft_string & src, ft_string & dst) const
{
	size_t len = 255;
	ssize_t got;
again:
	dst.resize(len);
	got = ::readlink(src.c_str(), &dst[0], len);
	if (got > 0 && (size_t)got == len)
	{
		len = len * 2 + 1;
		goto again;
	}
	if (got < 0)
		return false;
	dst.resize(got);
	return true;
}

/**
 * return true and set payload of cached inode if found, else add it to cache and return false
 * if false is returned, erase() must be called on the same inode when done with payload!
 */
bool ft_inode_cache_posix::find_or_add(const ft_string & inode, ft_string & payload)
{
	ft_string link_from;
	build_path(inode, link_from);

	if (::symlink(payload.c_str(), link_from.c_str()) != 0) {
		payload = inode;
		return true;
	}
	return false;
}

/** return true and set payload of cached inode if found, else return false */
bool ft_inode_cache_posix::find(const ft_string & inode, ft_string & result_payload) const
{
	ft_string link_from;
	build_path(inode, link_from);

	return readlink(link_from, result_payload);
}

/**
 * must be called if and only if find(inode) returned false
 */
void ft_inode_cache_posix::erase(const ft_string & inode)
{
	ft_string link_from;
	build_path(inode, link_from);

	if (::unlink(link_from.c_str()) != 0)
	{
		ff_log(FC_WARN, errno, "failed to remove symlink `%s' from inode cache `%s'", link_from.c_str(), path.c_str());
	}
}

void ft_inode_cache_posix::clear()
{
	FT_IO_NS ff_posix_remove_recursive(path);
}



FT_NAMESPACE_END
