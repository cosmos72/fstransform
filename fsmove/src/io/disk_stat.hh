/*
 * io/disk_stat.hh
 *
 *  Created on: Oct 05, 2011
 *      Author: max
 */

#ifndef FSMOVE_IO_DISK_STAT_HH
#define FSMOVE_IO_DISK_STAT_HH

#include "../check.hh"
#include "../types.hh"    // for ft_uoff, ft_string

FT_IO_NAMESPACE_BEGIN

/**
 * class to keep track of disk total and free space
 */
class fm_disk_stat {

private:
    enum {
        _1Mbyte = (ft_uoff)1 << 20,
        _1Gbyte = (ft_uoff)1 << 30
    };

    ft_string this_name;
    ft_uoff this_total, this_free;

public:
    /**
     * if file system is smaller than 64GB, critically low free space is THRESHOLD_MIN (1Mbyte).
     * if file system is between 64GB and 64TB, critically low free space is total disk space divided 65536 (i.e. 0.0015%).
     * if file system is larger than 64TB, critically low free space is THRESHOLD_MAX (1Gbyte).
     */
    enum {
        THRESHOLD_MIN = _1Mbyte,
        THRESHOLD_MAX = _1Gbyte,
    };
    /** constructor */
    fm_disk_stat();

    /** compiler-generated copy constructor is fine */
    // const fm_disk_stat & fm_disk_stat(const fm_disk_stat &);

    /** compiler-generated destructor is fine */
    // ~fm_disk_stat();

    /** compiler-generated assignment operator is fine */
    // const fm_disk_stat & operator=(const fm_disk_stat &);


    /** clear all data stored in this object */
    void clear();

    /** get the disk name */
    FT_INLINE const ft_string & get_name() const { return this_name; }
    /** set the disk name */
    FT_INLINE void set_name(const ft_string & name) { this_name = name; }



    /** return the total disk space */
    FT_INLINE ft_uoff get_total() const { return this_total; }
    /** set the total disk space */
    FT_INLINE void set_total(ft_uoff total) { this_total = total; }



    /** return the free disk space */
    FT_INLINE ft_uoff get_free() const { return this_free; }

    /**
     * set the free disk space.
     * returns 0, or error if free disk space becomes critically low
     */
    int set_free(ft_uoff free);

};



FT_IO_NAMESPACE_END

#endif /* FSMOVE_IO_IO_HH */
