/*
 * remap.cc
 *
 *  Created on: Feb 14, 2011
 *      Author: max
 */

#include "first.hh"

#include <cerrno>           // for errno
#include <cstdlib>          // for malloc()
#include <cstdio>           // for fprintf(), sprintf(), stdout, stderr
#include <cstring>          // for strcmp()

#include "log.hh"           // for ff_log()
#include "map.hh"           // for fr_map<T>
#include "vector.hh"        // for fr_vector<T>
#include "dispatch.hh"      // for fr_dispatch
#include "remap.hh"     // for fr_remap
#include "util.hh"          // for ff_strtoul()

#include "io/io.hh"         // for fr_io
#include "io/io_posix.hh"   // for fr_io_posix
#include "io/io_self_test.hh" // for fr_io_self_test
#include "io/util.hh"       // for ff_mkdir()

FT_NAMESPACE_BEGIN


enum { FC_DEVICE = FT_IO_NS fr_io_posix::FC_DEVICE, FC_FILE_COUNT = FT_IO_NS fr_io_posix::FC_FILE_COUNT };

static char const* const* label = FT_IO_NS fr_io::label;





/** constructor */
fr_remap::fr_remap()
    : this_job(NULL), this_io(NULL), this_ui(NULL)
{ }

/** destructor. calls quit_io() */
fr_remap::~fr_remap()
{
    quit_io();
    if (this_job != NULL) {
        delete this_job;
        this_job = NULL;
    }
    if (this_ui != NULL) {
        delete this_ui;
        this_ui = NULL;
    }
}

/**
 * high-level main method.
 * calls in sequence: init(argc, argv), run() and quit_io()
 *
 * expects argc == 4 and four arguments in argv:
 * program_name, DEVICE, LOOP-FILE and ZERO-FILE.
 *
 * return 0 if success, else error.
 * if invoked with the only argument "--help", calls usage() and immediately returns 0
 */
int fr_remap::main(int argc, char const* const* argv)
{
    fr_remap remapper;

    if (argc == 2 && !strcmp("--help", argv[1]))
        return remapper.usage(argv[0]);

    int err = remapper.init(argc, argv);

    if (err == 0)
        err = remapper.run();
    /*
     * note 1.2.2) fsremap::main() must check for unreported errors
     * and log them them with message "failed with unreported error"
     */
    if (!ff_log_is_reported(err))
        err = ff_log(FC_ERROR, err, "failed with unreported error");

    // not needed, destructor will call quit_io()
    // remapper.quit_io();

    return err;
}


/** print command-line usage to stdout and return 0 */
int fr_remap::usage(const char * program_name) {
    ff_log(FC_NOTICE, 0, "Usage: %s [OPTION]... %s %s %s\n", program_name, label[0], label[1], label[2]);
    ff_log(FC_NOTICE, 0, "");
    return ff_log
    (FC_NOTICE, 0, "Supported options:\n"
     "  --help               Print this help and exit\n"
     "  --                   End of options. treat subsequent parameters as arguments\n"
     "                         even if they start with '-'\n"
     "  -q, --quiet          Be quiet, print less output\n"
     "  -qq                  Be very quiet, only print warnings or errors\n"
     "  -v, --verbose        Be verbose, print what is being done\n"
     "  -vv                  Be very verbose, print a lot of detailed output\n"
     "  -vvv                 Be incredibly verbose (warning: prints TONS of output)\n"
     "  --progress-tty TTY   Show full-text progress in tty device TTY\n"
     "  -f, --force-run      Run even if some sanity checks fail\n"
     "  -n, --no-action, --simulate-run\n"
     "                       Do not actually read or write any disk block\n"
     "  -t, --dir DIR        Write storage and logs inside DIR (default: $HOME)\n"
     "  -j, --job JOB_ID     Set JOB_ID to use (default: autodetect)\n"
     "  --umount-cmd CMD     Command and args to unmount %s (default: /bin/umount %s)\n"
     "  -m, --mem-buffer RAM_SIZE[k|M|G|T|P|E|Z|Y]\n"
     "                       Set RAM buffer size (default: autodetect)\n"
     "  -s, --secondary-storage SECONDARY_SIZE[k|M|G|T|P|E|Z|Y]\n"
     "                       Set SECONDARY STORAGE file length (default: autodetect)\n"
     "  -xp, --exact-primary-storage PRIMARY_SIZE[k|M|G|T|P|E|Z|Y]\n"
     "                       Set *exact* PRIMARY STORAGE length, or fail\n"
     "                       (default: autodetect)\n"
     "  -xs, --exact-secondary-storage SECONDARY_SIZE[k|M|G|T|P|E|Z|Y]\n"
     "                       Set *exact* SECONDARY STORAGE length, or fail\n"
     "                       (default: autodetect)\n"
     "  --clear-all          Clear all free blocks after remapping (default)\n"
     "  --clear-minimal      DANGEROUS! Clear only overwritten free blocks\n"
     "                         after remapping\n"
     "  --clear-none         DANGEROUS! Do not clear any free blocks after remapping\n"
     "  --posix              Use POSIX I/O (default)\n"
     "  --test               Use test I/O. Arguments are:\n"
     "                         DEVICE-LENGTH LOOP-FILE-EXTENTS FREE-SPACE-EXTENTS\n"
     "  --self-test          Use self-test I/O. Performs self-test with random data\n",
     label[FC_DEVICE]);
}


int fr_remap::invalid_cmdline(const char * program_name, int err, const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    err = ff_vlog(FC_ERROR, err, fmt, args);
    va_end(args);

    ff_log(FC_NOTICE, 0, "Try `%s --help' for more information", program_name);
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
    ft_log_level level = FC_INFO, new_level;
    fr_io_kind io_kind;
    fr_clear_free_space new_clear;

    do {
        if ((err = check_is_closed()) != 0)
            break;

        if (argc == 0) {
            err = invalid_cmdline("fsremap", 0, "missing arguments: %s %s %s", label[0], label[1], label[2]);
            break;
        }

        const char * arg, * program_name = argv[0];
        ft_size io_args_n = 0;
        bool allow_opts = true;

        // skip program_name
        while (err == 0 && --argc) {
            arg = * ++argv;
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
                /* --progress-tty */
                else if (argc > 1 && (!strcmp(arg, "--progress-tty"))) {
                    args.ui_kind = FC_UI_TTY;
                    --argc, args.ui_arg = *++argv;
                }
                /* -t directory */
                else if (argc > 1 && (!strcmp(arg, "-t") || !strcmp(arg, "--dir"))) {
                    --argc, args.root_dir = *++argv;
                }
                /* --umount-cmd */
                else if (argc > 1 && (!strcmp(arg, "--umount-cmd"))) {
                    --argc, args.umount_cmd = *++argv;
                }
                /* -j job_id */
                else if (argc > 1 && (!strcmp(arg, "-j") || !strcmp(arg, "--job"))) {
                    if ((err = ff_str2un(argv[1], & args.job_id)) != 0) {
                        err = invalid_cmdline(program_name, err, "invalid job id '%s'", argv[1]);
                        break;
                    }
                    --argc, ++argv;
                }
                /* -m mem-buffer-size[k|M|G|T|P|E|Z|Y] */
                else if (argc > 1 && (!strcmp(arg, "-m") || !strcmp(arg, "--mem-buffer"))) {
                    if ((err = ff_str2un_scaled(argv[1], & args.storage_size[FC_MEM_BUFFER_SIZE])) != 0) {
                        err = invalid_cmdline(program_name, err, "invalid memory buffer size '%s'", argv[1]);
                        break;
                    }
                    --argc, ++argv;
                }
                /* -s secondary-storage-size[k|M|G|T|P|E|Z|Y] */
                else if (argc > 1 && (!strcmp(arg, "-s") || !strcmp(arg, "--secondary-storage"))) {
                    if ((err = ff_str2un_scaled(argv[1], & args.storage_size[FC_SECONDARY_STORAGE_SIZE])) != 0) {
                        err = invalid_cmdline(program_name, err, "invalid secondary storage size '%s'", argv[1]);
                        break;
                    }
                    --argc, ++argv;
                }
                /* -xp exact-primary-storage-size[k|M|G|T|P|E|Z|Y] */
                else if (argc > 1 && (!strcmp(arg, "-xp") || !strcmp(arg, "--exact-primary-storage"))) {
                    if ((err = ff_str2un_scaled(argv[1], & args.storage_size[FC_PRIMARY_STORAGE_EXACT_SIZE])) != 0) {
                        err = invalid_cmdline(program_name, err, "invalid primary storage exact size '%s'", argv[1]);
                        break;
                    }
                    --argc, ++argv;
                }
                /* -xs exact-secondary-storage-size[k|M|G|T|P|E|Z|Y] */
                else if (argc > 1 && (!strcmp(arg, "-xs") || !strcmp(arg, "--exact-secondary-storage"))) {
                    if ((err = ff_str2un_scaled(argv[1], & args.storage_size[FC_SECONDARY_STORAGE_EXACT_SIZE])) != 0) {
                        err = invalid_cmdline(program_name, err, "invalid secondary storage exact size '%s'", argv[1]);
                        break;
                    }
                    --argc, ++argv;
                }
                /* --clear-all, --clear-minimal, --clear-none */
                else if ((new_clear = FC_CLEAR_ALL,   !strcmp(arg, "--clear-all"))
                    || (new_clear = FC_CLEAR_MINIMAL, !strcmp(arg, "--clear-minimal"))
                    || (new_clear = FC_CLEAR_NONE,    !strcmp(arg, "--clear-none"))) {

                    if (args.job_clear == FC_CLEAR_AUTODETECT)
                        args.job_clear = new_clear;
                    else
                        err = invalid_cmdline(program_name, 0, "options --clear-all, --clear-minimal and --clear-none are mutually exclusive");
                }
                /* --posix, --test, --self-test */
                else if ((io_kind = FC_IO_POSIX,   !strcmp(arg, "--posix"))
                    || (io_kind = FC_IO_TEST,      !strcmp(arg, "--test"))
                    || (io_kind = FC_IO_SELF_TEST, !strcmp(arg, "--self-test")))
                {
                    if (args.io_kind == FC_IO_AUTODETECT)
                        args.io_kind = io_kind;
                    else
                        err = invalid_cmdline(program_name, 0, "options --posix, --test and --self-test are mutually exclusive");
                } else {
                    err = invalid_cmdline(program_name, 0, "unknown option: '%s'", arg);
                    break;
                }
                continue;
            }
            /** found an argument */
            if (io_args_n < FC_FILE_COUNT)
                args.io_args[io_args_n++] = arg;
            else
                err = invalid_cmdline(program_name, 0, "too many arguments");
        }

        if (err == 0) {
            /* if autodetect, clear all free blocks */
            if (args.job_clear == FC_CLEAR_AUTODETECT)
                args.job_clear = FC_CLEAR_ALL;

            /* if autodetect, use POSIX I/O */
            if (args.io_kind == FC_IO_AUTODETECT)
                args.io_kind = FC_IO_POSIX;

            if ((args.io_kind == FC_IO_POSIX || args.io_kind == FC_IO_TEST) && io_args_n < FC_FILE_COUNT) {
                switch (io_args_n) {
                    case 0:
                        err = invalid_cmdline(program_name, 0, "missing arguments: %s %s %s", label[0], label[1], label[2]);
                        break;
                    case 1:
                        err = invalid_cmdline(program_name, 0, "missing arguments: %s %s", label[1], label[2]);
                        break;
                    case 2:
                        err = invalid_cmdline(program_name, 0, "missing argument: %s", label[2]);
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

        /* always enable at least DEBUG level, to let fsremap.log collect all messages from DEBUG to FATAL */
        ff_log_set_threshold(level < FC_DEBUG ? level : FC_DEBUG);
        
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
        if ((err = init_job(args)) != 0)
            break;
        if ((err = init_ui(args)) != 0)
            break;
        if ((err = init_io(args)) != 0)
            break;

    } while (0);

    return err;
}


/** initialize job/persistence subsystem */
int fr_remap::init_job(const fr_args & args)
{
    if (this_job != NULL)
        return 0;

    fr_job * job = new fr_job();
    int err = job->init(args);
    if (err == 0)
        this_job = job;
    else
        delete job;
    return err;
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
    FT_UI_NS fr_ui_tty * ui_tty = NULL;
    int err;

    do {
        ui_tty = new FT_UI_NS fr_ui_tty();

        if ((err = ui_tty->init(arg)) != 0)
            break;

        this_ui = ui_tty;
    } while (0);

    if (err != 0 && ui_tty != NULL)
        delete ui_tty;

    return err;
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
        case FC_IO_POSIX:
            err = init_io_posix(args);
            break;
        case FC_IO_TEST:
            err = init_io_test(args);
            break;
        case FC_IO_SELF_TEST:
            err = init_io_self_test(args);
            break;
        default:
            ff_log(FC_ERROR, 0, "tried to initialize unknown I/O '%d': not POSIX, not self-test", (int) args.io_kind);
            err = -ENOSYS;
            break;
    }
    return err;
}


/**
 * initialize remapper to use POSIX I/O.
 * POSIX I/O requires three arguments in args.io_args: DEVICE, LOOP-FILE and ZERO-FILE.
 * return 0 if success, else error.
 */
int fr_remap::init_io_posix(const fr_args & args)
{
    int err;
    if ((err = pre_init_io()) == 0) {

        FT_IO_NS fr_io_posix * io_posix = new FT_IO_NS fr_io_posix(* this_job);

        if ((err = io_posix->open(args)) == 0)
            post_init_io(io_posix);
        else
            delete io_posix;
    }
    return err;
}

/**
 * initialize remapper to use test I/O.
 * test I/O requires three arguments in args.io_args: DEVICE-LENGTH, LOOP-FILE-EXTENTS and ZERO-FILE-EXTENTS.
 * return 0 if success, else error.
 */
int fr_remap::init_io_test(const fr_args & args)
{
    int err;
    if ((err = pre_init_io()) == 0) {

        FT_IO_NS fr_io_test * io_test = new FT_IO_NS fr_io_test(* this_job);

        if ((err = io_test->open(args)) == 0)
            post_init_io(io_test);
        else
            delete io_test;
    }
    return err;
}

/**
 * initialize remapper to use self-test I/O.
 * return 0 if success, else error.
 */
int fr_remap::init_io_self_test(const fr_args & args)
{
    int err;
    if ((err = pre_init_io()) == 0) {

        FT_IO_NS fr_io_self_test * io_self_test = new FT_IO_NS fr_io_self_test(* this_job);

        if ((err = io_self_test->open(args)) == 0)
            post_init_io(io_self_test);
        else
            delete io_self_test;
    }
    return err;
}

int fr_remap::pre_init_io()
{
    int err = check_is_closed();
    if (err == 0 && this_job == NULL) {
        ff_log(FC_ERROR, 0, "error: cannot start I/O subsystem, job must be initialized first");
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
    if (this_io != NULL) {
        delete this_io;
        this_io = NULL;
    }
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

        /* ask actual I/O subsystem to read LOOP-FILE and FREE-SPACE extents */
        if ((err = io.read_extents(loop_file_extents, free_space_extents)) != 0)
            break;

        /* persistence: save LOOP-FILE and FREE-SPACE extents to disk */
        if ((err = io.save_extents(loop_file_extents, free_space_extents)) != 0)
            break;

        io.close_extents();

        /* invoke fr_dispatch::main() to choose which fr_work<T> to instantiate, and run it */
        err = fr_dispatch::main(loop_file_extents, free_space_extents, io);

    } while (0);

    return err;
}



FT_NAMESPACE_END

