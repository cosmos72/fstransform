/*
 * io/io_posix.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_IO_IO_POSIX_HH
#define FSTRANSFORM_IO_IO_POSIX_HH

#include "../types.hh"    // for ft_uoff */
#include "io.hh"          // for ft_io */


FT_IO_NAMESPACE_BEGIN

class ft_io_posix: public ft_io
{
public:
    enum {
        FC_DEVICE = ft_io::FC_DEVICE,
        FC_LOOP_FILE = ft_io::FC_LOOP_FILE,
        FC_ZERO_FILE,
        FC_FILE_COUNT // must be equal to count of preceding enum constants
    };

private:
    typedef ft_io super_type;

    int fd[FC_FILE_COUNT];

protected:
    /**
     * retrieve LOOP-FILE extents and insert them into ret_list.
     * return 0 for success, else error (and ret_list contents will be UNDEFINED).
     *
     * must (and will) also check that device blocks count can be represented by ret_list,
     * by calling ret_list.extent_set_range(block_size, block_count)
     */
    virtual int loop_file_extents_list(ft_extent_list & ret_list);

    /**
     * retrieve FREE SPACE extents and insert them into ret_list.
     * so now we actually retrieve the extents used by ZERO-FILE.
     * and actually retrieve the extents used by ZERO-FILE.
     *
     * return 0 for success, else error (and ret_list contents will be UNDEFINED).
     */
    virtual int free_space_extents_list(ft_extent_list & ret_list);

public:
    /** default constructor */
    ft_io_posix();

    /** destructor. calls close() */
    virtual ~ft_io_posix();

    /** check for consistency and open DEVICE, LOOP-FILE and ZERO-FILE */
    int open(char const* const paths[FC_FILE_COUNT]);

    /** return true if this ft_io_posix is currently (and correctly) open */
    virtual bool is_open() const;

    /** close this I/O, including file descriptors to DEVICE, LOOP-FILE and ZERO-FILE */
    virtual void close();
};

FT_IO_NAMESPACE_END

#endif /* FSTRANSFORM_IO_IO_POSIX_HH */
