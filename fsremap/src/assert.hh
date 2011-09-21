/*
 * assert.h
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_ASSERT_HH
#define FSTRANSFORM_ASSERT_HH

#include "check.hh"

FT_EXTERN_C_BEGIN
FT_NAMESPACE_BEGIN

void ff_assert_fail0(const char * caller_file, const char * caller_func, int caller_line, const char * assertion);

FT_NAMESPACE_END
FT_EXTERN_C_END

#define ff_assert(expr) do { if (!(expr)) ff_assert_fail0(FT_THIS_FILE, FT_THIS_FUNCTION, FT_THIS_LINE, #expr ); } while (0)

#define ff_assert_fail(msg) ff_assert_fail0(FT_THIS_FILE, FT_THIS_FUNCTION, FT_THIS_LINE, (msg))


#endif /* FSTRANSFORM_ASSERT_HH */
