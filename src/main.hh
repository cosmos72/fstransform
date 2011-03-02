/*
 * main.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_MAIN_HH
#define FSTRANSFORM_MAIN_HH

#include "ctx.hh"

class ft_main : private ft_ctx
{
private:
    typedef ft_ctx super_type;

    const char * program_name;

    int invalid_cmdline(const char * fmt, ...);

    /* cannot call copy constructor */
    ft_main(const ft_main &);

    /* cannot call assignment operator */
    const ft_main & operator=(const ft_main &);

public:
    /** constructor */
    ft_main(const char * program_name);

    /** destructor */
    ~ft_main();

    /**
     * high-level main method.
     * calls in sequence open(argv[1], {argv[2], argv[3]}), run(), close().
     * if invoked with the only argument "--help", calls usage() and immediately returns 0
     */
    int main(int argc, char ** argv);

    /** print command-line usage to stdout and return 0 */
    int usage();

    /** return true if this ft_main is currently (and correctly) open */
    super_type::is_open;

    /**
     * open file descriptors (storing them in super-class ft_ctx) and perform consistency checks.
     * return 0 on success, 1 on error (prints by itself error message to stderr)
     */
    int open(const char * dev_path, const char * file_path[FC_FILE_COUNT]);

    /** close file descriptors */
    super_type::close;

    /**
     * perform actual work.
     * invoke ff_work_dispatch(* this).
     * return 0 if success, else error.
     */
    int run();
};


#endif /* FSTRANSFORM_MAIN_HH */
