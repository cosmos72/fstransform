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
 * main.hh
 *
 *  Created on: Mar 20, 2011
 *      Author: max
 */

#ifndef FSREMAP_ARGS_HH
#define FSREMAP_ARGS_HH

#include "types.hh"     // for ft_uint, ft_size

FT_NAMESPACE_BEGIN

enum fr_storage_size     { FC_MEM_BUFFER_SIZE, FC_SECONDARY_STORAGE_SIZE, FC_PRIMARY_STORAGE_EXACT_SIZE, FC_SECONDARY_STORAGE_EXACT_SIZE, FC_STORAGE_SIZE_N, };

enum fr_clear_free_space { FC_CLEAR_AUTODETECT, FC_CLEAR_ALL, FC_CLEAR_MINIMAL, FC_CLEAR_NONE, };
enum fr_job_id_kind      { FC_JOB_ID_AUTODETECT = 0 };
enum fr_io_kind          { FC_IO_AUTODETECT, FC_IO_TEST, FC_IO_SELF_TEST, FC_IO_POSIX, FC_IO_PREALLOC };
enum fr_mount_points     { FC_MOUNT_POINT_DEVICE = 0, FC_MOUNT_POINT_LOOP_FILE, FC_MOUNT_POINTS_N };
enum fr_ui_kind          { FC_UI_NONE, FC_UI_TTY };

class fr_args
{
public:
    const char * program_name;       // detected from command line. default: "fsremap"
    const char * root_dir;           // write logs and persistence files inside this folder. if NULL, will autodetect
    const char * io_args[3];         // some I/O will need less than 3 arguments
    const char * mount_points[FC_MOUNT_POINTS_N]; // device and loop file mount points. currently only needed by fr_io_prealloc
    const char * loop_dev;           // loop device. currently only needed by fr_io_prealloc
    const char * ui_arg;
    const char * cmd_losetup;        // 'losetup' command. currently only needed by fr_io_prealloc
    const char * cmd_umount;
    ft_size storage_size[FC_STORAGE_SIZE_N]; // if 0, will autodetect
    ft_uint job_id;                  // if FC_JOB_ID_AUTODETECT, will autodetect
    fr_clear_free_space job_clear;
    fr_io_kind io_kind;              // if FC_IO_AUTODETECT, will autodetect
    fr_ui_kind ui_kind;
    bool force_run;                  // if true, some sanity checks will be WARNINGS instead of ERRORS
    bool simulate_run;               // if true, remapping algorithm runs WITHOUT reading or writing device blocks
    bool ask_questions;              // if true, remapping algorithm will ask confirmation and read answer from stdin before actually starting

    fr_args();
};


FT_NAMESPACE_END

#endif /* FSREMAP_ARGS_HH */
