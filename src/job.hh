/*
 * log.hh
 *
 *  Created on: Mar 8, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_JOB_HH
#define FSTRANSFORM_JOB_HH

#include "types.hh"    // for ft_size

#include <cstdio>      // for FILE

#include <string>      // for std::string

FT_NAMESPACE_BEGIN

class ft_job
{
private:
    std::string fm_dir;
    ft_size fm_storage_size;
    ft_uint fm_id;

    FILE * fm_log_file;

    /** initialize logging subsystem */
    int init_log();

public:
    /** default constructor */
    ft_job();

    /** destructor. calls quit() */
    ~ft_job();

    /** initialize this job, or return error */
    int init(const char * root_dir = NULL, ft_uint job_id = 0, ft_size storage_size = 0);

    /** quit this job */
    void quit();

    /** return job_id, or 0 if not set */
    FT_INLINE ft_uint job_id() const { return fm_id; }

    /** return job_dir, or empty if not set */
    FT_INLINE const std::string & job_dir() const { return fm_dir; }

    /** return storage_size to use (in bytes), or 0 if not set */
    FT_INLINE ft_size job_storage_size() const { return fm_storage_size; }

    /** set storage_size to use (in bytes), or 0 if not set */
    FT_INLINE void job_storage_size(ft_size len) { fm_storage_size = len; }
};

FT_NAMESPACE_END


#endif /* FSTRANSFORM_JOB_HH */
