/*
 * features.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_FEATURES_HH
#define FSTRANSFORM_FEATURES_HH

#include <features.h>

#include "autoconf.hh"

#ifdef FT_HAVE_EXTERN_C
   /** in C++, define to extern "C" { ... }. in C, define as empty  */
#  define FT_EXTERN_C_BEGIN extern "C" {
#  define FT_EXTERN_C_END }
#else
#  define FT_EXTERN_C_BEGIN
#  define FT_EXTERN_C_END
#endif /* FT_HAVE_EXTERN_C */


#ifdef FT_HAVE_NAMESPACE
   /** in C++, define to namespace ft { ... }. in C, define as empty  */
#  define FT_NAMESPACE_BEGIN namespace ft {
#  define FT_NAMESPACE_END   }

#  define FT_IO_NAMESPACE_BEGIN FT_NAMESPACE_BEGIN namespace io {
#  define FT_IO_NAMESPACE_END   } FT_NAMESPACE_END

#  define FT_ARCH_NAMESPACE_BEGIN FT_NAMESPACE_BEGIN namespace arch {
#  define FT_ARCH_NAMESPACE_END   } FT_NAMESPACE_END

#  define FT_NS              ft::
#  define FT_IO_NS           FT_NS io::
#  define FT_ARCH_NS         FT_NS arch::
#else

#  define FT_NAMESPACE_BEGIN
#  define FT_NAMESPACE_END

#  define FT_IO_NAMESPACE_BEGIN
#  define FT_IO_NAMESPACE_END

#  define FT_ARCH_NAMESPACE_BEGIN
#  define FT_ARCH_NAMESPACE_END

#  define FT_NS
#  define FT_IO_NS
#  define FT_ARCH_NS

#endif /* FT_HAVE_NAMESPACE */


// list of types we want to instantiate ft_work<T> with
#define FT_TYPE_LIST(ft_prefix, ft_macro) \
      ft_macro(ft_prefix, ft_uint) \
      ft_macro(ft_prefix, ft_uoff)

/*
 * does compiler supports
 *   extern template class Foo<T>;
 *   extern template int foo<T>(T bar);
 * and
 *   template class Foo<T>;
 *   template int foo<T>(T bar);
 * to explicitly control template instantiation?
 */
#ifdef FT_HAVE_EXTERN_TEMPLATE

#  define FT_TEMPLATE_DECLARE(ft_macro)     FT_TYPE_LIST(extern template, ft_macro)
#  define FT_TEMPLATE_INSTANTIATE(ft_macro) FT_TYPE_LIST(       template, ft_macro)

#else /* !defined(FT_HAVE_EXTERN_TEMPLATE) */

#  define FT_TEMPLATE_DECLARE(ft_macro)
#  define FT_TEMPLATE_INSTANTIATE(ft_macro)

#endif /* FT_HAVE_EXTERN_TEMPLATE */

#endif /* FSTRANSFORM_FEATURES_HH */
