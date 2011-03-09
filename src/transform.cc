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
#include "work_dispatch.hh" // for ft_work_dispatch
#include "transform.hh"     // for ft_transform

#include "io/io.hh"         // for ft_io
#include "io/io_posix.hh"   // for ft_io_posix

#include "io/util.hh"       // for ff_mkdir()

FT_NAMESPACE_BEGIN


enum { FC_FILE_COUNT = FT_IO_NS ft_io_posix::FC_FILE_COUNT };

static char const* const* label = FT_IO_NS ft_io_posix::label;





/** constructor */
ft_transform::ft_transform()
    : job_dir_(NULL), job_log(NULL), fm_io(NULL)
{
    ff_log_init();
    ff_log_set_threshold(FC_TRACE);
}

/** destructor. calls quit_io() */
ft_transform::~ft_transform()
{
    quit_io();
    if (job_log != NULL) {
        ff_log_unregister(job_log);
        fclose(job_log);
        job_log = NULL;
    }
    if (job_dir_ != NULL)
        free(job_dir_);
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

    // not needed, destructor will call quit_io()
    // transformer.quit_io();

    return err;
}


/** print command-line usage to stdout and return 0 */
int ft_transform::usage(const char * program_name) {
    return ff_log(FC_NOTICE, 0, "Usage: %s %s %s %s\n", program_name, label[0], label[1], label[2]);
}


int ft_transform::invalid_cmdline(const char * program_name, const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    ff_vlog(FC_FATAL, 0, fmt, args);
    va_end(args);

    ff_log(FC_FATAL, 0, "Try `%s --help' for more information", program_name);
    return 1;
}



/** return EISCONN if transformer is initialized, else call quit_io() and return 0 */
int ft_transform::check_is_closed()
{
    int err = 0;
    if (is_initialized()) {
        ff_log(FC_ERROR, 0, "error: I/O subsystem already initialized");
        err = EISCONN;
    } else
        // quit_io() to make sure we are not left in a half-initialized status
        // (fm_io != NULL && !fm_io->is_open())
        quit_io();
    return err;
}

/** return 0 if transformer is initialized, else ENOTCONN */
int ft_transform::check_is_open()
{
    int err = 0;
    if (!is_initialized()) {
        ff_log(FC_ERROR, 0, "error: I/O subsystem not initialized");
        // quit_io() to make sure we are not left in a half-initialized status
        // (fm_io != NULL && !fm_io->is_open())
        quit_io();
        err = ENOTCONN;
    }
    return err;
}

/**
 * autodetect from command line which I/O to use and initialize it.
 * return 0 if success, else error.
 */
int ft_transform::init(int argc, char const* const* argv)
{
    int err;

    if ((err = check_is_closed()))
        ;
    else if (argc <= 1)
        err = invalid_cmdline(argv[0], "missing arguments: %s %s %s", label[0], label[1], label[2]);
    else if (argc <= 2)
        err = invalid_cmdline(argv[0], "missing arguments: %s %s", label[1], label[2]);
    else if (argc <= 3)
        err = invalid_cmdline(argv[0], "missing argument: %s", label[2]);

    if (err == 0)
        err = init_job();
    if (err == 0)
        err = init_io_posix(& argv[1]);

    return err;
}

int ft_transform::init_log()
{
    const char * log_name = "fstransform.log";
    ft_size log_name_len = strlen(log_name) + 1; // also count final '\0'

    char * log_file = (char *) malloc(job_dir__len + log_name_len);

    if (log_file == NULL)
        return ff_log(FC_ERROR, ENOMEM, "failed to allocate %lu bytes for log_file", (unsigned long)(job_dir__len + log_name_len));

    memcpy(log_file, job_dir_, job_dir__len);
    memcpy(log_file + job_dir__len, log_name, log_name_len);
    if ((job_log = fopen(log_file, "a")) == NULL)
        return ff_log(FC_ERROR, errno, "failed to open log file '%s'", log_file);

    ff_log_register(job_log);
    return 0;
}

/** initialize persistence subsystem */
int ft_transform::init_job()
{
    (void) FT_IO_NS ff_mkdir(".fstransform");

    ft_uint i;
    int err = 0;

    // 3*sizeof(ft_uint)+1 chars are enough to safely print (ft_uint)
    job_dir_ = (char *) malloc(20 + 3 * sizeof(ft_uint));
    if (job_dir_ == NULL)
        return ENOMEM;

    memcpy(job_dir_, ".fstransform/job.", 17);

    for (i = 1; i != (ft_uint)-1; i++) {
        // finish job_dir_ with '/' - needed by everybody using job_dir_
        sprintf(job_dir_ + 17, "%lu/", (unsigned long)i);
        job_dir__len = strlen(job_dir_);

        if ((err = FT_IO_NS ff_mkdir(job_dir_)) == 0
                && (err = init_log()) == 0)
        {
            ff_log(FC_NOTICE, 0, "started job %lu", (unsigned long)i);
            break;
        }
    }
    if (err != 0) {
        free(job_dir_);
        job_dir_ = NULL;
    }

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

        io_posix = new FT_IO_NS ft_io_posix();

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


/**
 * initialize transformer to use specified I/O. if success, stores a pointer to I/O object
 * WARNING: destructor and quit_io() will delete ft_io object,
 *          so only pass I/O object created with new()
 *          and delete them yourself ONLY if this call returned error!
 *
 * return 0 if success, else error.
 */
int ft_transform::init_io(FT_IO_NS ft_io * io) {
    int err;
    if ((err = check_is_closed()) == 0)
        fm_io = io;
    return err;
}

/** shutdown transformer. closes configured I/O and deletes it */
void ft_transform::quit_io()
{
    if (fm_io != NULL) {
        delete fm_io;
        fm_io = NULL;
    }
}

/**
 * perform actual work using configured I/O:
 * allocates ft_vector<ft_uoff> for both LOOP-FILE and FREE-SPACE extents,
 * calls fm_io->read_extents() to fill them, and finally invokes
 * ft_work_dispatch::main(loop_file_extents, free_space_extents, fm_io)
 *
 * return 0 if success, else error.
 */
int ft_transform::run()
{
    int err = 0;
    do {
        if ((err = check_is_open()) != 0)
            break;

        /* allocate ft_vector<ft_uoff> for both LOOP-FILE and FREE-SPACE extents */
        ft_vector<ft_uoff> loop_file_extents, free_space_extents;
        FT_IO_NS ft_io & io = * fm_io;

        /* ask actual I/O subsystem to read LOOP-FILE and FREE-SPACE extents */
        if ((err = io.read_extents(loop_file_extents, free_space_extents)) != 0)
            break;

        /* persistence: save LOOP-FILE and FREE-SPACE extents to disk */
        if ((err = io.write_extents(loop_file_extents, free_space_extents, job_dir_)) != 0)
            break;

        /* invoke ft_work_dispatch::main() to choose which ft_work<T> to instantiate, and run it */
        err = ft_work_dispatch::main(loop_file_extents, free_space_extents, io);

    } while (0);

    return err;
}



FT_NAMESPACE_END

