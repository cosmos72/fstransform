/*
 * job.cc
 *
 *  Created on: Mar 9, 2011
 *      Author: max
 */

#include "first.hh"

#include <cerrno>    // for ENOMEM
#include <cstdio>    // for sprintf()
#include <cstdlib>   // for malloc(), free(), getenv()
#include <cstring>   // for memcpy(), sprintf()

#include "io/util.hh"  // for ff_mkdir()
#include "job.hh"      // for ft_job
#include "log.hh"      // for ff_log*()


FT_NAMESPACE_BEGIN

/** default constructor */
ft_job::ft_job()
    : fm_dir(), fm_storage_size(0), fm_id(0), fm_log_file(NULL)
{ }

/** destructor. calls quit() */
ft_job::~ft_job()
{
    quit();
}

/** initialize logging subsystem, or return error */
int ft_job::init_log()
{
    std::string log_file_name = fm_dir;
    log_file_name += "/fstransform.log";

    const char * log_file = log_file_name.c_str();

    if ((fm_log_file = fopen(log_file, "a")) == NULL)
        return ff_log(FC_ERROR, errno, "failed to open log file '%s'", log_file);

    (void) setvbuf(fm_log_file, NULL, _IOLBF, 0);

    /* note 1.4.3) fstransform.log always uses FC_FMT_DATETIME_LEVEL_CALLER_MSG */
    ff_log_register(fm_log_file, FC_FMT_DATETIME_LEVEL_CALLER_MSG);
    return 0;
}


/** initialize this job, or return error */
int ft_job::init(const char * root_dir, ft_uint job_id, ft_size storage_size)
{
    const char * user_home = NULL;
    if (root_dir == NULL) {
        user_home = getenv("HOME");
        if (user_home != NULL) {
            fm_dir = user_home;
            fm_dir += '/';
        }
    } else {
        fm_dir = root_dir;
        fm_dir += '/';
    }
    fm_dir += ".fstransform";

    const char * path = fm_dir.c_str();

    if (root_dir == NULL && user_home == NULL)
        ff_log(FC_WARN, 0, "$HOME is not set, persistent storage will use sub-folders of '%s' in current directory", path);

    (void) FT_IO_NS ff_mkdir(path);

    fm_dir += "/job.";
    ft_size len = fm_dir.size();
    ft_uint i, job_min = 1, job_max = (ft_uint)-1;
    int err = 0;

    if (job_id != 0)
        /* force job_id */
        job_min = job_id, job_max = job_id + 1;

    path = fm_dir.c_str();

    for (i = job_min; i != job_max; i++) {
        // 1 + 3*sizeof(ft_uint) chars are enough to safely print (ft_uint)
        fm_dir.resize(len + 2 + 3*sizeof(ft_uint));
        sprintf(& fm_dir[len], "%"FS_ULL, (FT_ULL) i);
        fm_dir.resize(len + strlen(& fm_dir[len]));

        path = fm_dir.c_str();

        if ((err = FT_IO_NS ff_mkdir(path)) == 0
                && (err = init_log()) == 0)
        {
            ff_log(FC_NOTICE, 0, "starting job %"FS_ULL, (FT_ULL)i);
            ff_log(FC_INFO, 0, "job persistent data and logs will be in '%s'", path);
            break;
        }
    }
    if (i == job_max) {
        if (job_id != 0)
            err = ff_log(FC_ERROR, err, "failed to create persistent data folder '%s' for job id %"FS_ULL, path, (FT_ULL) job_id);
        else
            err = ff_log(FC_ERROR, err, "failed to locate a free job id, tried range %"FS_ULL"...%"FS_ULL, (FT_ULL) job_min, (FT_ULL) (job_max-1));
    }
    if (err == 0) {
        fm_storage_size = storage_size;
        fm_id = i;
    } else
        quit();

    return err;

}

void ft_job::quit()
{
    if (fm_log_file != NULL) {
        ff_log_unregister(fm_log_file);
        fclose(fm_log_file);
        fm_log_file = NULL;
    }
    fm_dir.clear();
    fm_storage_size = 0;
    fm_id = 0;
}

FT_NAMESPACE_END
