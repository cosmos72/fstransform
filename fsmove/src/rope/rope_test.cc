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
 * rope.cc
 *
 *  Created on: Mar 3, 2018
 *      Author: max
 */

#include "../first.hh"

#if defined(FT_HAVE_STDIO_H)
# include <stdio.h>     // for getchar()
#elif defined(FT_HAVE_CSTDIO)
# include <cstdio>      // for getchar()
#endif

#if defined(FT_HAVE_STDLIB_H)
# include <stdlib.h>     // for free()
#elif defined(FT_HAVE_CSTDLIB)
# include <cstdlib>      // for free()
#endif

#if defined(FT_HAVE_STRING_H)
# include <string.h>     // for strcmp(), strdup(), strlen()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>      // for strcmp(), strdup(), strlen()
#endif

#ifdef FT_HAVE_FT_UNSORTED_MAP
# include "../unsorted_map.hh" // for ft_unsorted_map<K,V>
# define ft_map ft_unsorted_map
#else
# include <map>             // for std::map<K,V>
# define ft_map std::map
#endif

#include "../log.hh"
#include "../io/io_posix_dir.hh"
#include "../cache/cache_mem.hh"
#include "../zstring.hh"

#include "rope_test.hh"
#include "rope_pool.hh"

FT_NAMESPACE_BEGIN

static ft_uoff recursive_readdir(ft_unsorted_map<ft_inode, ft_string> & cache,
                                 const ft_string & path)
{
	ft_uoff count = 0;
	io::ft_io_posix_dir dir;
	if (dir.open(path) != 0) {
		return count;
	}
	io::ft_io_posix_dirent * dirent = NULL;
	while (dir.next(dirent) == 0 && dirent != NULL) {
		if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
			continue;
		
		ft_string subpath = path;
		if (subpath[subpath.size() - 1] != '/')
			subpath += '/';
		subpath += dirent->d_name;
		cache[dirent->d_ino] = subpath;

#ifdef DT_DIR
		if (dirent->d_type == DT_DIR) {
			count += recursive_readdir(cache, subpath);
		}
#endif
	}
	return count;
}

static ft_uoff recursive_readdir_cstr(ft_unsorted_map<ft_inode, char *> & cache,
                                      const ft_string & path)
{
	ft_uoff count = 0;
	io::ft_io_posix_dir dir;
	if (dir.open(path) != 0) {
		return count;
	}
	io::ft_io_posix_dirent * dirent = NULL;
	while (dir.next(dirent) == 0 && dirent != NULL) {
		if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
			continue;

		ft_string subpath = path;
		if (subpath[subpath.size() - 1] != '/')
			subpath += '/';
		subpath += dirent->d_name;
		char * & cstr = cache[dirent->d_ino];
                if (cstr) {
                        free(cstr);
                        cstr = NULL;
                }
                cstr = strdup(subpath.c_str());
                if (!cstr)
                        throw std::bad_alloc();

		if (dirent->d_type == DT_DIR) {
			count += recursive_readdir_cstr(cache, subpath);
		}
	}
	return count;
}

static ft_uoff recursive_readdir_zstring(ft_unsorted_map<ft_inode, ft_string> & cache,
                                         const ft_string & path)
{
	ft_uoff count = 0;
	io::ft_io_posix_dir dir;
	if (dir.open(path) != 0) {
		return count;
	}
	io::ft_io_posix_dirent * dirent = NULL;
	while (dir.next(dirent) == 0 && dirent != NULL) {
		if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
			continue;

		ft_string subpath = path;
		if (subpath[subpath.size() - 1] != '/')
			subpath += '/';
		subpath += dirent->d_name;
		ft_string & compressed = cache[dirent->d_ino];
                compressed.resize(0);
                z(compressed, subpath);

		if (dirent->d_type == DT_DIR) {
			count += recursive_readdir_zstring(cache, subpath);
		}
	}
	return count;
}

static ft_uoff recursive_readdir_cachemem(ft_cache_mem<ft_inode, ft_string> & cache,
					  const ft_string & path)
{
	ft_uoff count = 0;
	io::ft_io_posix_dir dir;
	if (dir.open(path) != 0) {
		return count;
	}
	io::ft_io_posix_dirent * dirent = NULL;
	while (dir.next(dirent) == 0 && dirent != NULL) {
		if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
			continue;
		
		ft_string subpath = path, tmp;
		if (subpath[subpath.size() - 1] != '/')
			subpath += '/';
		subpath += dirent->d_name;
		tmp = subpath;
		cache.find_or_add(dirent->d_ino, tmp); // modifies tmp!

		if (dirent->d_type == DT_DIR) {
			count += recursive_readdir_cachemem(cache, subpath);
		}
	}
	return count;
}
	
static ft_uoff recursive_readdir_pool(ft_rope_pool & pool,
				      ft_cache_mem<ft_inode, ft_rope> & cache,
				      const ft_string & path)
{
	ft_uoff count = 0;
	io::ft_io_posix_dir dir;
	if (dir.open(path) != 0) {
		return count;
	}
	io::ft_io_posix_dirent * dirent = NULL;
	while (dir.next(dirent) == 0 && dirent != NULL) {
		if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
			continue;
		
		ft_string subpath = path;
		if (subpath[subpath.size() - 1] != '/')
			subpath += '/';
		subpath += dirent->d_name;
		ft_rope rope = pool.make(subpath);
		cache.find_or_add(dirent->d_ino, rope);
		
		if (dirent->d_type == DT_DIR) {
			count += recursive_readdir_pool(pool, cache, subpath);
		}
	}
	return count;
}

int rope_test(int argc, char ** argv)
{
	ft_string path = "/";
	if (argc > 1 && !strcmp(argv[1], "pool")) {
		ft_rope_pool pool;
		ft_cache_mem<ft_inode, ft_rope> cache;
		recursive_readdir_pool(pool, cache, path);
		fputs("recursive_readdir_pool() completed. check RAM usage and press ENTER\n", stdout);
		fflush(stdout);
		fgetc(stdin);
	} else if (argc > 1 && !strcmp(argv[1], "cachemem")) {
		ft_cache_mem<ft_inode, ft_string> cache;
		recursive_readdir_cachemem(cache, path);
		fputs("recursive_readdir_cachemem() completed. check RAM usage and press ENTER\n", stdout);
		fflush(stdout);
		fgetc(stdin);
	} else if (argc > 1 && !strcmp(argv[1], "zstring")) {
		ft_map<ft_inode, ft_string> cache;
		recursive_readdir_zstring(cache, path);
		fputs("recursive_zstring() completed. check RAM usage and press ENTER\n", stdout);
		fflush(stdout);
		fgetc(stdin);
	} else if (argc > 1 && !strcmp(argv[1], "cstr")) {
		ft_map<ft_inode, char *> cache;
		recursive_readdir_cstr(cache, path);
		fputs("recursive_readdir_cstr() completed. check RAM usage and press ENTER\n", stdout);
		fflush(stdout);
		fgetc(stdin);
	} else {
		ft_map<ft_inode, ft_string> cache;
		recursive_readdir(cache, path);
		fputs("recursive_readdir() completed. check RAM usage and press ENTER\n", stdout);
		fflush(stdout);
		fgetc(stdin);
	}
	return 0;
}


FT_NAMESPACE_END
