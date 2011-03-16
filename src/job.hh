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
    std::string this_dir;
    ft_size this_storage_size;

    FILE * this_log_file;
    ft_uint this_id;

    /** true if storage_size must be honored EXACTLY (to resume an existent job) */
    bool this_storage_size_exact;

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
    FT_INLINE ft_uint job_id() const { return this_id; }

    /** return job_dir, or empty if not set */
    FT_INLINE const std::string & job_dir() const { return this_dir; }

    /** return storage_size to use (in bytes), or 0 if not set */
    FT_INLINE ft_size job_storage_length() const { return this_storage_size; }

    /** set storage_size to use (in bytes), or 0 to unset it */
    FT_INLINE void job_storage_length(ft_size len) { this_storage_size = len; }

    /** return true if storage_size must be honored EXACTLY (to resume an existent job) */
    FT_INLINE bool job_storage_length_exact() const { return this_storage_size_exact; }

    /** set whether storage_size must be honored EXACTLY (to resume an existent job) */
    FT_INLINE void job_storage_length_exact(bool flag) { this_storage_size_exact = flag; }
};

FT_NAMESPACE_END


#endif /* FSTRANSFORM_JOB_HH */
