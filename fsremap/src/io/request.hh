/*
 * io/request.hh
 *
 *  Created on: Mar 15, 2011
 *      Author: max
 */

#ifndef FSREMAP_IO_REQUEST_HH
#define FSREMAP_IO_REQUEST_HH

#include "../types.hh"    // for ft_uoff
#include "../extent.hh"   // for fr_dir


FT_IO_NAMESPACE_BEGIN

/**
 * simple class containing details of an I/O request.
 *
 * class invariant: ff_can_sum(ff_max2(this_from, this_to), this_length)
 */
class ft_request {
private:
    ft_uoff this_from;
    ft_uoff this_to;
    ft_uoff this_length;
    fr_dir  this_dir;

public:
    /** construct a request with no pending copies */
    ft_request();

    /** construct a request with specified values */
    ft_request(ft_uoff from, ft_uoff to, ft_uoff length, fr_dir dir);

    /** compiler-generated copy constructor is ok */
    // ft_request(const ft_request &);

    /** compiler-generated assignment operator is ok */
    // const ft_request & operator=(const ft_request &);

    /** compiler-generated destructor is ok */
    // ~ft_request();


    /** forget and discard any requested copy */
    void clear();

    FT_INLINE ft_uoff from()   const { return this_from;   }
    FT_INLINE ft_uoff to()     const { return this_to;     }
    FT_INLINE ft_uoff length() const { return this_length; }
    FT_INLINE fr_dir  dir()    const { return this_dir;    }
    FT_INLINE bool    empty()  const { return this_length == 0; }

    FT_INLINE bool is_from_dev() const { return ff_is_from_dev(this_dir); }
    FT_INLINE bool is_to_dev()   const { return ff_is_to_dev(this_dir);   }

    const char * label_from() const;
    const char * label_to()   const;

    /**
     * forget any requested copy and set this request to specified values.
     * returns EOVERFLOW if max(from,to)+length overflows ft_uoff
      */
    int assign(ft_uoff from, ft_uoff to, ft_uoff length, fr_dir dir);

    /**
     * coalesce this request with specified values, or return error if they cannot be coalesced.
     * possible errors:
     * ENOTDIR   the two requests are not in the same direction
     * EINVAL    the two requests are not consecutive, or coalescing them would overflow
     */
    int coalesce(ft_uoff from, ft_uoff to, ft_uoff length, fr_dir dir);

    /**
     * remove 'length' bytes from the beginning of this request
     */
    int remove_front(ft_uoff length);
};

FT_IO_NAMESPACE_END

#endif /* FSREMAP_IO_REQUEST_HH */
