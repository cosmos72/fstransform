/*
 * io.template.hh
 *
 *  Created on: Mar 01, 2011
 *      Author: max
 */

#include "../first.hh"

#include <cerrno>       // for EFBIG

#include "../vector.hh" // for ft_vector<T>
#include "io.hh"        // for ft_io

FT_IO_NAMESPACE_BEGIN

/**
 * retrieve LOOP-FILE extents and insert them into ret_map.
 * return 0 for success, else error (and ret_map contents will be unchanged).
 *
 * also checks that device blocks count can be represented by T
 */
template<class T>
int ft_io::loop_file_extents(ft_map<T> & ret_map)
{
    ft_vector<T> extents_list;
    int err = loop_file_extents_list(extents_list);
    if (err == 0) {
        /*
         * sort LOOP-FILE extents by fm_physical.
         * needed by ret_map.append0() immediately below
         */
        extents_list.sort_by_physical();
        ret_map.append0(extents_list);
    }
    return err;
}


/**
 * given LOOP-FILE extents, compute DEVICE extents and insert them into ret_map.
 *
 * implementation: computes union of specified loop_file_map
 * and free_space_extents_list(), then complements the union.
 *
 * return 0 for success, else error (and ret_map contents will be unchanged).
 */
template<class T>
int ft_io::device_extents(const ft_map<T> & loop_file_map, ft_map<T> & ret_map)
{
    ft_vector<T> extents_list;
    int err = free_space_extents_list(extents_list);
    if (err == 0) {
        /*
         * compute the union of LOOP-FILE extents (in loop_file_map)
         * and FREE-SPACE extents (in extents_list)
         */
        extents_list.append(loop_file_map);

        /*
         * sort the extents union by fm_physical.
         * needed by ret_map.complement0() immediately below
         */
        extents_list.sort_by_physical();
        ret_map.complement0(extents_list, dev_len);
    }
    return err;
}

FT_IO_NAMESPACE_END
