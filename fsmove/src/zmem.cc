/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2017 Massimiliano Ghilardi
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
 * move.cc
 *
 *  Created on: Jan 22, 2017
 *      Author: max
 */

#include "first.hh"

#include "log.hh"
#include "zmem.hh"

#include <zlib.h>

#if defined(FT_HAVE_STDLIB_H)
# include <stdlib.h>       // for calloc(), realloc(), free()
#elif defined(FT_HAVE_CSTDLIB)
# include <cstdlib>        // for calloc(), realloc(), free()
#endif

#if defined(FT_HAVE_STRING_H)
# include <string.h>       // for memset()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>        // for memset()
#endif

FT_NAMESPACE_BEGIN


bool zmem::alloc(ft_size new_size)
{
    if (new_size == 0)
        new_size = 1;
    
    void * new_address = ::calloc(1, new_size);
    if (new_address != NULL)
    {
        address = new_address;
        size = new_size;
    }
    return new_address != NULL;
}

bool zmem::free()
{
    void * old_address = address;
    if (old_address != NULL)
        ::free(old_address);
    address = NULL;
    size = 0;
    return old_address != NULL;
}

bool zmem::do_compress()
{
    z_stream z = { }; /* zero-initialize */
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    
    int err = deflateInit(&z, 3);
    if (err != Z_OK) {
        ff_log(FC_ERROR, 0, "zlib deflateInit() failed: %s",
               z.msg != NULL ? z.msg
               : err == Z_MEM_ERROR ? "out of memory!"
               : err == Z_STREAM_ERROR ? "invalid compression level"
               : err == Z_VERSION_ERROR ? "incompatible zlib version"
               : "unknown error");
        return false;
    }
    
    ft_size new_size = deflateBound(&z, size);
    void * new_address = ::malloc(new_size);
    if (address == NULL) {
        ff_log(FC_ERROR, errno, "malloc(%"FT_ULL") failed", (ft_ull)new_size);
        err = Z_MEM_ERROR;
        goto cleanup;
    }
    z.next_in = (unsigned char *)address;
    z.avail_in = size;
    z.next_out = (unsigned char *)new_address;
    z.avail_out = new_size;
    err = deflate(&z, Z_FINISH);
    if (err != Z_STREAM_END) {
        ff_log(FC_ERROR, 0, "zlib deflate(Z_FINISH) returned %d instead of Z_STREAM_END", err);
        goto cleanup;
    }
    if (z.avail_out > 0 && z.avail_out < new_size) {
        new_size -= z.avail_out;
        void * new2_address = ::realloc(new_address, new_size);
        if (new2_address != NULL)
            new_address = new2_address;
    }
    
cleanup:
    deflateEnd(&z);
    if (err == Z_STREAM_END) {
        ::free(address);
        address = new_address;
        size = ~new_size; /* mark as compressed */
    } else {
        ::free(new_address);
    }
    return err == Z_STREAM_END;
}

void * zmem::do_uncompress()
{
    z_stream z = { }; /* zero-initialize */
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    
    int err = inflateInit(&z);
    if (err != Z_OK) {
        ff_log(FC_ERROR, 0, "zlib inflateInit() failed: %s",
               z.msg != NULL ? z.msg
               : err == Z_MEM_ERROR ? "out of memory!"
               : err == Z_STREAM_ERROR ? "invalid compression level"
               : err == Z_VERSION_ERROR ? "incompatible zlib version"
               : "unknown error");
        return NULL;
    }
    
    ft_size old_size = ~size;
    ft_size new_size = old_size * 3;
    void * new_address = ::malloc(new_size);
    if (new_address == NULL) {
        inflateEnd(&z);
        return NULL;
    }
    z.next_in = (unsigned char *)address;
    z.avail_in = size;
    z.next_out = (unsigned char *)new_address;
    z.avail_out = new_size;
    while ((err = inflate(&z, Z_NO_FLUSH)) == Z_OK) {
        if (z.avail_out == 0) {
            ft_size new2_size = new_size << 1;
            void * new2_address = ::realloc(new_address, new2_size);
            if (new2_address == NULL) {
                ff_log(FC_ERROR, errno, "realloc(%"FT_ULL" -> %"FT_ULL") failed", (ft_ull)new_size, (ft_ull)new2_size);
                err = Z_MEM_ERROR;
                goto cleanup;
            }
            z.next_out = (unsigned char *)new2_address + new_size;
            z.avail_out = new2_size - new_size;
            new_address = new2_address;
            new_size = new2_size;
        }
    }
    if (err != Z_STREAM_END) {
        ff_log(FC_ERROR, 0, "zlib inflate(Z_FINISH) returned %d instead of Z_STREAM_END", err);
        goto cleanup;
    }
    if (z.avail_out != 0) {
        /* try to shrink */
        new_size -= z.avail_out;
        void * new2_address = ::realloc(new_address, new_size);
        if (new2_address)
            new_address = new2_address;
    }
    
cleanup:
    inflateEnd(&z);
    if (err == Z_STREAM_END) {
        ::free(address);
        address = new_address;
        size = new_size;
    } else {
        ::free(new_address);
        new_address = NULL;
    }
    return new_address;
}


FT_NAMESPACE_END
