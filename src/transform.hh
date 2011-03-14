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

#include "io/io.hh"        // for ft_io
#include "io/io_posix.hh"  // for ft_io_posix
#include "io/io_null.hh"   // for ft_io_null

FT_NAMESPACE_BEGIN

class ft_transform
{
private:
    ft_job * fm_job;
    FT_IO_NS ft_io * fm_io;

    static int invalid_cmdline(const char * program_name, int err, const char * fmt, ...);

    static int invalid_verbosity(const char * program_name);

    /** return EISCONN if transformer is initialized, else call quit_io() and return 0 */
    int check_is_closed();

    /** return 0 if transformer is initialized, else call quit_io() and return ENOTCONN */
    int check_is_open();

    /** initialize job/persistence subsystem */
    int init_job(const char * root_dir, ft_uint job_id, ft_size storage_size);

public:

    struct ft_args
    {
        const char * root_dir;   // if NULL, will autodetect
        const char * io_name;    // if NULL, will autodetect
        const char * io_args[3]; // some I/O will need less than 3 arguments
        ft_size storage_size;    // if 0, will autodetect
        ft_uint job_id;          // if 0, will autodetect
    };

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


    FT_INLINE bool is_initialized() const { return fm_io != NULL && fm_io->is_open(); }

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
     * initialize transformer to use specified I/O. if success, stores a pointer to I/O object
     * destructor and quit_io() will delete ft_io object,
     *          so only pass I/O object created with new()
     *          and delete them yourself ONLY if this call returned error!
     *
     * return 0 if success, else error.
     */
    int init_io(FT_IO_NS ft_io * io);

    /**
     * initialize transformer to use POSIX I/O.
     * requires three arguments: DEVICE, LOOP-FILE and ZERO-FILE to be passed in path[].
     * return 0 if success, else error.
     */
    int init_io_posix(char const* const path[FT_IO_NS ft_io_posix::FC_FILE_COUNT]);

    /**
     * initialize transformer to use I/O emulation. requires two arguments to be passed in path[]:
     * file path containing LOOP-FILE extents and file path containing FREE-SPACE extents.
     * return 0 if success, else error.
     */
    int init_io_emul(char const* const path[FT_IO_NS ft_io_null::FC_FILE_COUNT]);

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
