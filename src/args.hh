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

enum ft_storage_size { FC_MEM_BUFFER_SIZE, FC_SECONDARY_STORAGE_SIZE, FC_PRIMARY_STORAGE_EXACT_SIZE, FC_SECONDARY_STORAGE_EXACT_SIZE, FC_STORAGE_SIZE_N, };

enum ft_clear_free_space { FC_CLEAR_AUTODETECT, FC_CLEAR_ALL, FC_CLEAR_MINIMAL, FC_CLEAR_NONE, };

enum ft_io_kind { FC_IO_AUTODETECT, FC_IO_POSIX, FC_IO_TEST, FC_IO_SELF_TEST };
enum ft_ui_kind { FC_UI_NONE, FC_UI_TTY };

class ft_args
{
public:
    const char * root_dir;   // if NULL, will autodetect
    const char * io_args[3]; // some I/O will need less than 3 arguments
    const char * ui_arg;
    ft_size storage_size[FC_STORAGE_SIZE_N];    // if 0, will autodetect
    ft_uint job_id;          // if 0, will autodetect
    ft_clear_free_space job_clear;
    ft_io_kind io_kind;      // if 0, will autodetect
    ft_ui_kind ui_kind;
    bool force_run;          // if true, some sanity checks will be WARNINGS instead of ERRORS
    bool simulate_run;       // if true, relocation algorithm runs WITHOUT reading or writing device blocks

    ft_args();
};


FT_NAMESPACE_END

#endif /* FSTRANSFORM_ARGS_HH */
