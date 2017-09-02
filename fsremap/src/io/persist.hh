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
 * io/persist.hh
 *
 *  Created on: Mar 6, 2011
 *      Author: max
 */

#ifndef FSREMAP_PERSIST_HH
#define FSREMAP_PERSIST_HH

#include "../job.hh"   // for fr_job

#if defined(FT_HAVE_STDIO_H)
# include <stdio.h>        // for FILE. also for fopen(), fclose(), fprintf() and fscanf() used in persist.cc
#elif defined(FT_HAVE_CSTDIO)
# include <cstdio>         // for FILE. also for fopen(), fclose(), fprintf() and fscanf() used in persist.cc
#endif

FT_IO_NAMESPACE_BEGIN

class fr_persist
{
private:
    ft_ull this_progress1, this_progress2;
    ft_string this_persist_path;

    FILE * this_persist_file;

    fr_job & this_job;

    /** true while replaying persistence */
    bool this_replaying;

    /** cannot call copy constructor */
    fr_persist(const fr_persist &);

    /** cannot call assignment operator */
    const fr_persist & operator=(const fr_persist &);

    /** try to read data from persistence fle */
    int do_read(ft_ull & progress1, ft_ull & progress2);

    /** try to write data into persistence fle */
    int do_write(ft_ull progress1, ft_ull progress2);

    /** flush this_persist_file to disk: calls fflush() then fdatasync() or fsync() */
    int do_flush();

public:
    /** constructor */
    fr_persist(fr_job & job);

    /** return job */
    FT_INLINE fr_job & job() { return this_job; }

    /** create and open persistence file job.job_dir() + "/fsremap.persist" */
    int open();

    /** return true if replaying persistence file */
    FT_INLINE bool is_replaying() const { return this_replaying; }

    /**
     * get exact primary/secondary storage sizes.
     * also verify that sizes in persistence file (if present) match ones from command line (if specified).
     * if no exact sizes are available, sets them to 0.
     */
    int get_storage_sizes_exact(ft_size & primary_size_exact, ft_size & secondary_size_exact);

    /** set exact primary/secondary storage sizes.  */
    int set_storage_sizes_exact(ft_size primary_size_exact, ft_size secondary_size_exact);

    /** read a step from persistence fle */
    int read(ft_ull & progress1, ft_ull & progress2);

    /** read or write a step in persistence file */
    int next(ft_ull progress1, ft_ull progress2);

    /** close persistence file */
    int close();

    /** destructor */
    ~fr_persist();
};


FT_IO_NAMESPACE_END

#endif /* FSREMAP_PERSIST_HH */
