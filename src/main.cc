/*
 * main.cc
 *
 *  Created on: Feb 14, 2011
 *      Author: max
 */

#include "first.hh"

#include <cerrno>         /* for errno                     */
#include <cstdio>         /* for fprintf(), stdout, stderr */
#include <cstring>        /* for strcmp()                  */

#include <sys/types.h>    /* for open()                    */
#include <sys/stat.h>     /*  "    "                       */
#include <fcntl.h>        /*  "    "                       */
#include <unistd.h>       /* for close()                   */

#include "fail.hh"        /* for ff_fail()                               */
#include "map.hh"         /* for ft_map<T>                               */
#include "vector.hh"      /* for ft_vector<T>                            */
#include "file_util.hh"   /* for ff_blkdev(), ff_blkdev_size(), ff_dev() */
#include "work.hh"        /* for ff_work_dispatch()                      */
#include "main.hh"        /* for ft_main                                 */
    


int main(int argc, char ** argv) {
    ft_main main_(argv[0]);
    return main_.main(argc, argv);
}


/** constructor */
ft_main::ft_main(const char * prog_name)
    : super_type(), program_name(prog_name != NULL ? prog_name : "fstransform")
{ }

/** destructor */
ft_main::~ft_main()
{ }

/**
 * high-level main method.
 * calls in sequence open(argv[1], {argv[2], argv[3]}), run(), close().
 * if invoked with the only argument "--help", calls usage() and immediately returns 0
 */
int ft_main::main(int argc, char ** argv)
{
    const char * file_path[FC_FILE_COUNT] = { NULL, NULL }, * dev_path;
    int err;

    if (argc == 2 && !strcmp("--help", argv[1]))
        return usage();
    else if (argc <= 1)
        return invalid_cmdline("missing arguments: %s %s %s", fc_dev_label, fc_file_label[0], fc_file_label[1]);
    else if (argc <= 2)
        return invalid_cmdline("missing arguments: %s %s", fc_file_label[0], fc_file_label[1]);
    else if (argc <= 3)
        return invalid_cmdline("missing argument: %s", fc_file_label[1]);

    dev_path = argv[1];
    file_path[0] = argv[2]; /* loop-file */
    file_path[1] = argv[3]; /* zero-file */

    if ((err = open(dev_path, file_path)) == 0)
        err = run();

    close();
    return err;
}


int ft_main::invalid_cmdline(const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    ff_vfail(0, fmt, args);
    va_end(args);

    fprintf(stderr, "Try `%s --help' for more information\n", program_name);
    return 1;
}


/** print command-line usage to stdout and return 0 */
int ft_main::usage() {
    fprintf(stdout, "Usage: %s %s %s %s\n", program_name, fc_dev_label, fc_file_label[0], fc_file_label[1]);
    return 0;
}


/**
 * open file descriptors (storing them in super-class ft_ctx) and perform consistency checks.
 * return 0 on success, 1 on error (prints by itself error message to stderr).
 */
int ft_main::open(const char * dev_path, const char * file_path[FC_FILE_COUNT])
{
    ft_size i;
	int err = 0, dev_fd = -1;
	ft_dev dev_rdev, file_dev[FC_FILE_COUNT];


	do {
		if ((dev_fd = ::open(dev_path, O_RDONLY)) < 0) {
			err = ff_fail(errno, "error opening device '%s'", dev_path);
			break;
		}
		if ((err = ff_blkdev(dev_fd, & dev_rdev)) != 0) {
			err = ff_fail(err, "error in fstat('%s')", dev_path);
			break;
		}
		if ((err = ff_blkdev_size(dev_fd, & this->fm_dev_length)) != 0) {
			err = ff_fail(err, "error in ioctl('%s', BLKGETSIZE64)", dev_path);
			break;
		}
		for (i = 0; i < FC_FILE_COUNT; i++) {
			if ((this->fm_file_fd[i] = ::open(file_path[i], O_RDONLY)) < 0) {
				err = ff_fail(errno, "error opening %s '%s'", fc_file_label[i], file_path[i]);
				break;
			}
			if ((err = ff_dev(this->fm_file_fd[i], & file_dev[i])) != 0) {
				err = ff_fail(err, "error in %s fstat('%s')", fc_file_label[i], file_path[i]);
				break;
			}
			if (dev_rdev != file_dev[i]) {
				err = ff_fail(EINVAL, "invalid arguments: '%s' is device 0x%04x, but %s '%s' is contained in device 0x%04x\n",
						dev_path, (unsigned)dev_rdev, file_path[i], (unsigned)file_dev[i]);
				break;
			}
		}
	} while (0);

	if (dev_fd >= 0)
	    ::close(dev_fd);
	if (err != 0)
		close();
	return err;
}



/**
 * perform actual work.
 * invoke ff_work_dispatch(* this).
 * return 0 if success, else error.
 */
int ft_main::run()
{
    /**
     * force conversion to (const super_type &):
     * we want to pass the bare minimum to ff_work_dispatch()
     */
    const ft_ctx & ctx = * this;
    return ff_work_dispatch(ctx);
}

