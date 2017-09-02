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
 * zmem.hh
 *
 *  Created on: Jan 22, 2017
 *      Author: max
 */

#ifndef FSTRANSFORM_ZMEM_HH
#define FSTRANSFORM_ZMEM_HH

#include "zfwd.hh"

#if defined(FT_HAVE_LIMITS_H)
# include <limits.h>       // for CHAR_BIT
#elif defined(FT_HAVE_CLIMITS)
# include <climits>        // for CHAR_BIT
#endif

FT_NAMESPACE_BEGIN

class zmem
{
private:
    bool do_compress();
    void * do_decompress();

protected:
    friend class zpool_base;
    enum { compressed_shift = (sizeof(ft_size) * CHAR_BIT) - 1 };

    void * address;
    ft_size size; /* if mem is compressed, contains ~actual_size */

public:
    inline zmem() : address(NULL), size(0)
    {
    }

    inline bool compressed() const
    {
        return size >> compressed_shift;
    }

    inline bool current_size() const
    {
        return compressed() ? size : ~size;
    }

    bool alloc_page(ft_size new_size);
    bool free_page();

    inline bool compress_page()
    {
        if (address != NULL && !compressed())
            return do_compress();
        return address != NULL; /* already decompressed, or does not exist */
    }

    inline void * decompress_page()
    {
        if (address != NULL && compressed())
            return do_decompress();
        return address; /* already compressed, or does not exist */
    }
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_ZMEM_HH */