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
#include "io/io_emul.hh"   // for ft_io_emul

FT_NAMESPACE_BEGIN

class ft_transform
{
private:
    char * job_dir_;
    ft_size job_dir__len;

    FILE * job_log;

    FT_IO_NS ft_io * fm_io;

    int invalid_cmdline(const char * program_name, const char * fmt, ...);

    /** return EISCONN if transformer is initialized, else call quit_io() and return 0 */
    int check_is_closed();

    /** return 0 if transformer is initialized, else call quit_io() and return ENOTCONN */
    int check_is_open();

    /** initialize logging subsystem */
    int init_log();

    /** initialize persistence subsystem */
    int init_job();

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


    FT_INLINE bool is_initialized() const { return fm_io != NULL && fm_io->is_open(); }

    /**
     * autodetect from command line which I/O to use and initialize it.
     * return 0 if success, else error.
     */
    int init(int argc, char const* const* argv);

    /**
     * initialize transformer to use specified I/O. if success, stores a pointer to I/O object
     * WARNING: destructor and quit_io() will delete ft_io object,
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
    int init_io_emul(char const* const path[FT_IO_NS ft_io_emul::FC_FILE_COUNT]);

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
