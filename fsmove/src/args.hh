/*
 * main.hh
 *
 *  Created on: Mar 20, 2011
 *      Author: max
 */

#ifndef FSMOVE_ARGS_HH
#define FSMOVE_ARGS_HH

#include "types.hh"     // for ft_uint, ft_size
#include "io/io.hh"     // for FC_ARGS_COUNT

FT_NAMESPACE_BEGIN

enum fm_io_kind { FC_IO_AUTODETECT, FC_IO_POSIX };
enum fm_ui_kind { FC_UI_NONE };

class fm_args
{
public:
    const char * io_args[FT_IO_NS fm_io::FC_ARGS_COUNT];
    fm_io_kind io_kind;      // if FC_IO_AUTODETECT, will autodetect
    fm_ui_kind ui_kind;      // default is FC_UI_NONE
    bool force_run;          // if true, some sanity checks will be WARNINGS instead of ERRORS
    bool simulate_run;       // if true, move algorithm runs WITHOUT actually moving any file/directory/special-device

    fm_args();
};


FT_NAMESPACE_END

#endif /* FSMOVE_ARGS_HH */
