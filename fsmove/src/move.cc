/*
 * move.cc
 *
 *  Created on: Aug 18, 2011
 *      Author: max
 */

#include "first.hh"

#include "move.hh"
#include "io/io.hh"        // for fm_io
#include "io/io_posix.hh"  // for fm_io_posix

#include <cstring>         // for strcmp()
#include <vector>          // for std::vector


FT_NAMESPACE_BEGIN

enum { FC_ARGS_COUNT = FT_IO_NS fm_io::FC_ARGS_COUNT };

static char const* const* label = FT_IO_NS fm_io::label;


/** default constructor */
fm_move::fm_move()
  : this_io(0), this_eta(), this_work_total(0)
{ }

/** destructor. calls quit() */
fm_move::~fm_move()
{
    quit();
}


/**
 * checks that I/O is open.
 * if success, stores a reference to I/O object.
 */
int fm_move::init(FT_IO_NS fm_io & io)
{
    int err = 0;
    if (this_io->is_open())
        this_io = & io;
    else
        err = ENOTCONN; // I/O is not open !
    return err;
}

/** performs cleanup. called by destructor, you can also call it explicitly after (or instead of) run()  */
void fm_move::quit()
{
    if (this_io != 0) {
        if (this_io->is_open())
            this_io->close();
        this_io = 0;
    }
}

bool fm_move::is_initialized() const
{
    return this_io != NULL && this_io->is_open();
}

/** return EISCONN if remapper is initialized, else call quit_io() and return 0 */
int fm_move::check_is_closed()
{
    int err = 0;
    if (is_initialized()) {
        ff_log(FC_ERROR, 0, "error: I/O subsystem already started");
        /* error is already reported, flip sign */
        err = -EISCONN;
    } else
        // quit() to make sure we are not left in a half-initialized status
        // (this_io != NULL && !this_io->is_open())
        quit();
    return err;
}

/** return 0 if remapper is initialized, else ENOTCONN */
int fm_move::check_is_open()
{
    int err = 0;
    if (!is_initialized()) {
        ff_log(FC_ERROR, 0, "error: I/O subsystem not started");
        // quit() to make sure we are not left in a half-initialized status
        // (this_io != NULL && !this_io->is_open())
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
int fm_move::main(int argc, char ** argv)
{
    fm_move mover;

    if (argc == 2 && !strcmp("--help", argv[1]))
        return mover.usage(argv[0]);


    int err = mover.init(argc, argv);

    if (err == 0)
        err = mover.run();

    /*
     * note 1.2.2) fsmove::main() must check for unreported errors
     * and log them them with message "failed with unreported error"
     */
    if (!ff_log_is_reported(err))
        err = ff_log(FC_ERROR, err, "failed with unreported error");

    return err;
}

/** print command-line usage to stdout and return 0 */
int fm_move::usage(const char * program_name) {
    ff_log(FC_NOTICE, 0, "Usage: %s [OPTION]... %s %s [--exclude FILE...]\n", program_name, label[0], label[1]);
    ff_log(FC_NOTICE, 0, "");
    return ff_log
    (FC_NOTICE, 0, "Supported options:\n"
     "  --help                Print this help and exit\n"
     "  --                    End of options. treat subsequent parameters as arguments\n"
     "                          even if they start with '-'\n"
     "  -q, --quiet           Be quiet, print less output\n"
     "  -qq                   Be very quiet, only print warnings or errors\n"
     "  -v, --verbose         Be verbose, print what is being done\n"
     "  -vv                   Be very verbose, print a lot of detailed output\n"
     "  -vvv                  Be incredibly verbose (warning: prints TONS of output)\n"
     "  -f, --force-run       Run even if some sanity checks fail\n"
     "  -n, --no-action, --simulate-run\n"
     "                        Do not actually write any file or directory\n"
     "  --posix               Use POSIX I/O (default)\n"
     "  -e, --exclude FILE... Skip these files, i.e. do not move them.\n"
     "                        Must be last argument\n");
}

int fm_move::invalid_cmdline(const char * program_name, int err, const char * fmt, ...)
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
int fm_move::init(int argc, char const* const* argv)
{
    fm_args args;
    int err;
    ft_log_level level = FC_INFO, new_level;
    fm_io_kind io_kind;

    do {
        if ((err = check_is_closed()) != 0)
            break;

        if (argc == 0) {
            err = invalid_cmdline("fsmove", 0, "missing arguments: %s %s", label[0], label[1], label[2]);
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
                args.exclude_list = argv;
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
                /* -f force run: degrade failed sanity checks from ERRORS (which stop execution) to WARNINGS (which let execution continue) */
                else if (!strcmp(arg, "-f") || !strcmp(arg, "--force-run")) {
                    args.force_run = true;
                }
                /* -n simulate run: do not read or write device blocks  */
                else if (!strcmp(arg, "-n") || !strcmp(arg, "--no-action") || !strcmp(arg, "--simulate-run")) {
                    args.simulate_run = true;
                }
                /* --posix */
                else if ((io_kind = FC_IO_POSIX,   !strcmp(arg, "--posix")))
                {
                    if (args.io_kind == FC_IO_AUTODETECT)
                        args.io_kind = io_kind;
                    else
                        err = invalid_cmdline(program_name, 0, "option --posix can only be specified once");
                } else {
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

        if (err == 0) {
            /* if autodetect, use POSIX I/O */
            if (args.io_kind == FC_IO_AUTODETECT)
                args.io_kind = FC_IO_POSIX;

            if (args.io_kind == FC_IO_POSIX && io_args_n < FC_ARGS_COUNT) {
                switch (io_args_n) {
                    case 0:
                        err = invalid_cmdline(program_name, 0, "missing arguments: %s %s", label[0], label[1]);
                        break;
                    case 1:
                        err = invalid_cmdline(program_name, 0, "missing argument: %s", label[1]);
                        break;
                }
            }
        }

    } while (0);

    if (err == 0) {
        if (level <= FC_DEBUG) {
            /* note 1.4.1) -v enables FC_FMT_LEVEL_MSG also for stdout/stderr */
            /* note 1.4.2) -vv enables FC_FMT_DATETIME_LEVEL_MSG also for stdout/stderr */
            ft_log_fmt format = level == FC_DEBUG ? FC_FMT_LEVEL_MSG : FC_FMT_DATETIME_LEVEL_MSG;

            ff_log_register_range(stdout, format, level,   FC_NOTICE);
            ff_log_register_range(stderr, format, FC_WARN, FC_FATAL);
        }
        if (level > FC_INFO)
            ff_log_unregister_range(stdout, FC_INFO, (ft_log_level)(level - 1));

        ff_log_set_threshold(level);

        err = init(args);
    }

    return err;
}

/**
 * initialize all subsystems (job, I/O, log...) using specified arguments
 * return 0 if success, else error.
 */
int fm_move::init(const fm_args & args)
{
    FT_IO_NS fm_io * io = NULL;
    int err = 0;

    switch (args.io_kind) {
        case FC_IO_POSIX:
            if ((err = check_is_closed()) != 0)
                break;

            io = new FT_IO_NS fm_io_posix();
            break;
        default:
            ff_log(FC_ERROR, 0, "tried to initialize unknown I/O '%d': not POSIX", (int) args.io_kind);
            err = -ENOSYS;
            break;
    }

    if (io != NULL) {
        if ((err = io->open(args)) == 0)
            this_io = io;
        else
            delete io;
    }
    return err;
}

/**
 * analysis phase of recursive move algorithm,
 * must be executed after init() and before run() or move()
 */
int fm_move::analyze()
{
    // TODO analyze
    return 0;
}


/**
 * main recursive move algorithm.
 * calls in sequence analyze() and move()
 */
int fm_move::run()
{
    int err = 0;
    do {
        if ((err = analyze()))
            break;
        if ((err = move()))
            break;
    } while (0);
    return err;
}

/** core of recursive move algorithm, actually moves the whole source tree into target */
int fm_move::move()
{
    return this_io->move();
}


FT_NAMESPACE_END
