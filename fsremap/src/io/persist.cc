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
 * io/persist.cc
 *
 *  Created on: Mar 6, 2011
 *      Author: max
 */

#include "../first.hh"

#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>      // for errno, EISCONN, ENOTCONN
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>       // for errno, EISCONN, ENOTCONN
#endif

#if defined(FT_HAVE_STRING_H)
# include <string.h>     // for strlen(), strcmp()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>      // for strlen(), strcmp()
#endif

#include "../args.hh"    // for FC_PRIMARY_STORAGE_EXACT_SIZE, FC_SECONDARY_STORAGE_EXACT_SIZE
#include "../log.hh"     // for ff_log()
#include "persist.hh"    // for fr_persist

FT_IO_NAMESPACE_BEGIN

/** constructor */
fr_persist::fr_persist(fr_job & job)
    : this_progress1((ft_ull)-1), this_progress2((ft_ull)-1),
      this_persist_path(), this_persist_file(NULL),
      this_job(job), this_replaying(false)
{ }


/** create and open persistence file job.job_dir() + "/fsremap.persist" */
int fr_persist::open()
{
    if (this_persist_file != NULL) {
        ff_log(FC_ERROR, 0, "unexpected call to open(), persistence is already initialized");
        // return error as already reported
        return -EISCONN;
    }

    this_persist_path = this_job.job_dir();
    this_persist_path += "/fsremap.persist";

    const char * persist_path = this_persist_path.c_str();

    // fopen(... "a+") = Open for reading and appending. The file is created if it does not exist.
    // The initial file position for reading is at the beginning of the file,
    // but output is always appended to the end of the file

    if ((this_persist_file = fopen(persist_path, "a+")) == NULL)
        return ff_log(FC_ERROR, errno, "failed to open persistence file '%s'", persist_path);

    this_replaying = this_job.resuming_job();
    const bool simulated = this_job.simulate_run();
    const char * header = simulated ? "simulated job" : "real job";
    const char * other_header = simulated ? "real job" : "simulated job";
    int err = 0;
    if (this_replaying) {
        enum { FT_LINE_LEN = 80 };
        char line[FT_LINE_LEN + 1] = { '\0' };
        if (fgets(line, FT_LINE_LEN, this_persist_file) == NULL)
            err = ff_log(FC_ERROR, errno, "I/O error reading from persistence file '%s'", this_persist_path.c_str());
        else {
            // paranoia
            line[FT_LINE_LEN] = '\0';
            ft_size line_len = strlen(line);

            // remove final '\n' if present
            if (line_len && line[line_len - 1] == '\n')
                line[--line_len] = '\0';

            if (!strcmp(header, line)) {
                err = do_read(this_progress1, this_progress2);
            } else if (!strcmp(other_header, line)) {
                ff_log(FC_ERROR, 0, "tried to resume a %s: you MUST%s specify option '-n'%s",
                        other_header, simulated ? " NOT" : "", simulated ? "" : " to simulate again");
                err = -EINVAL;
            } else {
                ff_log(FC_ERROR, 0, "corrupted persistence file '%s': expected header '%s', found '%s'",
                        persist_path, header, line);
                err = -EINVAL;
            }
        }
    } else {
        if (fprintf(this_persist_file, "%s\n", header) < 0)
            err = ff_log(FC_ERROR, errno, "I/O error writing to persistence file '%s'", this_persist_path.c_str());
        else
            err = do_flush();
    }
    return err;
}



/**
 * get exact primary/secondary storage sizes.
 * also verify that sizes in persistence file (if present) match ones from command line (if specified).
 * if no exact sizes are available, sets them to 0.
 */
int fr_persist::get_storage_sizes_exact(ft_size & size1, ft_size & size2)
{
    const ft_size job_size1 = this_job.job_storage_size(FC_PRIMARY_STORAGE_EXACT_SIZE);
    const ft_size job_size2 = this_job.job_storage_size(FC_SECONDARY_STORAGE_EXACT_SIZE);

    if (!this_replaying) {
        size1 = job_size1;
        size2 = job_size2;
        return 0;
    }

    // compare job_{primary,secondary}_size with the ones in persistence (if present)
    ft_ull persist_size1 = 0, persist_size2 = 0;
    int err = read(persist_size1, persist_size2);
    if (err != 0)
        return err;
   
    if (persist_size1 != 0 && job_size1 != 0
        && persist_size1 != (ft_ull) job_size1)
    {
        ff_log(FC_ERROR, 0, "mismatched primary storage exact size: %"FT_ULL" bytes requested from command line, %"FT_ULL
               " bytes found in persistence file", (ft_ull) job_size1, persist_size1);
        err = -EINVAL;
    }
    if (persist_size2 != 0 && job_size2 != 0
        && persist_size2 != (ft_ull) job_size2)
    {
        ff_log(FC_ERROR, 0, "mismatched secondary storage exact size: %"FT_ULL" bytes requested from command line, %"FT_ULL
               " bytes found in persistence file", (ft_ull) job_size2, persist_size2);
        err = -EINVAL;
    }
    if (err != 0)
        return err;
   
    // reuse persisted primary/secondary exact size.
    // ABSOLUTELY needed to reproduce the same operations while replaying
    size1 = (ft_size) persist_size1;
    size2 = (ft_size) persist_size2;
    return err;
}


/** set exact primary/secondary storage sizes.  */
int fr_persist::set_storage_sizes_exact(ft_size size1, ft_size size2)
{
    int err = 0;
    if (!this_replaying)
        err = do_write((ft_ull) size1, (ft_ull) size2);
    return err;
}

/** read a step from persistence fle */
int fr_persist::read(ft_ull & progress1, ft_ull & progress2)
{
    if (!this_replaying)
        ff_log(FC_ERROR, 0, "tried to read after end of persistence file '%s'", this_persist_path.c_str());
    
    progress1 = this_progress1;
    progress2 = this_progress2;
    
    return do_read(this_progress1, this_progress2);
}


/** read or write a step in persistence fle */
int fr_persist::next(ft_ull progress1, ft_ull progress2)
{
    ff_log(FC_DEBUG, 0, "blocks left: device = %"FT_ULL", storage = %"FT_ULL", replaying = %s",
            progress1, progress2, this_replaying ? "true" : "false");

    if (!this_replaying)
        return do_write(progress1, progress2);
    
    if (progress1 != this_progress1 || progress2 != this_progress2) {
        ff_log(FC_ERROR, 0, "unexpected values found while replaying persistence file '%s'",
                this_persist_path.c_str());
        ff_log(FC_ERROR, 0, "\texpected %"FT_ULL" %"FT_ULL", found %"FT_ULL" %"FT_ULL"\n",
                progress1, progress2, this_progress1, this_progress2);
        return -EINVAL;
    }
    return do_read(this_progress1, this_progress2);
}


/** try to read data from persistence fle */
int fr_persist::do_read(ft_ull & progress1, ft_ull & progress2)
{
    int err = 0;
    if (this_replaying) {
        int items = fscanf(this_persist_file, "%"FT_ULL"\t%"FT_ULL"\n", & progress1, & progress2);
        if (items == 2) {
            /* ok */
        } else if (feof(this_persist_file)) {
            this_replaying = false;
        } else {
            ff_log(FC_ERROR, 0, "corrupted persistence file '%s'! expecting two numeric values, found %d",
                    this_persist_path.c_str(), (int) items);
            err = -EFAULT;
        }
    }
    return err;
}

/** try to write data into persistence fle */
int fr_persist::do_write(ft_ull progress1, ft_ull progress2)
{
    if (fprintf(this_persist_file, "%"FT_ULL"\t%"FT_ULL"\n", progress1, progress2) <= 0)
        return ff_log(FC_ERROR, errno, "I/O error writing to persistence file '%s'", this_persist_path.c_str());
    return do_flush();
}

/** flush this_persist_file to disk: calls fflush() then fdatasync() or fsync() */
int fr_persist::do_flush()
{
    if (fflush(this_persist_file) == 0) {

        int fd = fileno(this_persist_file);
        if (fd < 0)
            return ff_log(FC_ERROR, errno, "fileno('%s') failed: return value = %d", this_persist_path.c_str(), fd);

#if defined(FT_HAVE_FDATASYNC)
        if (fdatasync(fd) == 0)
#elif defined(FT_HAVE_FSYNC)
        if (fsync(fd) == 0)
#else
        (void) sync();
#endif
            return 0;
    }
    return ff_log(FC_ERROR, errno, "I/O error flushing persistence file '%s'", this_persist_path.c_str());
}


/** close persistence file */
int fr_persist::close()
{
    if (this_persist_file != NULL) {
        if (fclose(this_persist_file) != 0) {
            return ff_log(FC_ERROR, errno, "failed to close persistence file '%s'", this_persist_path.c_str());
        }
        this_persist_file = NULL;
    }
    return 0;
}


/** destructor */
fr_persist::~fr_persist()
{
    (void) close();
}

FT_IO_NAMESPACE_END
