/*
 * io/io.hh
 *
 *  Created on: Sep 20, 2011
 *      Author: max
 */

#ifndef FSMOVE_IO_IO_HH
#define FSMOVE_IO_IO_HH

#include "../check.hh"

#include "../types.hh"       // for ft_string-
#include "../fwd.hh"         // for fm_args
#include "../inode_cache.hh" // for fm_inode_cache

#include <set>               // for std::set


FT_IO_NAMESPACE_BEGIN

/**
 * abstract base class for all I/O implementations
 * that actually move files around
 */
class fm_io {

private:
    fm_inode_cache this_inode_cache;
    std::set<ft_string> this_exclude_set;

    ft_string this_source_root, this_target_root;
    bool this_force_run, this_simulate_run;

protected:
    FT_INLINE const ft_string * inode_cache_find_or_add(ft_inode inode, const ft_string & path) {
        return this_inode_cache.find_or_add(inode, path);
    }
    FT_INLINE const ft_string * inode_cache_find(ft_inode inode, const ft_string & path) const {
        return this_inode_cache.find(inode, path);
    }
    FT_INLINE void inode_cache_erase(ft_inode inode) {
        return this_inode_cache.erase(inode);
    }

public:
    enum {
        FC_SOURCE_ROOT = 0, FC_TARGET_ROOT,
        FC_ARGS_COUNT = 2, // must be equal to count of preceding enum constants
    };

    static char const * const label[]; // SOURCE, TARGET

    /** constructor */
    fm_io();

    /**
     * destructor.
     * sub-classes must override it to call close()
     */
    virtual ~fm_io();

    /** return true if this ft_io is currently (and correctly) open */
    virtual bool is_open() const = 0;

    /**
     * open this fm_io.
     * sub-classes must override this method to perform appropriate initialization
     */
    virtual int open(const fm_args & args);

    /** core of recursive move algorithm, actually moves the whole source tree into target */
    virtual int move() = 0;

    /**
     * close this fm_io.
     * sub-classes must override this method to perform appropriate cleanup
     */
    virtual void close();

    /**
     * return the set of source files NOT to move
     */
    FT_INLINE const std::set<ft_string> & exclude_set() const { return this_exclude_set; }

    /**
     * return the top-most source path to move from
     */
    FT_INLINE const ft_string & source_root() const { return this_source_root; }

    /**
     * return the top-most target path to move to
     */
    FT_INLINE const ft_string & target_root() const { return this_target_root; }

    /**
     * return the force_run flag
     */
    FT_INLINE bool force_run() const { return this_force_run; }

    /**
     * return the simulate_run flag
     */
    FT_INLINE bool simulate_run() const { return this_simulate_run; }
};



FT_IO_NAMESPACE_END

#endif /* FSMOVE_IO_IO_HH */
