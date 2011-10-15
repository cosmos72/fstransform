/*
 * fwd.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSREMAP_FWD_HH
#define FSREMAP_FWD_HH

#include "check.hh"

FT_NAMESPACE_BEGIN
class fr_args;
class fr_remap;
class fr_dispatch;
class fr_job;

template<typename T> struct fr_extent_key;
template<typename T> struct fr_extent_payload;
template<typename T> class  fr_extent;
template<typename T> class  fr_vector;
template<typename T> class  fr_map;
template<typename T> class  fr_pool_entry;
template<typename T> class  fr_pool;
template<typename T> class  fr_work;
FT_NAMESPACE_END


FT_IO_NAMESPACE_BEGIN
class fr_io;
class fr_io_posix;
class fr_io_test;
class fr_io_self_test;
FT_IO_NAMESPACE_END

FT_UI_NAMESPACE_BEGIN
class fr_ui;
class fr_ui_tty;
FT_UI_NAMESPACE_END

#endif /* FSREMAP_FWD_HH */
