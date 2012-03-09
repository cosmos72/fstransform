/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 * 
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * io/io.hh
 *
 *  Created on: Mar 1, 2011
 *      Author: max
 */

#ifndef FSREMAP_IO_IO_HH
#define FSREMAP_IO_IO_HH

#include "../check.hh"

#include "../types.hh"       // for ft_uoff, ft_size, ft_string
#include "../fwd.hh"         // for fr_args forward declaration
#include "../job.hh"         // for fr_job
#include "../extent.hh"      // for fr_extent<T>
#include "../vector.hh"      // for fr_vector<T>
#include "../map.hh"         // for fr_map<T>
#include "../ui/ui.hh"       // for fr_ui

#include "request.hh"        // for ft_request


FT_IO_NAMESPACE_BEGIN

/**
 * abstract base class for all I/O implementations
 * that actually read and write on DEVICE
 */
class fr_io
{
public:
    enum {
        FC_DEVICE = 0, FC_LOOP_FILE
    };

    static char const * const label[]; // device, loop-file (and also others, but don't tell)
    static char const * const LABEL[]; // DEVICE, LOOP-FILE (and also others, but don't tell)

    enum {
        FC_IO_EXTENTS_FILE_COUNT = 2,
    };
    static char const* const extents_filename[]; // "/loop_extents.txt", "/free_space_extents.txt"


private:
    fr_vector<ft_uoff> this_primary_storage;
    fr_extent<ft_uoff> this_secondary_storage;
    fr_vector<ft_uoff> request_vec;

    ft_uoff this_dev_length, this_loop_file_length, this_eff_block_size_log2;
    const char * this_dev_path;
    const char * this_umount_cmd;
    fr_job & this_job;
    FT_UI_NS fr_ui * this_ui;
    fr_dir request_dir;
    bool this_delegate_ui;


    /* cannot call copy constructor */
    fr_io(const fr_io &);

    /* cannot call assignment operator */
    const fr_io & operator=(const fr_io &);

protected:
    enum {
        FC_ZERO_FILE = FC_LOOP_FILE + 1,
        FC_SECONDARY_STORAGE,
        FC_PRIMARY_STORAGE,
        FC_STORAGE,
        FC_FREE_SPACE,
    };

    /** remember device length */
    FT_INLINE void dev_length(ft_uoff dev_length) { this_dev_length = dev_length; }

    /** remember loop file length */
    FT_INLINE void loop_file_length(ft_uoff loop_file_length) { this_loop_file_length = loop_file_length; }

    /** remember device path */
    FT_INLINE void dev_path(const char * dev_path) { this_dev_path = dev_path; }

    /** compute and return log2() of effective block size and remember it */
    ft_uoff effective_block_size_log2(ft_uoff block_size_bitmask);

    /** return (-)EOVERFLOW if request from/to + length overflow specified maximum value */
    static int validate(const char * type_name, ft_uoff type_max, fr_dir dir, ft_uoff from, ft_uoff to, ft_uoff length);

    /** return (-)EOVERFLOW if request from/to + length overflow specified maximum value */
    FT_INLINE static int validate(const char * type_name, ft_uoff type_max, fr_dir dir, const fr_extent<ft_uoff> & extent)
    {
        return validate(type_name, type_max, dir, extent.physical(), extent.logical(), extent.length());
    }

    /** invoked by derived classes to tell whether they will invoke ui methods by themselves (default: false) */
    FT_INLINE void delegate_ui(bool flag_delegate_ui) { this_delegate_ui = flag_delegate_ui; }

    /**
     * retrieve LOOP-FILE extents and FREE-SPACE extents and insert them into
     * the vectors loop_file_extents and free_space_extents.
     * the vectors will be ordered by extent ->logical.
     *
     * return 0 for success, else error (and vectors contents will be UNDEFINED).
     *
     * if success, also returns in ret_effective_block_size_log2 the log2()
     * of device effective block size.
     * the device effective block size is defined as follows:
     * it is the largest power of 2 that exactly divides all physical,
     * logical and lengths in all returned extents (both for LOOP-FILE
     * and for FREE-SPACE) and that also exactly exactly divides device length.
     *
     * must be overridden by sub-classes.
     *
     * a common trick subclasses may use to implement this method
     * is to fill the device's free space with a ZERO-FILE,
     * and actually retrieve the extents used by ZERO-FILE.
     */
    virtual int read_extents(fr_vector<ft_uoff> & loop_file_extents,
                             fr_vector<ft_uoff> & free_space_extents,
                             ft_uoff & ret_effective_block_size_log2) = 0;

    /**
     * get writable reference to secondary_storage.
     * must be called by create_storage() to set the details of secondary_storage
     */
    FT_INLINE fr_extent<ft_uoff> & secondary_storage() { return this_secondary_storage; }

    /**
     * flush any pending copy, i.e. actually call copy_bytes(request).
     * return 0 if success, else error
     * on return, 'ret_copied' will be increased by the number of blocks actually copied (NOT queued for copying),
     * which could be > 0 even in case of errors
     */
    int flush_queue();


    /**
     * actually copy a list of fragments from DEVICE or FREE-STORAGE, to STORAGE to FREE-DEVICE.
     * must be implemented by sub-classes.
     * note: parameters are in bytes!
     * return 0 if success, else error.
     */
    virtual int flush_copy_bytes(fr_dir dir, fr_vector<ft_uoff> & request_vec) = 0;

    /**
     * flush any I/O specific buffer
     * return 0 if success, else error
     * default implementation: do nothing
     */
    virtual int flush_bytes();

    /**
     * write zeroes to device (or to storage).
     * used to remove device-renumbered blocks once remapping is finished
     */
    virtual int zero_bytes(fr_to to, ft_uoff offset, ft_uoff length) = 0;

public:
    /** constructor */
    fr_io(fr_job & job);

    /**
     * destructor.
     * sub-classes must override it to call close() if they override close()
     */
    virtual ~fr_io();

    /** return true if this fr_io is currently (and correctly) open */
    virtual bool is_open() const = 0;

    /**
     * open this fr_io.
     * sub-classes must override this method to perform appropriate initialization,
     * and the first thing sub-classes open() must do is to call fr_io::open().
     */
    virtual int open(const fr_args & args);

    /**
     * close this fr_io.
     * sub-classes must override this method to perform appropriate cleanup
     */
    virtual void close();

    /** return device length (in bytes), or 0 if not open */
    FT_INLINE ft_uoff dev_length() const { return this_dev_length; }

    /** return loop file length (in bytes), or 0 if not open */
    FT_INLINE ft_uoff loop_file_length() const { return this_loop_file_length; }

    /** return device path, or NULL if not open */
    FT_INLINE const char * dev_path() const { return this_dev_path; }

    /**
     * return umount command, or NULL if not specified by command line.
     * umount command is set by open(const fr_args & args).
     */
    FT_INLINE const char * umount_cmd() const { return this_umount_cmd; }

    /** return log2 of effective block size, or 0 if not open */
    FT_INLINE ft_uoff effective_block_size_log2() const { return this_eff_block_size_log2; }



    /** return job */
    FT_INLINE fr_job & job() const { return this_job; }

    /** return job_id, or 0 if not set */
    FT_INLINE ft_size job_id() const { return this_job.job_id(); }

    /** return job_dir, or "" if not set */
    FT_INLINE const ft_string & job_dir() const { return this_job.job_dir(); }

    /** return storage_size to use (in bytes), or 0 if not set */
    FT_INLINE ft_size job_storage_size(fr_storage_size which) const { return this_job.job_storage_size(which); }

    /** set storage_size to use (in bytes), or 0 to unset it */
    FT_INLINE void job_storage_size(fr_storage_size which, ft_size len) { this_job.job_storage_size(which, len); }

    /**
     * return which free blocks to clear after remapping:
     * all, only blocks used as primary storage or renumbered device, or none
     */
    FT_INLINE fr_clear_free_space job_clear() const { return this_job.job_clear(); }

    /**
     * set which free blocks to clear after remapping:
     * all, only blocks used as primary storage or renumbered device, or none
     */
    FT_INLINE void job_clear(fr_clear_free_space clear) { this_job.job_clear(clear); }


    /* return the UI currently use, or NULL if not set */
    FT_INLINE FT_UI_NS fr_ui * ui() const { return this_ui; }

    /* set the UI to use. specify NULL to unset */
    FT_INLINE void ui(FT_UI_NS fr_ui * ui) { this_ui = ui; }


    /**
     * return true if I/O classes should be less strict on sanity checks
     * and generate WARNINGS (and keep going) for failed sanity checks
     * instead of generating ERRORS (and quitting)
     */
    FT_INLINE bool force_run() const { return this_job.force_run(); }

    /** return true if subclasses should simulate run, i.e. run WITHOUT reading or writing device blocks */
    FT_INLINE bool simulate_run() const { return this_job.simulate_run(); }

    /** return true if program can ask questions to the user and read answers from stdin */
    FT_INLINE bool ask_questions() const { return this_job.ask_questions(); }

    /**
     * calls the 3-argument version of read_extents() and, if successful,
     * calls effective_block_size_log2() to compute and remember effective block size
     */
    int read_extents(fr_vector<ft_uoff> & loop_file_extents,
                     fr_vector<ft_uoff> & free_space_extents);


    /**
     * loads extents from file job.job_dir() + '/loop_extents.txt' and job.job_dir() + '/free_space_extents.txt'
     * by calling the function ff_load_extents_file()
     * if successful, calls effective_block_size_log2() to compute and remember effective block size
     */
    int load_extents(fr_vector<ft_uoff> & loop_file_extents,
                     fr_vector<ft_uoff> & free_space_extents);

    /**
     * saves extents to files job.job_dir() + '/loop_extents.txt' and job.job_dir() + '/free_space_extents.txt'
     * by calling the function ff_save_extents_file()
     */
    int save_extents(const fr_vector<ft_uoff> & loop_file_extents,
                     const fr_vector<ft_uoff> & free_space_extents) const;

    /**
     * close the file descriptors for LOOP-FILE and ZERO-FILE
     */
    virtual void close_extents() = 0;



    /**
     * get writable reference to primary_storage.
     * must be called before create_storage() to set the details of primary_storage
     */
    FT_INLINE fr_vector<ft_uoff> & primary_storage() { return this_primary_storage; }

    /** get const reference to primary_storage */
    FT_INLINE const fr_vector<ft_uoff> & primary_storage() const { return this_primary_storage; }

    /**
     * create and open SECONDARY-STORAGE job.job_dir() + '/storage.bin' and fill it with 'secondary_len' bytes of zeros.
     * setup a virtual storage composed by this->primary_storage extents inside DEVICE, plus secondary-storage extents.
     * return 0 if success, else error
     */
    virtual int create_storage(ft_size secondary_len, ft_size mem_buffer_len) = 0;


    /** call umount(8) on dev_path() */
    virtual int umount_dev() = 0;


    /**
     * called once by work<T>::relocate() immediately before starting the remapping phase.
     *
     * must be overridden by sub-classes to check that last device block to be written is actually writable.
     * Reason: at least on Linux, if a filesystems is smaller than its containing device, it often limits to its length the writable blocks in the device.
     *
     * default implementation: do nothing and return success (0)
     */
    virtual int check_last_block();

    /**
     * perform buffering and coalescing of copy requests.
     * queues a copy of single fragment from DEVICE or FREE-STORAGE, to STORAGE to FREE-DEVICE.
     * calls flush_queue() as needed to actually perform any copy that cannot be buffered or coalesced.
     * note: parameters are in bytes!
     * return 0 if success, else error
     *
     * on return, this->ret_copied will be increased by the number of blocks actually copied or queued for copying,
     * which could be > 0 even in case of errors
     */
    int copy_bytes(fr_dir dir, ft_uoff from_physical, ft_uoff to_physical, ft_uoff length);

    /**
     * copy a single fragment from DEVICE to FREE-STORAGE, or from STORAGE to FREE-DEVICE or from DEVICE to FREE-DEVICE
     * (STORAGE to FREE-STORAGE copies could be supported easily, but are not considered useful)
     * note: parameters are in blocks!
     * note: implementations may accumulate (queue) copy requests, actual copy and error reporting may be delayed until flush()
     *
     * return 0 if success, else error
     *
     * on return, this->ret_copied will be increased by the number of blocks actually copied or queued for copying,
     * which could be > 0 even in case of errors
     */
    template<typename T>
    int copy(fr_dir dir, T from_physical, T to_physical, T length)
    {
        return copy_bytes(dir,
                          (ft_uoff)from_physical << this_eff_block_size_log2,
                          (ft_uoff)to_physical << this_eff_block_size_log2,
                          (ft_uoff)length  << this_eff_block_size_log2);
    }

    /**
     * flush any pending copy (call copy_bytes() through flush_queue()),
     * plus flush any I/O specific buffer (call flush_bytes())
     * return 0 if success, else error
     */
    int flush();

    /**
     * write zeroes to device (or to storage).
     * used to remove device-renumbered blocks once remapping is finished
     * and clean the remaped file-system
     * note: parameters are in blocks!
     */
    template<typename T>
    int zero(fr_to to, T offset, T length)
    {
        ft_uoff offset_bytes = (ft_uoff)offset << this_eff_block_size_log2;
        ft_uoff length_bytes = (ft_uoff)length  << this_eff_block_size_log2;

        if (this_ui != 0 && !this_delegate_ui)
            this_ui->show_io_write(to, offset_bytes, length_bytes);

        return zero_bytes(to, offset_bytes, length_bytes);
    }

    /**
     * write zeroes to primary storage.
     * used to remove primary-storage once remapping is finished
     * and clean the remaped file-system
     */
    virtual int zero_primary_storage() = 0;


    /** called after relocate() and clear_free_space(). closes storage */
    virtual int close_storage() = 0;
};



FT_IO_NAMESPACE_END



#endif /* FSREMAP_IO_IO_HH */
