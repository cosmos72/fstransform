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
 * move.cc
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#include "first.hh"

#include "transform.hh"

#if defined(FT_HAVE_STRING_H)
# include <string.h>       // for strcmp()
#elif defined(FT_HAVE_CSTRING)
# include <cstring>        // for strcmp()
#endif

FT_NAMESPACE_BEGIN

/** default constructor */
ft_transform::ft_transform()
  : quit_immediately(false)
{ }

/** destructor. calls quit() */
ft_transform::~ft_transform()
{
    quit();
}


int ft_transform::init()
{
    return err;
}

/** performs cleanup. called by destructor, you can also call it explicitly after (or instead of) run()  */
void ft_transform::quit()
{
}

bool ft_transform::is_initialized() const
{
    return false;
}

/** return EISCONN if remapper is initialized, else call quit_io() and return 0 */
int ft_transform::check_is_closed()
{
    int err = 0;
    if (is_initialized()) {
        ff_log(FC_ERROR, 0, "error: already initialized");
        /* error is already reported, flip sign */
        err = -EISCONN;
    } else
        // quit() to make sure we are not left in a half-initialized status
        quit();
    return err;
}

/** return 0 if remapper is initialized, else ENOTCONN */
int ft_transform::check_is_open()
{
    int err = 0;
    if (!is_initialized()) {
        ff_log(FC_ERROR, 0, "error: not initialized");
        // quit() to make sure we are not left in a half-initialized status
        quit();
        /* error is already reported, flip sign */
        err = -ENOTCONN;
    }
    return err;
}

/**
 * high-level do-everything method. calls in sequence init(), run() and cleanup().
 * return 0 if success, else error.
 */
int ft_transform::main(int argc, char ** argv)
{
    ft_transform transformer;

    int err = transformer.init(argc, argv);

    if (err == 0 && !transformer.quit_immediately)
        err = transformer.run();

    /*
     * note 1.2.2) fsmove::main() must check for unreported errors
     * and log them them with message "failed with unreported error"
     */
    if (!ff_log_is_reported(err))
        err = ff_log(FC_ERROR, err, "failed with unreported error");

    return err;
}

/** print command-line usage to stdout and return 0 */
int ft_transform::usage(const char * program_name)
{
    quit_immediately = true;

    ff_log(FC_NOTICE, 0, "Usage: %s [OPTION]... DEVICE FILESYSTEM_TYPE", program_name);
    ff_log(FC_NOTICE, 0, "Convert device filesystem to a different filesystem type,");
    ff_log(FC_NOTICE, 0, "preserving its contents");

    return ff_log(FC_NOTICE, 0,
     "Mandatory arguments to long options are mandatory for short options too.\n"
     "  --                    end of options. treat subsequent parameters as arguments\n"
     "                          even if they start with '-'\n"
     "  -e, --exclude FILE... skip these files, i.e. do not move them.\n"
     "                          must be last argument\n"
     "  -f, --force-run       run even if some safety checks fail\n"
     "      --io=posix        use POSIX I/O and move files (default)\n"
#ifdef FT_HAVE_FM_IO_IO_PREALLOC
     "      --io=prealloc     use POSIX I/O and preallocate files (do NOT move them)\n"
#endif
     "      --inode-cache-mem use in-memory inode cache (default)\n"
     "      --inode-cache=DIR create and use directory DIR for inode cache\n"
	 "      --log-color=MODE  set messages color. MODE is one of:"
	 "                          auto (default), none, ansi\n"
	 "      --log-format=FMT  set messages format. FMT is one of:\n"
	 "                          msg (default), level_msg, time_level_msg,\n"
     "                          time_level_function_msg\n"
     "  -n, --no-action, --simulate-run\n"
     "                        do not actually move any file or directory\n"
     "  -q, --quiet           be quiet\n"
     "  -qq                   be very quiet, only print warnings or errors\n"
     "  -v, --verbose         be verbose, print what is being done\n"
     "  -vv                   be very verbose, print a lot of detailed output\n"
     "  -vvv                  be incredibly verbose (warning: prints LOTS of output)\n"
     "      --help            display this help and exit\n"
     "      --version         output version information and exit\n");
}

/** output version information and return 0 */
int ft_transform::version()
{
    quit_immediately = true;

    return ff_log(FC_NOTICE, 0,
            "fsmove (fstransform utilities) " FT_VERSION "\n"
            "Copyright (C) 2011-2014 Massimiliano Ghilardi\n"
            "\n"
            "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
            "This is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n");
}

int ft_transform::invalid_cmdline(const char * program_name, int err, const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    err = ff_vlog(FC_ERROR, err, fmt, args);
    va_end(args);

    ff_log(FC_NOTICE, 0, "Try `%s --help' for more information", program_name);
    /* mark error as reported */
    return err ? err : -EINVAL;
}


/**
 * parse command line and initialize all subsystems (job, I/O, log...)
 * return 0 if success, else error.
 *
 * implementation: parse command line, fill a fm_args and call init(const fm_args &)
 */
int ft_transform::init(int argc, char const* const* argv)
{
    fm_args args;
    int err;
    ft_log_fmt format = FC_FMT_MSG;
    ft_log_level level = FC_INFO, new_level;
    ft_log_color color = FC_COL_AUTO;
    bool format_set = false;

    do {
        if ((err = check_is_closed()) != 0)
            break;

        if (argc == 0) {
            err = invalid_cmdline("fsmove", 0, "missing arguments: %s %s", LABEL[0], LABEL[1]);
            break;
        }

        const char * arg, * program_name = argv[0];
        ft_size io_args_n = 0;
        bool allow_opts = true;

        // skip program_name
        while (err == 0 && --argc) {
            arg = * ++argv;

            /* -e is allowed even after '--', but must be last argument */
            if (io_args_n == FC_ARGS_COUNT && argc > 0 && arg[0] == '-'
                && (!strcmp(arg, "-e") || !strcmp(arg, "--exclude")) )
            {
                args.exclude_list = argv + 1;
                // -e uses all remaining arguments, so stop processing them
                break;
            }

            if (allow_opts && arg[0] == '-') {

                /* -- end of options*/
                if (!strcmp(arg, "--"))
                    allow_opts = false;

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
                        err = invalid_cmdline(program_name, 0, "options -q, -qq, -v, -vv, -vvv, --quiet, --verbose are mutually exclusive");
                        break;
                    }
                }
                else if (!strncmp(arg, "--log-color=", 12)) {
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
                else if (!strncmp(arg, "--x-log-", 8)) {
                    /* --x-log-FILE=LEVEL */
                	arg += 8;
                	const char * equal = strchr(arg, '=');

					ft_mstring logger_name(arg, equal ? equal - arg : strlen(arg));
					ft_log_level logger_level = equal ? (ft_log_level) atoi(equal+1) : FC_INFO;
					ft_log::get_logger(logger_name).set_level(logger_level);
                }
                /* -f force run: degrade failed sanity checks from ERRORS (which stop execution) to WARNINGS (which let execution continue) */
                else if (!strcmp(arg, "-f") || !strcmp(arg, "--force-run")) {
                    args.force_run = true;
                }
                /* -n simulate run: do not read or write device blocks  */
                else if (!strcmp(arg, "-n") || !strcmp(arg, "--no-action") || !strcmp(arg, "--simulate-run")) {
                    args.simulate_run = true;
                }
                /* --io=posix */
                else if ((io_kind = FC_IO_POSIX, !strcmp(arg, "--io=posix"))
#ifdef FT_HAVE_FM_IO_IO_PREALLOC
                         || (io_kind = FC_IO_PREALLOC, !strcmp(arg, "--io=prealloc"))
#endif
                        )
                {
                    if (args.io_kind == FC_IO_AUTODETECT)
                        args.io_kind = io_kind;
                    else {
#ifdef FT_HAVE_FM_IO_IO_PREALLOC
                        err = invalid_cmdline(program_name, 0, "options --io=posix and --io=prealloc are mutually exclusive");
#else
                        err = invalid_cmdline(program_name, 0, "option --io=posix can be specified only once");
#endif
                    }
                }
                else if (!strcmp(arg, "--inode-cache-mem")) {
                       args.inode_cache_path = NULL;
                }
                else if (!strncmp(arg, "--inode-cache=", 14)) {
                    // do not allow empty dir name
                    if (arg[14] != '\0')
                        args.inode_cache_path = arg + 14;
                }
                else if (!strcmp(arg, "--help")) {
                    return usage(args.program_name);
                }
                else if (!strcmp(arg, "--version")) {
                    return version();
                }
                else {
                    err = invalid_cmdline(program_name, 0, "unknown option: '%s'", arg);
                    break;
                }
                continue;
            }
            /** found an argument */
            if (io_args_n < FC_ARGS_COUNT)
                args.io_args[io_args_n++] = arg;
            else
                err = invalid_cmdline(program_name, 0, "too many arguments");
        }

        if (err != 0)
        	break;

		/* if autodetect, use POSIX I/O */
		if (args.io_kind == FC_IO_AUTODETECT)
			args.io_kind = FC_IO_POSIX;

		if (args.io_kind == FC_IO_POSIX || args.io_kind == FC_IO_PREALLOC) {

			if (io_args_n == 0) {
				err = invalid_cmdline(program_name, 0, "missing arguments: %s %s", LABEL[0], LABEL[1]);
				break;
			} else if (io_args_n == 1) {
				err = invalid_cmdline(program_name, 0, "missing argument: %s", LABEL[1]);
				break;
			}

			const char * source_root = args.io_args[FT_IO_NS fm_io::FC_SOURCE_ROOT];
			if (args.inode_cache_path != NULL && source_root != NULL
					&& (args.inode_cache_path[0] == '/') != (source_root[0] == '/'))
			{
				err = invalid_cmdline(program_name, 0,
						"relative/absolute path mismatch between source directory `%s'\n"
						"\tand inode-cache directory `%s':\n"
						"\tthey must be either both absolute, i.e. starting with `/',\n"
						"\tor both relative, i.e. NOT starting with `/'",
						source_root,
						args.inode_cache_path);
				break;
			}
		}

    } while (0);

    if (err == 0) {
        ft_log::get_root_logger().set_level(level);

        /* note 1.4.1) -v sets format FC_FMT_LEVEL_MSG */
        /* note 1.4.2) -vv sets format FC_FMT_DATETIME_LEVEL_MSG */
        if (!format_set)
        	format = level < FC_DEBUG ? FC_FMT_DATETIME_LEVEL_MSG : level == FC_DEBUG ? FC_FMT_LEVEL_MSG : FC_FMT_MSG;

        // no dot alter appenders min_level
        ft_log_appender::reconfigure_all(format, FC_LEVEL_NOT_SET, color);

        err = init(args);
    }

    return err;
}

/**
 * initialize all subsystems (job, I/O, log...) using specified arguments
 * return 0 if success, else error.
 */
int ft_transform::init(const fm_args & args)
{
    FT_IO_NS fm_io * io = NULL;
    int err = 0;

    switch (args.io_kind) {
        case FC_IO_POSIX:
            if ((err = check_is_closed()) != 0)
                break;

            io = new FT_IO_NS fm_io_posix();
            break;
#ifdef FT_HAVE_FM_IO_IO_PREALLOC
        case FC_IO_PREALLOC:
            if ((err = check_is_closed()) != 0)
                break;

            io = new FT_IO_NS fm_io_prealloc();
            break;
#endif
        default:
            ff_log(FC_ERROR, 0, "tried to initialize unknown I/O '%d': not POSIX"
#ifdef FT_HAVE_FM_IO_IO_PREALLOC
                   ", not PREALLOC"
#endif
                   , (int) args.io_kind);
            err = -ENOSYS;
            break;
    }

    return err;
}

/**
 * transformation algorithm entry point. calls transform()
 */
int ft_transform::run()
{
    return transform();
}

/** core of transformation algorithm */
int ft_transform::transform()
{
    return 0
}


FT_NAMESPACE_END
