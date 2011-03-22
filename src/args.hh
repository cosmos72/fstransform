/*
 * main.hh
 *
 *  Created on: Mar 20, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_ARGS_HH
#define FSTRANSFORM_ARGS_HH

#include "types.hh"     // for ft_uint, ft_size

FT_NAMESPACE_BEGIN

enum ft_storage_size { FC_MEM_BUFFER_SIZE, FC_SECONDARY_STORAGE_SIZE, FC_PRIMARY_STORAGE_EXACT_SIZE, FC_SECONDARY_STORAGE_EXACT_SIZE, FC_STORAGE_SIZE_N };

class ft_args
{
public:
    const char * root_dir;   // if NULL, will autodetect
    const char * io_name;    // if NULL, will autodetect
    const char * io_args[3]; // some I/O will need less than 3 arguments
    ft_size storage_size[FC_STORAGE_SIZE_N];    // if 0, will autodetect
    ft_uint job_id;          // if 0, will autodetect
    bool force_run;          // if true, some sanity checks will be WARNINGS instead of ERRORS
    bool simulate_run;       // if true, relocation algorithm runs WITHOUT reading or writing device blocks

    ft_args();
};


FT_NAMESPACE_END

#endif /* FSTRANSFORM_ARGS_HH */
