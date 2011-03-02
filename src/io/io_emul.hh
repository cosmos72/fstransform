/*
 * io/io_posix.hh
 *
 *  Created on: Feb 28, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_IO_IO_EMUL_HH
#define FSTRANSFORM_IO_IO_EMUL_HH

#include "../types.hh"    // for ft_uoff */
#include "io.hh"          // for ft_io */
FT_IO_NAMESPACE_BEGIN

class ft_io_emul: public ft_io
{
private:
    typedef ft_io super_type;

protected:
    /**
     * retrieve LOOP-FILE extents and insert them into ret_list.
     * return 0 for success, else error (and ret_list contents will be UNDEFINED).
     *
     * must be overridden by sub-classes
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
    ft_io_emul();

    /** destructor. calls close() */
    virtual ~ft_io_emul();

    /** check for consistency and open DEVICE, LOOP-FILE and ZERO-FILE */
    int open();

    /** return true if this ft_io_emul is currently (and correctly) open */
    virtual bool is_open() const;

    /** close this I/O, including file descriptors to DEVICE, LOOP-FILE and ZERO-FILE */
    virtual void close();
};

FT_IO_NAMESPACE_END

#endif /* FSTRANSFORM_IO_IO_EMUL_HH */
