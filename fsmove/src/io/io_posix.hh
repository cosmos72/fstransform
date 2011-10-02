/*
 * io/io_posix.hh
 *
 *  Created on: Sep 22, 2011
 *      Author: max
 */

#ifndef FSMOVE_IO_IO_POSIX_HH
#define FSMOVE_IO_IO_POSIX_HH

#include "../types.hh"    // for ft_string */
#include "io.hh"          // for fm_io */


FT_IO_NAMESPACE_BEGIN

/**
 * class performing I/O on POSIX systems
 */
class fm_io_posix: public fm_io
{
private:
    typedef fm_io super_type;


    // APPROX_BLOCK_SIZE is just for tuning creation of holes
    // no need to find the exact value for the current file system
    enum { APPROX_BLOCK_SIZE = 4096 };

    /**
     * fill 'stat' with information about the file/directory/special-device 'path'
     */
    int stat(const ft_string & path, ft_stat & stat);

    /**
     * move a single file/socket/device or a whole directory tree
     */
    int move(const ft_string & source_path, const ft_string & target_path);

    /**
     * move the single regurlar file 'source_path' to 'target_path'.
     */
    int move_file(const ft_string & source_path, const ft_stat & source_stat, const ft_string & target_path);

    /**
     * move the single special-device 'source_path' to 'target_path'.
     */
    int move_special(const ft_string & source_path, const ft_stat & source_stat, const ft_string & target_path);

    /**
     * try to rename a file, directory or special-device from 'source_path' to 'target_path'.
     */
    int move_rename(const char * source, const char * target);

    /**
     * copy file/stream contents from in_fd to out_fd
     */
    int copy_stream(int in_fd, int out_fd, const char * source, const char * target);

    /**
     * scan memory for blocksize-length and blocksize-aligned sections full of zeroes
     * return length of zeroed area at the beginning of scanned memory.
     * returned length is rounded down to block_size
     */
    static size_t hole_length(const char * mem, ft_size mem_len);

    /**
     * scan memory for blocksize-length and blocksize-aligned sections NOT full of zeroes
     * return length of NON-zeroed area at the beginning of scanned memory.
     * returned length is rounded UP to block_size
     */
    static size_t nonhole_length(const char * mem, ft_size mem_len);

    /**
     * write bytes to out_fd, retrying in case of short writes or interrupted system calls.
     * returns 0 for success, else error
     */
    static int full_write(int out_fd, const char * data, ft_size len, const char * target_path);
    
    /**
     * check inode_cache for hard links and recreate them.
     * must be called if and only if stat.st_nlink > 1.
     *
     * returns EAGAIN if inode was not in inode_cache
     */
    int hard_link(const ft_stat & stat, const ft_string & target_path);

    /**
     * copy the permission bits, owner/group and timestamps from 'stat' to 'target'
     */
    int copy_stat(const char * target, const ft_stat & stat);

    /** create a target directory, copying its mode and other meta-data from 'stat' */
    int create_dir(const ft_string & path, const ft_stat & stat);

    /** remove a source directory, which must be empty */
    int remove_dir(const ft_string & path);

public:
    /** constructor */
    fm_io_posix();

    /** destructor. calls close() */
    virtual ~fm_io_posix();

    /** return true if this fr_io_posix is currently (and correctly) open */
    virtual bool is_open() const;

    /** check for consistency and open SOURCE_ROOT, TARGET_ROOT */
    virtual int open(const fm_args & args);

    /** core of recursive move algorithm, actually moves the whole source tree into target */
    virtual int move();

    /** close this I/O, including file descriptors to DEVICE, LOOP-FILE, ZERO-FILE and SECONDARY-STORAGE */
    virtual void close();
};

FT_IO_NAMESPACE_END

#endif /* FSMOVE_IO_IO_POSIX_HH */
