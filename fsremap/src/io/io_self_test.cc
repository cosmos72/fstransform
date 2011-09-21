/*
 * io/io_self_test.cc
 *
 *  Created on: Mar 23, 2011
 *      Author: max
 */

#include "../first.hh"

#include "../log.hh"         // for ff_log()
#include "../util.hh"        // for ff_min2(), ff_max2(), ff_random()
#include "io_self_test.hh"   // for fr_io_self_test


FT_IO_NAMESPACE_BEGIN


/** constructor */
fr_io_self_test::fr_io_self_test(fr_job & job)
: super_type(job), this_block_size_log2(0)
{ }


/** destructor. calls close() */
fr_io_self_test::~fr_io_self_test()
{
    close();
}

/** return true if this fr_io_self_test is currently (and correctly) open */
bool fr_io_self_test::is_open() const
{
    return dev_length() != 0;
}

/** check for consistency and load LOOP-FILE and ZERO-FILE extents list from files */
int fr_io_self_test::open()
{
    if (is_open()) {
        // already open!
        ff_log(FC_ERROR, 0, "unexpected call, I/O is already open");
        return EISCONN;
    }
    /*
     * block_size_log_2 is a random number in the range [4,16]
     * thus block_size is one of 2^4, 2^5 ... 2^15, 2^16
     */
    this_block_size_log2 = (ft_uoff) ff_random(12) + 4;

    /* dev_len is a random number in the range [block_size, 8GB * block_size] */
    ft_uoff dev_len = (1 + ff_random(1 + 2 * (ft_ull)0xfffffffful)) << this_block_size_log2;

    dev_length(dev_len);
    dev_path("<self-test-device>");

    double pretty_len;
    const char * pretty_label = ff_pretty_size(dev_len, & pretty_len);
    ff_log(FC_INFO, 0, "%s%s length is %.2f %sbytes", sim_msg, label[FC_DEVICE], pretty_len, pretty_label);
    
    return 0;
}


/** close this I/O, including file descriptors to DEVICE, LOOP-FILE and ZERO-FILE */
void fr_io_self_test::close()
{
    this_block_size_log2 = 0;
    super_type::close();
}

/** close any resource associated to LOOP-FILE and ZERO-FILE extents */
void fr_io_self_test::close_extents()
{ }


/**
 * retrieve LOOP-FILE extents and FREE-SPACE extents and append them into
 * the vectors loop_file_extents and free_space_extents.
 * the vectors will be ordered by extent ->logical.
 *
 * return 0 for success, else error (and vectors contents will be UNDEFINED).
 *
 * implementation: load extents list from files
 * (for example they could be the job persistence files)
 */
int fr_io_self_test::read_extents(fr_vector<ft_uoff> & loop_file_extents,
                                 fr_vector<ft_uoff> & free_space_extents,
                                 ft_uoff & ret_block_size_bitmask)
{
    if (!is_open())
        return ENOTCONN; // not open!

    ft_uoff dev_len = dev_length(), free_len = ff_random(dev_len >> this_block_size_log2) << this_block_size_log2;

    fr_map<ft_uoff> loop_file_map, free_space_map;

    invent_extents(loop_file_map, dev_len, ret_block_size_bitmask);
    invent_extents(free_space_map, free_len, ret_block_size_bitmask);

    /* remove from FREE-SPACE any extent->physical already present in LOOP-FILE */
    fr_map<ft_uoff> intersect_map;
    intersect_map.intersect_all_all(loop_file_map, free_space_map, FC_PHYSICAL2);
    free_space_map.remove_all(intersect_map);

    loop_file_extents.insert(loop_file_extents.end(), loop_file_map.begin(), loop_file_map.end());
    loop_file_extents.sort_by_logical();

    free_space_extents.insert(free_space_extents.end(), free_space_map.begin(), free_space_map.end());
    free_space_extents.sort_by_logical();

    return 0;
}

/** fill ret_extents with random (but consistent) extents. extents will stop at 'file_len' bytes */
void fr_io_self_test::invent_extents(fr_map<ft_uoff> & extent_map, ft_uoff file_len, ft_uoff & ret_block_size_bitmask) const
{
    fr_vector<ft_uoff> extent_vec;

    file_len >>= this_block_size_log2;
    ft_uoff pos = 0, hole, len, max_extent_len = ff_max2(file_len >> 16, (ft_uoff)0x100);
    fr_extent<ft_uoff> extent;
    while (pos < file_len) {
        /* make some holes in physical layout */
        hole = ff_random(ff_min2(max_extent_len >> 4, file_len - pos - 1));
        len = 1 + ff_min2((ft_uoff)ff_random(max_extent_len), file_len - pos - hole - 1); // length == 0 is not valid!
        ret_block_size_bitmask |= extent.physical() = (pos + hole) << this_block_size_log2;
        extent.logical() = 0;
        ret_block_size_bitmask |= extent.length() = len << this_block_size_log2;

        /* on average, one extent in 1024 is FC_EXTENT_ZEROED */
        extent.user_data() = ff_random(1023) == 0 ? FC_EXTENT_ZEROED : FC_DEFAULT_USER_DATA;

        extent_vec.push_back(extent);
        pos += hole + len;
    }
    /* shuffle the extents list and set ->logical */
    ft_size i, r, n = extent_vec.size();
    pos = 0;
    for (i = 0; i + 1 < n; i++) {
        r = ff_random(n - i - 1);
        if (r != 0)
            std::swap(extent_vec[i], extent_vec[i + r]);
        fr_extent<ft_uoff> & extent_i = extent_vec[i];
        /* also make some holes in logical layout */
        if ((pos += ff_random(ff_min2(max_extent_len, file_len - pos) >> 8)) >= file_len)
            break;
        ret_block_size_bitmask |= extent_i.logical() = pos << this_block_size_log2;
        pos += extent_i.length() >> this_block_size_log2;
        extent_map.insert(extent_i);
    }

    if (i + 1 == n) {
        if ((pos += ff_random(ff_min2(max_extent_len, file_len - pos) >> 8)) < file_len) {
            fr_extent<ft_uoff> & extent_i = extent_vec[i];
            ret_block_size_bitmask |= extent_i.logical() = pos << this_block_size_log2;
            pos += extent_i.length() >> this_block_size_log2;
            extent_map.insert(extent_i);
        }
    }
}

FT_IO_NAMESPACE_END
