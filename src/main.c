/*
 * main.c
 *
 *  Created on: Feb 14, 2011
 *      Author: max
 */

#include "first.h"


#include <errno.h>      /* for errno        */
#include <stdlib.h>     /* for free()       */
#include <stdio.h>      /* for printf()     */

#include <sys/types.h>  /* for open()       */
#include <sys/stat.h>   /*  "    "          */
#include <fcntl.h>      /*  "    "          */
#include <unistd.h>     /* for close()      */

#include "fail.h"       /* for ff_fail()    */
#include "filemap.h"    /* for ff_filemap() */
#include "fileutil.h"   /* for ff_filedev() */

#include "translate.h"  /* for ff_map_complement() */

static int ff_invalid_cmdline(const char * arg0, const char * msg) {
	fprintf(stderr, "%s\nTry `%s --help' for more information", msg, arg0);
	return 1;
}

/* open loop_path, perform checks and return 0 on success, 1 on error */
static int ff_init(const char * dev_path, const char * loop_path, off_t * ret_dev_length, int * ret_loop_fd) {
	ft_off dev_length;
	int dev_fd = -1, loop_fd = -1, err = 0;
	ft_dev dev_rdev, loop_dev;

	do {
		if ((dev_fd = open(dev_path, O_RDONLY)) < 0) {
			err = ff_fail(errno, "error opening device '%s'", dev_path);
			break;
		}
		if ((err = ff_blkdev(dev_fd, & dev_rdev)) != 0) {
			err = ff_fail(errno, "error in fstat('%s')", dev_path);
			break;
		}
		if ((err = ff_blkdev_size(dev_fd, & dev_length)) != 0) {
			err = ff_fail(errno, "error in ioctl('%s', BLKGETSIZE64)", dev_path);
			break;
		}
		if ((loop_fd = open(loop_path, O_RDONLY)) < 0) {
			err = ff_fail(errno, "error opening loop-file '%s'", loop_path);
			break;
		}
		if ((err = ff_dev(loop_fd, & loop_dev)) != 0) {
			err = ff_fail(err, "error in fstat('%s')", loop_path);
			break;
		}
		if (dev_rdev != loop_dev) {
			fprintf(stderr, "invalid arguments: '%s' is device 0x%04x, but loop-file '%s' is contained in device 0x%04x\n",
					dev_path, (unsigned)dev_rdev, loop_path, (unsigned)loop_dev);
			err = EINVAL;
			break;
		}
		* ret_dev_length = dev_length;
		* ret_loop_fd = loop_fd;

	} while (0);

	if (dev_fd >= 0)
		close(dev_fd);

	if (err != 0) {
		if (loop_fd >= 0)
			close(loop_fd);
	}
	return err;
}

/* close loop_fd. return 0 for success, else error */
static int ff_quit(int loop_fd) {
	if (close(loop_fd) != 0)
		return errno;
	return 0;
}



static void ff_map_show(ft_map * map) {
	ft_extent * extent = map->fc_extents;
	ft_size i, n;
	for (i = 0, n = map->fc_size; i < n; i++) {
		printf("extent %lu\t: logical %5llu,\tphysical %5llu,\tlength %llu\n", (unsigned long)i,
				(unsigned long long)extent[i].fc_logical,
				(unsigned long long)extent[i].fc_physical,
				(unsigned long long)extent[i].fc_length);
	}
	printf("\n");
}


static int ff_work(off_t dev_length, int loop_fd) {
    ft_map * map = NULL;
    int err = ff_filemap(loop_fd, & map);
    if (err == 0) {
    	if ((map = ff_map_complement(map, dev_length)) == NULL)
    		return ff_fail(ENOMEM, "failed to compute blocks relocation");

    	ff_map_show(map);
    }
    free(map);
    return err;
}





int main(int argc, char ** argv) {
	const char * dev_path, * loop_path;
	off_t dev_length;
	int err, loop_fd;

	if (argc <= 1)
		return ff_invalid_cmdline(argv[0], "missing argument: device name");
	else if (argc == 2)
		return ff_invalid_cmdline(argv[0], "missing argument: loop-file name");

	dev_path = argv[1];
	loop_path = argv[2];

	if ((err = ff_init(dev_path, loop_path, & dev_length, & loop_fd)) != 0)
		return err;

	err = ff_work(dev_length, loop_fd);
	ff_quit(loop_fd);
	return err;
}
