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
    : fm_dir_(), fm_id(0), fm_log_file(NULL)
{ }

/** destructor. calls quit() */
ft_job::~ft_job()
{
    quit();
}

/** initialize logging subsystem, or return error */
int ft_job::init_log()
{
    std::string log_name = fm_dir_;
    log_name += "fstransform.log";

    const char * log_file = log_name.c_str();

    if ((fm_log_file = fopen(log_file, "a")) == NULL)
        return ff_log(FC_ERROR, errno, "failed to open log file '%s'", log_file);

    ff_log_register(fm_log_file);
    return 0;
}


/** initialize this job, or return error */
int ft_job::init(const char * root_dir, ft_uint job_id)
{
    const char * user_home = NULL;
    if (root_dir == NULL) {
        user_home = getenv("HOME");
        if (user_home != NULL) {
            fm_dir_ = user_home;
            fm_dir_ += '/';
        }
    } else {
        fm_dir_ = root_dir;
        fm_dir_ += '/';
    }
    fm_dir_ += ".fstransform";

    const char * path = fm_dir_.c_str();

    if (root_dir == NULL && user_home == NULL)
        ff_log(FC_WARN, 0, "$HOME is not set, persistent storage will use sub-folders of '%s' in current directory", path);

    (void) FT_IO_NS ff_mkdir(path);

    fm_dir_ += "/job.";
    ft_size len = fm_dir_.size();
    ft_uint i, job_min = 1, job_max = (ft_uint)-1;
    int err = 0;

    if (job_id != 0)
        /* force job_id */
        job_min = job_id, job_max = job_id + 1;

    path = fm_dir_.c_str();

    for (i = job_min; i != job_max; i++) {
        // 2 + 3*sizeof(ft_uint) chars are enough to safely print (ft_uint) and '/'
        fm_dir_.resize(len + 2 + 3*sizeof(ft_uint));
        // end job_dir_ with '/' - needed by everybody using job_dir_
        sprintf(& fm_dir_[len], "%"FS_ULL"/", (FT_ULL) i);
        fm_dir_.resize(len + strlen(& fm_dir_[len]));

        path = fm_dir_.c_str();

        if ((err = FT_IO_NS ff_mkdir(path)) == 0
                && (err = init_log()) == 0)
        {
            ff_log(FC_NOTICE, 0, "starting job %"FS_ULL", persistent storage in '%s'", (FT_ULL)i, path);
            break;
        }
    }
    if (i == job_max) {
        if (job_id != 0)
            err = ff_log(FC_ERROR, err, "failed to create persistent storage in '%s' for job id %"FS_ULL, path, (FT_ULL) job_id);
        else
            err = ff_log(FC_ERROR, err, "failed to locate a free job id, tried range %"FS_ULL"...%"FS_ULL, (FT_ULL) job_min, (FT_ULL) (job_max-1));
    }
    if (err == 0)
        fm_id = i;
    else
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
    if (!fm_dir_.empty())
        fm_dir_.clear();

    fm_id = 0;
}

FT_NAMESPACE_END
