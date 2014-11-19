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
 * remap.cc
 *
 *  Created on: Feb 14, 2011
 *      Author: max
 */

#include "first.hh"

#if defined(FT_HAVE_STRING_H)
# include <string.h>       // for strcmp()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>        // for strcmp()
#endif

#if defined(FT_HAVE_STRING_H)
# include <stdlib.h>       // for atoi()
#elif defined(FT_HAVE_CSTRING)
# include <cstdlib>        // for atoi()
#endif

#include "log.hh"             // for ff_log()
#include "map.hh"             // for fr_map<T>
#include "vector.hh"          // for fr_vector<T>
#include "dispatch.hh"        // for fr_dispatch
#include "remap.hh"           // for fr_remap
#include "misc.hh"            // for ff_strtoul()

#include "io/io.hh"           // for fr_io
#include "io/io_posix.hh"     // for fr_io_posix
#ifdef FT_HAVE_IO_PREALLOC
# include "io/io_prealloc.hh"  // for fr_io_prealloc
#endif
#include "io/io_self_test.hh" // for fr_io_self_test
#include "io/util_dir.hh"     // for ff_mkdir()


FT_NAMESPACE_BEGIN


enum {
    FC_DEVICE = FT_IO_NS fr_io_posix::FC_DEVICE,
    FC_LOOP_FILE = FT_IO_NS fr_io_posix::FC_LOOP_FILE,
    FC_FILE_COUNT = FT_IO_NS fr_io_posix::FC_FILE_COUNT
};

static char const* const* label = FT_IO_NS fr_io::label;
static char const* const* LABEL = FT_IO_NS fr_io::LABEL;





/** constructor */
fr_remap::fr_remap()
    : this_job(NULL), this_persist(NULL), this_io(NULL), this_ui(NULL), quit_immediately(false)
{ }

/** destructor. calls quit_io(), quit_ui() and quit_job_persist() */
fr_remap::~fr_remap()
{
    quit_io();
    quit_ui();
    quit_job_persist();
}

/**
 * high-level main method.
 * calls in sequence: init(argc, argv), run() and quit_io()
 *
 * expects argc == 4 and four arguments in argv:
 * program_name, DEVICE, LOOP-FILE and ZERO-FILE.
 *
 * return 0 if success, else error.
 * if invoked with the argument "--help" or "--version", calls usage() or version() and immediately returns 0
 */
int fr_remap::main(int argc, char const* const* argv)
{
    fr_remap remapper;

    int err = remapper.init(argc, argv);

    if (err == 0 && !remapper.quit_immediately)
        err = remapper.run();
    /*
     * note 1.2.2) fsremap::main() must check for unreported errors
     * and log them them with message "failed with unreported error"
     */
    if (!ff_log_is_reported(err))
        err = ff_log(FC_ERROR, err, "failed with unreported error");

    return err;
}


/** print command-line usage to stdout and return 0 */
int fr_remap::usage(const char * program_name)
{
    quit_immediately = true;

    ff_log(FC_NOTICE, 0, "Usage: %s [OPTION]... %s %s [%s]", program_name, LABEL[0], LABEL[1], LABEL[2]);
    ff_log(FC_NOTICE, 0, "  or:  %s [OPTION]... --resume-job=JOB_ID %s", program_name, LABEL[0]);
    ff_log(FC_NOTICE, 0, "Replace the contents of %s with the contents of %s, i.e. write %s onto %s",
            LABEL[FC_DEVICE], LABEL[FC_LOOP_FILE], LABEL[FC_LOOP_FILE], LABEL[FC_DEVICE]);
    ff_log(FC_NOTICE, 0, "even if %s is inside a file system _inside_ %s\n", LABEL[FC_LOOP_FILE], LABEL[FC_DEVICE]);

    return ff_log
    (FC_NOTICE, 0, "Mandatory arguments to long options are mandatory for short options too.\n"
     "  --                    end of options. treat subsequent parameters as arguments\n"
     "                          even if they start with '-'\n"
     "  -a, --no-questions    automatic run: do not ask any question\n"
     "      --clear=all       clear all free blocks after remapping (default)\n"
     "      --clear=minimal   (DANGEROUS) clear only overwritten free blocks\n"
     "                          after remapping\n"
     "      --clear=none      (DANGEROUS) do not clear any free blocks after remapping\n"
     "      --cmd-umount=CMD  command to unmount %s (default: /bin/umount)\n"
     "      --cmd-losetup=CMD 'losetup' command (default: /sbin/losetup)\n"
     "      --color=MODE      set messages color. MODE is one of:"
     "                          auto (default), none, ansi\n"
#ifdef FT_HAVE_IO_PREALLOC
     "      --device-mount-point=DIR\n"
     "                        set device mount point (needed by --io=prealloc)\n"
#endif
     "  -f, --force-run       continue even if some sanity checks fail\n"
     "      --io=posix        use posix I/O (default)\n"
#ifdef FT_HAVE_IO_PREALLOC
     "      --io=prealloc     use posix I/O with EXPERIMENTAL preallocated files\n"
#endif
     "      --io=self-test    perform in-memory self-test with random data\n"
     "      --io=test         use test I/O. Arguments are:\n"
     "                          DEVICE-LENGTH LOOP-FILE-EXTENTS FREE-SPACE-EXTENTS\n"
#ifdef FT_HAVE_IO_PREALLOC
     "      --loop-device=LOOP-DEVICE\n"
     "                        loop device to disconnect (needed by --io=prealloc)\n"
     "      --loop-mount-point=DIR\n"
     "                        set loop file mount point (needed by --io=prealloc)\n"
#endif
     "  -m, --mem-buffer=RAM_SIZE[k|M|G|T|P|E|Z|Y]\n"
     "                        set RAM buffer size (default: autodetect)\n"
     "  -n, --no-action, --simulate-run\n"
     "                        do not actually read or write any disk block\n"
     "      --questions=MODE  set interactive mode. MODE is one of:\n"
     "                          no: never ask questions, abort on errors (default)\n"
     "                          yes: ask questions in case of user-fixable errors\n"
     "                          extra: also ask confirmation before dangerous steps\n"
     "  -q, --quiet           be quiet, print less output\n"
     "  -qq                   be very quiet, only print warnings or errors\n"
     "      --resume-job=NUM  resume an interrupted job\n"
     "  -s, --secondary-storage=SECONDARY_SIZE[k|M|G|T|P|E|Z|Y]\n"
     "                        set secondary storage file length (default: autodetect)\n"
     "  -t, --temp-dir=DIR    write storage and log files inside DIR\n"
     "                          (default: /var/tmp/fstransform)\n"
     "      --ui-tty=TTY      show full-text progress on tty device TTY\n"
     "  -v, --verbose         be verbose\n"
     "  -vv                   be very verbose\n"
     "  -vvv                  be incredibly verbose (warning: prints lots of output)\n"
     "  -xp, --exact-primary-storage=PRIMARY_SIZE[k|M|G|T|P|E|Z|Y]\n"
     "                        set *exact* primary storage length, or fail\n"
     "                          (default: autodetect)\n"
     "  -xs, --exact-secondary-storage=SECONDARY_SIZE[k|M|G|T|P|E|Z|Y]\n"
     "                        set *exact* secondary storage length, or fail\n"
     "                          (default: autodetect)\n"
     "      --x-OPTION=VALUE  set internal, undocumented option. for maintainers only\n"
     "      --help            display this help and exit\n"
     "      --version         output version information and exit\n",
     LABEL[FC_DEVICE]);
}


/** output version information and return 0 */
int fr_remap::version()
{
    quit_immediately = true;

    return ff_log(FC_NOTICE, 0,
            "fsremap (fstransform utilities) " FT_VERSION "\n"
            "Copyright (C) 2011-2014 Massimiliano Ghilardi\n"
            "\n"
            "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
            "This is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n");
}


int fr_remap::invalid_cmdline(const fr_args & args, int err, const char * fmt, ...)
{
    va_list vargs;

    va_start(vargs, fmt);
    err = ff_vlog(FC_ERROR, err, fmt, vargs);
    va_end(vargs);

    ff_log(FC_NOTICE, 0, "Try `%s --help' for more information", args.program_name);
    /* mark error as reported */
    return err ? err : -EINVAL;
}

/** return EISCONN if remapper is initialized, else call quit_io() and return 0 */
int fr_remap::check_is_closed()
{
    int err = 0;
    if (is_initialized()) {
        ff_log(FC_ERROR, 0, "error: I/O subsystem already started");
        /* error is already reported, flip sign */
        err = -EISCONN;
    } else
        // quit_io() to make sure we are not left in a half-initialized status
        // (this_io != NULL && !this_io->is_open())
        quit_io();
    return err;
}

/** return 0 if remapper is initialized, else ENOTCONN */
int fr_remap::check_is_open()
{
    int err = 0;
    if (!is_initialized()) {
        ff_log(FC_ERROR, 0, "error: I/O subsystem not started");
        // quit_io() to make sure we are not left in a half-initialized status
        // (this_io != NULL && !this_io->is_open())
        quit_io();
        /* error is already reported, flip sign */
        err = -ENOTCONN;
    }
    return err;
}

static ft_size ff_str_index_of_plus_1(const char * haystack, char needle)
{
    const char * match = strchr(haystack, (int) needle);
    if (match != NULL)
        return match - haystack + 1;
    return strlen(haystack);
}
/**
 * parse from command line and initialize all subsystems (job, I/O, log...)
 * return 0 if success, else error.
 *
 * implementation: parse command line, fill a fr_args and call init(const fr_args &)
 */
int fr_remap::init(int argc, char const* const* argv)
{
    fr_args args;
    int err;
    fr_io_kind io_kind;
    fr_clear_free_space new_clear;
    ft_log_fmt format = FC_FMT_MSG;
    ft_log_level level = FC_INFO, new_level;
    ft_log_color color = FC_COL_AUTO;
    bool format_set = false;

    do {
        if ((err = check_is_closed()) != 0)
            break;

        if (argc == 0) {
            err = invalid_cmdline(args, 0, "missing arguments: %s %s [%s]", LABEL[0], LABEL[1], LABEL[2]);
            break;
        }

        const char * arg;
        ft_size io_args_n = 0;
        bool allow_opts = true;

        args.program_name = argv[0];
        
        // skip program_name
        while (err == 0 && --argc) {
            arg = * ++argv;
            if (allow_opts && arg[0] == '-') {

                ft_size opt_len = ff_str_index_of_plus_1(arg, '=');
                bool is_short_opt = arg[1] != '-';

                const char * opt_arg = argc > 1 && is_short_opt ? argv[1] : arg + opt_len;

                /* -- means end of options*/
                if (!strcmp(arg, "--"))
                    allow_opts = false;

                /* -a, --automated run automatically without asking any confirmation  */
                else if (!strcmp(arg, "-a") || !strcmp(arg, "--automated")) {
                    args.ask_questions = false;
                }
                /* --clear=all, --clear=minimal, --clear=none */
                else if ((new_clear = FC_CLEAR_ALL,   !strcmp(arg, "--clear=all"))
                    || (new_clear = FC_CLEAR_MINIMAL, !strcmp(arg, "--clear=minimal"))
                    || (new_clear = FC_CLEAR_NONE,    !strcmp(arg, "--clear=none"))) {

                    if (args.job_clear == FC_CLEAR_AUTODETECT)
                        args.job_clear = new_clear;
                    else
                        err = invalid_cmdline(args, 0,
                                "options --clear=all, --clear=minimal and --clear=none are mutually exclusive");
                }
                /* --cmd-losetup=CMD */
                else if (!strncmp(arg, "--cmd-losetup=", opt_len)) {
                    args.cmd_losetup = opt_arg;
                }
                /* --cmd-umount=CMD */
                else if (!strncmp(arg, "--cmd-umount=", opt_len)) {
                    args.cmd_umount = opt_arg;
                }
                /* -f, --force-run: consider failed sanity checks as WARNINGS (which let execution continue) instead of ERRORS (which stop execution) */
                else if (!strcmp(arg, "-f") || !strcmp(arg, "--force-run")) {
                    args.force_run = true;
                }
                /* -i, --interactive: ask confirmation after analysis, before starting real work */
                else if (!strcmp(arg, "-i") || !strcmp(arg, "--interactive")) {
                    args.ask_questions = true;
                }
                /* --io=test, --io=self-test, --io=posix, --io=prealloc */
                else if ((io_kind = FC_IO_TEST,        !strcmp(arg, "--io=test"))
                        || (io_kind = FC_IO_SELF_TEST, !strcmp(arg, "--io=self-test"))
                        || (io_kind = FC_IO_POSIX,     !strcmp(arg, "--io=posix"))
#ifdef FT_HAVE_IO_PREALLOC
                        || (io_kind = FC_IO_PREALLOC,  !strcmp(arg, "--io=prealloc"))
#endif
                        )
                {
                    if (args.io_kind == FC_IO_AUTODETECT)
                        args.io_kind = io_kind;
                    else
                        err = invalid_cmdline(args, 0,
                                "options --io=posix, --io=prealloc, --io=test and --io=self-test are mutually exclusive");
                }
                else if (!strncmp(arg, "--loop-device=", opt_len)) {
                    args.loop_dev = opt_arg;
                }
                /* -m, --mem-buffer=RAM_SIZE[k|M|G|T|P|E|Z|Y] */
                else if ((argc > 1 && !strcmp(arg, "-m")) || !strncmp(arg, "--mem-buffer=", opt_len)) {
                    
                    if ((err = ff_str2un_scaled(opt_arg, & args.storage_size[FC_MEM_BUFFER_SIZE])) != 0) {
                        err = invalid_cmdline(args, err, "invalid memory buffer size '%s'", opt_arg);
                        break;
                    }
                    if (is_short_opt)
                        --argc, ++argv;
                }
                else if (!strncmp(arg, "--device-mount-point=", opt_len)) {
                    args.mount_points[FC_MOUNT_POINT_DEVICE] = opt_arg;
                }
                else if (!strncmp(arg, "--loop-mount-point=", opt_len)) {
                    args.mount_points[FC_MOUNT_POINT_LOOP_FILE] = opt_arg;
                }
                /* -n, --no-action, --simulate-run: do not read or write device blocks  */
                else if (!strcmp(arg, "-n") || !strcmp(arg, "--no-action") || !strcmp(arg, "--simulate-run")) {
                    args.simulate_run = true;
                }
                /* --questions=[no|yes|extra] */
                else if (!strncmp(arg, "--questions=", opt_len))
                {
                	args.ask_questions = !strcmp("extra", opt_arg);
                }
                /* --resume-job=JOB_ID */
                else if (!strncmp(arg, "--resume-job=", opt_len)) {
                    if (args.job_id != FC_JOB_ID_AUTODETECT) {
                        err = invalid_cmdline(args, err, "option --resume-job=JOB_ID can be specified only once");
                        break;
                    }
                    if ((err = ff_str2un(opt_arg, & args.job_id)) != 0 || args.job_id == FC_JOB_ID_AUTODETECT) {
                        err = invalid_cmdline(args, err, "invalid job id '%s'", opt_arg);
                        break;
                    }
                    if (is_short_opt)
                        --argc, ++argv;
                }
                /* -s, --secondary-storage=SECONDARY_SIZE[k|M|G|T|P|E|Z|Y] */
                else if ((argc > 1 && !strcmp(arg, "-s")) || !strncmp(arg, "--secondary-storage=", opt_len)) {

                    if ((err = ff_str2un_scaled(opt_arg, & args.storage_size[FC_SECONDARY_STORAGE_SIZE])) != 0) {
                        err = invalid_cmdline(args, err, "invalid secondary storage size '%s'", opt_arg);
                        break;
                    }
                    if (is_short_opt)
                        --argc, ++argv;
                }
                /* -t, --temp-dir=DIR */
                else if ((argc > 1 && !strcmp(arg, "-t")) || !strncmp(arg, "--temp-dir=", opt_len)) {
                    args.root_dir = opt_arg;
                    if (is_short_opt)
                        --argc, ++argv;
                }
                /* --ui-tty=TTY */
                else if (!strncmp(arg, "--ui-tty=", opt_len)) {
                    args.ui_kind = FC_UI_TTY;
                    args.ui_arg = opt_arg;
                }
                /* -xp, --exact-primary-storage=PRIMARY_SIZE[k|M|G|T|P|E|Z|Y] */
                else if ((argc > 1 && !strcmp(arg, "-xp")) || !strncmp(arg, "--exact-primary-storage=", opt_len)) {
                    
                    if ((err = ff_str2un_scaled(opt_arg, & args.storage_size[FC_PRIMARY_STORAGE_EXACT_SIZE])) != 0) {
                        err = invalid_cmdline(args, err, "invalid primary storage exact size '%s'", opt_arg);
                        break;
                    }
                    if (is_short_opt)
                        --argc, ++argv;
                }
                /* -xs, --exact-secondary-storage=SECONDARY_SIZE[k|M|G|T|P|E|Z|Y] */
                else if ((argc > 1 && !strcmp(arg, "-xs")) || !strncmp(arg, "--exact-secondary-storage=", opt_len)) {
                    
                    if ((err = ff_str2un_scaled(opt_arg, & args.storage_size[FC_SECONDARY_STORAGE_EXACT_SIZE])) != 0) {
                        err = invalid_cmdline(args, err, "invalid secondary storage exact size '%s'", opt_arg);
                        break;
                    }
                    if (is_short_opt)
                        --argc, ++argv;
                }
                /* --x-log-FILE=LEVEL */
                else if (!strncmp(arg, "--x-log-", 8)) {
                    ft_mstring logger_name(arg + 8, opt_len - 9); // 9 == 8 for "--x-log-" plus 1 for '='
                    ft_log_level logger_level = (ft_log_level) atoi(opt_arg);
                    ft_log::get_logger(logger_name).set_level(logger_level);
                }
                /* -q, --quiet decrease verbosity by one */
                /* -qq decrease verbosity by two */
                /* -v, --verbose increase verbosity by one */
                /* -vv increase verbosity by two */
                /* -vvv increase verbosity by three */
                else if ((new_level = FC_WARN, !strcmp(arg, "-qq"))
                    || (new_level = FC_NOTICE, !strcmp(arg, "-q") || !strcmp(arg, "--quiet"))
                    || (new_level = FC_DEBUG, !strcmp(arg, "-v") || !strcmp(arg, "--verbose"))
                    || (new_level = FC_TRACE, !strcmp(arg, "-vv"))
                    || (new_level = FC_DUMP, !strcmp(arg, "-vvv")))
                {
                    if (level == FC_INFO)
                        level = new_level;
                    else {
                        err = invalid_cmdline(args, 0,
                                "options -q, -qq, -v, -vv, -vvv, --quiet, --verbose are mutually exclusive");
                        break;
                    }
				} else if (!strncmp(arg, "--log-color=", 12)) {
					/* --color=(auto|none|ansi) */
					arg += 12;
					if (!strcmp(arg, "ansi"))
						color = FC_COL_ANSI;
					else if (!strcmp(arg, "none"))
						color = FC_COL_NONE;
					else
						color = FC_COL_AUTO;
                }
                else if (!strncmp(arg, "--log-format=", 13)) {
                    /* --color=(auto|none|ansi) */
                	arg += 13;
                	if (!strcmp(arg, "level_msg"))
                		format = FC_FMT_LEVEL_MSG;
                	else if (!strcmp(arg, "time_level_msg"))
                		format = FC_FMT_DATETIME_LEVEL_MSG;
                	else if (!strcmp(arg, "time_level_function_msg"))
                		format = FC_FMT_DATETIME_LEVEL_CALLER_MSG;
                	else
                		format = FC_FMT_MSG;
                	format_set = true;
                }
				else if (!strcmp(arg, "--help")) {
                    return usage(args.program_name);
                }
				else if (!strcmp(arg, "--version")) {
                    return version();
                }
				else {
                    err = invalid_cmdline(args, 0, "unknown option: '%s'", arg);
                    break;
                }
                continue;
            }
            /** found an argument */
            if (io_args_n < FC_FILE_COUNT)
                args.io_args[io_args_n++] = arg;
            else
                err = invalid_cmdline(args, 0, "too many arguments");
        }

        if (err != 0)
        	break;

		/* if autodetect, clear all free blocks */
		if (args.job_clear == FC_CLEAR_AUTODETECT)
			args.job_clear = FC_CLEAR_ALL;

		/* if autodetect, use POSIX I/O */
		if (args.io_kind == FC_IO_AUTODETECT)
			args.io_kind = FC_IO_POSIX;

		if (args.io_kind == FC_IO_POSIX || args.io_kind == FC_IO_PREALLOC) {
			if (args.job_id == FC_JOB_ID_AUTODETECT) {
				if (io_args_n == 0) {
					err = invalid_cmdline(args, 0, "missing arguments: %s %s [%s]", LABEL[0], LABEL[1], LABEL[2]);
				} else if (io_args_n == 1) {
					err = invalid_cmdline(args, 0, "missing arguments: %s [%s]", LABEL[1], LABEL[2]);
				} else if (io_args_n == 2 || io_args_n == 3) {
					 /* ok */
				} else
					err = invalid_cmdline(args, 0, "too many arguments");
			} else {
				if (io_args_n == 0) {
					err = invalid_cmdline(args, 0, "missing argument: %s", LABEL[0]);
				} else if (io_args_n == 1) {
					/* ok */
				} else
					err = invalid_cmdline(args, 0, "too many arguments");
			}
		} else if (args.io_kind == FC_IO_TEST) {
			if (io_args_n == 0) {
				err = invalid_cmdline(args, 0, "missing arguments: %s %s %s", LABEL[0], LABEL[1], LABEL[2]);
			} else if (io_args_n == 1) {
				err = invalid_cmdline(args, 0, "missing arguments: %s %s", LABEL[1], LABEL[2]);
			} else if (io_args_n == 2) {
				err = invalid_cmdline(args, 0, "missing argument: %s", LABEL[2]);
			} else if (io_args_n == 3) {
				/* ok */
			} else
				err = invalid_cmdline(args, 0, "too many arguments");
        }
    } while (0);

    if (err == 0) {
        /*
         * always enable at least DEBUG level, to let let the appender installed by fr_job::init_log()
         * intercept all messages from DEBUG to FATAL.
         * we avoid spamming the user by setting stdout appender->min_level = level below
         */
        ft_log::get_root_logger().set_level(level < FC_DEBUG ? level : FC_DEBUG);

        /* note 1.4.1) -v sets format FC_FMT_LEVEL_MSG */
        /* note 1.4.2) -vv sets format FC_FMT_DATETIME_LEVEL_MSG */
        if (!format_set)
        	format = level < FC_DEBUG ? FC_FMT_DATETIME_LEVEL_MSG : level == FC_DEBUG ? FC_FMT_LEVEL_MSG : FC_FMT_MSG;

        // set stdout appender->min_level, since we played tricks with root_logger->level above.
        ft_log_appender::reconfigure_all(format, level, color);

        err = init(args);
    }

    return err;
}


/**
 * initialize all subsystems (job, I/O, log...) using specified arguments
 * return 0 if success, else error.
 */
int fr_remap::init(const fr_args & args)
{
    int err;
    do {
        if ((err = init_job_persist(args)) != 0)
            break;
        if ((err = init_ui(args)) != 0)
            break;
        if ((err = init_io(args)) != 0)
            break;

    } while (0);

    return err;
}


/** initialize job/persistence subsystem */
int fr_remap::init_job_persist(const fr_args & args)
{
    fr_job * job = this_job;
    FT_IO_NS fr_persist * persist = this_persist;
    int err = 0;

    if ((job == NULL) != (persist == NULL)) {
        ff_log(FC_FATAL, 0, "BUG! half-initialized state detected in init_job_persist(): job %s NULL, persist %s NULL",
                (job == NULL ? "==" : "!="), (persist == NULL ? "==" : "!="));
        return -EINVAL;
    }

    if (job == NULL) {
        job = new fr_job();
        if ((err = job->init(args)) != 0) {
            quit_job_persist();
            return err;
        }
    }
    if (persist == NULL) {
        persist = new FT_IO_NS fr_persist(* job);
        if ((err = persist->open()) != 0) {
            quit_job_persist();
            return err;
        }
    }
    this_job = job;
    this_persist = persist;
    return err;
}

/** quit job/persistence subsystem */
void fr_remap::quit_job_persist()
{
    delete this_persist;
    delete this_job;
    this_persist = NULL;
    this_job = NULL;
    quit_immediately = false;
}

/** initialize UI subsystem */
int fr_remap::init_ui(const fr_args & args)
{
    if (this_ui != NULL) {
        ff_log(FC_ERROR, 0, "unexpected call to init_ui(): UI subsystem is already initialized");
        /* mark error as reported */
        return -EISCONN;
    }

    int err;
    switch (args.ui_kind) {
        case FC_UI_NONE:
            err = 0;
            break;
        case FC_UI_TTY:
            err = init_ui_tty(args.ui_arg);
            break;
        default:
            ff_log(FC_ERROR, 0, "tried to initialize unknown UI '%d': not tty", (int) args.ui_kind);
            err = -ENOSYS;
            break;
    }
    return err;
}

/** initialize UI subsystem */
int fr_remap::init_ui_tty(const char * arg)
{
    FT_UI_NS fr_ui_tty * ui_tty = new FT_UI_NS fr_ui_tty();

    int err = ui_tty->init(arg);
    if (err == 0)
        this_ui = ui_tty;
    else
        delete ui_tty;

    return err;
}

/** quit UI subsystem */
void fr_remap::quit_ui()
{
    delete this_ui;
    this_ui = NULL;
}


/**
 * choose the I/O to use, create and initialize it. if success, stores a pointer to I/O object.
 *
 * return 0 if success, else error.
 */
int fr_remap::init_io(const fr_args & args)
{
    int err;
    switch (args.io_kind) {
        case FC_IO_TEST:
            err = init_io_class<FT_IO_NS fr_io_test>(args);
            break;
        case FC_IO_SELF_TEST:
            err = init_io_class<FT_IO_NS fr_io_self_test>(args);
            break;
        case FC_IO_POSIX:
            err = init_io_class<FT_IO_NS fr_io_posix>(args);
            break;
#ifdef FT_HAVE_IO_PREALLOC
        case FC_IO_PREALLOC:
            err = init_io_class<FT_IO_NS fr_io_prealloc>(args);
            break;
#endif
        default:
            ff_log(FC_ERROR, 0, "tried to initialize unknown I/O '%d': not POSIX, not PREALLOC, not TEST, not SELF-TEST", (int) args.io_kind);
            err = -ENOSYS;
            break;
    }
    return err;
}


/**
 * initialize remapper to use I/O type IO_T.
 *
 * args depend on I/O type:
 * POSIX and PREALLOC I/O require two or three arguments in args.io_args: DEVICE, LOOP-FILE and optionally ZERO-FILE;
 * test I/O requires three arguments in args.io_args: DEVICE-LENGTH, LOOP-FILE-EXTENTS and ZERO-FILE-EXTENTS;
 * self-test I/O does not require any argument in args.io_args;
 * return 0 if success, else error.
 */
template<class IO_T>
    int fr_remap::init_io_class(const fr_args & args)
{
    int err;
    if ((err = pre_init_io()) == 0) {

        IO_T * io = new IO_T(* this_persist);

        if ((err = io->open(args)) == 0)
            post_init_io(io);
        else
            delete io;
    }
    return err;
}

int fr_remap::pre_init_io()
{
    int err = check_is_closed();
    if (err == 0 && this_persist == NULL) {
        ff_log(FC_ERROR, 0, "error: cannot start I/O subsystem, persistence must be initialized first");
        err = -ENOTCONN;
    }
    return err;
}

void fr_remap::post_init_io(FT_IO_NS fr_io * io)
{
    this_io = io;
    io->ui(this_ui);
}


/** shutdown remapper. closes configured I/O and deletes it */
void fr_remap::quit_io()
{
    delete this_io;
    this_io = NULL;
}

/**
 * perform actual work using configured I/O:
 * allocates fr_vector<ft_uoff> for both LOOP-FILE and FREE-SPACE extents,
 * calls this_io->read_extents() to fill them, and finally invokes
 * fr_dispatch::main(loop_file_extents, free_space_extents, this_io)
 *
 * return 0 if success, else error.
 */
int fr_remap::run()
{
    int err = 0;
    do {
        if ((err = check_is_open()) != 0)
            break;

        FT_IO_NS fr_io & io = * this_io;
        const char * dev_path = ff_if_null(io.dev_path(), "<unknown>");

        ff_log(FC_INFO, 0, "analyzing %s '%s', this may take some minutes ...", label[FC_DEVICE], dev_path);

        /* allocate fr_vector<ft_uoff> for both LOOP-FILE and FREE-SPACE extents */
        fr_vector<ft_uoff> loop_file_extents, free_space_extents;

        // preallocated extents in files inside loop file
        // which do NOT have a correspondence in files inside device:
        // they must be cleared once remapping is completed!
        fr_vector<ft_uoff> to_zero_extents;


        /* ask actual I/O subsystem to read LOOP-FILE and FREE-SPACE extents */
        if ((err = io.read_extents(loop_file_extents, free_space_extents, to_zero_extents)) != 0)
            break;

        /* persistence: save LOOP-FILE and FREE-SPACE extents to disk */
        if ((err = io.save_extents(loop_file_extents, free_space_extents, to_zero_extents)) != 0)
            break;

        io.close_extents();

        /* invoke fr_dispatch::main() to choose which fr_work<T> to instantiate, and run it */
        err = fr_dispatch::main(loop_file_extents, free_space_extents, to_zero_extents, io);

    } while (0);

    return err;
}



FT_NAMESPACE_END

