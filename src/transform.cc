/*
 * main.cc
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
#include "map.hh"           // for ft_map<T>
#include "vector.hh"        // for ft_vector<T>
#include "dispatch.hh"      // for ft_dispatch
#include "transform.hh"     // for ft_transform
#include "util.hh"          // for ff_strtoul()

#include "io/io.hh"         // for ft_io
#include "io/io_posix.hh"   // for ft_io_posix
#include "io/util.hh"       // for ff_mkdir()

FT_NAMESPACE_BEGIN


enum { FC_DEVICE = FT_IO_NS ft_io_posix::FC_DEVICE, FC_FILE_COUNT = FT_IO_NS ft_io_posix::FC_FILE_COUNT };

static char const* const* label = FT_IO_NS ft_io_posix::label;





/** constructor */
ft_transform::ft_transform()
    : this_job(NULL), this_io(NULL)
{ }

/** destructor. calls quit_io() */
ft_transform::~ft_transform()
{
    quit_io();
    if (this_job != NULL) {
        delete this_job;
        this_job = NULL;
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
int ft_transform::main(int argc, char const* const* argv)
{
    ft_transform transformer;

    if (argc == 2 && !strcmp("--help", argv[1]))
        return transformer.usage(argv[0]);

    int err = transformer.init(argc, argv);

    if (err == 0)
        err = transformer.run();
    /*
     * note 1.2.2) fstransform::main() must check for unreported errors
     * and log them them with message "failed with unreported error"
     */
    if (!ff_log_is_reported(err))
        err = ff_log(FC_ERROR, err, "failed with unreported error");

    // not needed, destructor will call quit_io()
    // transformer.quit_io();

    return err;
}


/** print command-line usage to stdout and return 0 */
int ft_transform::usage(const char * program_name) {
    return ff_log(FC_NOTICE, 0, "Usage: %s %s %s %s\n", program_name, label[0], label[1], label[2]);
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

int ft_transform::invalid_verbosity(const char * program_name)
{
    return invalid_cmdline(program_name, 0, "options -q, -qq, -v, -vv, --quiet, --verbose are mutually exclusive");
}

/** return EISCONN if transformer is initialized, else call quit_io() and return 0 */
int ft_transform::check_is_closed()
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

/** return 0 if transformer is initialized, else ENOTCONN */
int ft_transform::check_is_open()
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
 * implementation: parse command line, fill a ft_args and call init(const ft_args &)
 */
int ft_transform::init(int argc, char const* const* argv)
{
    int err;

    ft_args args = {
            NULL, /* root_dir */
            NULL, /* io_name */
            { NULL, NULL, NULL }, /* io_args[3] */
            0,    /* secondary_storage_size */
            0,    /* job_id */
    };

    ft_log_level level = FC_INFO;

    do {
        if ((err = check_is_closed()) != 0)
            break;

        if (argc == 0) {
            err = invalid_cmdline("fstransform", 0, "missing arguments: %s %s %s", label[0], label[1], label[2]);
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
                else if (!strcmp(arg, "-q") || !strcmp(arg, "--quiet")) {
                    if (level == FC_INFO)
                        level = FC_NOTICE;
                    else {
                        err = invalid_verbosity(program_name);
                        break;
                    }
                }
                /* -qq decrease verbosity by two */
                else if (!strcmp(arg, "-qq")) {
                    if (level == FC_INFO)
                        level = FC_WARN;
                    else {
                        err = invalid_verbosity(program_name);
                        break;
                    }
                }
                /* -v, --verbose increase verbosity by one */
                else if (!strcmp(arg, "-v") || !strcmp(arg, "--verbose")) {
                    if (level == FC_INFO)
                        level = FC_DEBUG;
                    else {
                        err = invalid_verbosity(program_name);
                        break;
                    }
                }
                /* -vv increase verbosity by two */
                else if (!strcmp(arg, "-vv")) {
                    if (level == FC_INFO)
                        level = FC_TRACE;
                    else {
                        err = invalid_verbosity(program_name);
                        break;
                    }
                }
                /* -t directory */
                else if (argc > 1 && (!strcmp(arg, "-t") || !strcmp(arg, "--dir")))
                    --argc, args.root_dir = *++argv;

                /* -m storage_size[k|M|G|T|P|E|Z|Y] */
                else if (argc > 1 && (!strcmp(arg, "-m") || !strcmp(arg, "--mmap-storage"))) {
                    if ((err = ff_str2un_scaled(argv[1], & args.storage_size)) != 0) {
                        err = invalid_cmdline(program_name, err, "invalid storage size '%s'", argv[1]);
                        break;
                    }
                    --argc, ++argv;
                }
                /* -j job_id */
                else if (argc > 1 && (!strcmp(arg, "-j") || !strcmp(arg, "--job"))) {
                    if ((err = ff_str2un(argv[1], & args.job_id)) != 0) {
                        err = invalid_cmdline(program_name, err, "invalid job id '%s'", argv[1]);
                        break;
                    }
                    --argc, ++argv;
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

        if (err == 0 && io_args_n < FC_FILE_COUNT) {
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

    } while (0);

    if (err == 0) {
        if (level < ff_log_get_threshold())
            ff_log_set_threshold(level);

        if (level <= FC_DEBUG) {
            /* note 1.4.1) -v enables FC_FMT_LEVEL_MSG also for stdout/stderr */
            /* note 1.4.2) -vv enables FC_FMT_DATETIME_LEVEL_MSG also for stdout/stderr */
            ft_log_fmt format = level == FC_DEBUG ? FC_FMT_LEVEL_MSG : FC_FMT_DATETIME_LEVEL_MSG;

            ff_log_register(stdout, format, level,   FC_NOTICE);
            ff_log_register(stderr, format, FC_WARN, FC_FATAL);
        }
        err = init(args);
    }

    return err;
}


/**
 * initialize all subsystems (job, I/O, log...) using specified arguments
 * return 0 if success, else error.
 */
int ft_transform::init(const ft_args & args)
{
    int err;
    do {
        if ((err = init_job(args.root_dir, args.job_id, args.storage_size)) != 0)
            break;

        if ((err = init_io_posix(args.io_args)) != 0)
            break;
    } while (0);

    return err;
}


/** initialize job/persistence subsystem */
int ft_transform::init_job(const char * root_dir, ft_uint job_id, ft_size storage_size)
{
    if (this_job != NULL)
        return 0;

    ft_job * job = new ft_job();
    int err = job->init(root_dir, job_id, storage_size);
    if (err == 0)
        this_job = job;
    else
        delete job;
    return err;
}

/**
 * initialize transformer to use specified I/O. if success, stores a pointer to I/O object
 * destructor and quit_io() will delete ft_io object,
 *          so only pass I/O object created with new()
 *          and delete them yourself ONLY if this call returned error!
 *
 * return 0 if success, else error.
 */
int ft_transform::init_io(FT_IO_NS ft_io * io) {
    int err;
    if ((err = check_is_closed()) == 0)
        this_io = io;
    return err;
}

/**
 * initialize transformer to use POSIX I/O.
 * requires three arguments: DEVICE, LOOP-FILE and ZERO-FILE to be passed in path[].
 * return 0 if success, else error.
 */
int ft_transform::init_io_posix(char const* const path[FT_IO_NS ft_io_posix::FC_FILE_COUNT])
{
    FT_IO_NS ft_io_posix * io_posix = NULL;
    int err = 0;
    do {
        if ((err = check_is_closed()) != 0)
            break;
        if (this_job == NULL) {
            ff_log(FC_ERROR, 0, "error: cannot start I/O subsystem, job must be initialized first");
            err = -ENOTCONN;
            break;
        }

        io_posix = new FT_IO_NS ft_io_posix(* this_job);

        if ((err = io_posix->open(path)) != 0)
            break;

        err = init_io(io_posix);

    } while (0);

    if (err != 0) {
        if (io_posix != NULL)
            delete io_posix;
    }
    return err;
}



/** shutdown transformer. closes configured I/O and deletes it */
void ft_transform::quit_io()
{
    if (this_io != NULL) {
        delete this_io;
        this_io = NULL;
    }
}

/**
 * perform actual work using configured I/O:
 * allocates ft_vector<ft_uoff> for both LOOP-FILE and FREE-SPACE extents,
 * calls this_io->read_extents() to fill them, and finally invokes
 * ft_dispatch::main(loop_file_extents, free_space_extents, this_io)
 *
 * return 0 if success, else error.
 */
int ft_transform::run()
{
    int err = 0;
    do {
        if ((err = check_is_open()) != 0)
            break;

        FT_IO_NS ft_io & io = * this_io;
        const char * dev_path = ff_if_null(io.dev_path(), "<unknown>");

        ff_log(FC_INFO, 0, "analyzing %s '%s', this may take some minutes ...", label[FC_DEVICE], dev_path);

        /* allocate ft_vector<ft_uoff> for both LOOP-FILE and FREE-SPACE extents */
        ft_vector<ft_uoff> loop_file_extents, free_space_extents;

        /* ask actual I/O subsystem to read LOOP-FILE and FREE-SPACE extents */
        if ((err = io.read_extents(loop_file_extents, free_space_extents)) != 0)
            break;

        /* persistence: save LOOP-FILE and FREE-SPACE extents to disk */
        if ((err = io.write_extents(loop_file_extents, free_space_extents)) != 0)
            break;

        io.close_extents();

        /* invoke ft_dispatch::main() to choose which ft_work<T> to instantiate, and run it */
        err = ft_dispatch::main(loop_file_extents, free_space_extents, io);

    } while (0);

    return err;
}



FT_NAMESPACE_END

