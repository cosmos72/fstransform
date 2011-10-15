/*
 * io/util_posix.cc
 *
 *  Created on: Feb 24, 2011
 *      Author: max
 */
#include "../first.hh"

#include <cerrno>         // for errno, EOVERFLOW, ENOTBLK, EINTR
#include <cstdlib>        // for exit()
#include <unistd.h>       // for fork(), execv()
#include <sys/ioctl.h>    // for ioctl()
#include <sys/types.h>    // for waitpid()
#include <sys/wait.h>     // for   "
#include <linux/fs.h>     // for BLKGETSIZE64

#include "../types.hh"    // for ft_u64, ft_stat
#include "../log.hh"      // for ff_log()
#include "../util.hh"     // for ff_min2<T>()
#include "util_posix.hh"  // for ff_posix_ioctl(), ff_posix_stat(), ff_posix_size(), ff_filedev()


FT_IO_NAMESPACE_BEGIN



/** invoke ioctl() */
int ff_posix_ioctl(int fd, int request, void * arg)
{
    return ioctl(fd, request, arg) == 0 ? 0 : errno;
}

/** return file stats in (*ret_stat) */
int ff_posix_stat(int fd, ft_stat * ret_stat)
{
    int err = fstat(fd, ret_stat);
    if (err != 0)
        err = errno;
    return err;
}


/** return file size in (*ret_size) */
int ff_posix_size(int fd, ft_uoff * ret_size)
{
    ft_stat st_buf;
    int err = ff_posix_stat(fd, & st_buf);
    if (err == 0) {
        ft_uoff file_size = (ft_uoff) st_buf.st_size;
        if ((ft_off) file_size == st_buf.st_size)
            * ret_size = file_size;
        else
            err = EOVERFLOW; // file size cannot be represented by ft_uoff!
    }
    return err;
}

/** return ID of device containing file in (*ret_dev) */
int ff_posix_dev(int fd, dev_t * ret_dev)
{
    ft_stat st_buf;
    int err = ff_posix_stat(fd, & st_buf);
    if (err == 0)
        * ret_dev = st_buf.st_dev;
    return err;
}



/** if file is special block device, return its device ID in (*ret_dev) */
int ff_posix_blkdev_dev(int fd, ft_dev * ret_dev)
{
    ft_stat st_buf;
    int err = ff_posix_stat(fd, & st_buf);
    if (err == 0) {
        if ((st_buf.st_mode & S_IFBLK))
            * ret_dev = st_buf.st_rdev;
        else
            err = ENOTBLK;
    }
    return err;
}

/** if file is special block device, return its length in (*ret_size) */
int ff_posix_blkdev_size(int fd, ft_uoff * ret_size)
{
    ft_u64 size_buf;
    int err = ff_posix_ioctl(fd, BLKGETSIZE64, & size_buf);
    if (err == 0) {
        ft_uoff dev_size = (ft_uoff) size_buf;
        if ((ft_u64) dev_size == size_buf)
            * ret_size = dev_size;
        else
            err = EOVERFLOW; // device size cannot be represented by ft_uoff!
    }
    return err;
}


/** return this process PID in (*ret_pid) */
int ff_posix_pid(ft_pid * ret_pid)
{
    * ret_pid = getpid();
    return 0;
}

/** create a directory */
int ff_posix_mkdir(const char * path, ft_mode mode)
{
    int err = mkdir(path, mode);
    return err == 0 ? err : errno;
}



/**
 * seek file descriptor to specified position from file beginning.
 * note: if an error is returned, file descriptor position will be undefined!
 */
int ff_posix_lseek(int fd, ft_uoff pos)
{
    off_t pos_s = (off_t)pos;

    if (pos_s < 0 || pos != (ft_uoff) pos_s)
        return EOVERFLOW;
    if ((pos_s = lseek(fd, pos_s, SEEK_SET)) < 0)
        return errno;
    if (pos != (ft_uoff) pos_s)
        return ESPIPE;
    return 0;
}



/**
 * read from a file descriptor.
 * keep retrying in case of EINTR or short reads.
 */
int ff_posix_read(int fd, void * mem, ft_uoff length)
{
    ft_uoff chunk, max = (ft_uoff)((size_t)(ssize_t)-1 >> 1); /**< max = std::numeric_limits<ssize_t>::max() */
    ssize_t got;

    while (length != 0) {
        chunk = ff_min2(length, max);
        while ((got = ::read(fd, mem, (size_t)chunk)) < 0 && errno == EINTR)
            ;
        if (got < 0)
            return errno;
        if (got == 0)
            /* end-of-file */
            break;
        if ((ft_uoff) got >= length)
            break;
        mem = (void *)((char *)mem + got);
        length -= (ft_uoff) got;
    }
    return 0;
}


/**
 * write to a file descriptor.
 * keep retrying in case of EINTR or short writes.
 */
int ff_posix_write(int fd, const void * mem, ft_uoff length)
{
    ft_uoff chunk, max = (ft_uoff)((size_t)(ssize_t)-1 >> 1); /**< max = std::numeric_limits<ssize_t>::max() */
    ssize_t sent;

    while (length != 0) {
        chunk = ff_min2(length, max);
        while ((sent = ::write(fd, mem, (size_t)chunk)) < 0 && errno == EINTR)
            ;
        if (sent < 0)
            return errno;
        if (sent == 0)
            /* end-of-file */
            break;
        if ((ft_uoff) sent >= length)
            break;
        mem = (const void *)((const char *)mem + sent);
        length -= (ft_uoff) sent;
    }
    return 0;
}

/**
 * spawn a system command, typically with fork()+execv(), wait for it to complete and return its exit status.
 * argv[0] is conventionally the program name.
 * argv[1...] are program arguments and must be terminated with a NULL pointer.
 */
int ff_posix_exec(const char * path, const char * const argv[])
{
    int err;
    pid_t pid = ::fork();
    if (pid == 0) {
        /* child */
        ::execvp(path, (char * const *)argv);

        /* if we reach here, execvp() failed! */
        ff_log(FC_ERROR, err = errno, "execvp(%s) failed");

        /* exit() can only return one-byte exit status */
        err &= 0xff;
        if (err == 0) {
            err = (ECHILD > 0 ? ECHILD : -ECHILD) & 0xff;
            if (err == 0)
                err = 1;
        }
        ::exit(err);
    } else if (pid == (pid_t)-1) {
        err = ff_log(FC_ERROR, errno, "fork() failed");
    } else {
        /* parent */
        err = -ECHILD; // assume failure unless proved successful...
        int status = 0;

        if (waitpid(pid, & status, 0/*options*/) != pid)
            err = ff_log(FC_ERROR, errno, "error in waitpid(), assuming command '%s' failed", path);
        else if (WIFEXITED(status)) {

            status = WEXITSTATUS(status);
            if (status == 0)
                err = 0; // proved successful!
            else
                ff_log(FC_ERROR, 0, "command '%s' exited with non-zero exit status %d", path, status);

        } else if (WIFSIGNALED(status))
            ff_log(FC_ERROR, 0, "command '%s' died with signal %d", path, (int)WTERMSIG(status));
        else
            ff_log(FC_ERROR, 0, "waitpid() returned unknown status %d, assuming command '%s' failed", status, path);
    }
    return err;
}

FT_IO_NAMESPACE_END
