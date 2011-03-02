/*
 * main.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_TRANSFORM_HH
#define FSTRANSFORM_TRANSFORM_HH

#include "io/io.hh"        // for ft_io
#include "io/io_posix.hh"  // for ft_io_posix

FT_NAMESPACE_BEGIN

class ft_transform
{
private:
    FT_IO_NS ft_io * fm_io;

    int invalid_cmdline(const char * program_name, const char * fmt, ...);

    /** return EISCONN if transformer is initialized, else call quit() and return 0 */
    int check_is_closed();

    /** return 0 if transformer is initialized, else call quit() and return ENOTCONN */
    int check_is_open();

public:

    /** constructor */
    ft_transform();

    /** destructor. calls quit() */
    ~ft_transform();

    /**
     * high-level main method.
     * calls in sequence: init(argc, argv), run(), quit()
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
     * initialize transformer to use specified I/O. stores a pointer to I/O object
     * WARNING: destructor and quit() will delete ft_io object,
     *          so only pass I/O object created with new() and do not try to delete them yourself
     *
     * return 0 if success, else error.
     */
    int init(FT_IO_NS ft_io * io);

    /**
     * initialize transformer to use POSIX I/O.
     * requires three arguments: DEVICE, LOOP-FILE and ZERO-FILE to be passed in path[].
     * return 0 if success, else error.
     */
    int init_posix(char const* const path[FT_IO_NS ft_io_posix::FC_FILE_COUNT]);

    /**
     * perform actual work using configured I/O
     * return 0 if success, else error.
     */
    int run();

    /** shutdown transformer. closes configured I/O and deletes it */
    void quit();
};


FT_NAMESPACE_END

#endif /* FSTRANSFORM_TRANSFORM_HH */
