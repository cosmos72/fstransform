/*
 * main.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_TRANSFORM_HH
#define FSTRANSFORM_TRANSFORM_HH

#include "check.hh"

#include <cstdio>          // for FILE *

#include "args.hh"         // for ft_args
#include "io/io.hh"        // for ft_io
#include "io/io_posix.hh"  // for ft_io_posix
#include "io/io_test.hh"   // for ft_io_test
#include "ui/ui.hh"        // for ft_ui
#include "ui/ui_tty.hh"    // for ft_ui_tty

FT_NAMESPACE_BEGIN

class ft_transform
{
private:
    ft_job * this_job;
    FT_IO_NS ft_io * this_io;
    FT_UI_NS ft_ui * this_ui;

    static int invalid_cmdline(const char * program_name, int err, const char * fmt, ...);

    /** return EISCONN if transformer is initialized, else call quit_io() and return 0 */
    int check_is_closed();

    /** return 0 if transformer is initialized, else call quit_io() and return ENOTCONN */
    int check_is_open();

    /** initialize job/persistence subsystem */
    int init_job(const ft_args & argsd);


    /** initialize UI subsystem */
    int init_ui(const ft_args & args);
    /** initialize tty UI subsystem */
    int init_ui_tty(const char * arg);


    int pre_init_io();
    void post_init_io(FT_IO_NS ft_io * io);

public:

    /** constructor */
    ft_transform();

    /** destructor. calls quit_io() */
    ~ft_transform();

    /**
     * high-level main method.
     * calls in sequence: init(argc, argv), run(), quit_io()
     *
     * expects argc == 4 and four arguments in argv:
     * program_name, DEVICE, LOOP-FILE and ZERO-FILE.
     *
     * return 0 if success, else error.
     * if invoked with the only argument "--help", calls usage() and immediately returns 0
     */
    static int main(int argc, char const* const* argv);

    /** print command-line usage to stdout and return 0 */
    int usage(const char * program_name);


    FT_INLINE bool is_initialized() const { return this_io != NULL && this_io->is_open(); }

    /**
     * parse from command line and initialize all subsystems (job, I/O, log...)
     * return 0 if success, else error.
     *
     * implementation: parse command line, fill a ft_args and call init(const ft_args &)
     */
    int init(int argc, char const* const* argv);

    /**
     * initialize all subsystems (job, I/O, log...) using specified arguments
     * return 0 if success, else error.
     */
    int init(const ft_args & args);

    /**
     * allocate, open and use I/O specified in args.
     * if success, stores a pointer to I/O object
     * destructor and quit_io() will delete ft_io object.
     *
     * return 0 if success, else error.
     */
    int init_io(const ft_args & args);

    /**
     * initialize transformer to use POSIX I/O.
     * requires three arguments: DEVICE, LOOP-FILE and ZERO-FILE to be passed in path[].
     * return 0 if success, else error.
     */
    int init_io_posix(char const* const path[FT_IO_NS ft_io_posix::FC_FILE_COUNT]);

    /**
     * initialize transformer to use test I/O.
     * requires three arguments: DEVICE-LENGTH, LOOP-FILE-EXTENTS and ZERO-FILE-EXTENTS to be passed in arg[].
     * return 0 if success, else error.
     */
    int init_io_test(char const* const arg[FT_IO_NS ft_io_posix::FC_FILE_COUNT]);

    /**
     * initialize transformer to use self-test I/O.
     * return 0 if success, else error.
     */
    int init_io_self_test();

    /**
     * perform actual work using configured I/O
     * return 0 if success, else error.
     */
    int run();

    /** close configured I/O and delete it */
    void quit_io();
};


FT_NAMESPACE_END

#endif /* FSTRANSFORM_TRANSFORM_HH */
