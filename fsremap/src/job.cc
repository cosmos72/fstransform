/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 2 of the License, or
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
 * job.cc
 *
 *  Created on: Mar 9, 2011
 *      Author: max
 */

#include "first.hh"


#if defined(FT_HAVE_ERRNO_H)
# include <errno.h>        // for errno
#elif defined(FT_HAVE_CERRNO)
# include <cerrno>         // for errno
#endif
#if defined(FT_HAVE_STDLIB_H)
# include <stdlib.h>       // for malloc(), free(), getenv()
#elif defined(FT_HAVE_CSTDLIB)
# include <cstdlib>        // for malloc(), free(), getenv()
#endif
#if defined(FT_HAVE_STRING_H)
# include <string.h>     // for memcpy()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>      // for memcpy()
#endif


#include "args.hh"    // for FC_JOB_ID_AUTODETECT
#include "job.hh"     // for fr_job
#include "io/util_dir.hh" // for ff_mkdir()


FT_NAMESPACE_BEGIN

/** default constructor */
fr_job::fr_job()
    : this_dir(), this_log_file(NULL), this_log_appender(NULL),
    this_id(FC_JOB_ID_AUTODETECT), this_clear(FC_CLEAR_AUTODETECT),
    this_force_run(false), this_simulate_run(false), this_resume_job(false), this_ask_questions(false)
{
    for (ft_size i = 0; i < FC_STORAGE_SIZE_N; i++)
        this_storage_size[i] = 0;
}

/** destructor. calls quit() */
fr_job::~fr_job()
{
    quit();
}

/** initialize this job, or return error */
int fr_job::init(const fr_args & args)
{
    if (args.root_dir != NULL)
        this_dir = args.root_dir;
    else
        this_dir = "/var/tmp";

    const char * path = this_dir.c_str();
    (void) FT_IO_NS ff_mkdir(path);

    this_dir += "/fstransform";
    path = this_dir.c_str();
    (void) FT_IO_NS ff_mkdir(path);

    this_dir += "/fsremap.job.";
    ft_size len = this_dir.size();
    ft_uint i, job_min = 1, job_max = 1000000;
    int err = 0;

    /* copy flags */
    this_resume_job = args.job_id != FC_JOB_ID_AUTODETECT;
    this_force_run = args.force_run;
    this_simulate_run = args.simulate_run;
    this_ask_questions = args.ask_questions;

    if (this_resume_job)
        /* force job_id */
        job_min = args.job_id, job_max = args.job_id + 1;

    path = this_dir.c_str();

    for (i = job_min; i != job_max; i++) {
        // 1 + 3*sizeof(ft_uint) chars are enough to safely print (ft_uint)
        this_dir.resize(len + 2 + 3*sizeof(ft_uint));
        sprintf(& this_dir[len], "%" FT_ULL , (ft_ull) i);
        this_dir.resize(len + strlen(& this_dir[len]));

        path = this_dir.c_str();

        if (!this_resume_job)
            err = FT_IO_NS ff_mkdir(path);

        if (err == 0 && (err = init_log()) == 0) {
            ff_log(FC_NOTICE, 0, "fsremap: %s job %" FT_ULL ", persistence data and logs are in '%s'",
                   this_resume_job ? "resuming" : "starting", (ft_ull)i, path);
            if (!this_resume_job && !this_simulate_run && args.io_kind != FC_IO_SELF_TEST) {
                ff_log(FC_NOTICE, 0, "if this job is interrupted, for example by a power failure,");
                ff_log(FC_NOTICE, 0, "you CAN RESUME it with: %s%s -q --resume-job=%" FT_ULL " -- %s",
                       args.program_name, this_simulate_run ? " -n" : "", (ft_ull)i, args.io_args[0]);
            }
            break;
        }
    }
    if (i == job_max) {
        if (this_resume_job)
            err = ff_log(FC_ERROR, err, "failed to resume job id %" FT_ULL  " from directory '%s'", (ft_ull) args.job_id, path);
        else
            err = ff_log(FC_ERROR, err, "failed to locate a free job id, tried range %" FT_ULL "...%" FT_ULL , (ft_ull) job_min, (ft_ull) (job_max-1));
    }
    if (err != 0) {
        quit();
        return err;
    }

    for (ft_size l = 0; l < FC_STORAGE_SIZE_N; l++)
        this_storage_size[l] = args.storage_size[l];
    this_id = i;
    this_clear = args.job_clear;


    return err;

}

/** initialize logging subsystem, or return error */
int fr_job::init_log()
{
    ft_string log_file_name = this_dir;
    log_file_name += "/fsremap.log";

    const char * log_file = log_file_name.c_str();

    if ((this_log_file = fopen(log_file, "a")) == NULL)
        return ff_log(FC_ERROR, errno, "failed to open log file '%s'", log_file);

    (void) setvbuf(this_log_file, NULL, _IOLBF, 0);

    /* note 1.4.3) fsremap.log always uses FC_FMT_DATETIME_LEVEL_CALLER_MSG */
    this_log_appender = new ft_log_appender(this_log_file, FC_FMT_DATETIME_LEVEL_CALLER_MSG);

    ft_log::get_root_logger().add_appender(* this_log_appender);
    return 0;
}

void fr_job::quit()
{
    if (this_log_appender != NULL) {
        ft_log::get_root_logger().remove_appender(* this_log_appender);
        this_log_appender = NULL;
    }
    if (this_log_file != NULL) {
        fclose(this_log_file);
        this_log_file = NULL;
    }
    this_dir.clear();
    for (ft_size i = 0; i < FC_STORAGE_SIZE_N; i++)
        this_storage_size[i] = 0;
    this_id = FC_JOB_ID_AUTODETECT;
}

FT_NAMESPACE_END
