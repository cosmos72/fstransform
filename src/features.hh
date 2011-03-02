/*
 * features.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_FEATURES_HH
#define FSTRANSFORM_FEATURES_HH

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

#  define FT_NS              ft::
#  define FT_IO_NS           io::
#else

#  define FT_NAMESPACE_BEGIN
#  define FT_NAMESPACE_END

#  define FT_IO_NAMESPACE_BEGIN
#  define FT_IO_NAMESPACE_END

#  define FT_IO_NS
#  define FT_NS

#endif /* FT_HAVE_NAMESPACE */


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

#  define FT_EXTERN_TEMPLATE_LIST(ft_extern_template_prefix, ft_class_or_function_macro) \
     ft_extern_template_prefix ft_class_or_function_macro(ft_u32) \
     ft_extern_template_prefix ft_class_or_function_macro(ft_u64)

#  define FT_EXTERN_TEMPLATE_DECLARE(ft_class_or_function_macro_list)     ft_class_or_function_macro_list(extern template, FT_EXTERN_TEMPLATE_LIST)
#  define FT_EXTERN_TEMPLATE_INSTANTIATE(ft_class_or_function_macro_list) ft_class_or_function_macro_list(       template, FT_EXTERN_TEMPLATE_LIST)

#else /* !defined(FT_HAVE_EXTERN_TEMPLATE) */
#  define FT_EXTERN_TEMPLATE_LIST(ft_extern_template_prefix, ft_extern_template_macro)
#  define FT_EXTERN_TEMPLATE_DECLARE(ft_class_or_function_macro)
#  define FT_EXTERN_TEMPLATE_INSTANTIATE(ft_class_or_function_macro)
#endif /* FT_HAVE_EXTERN_TEMPLATE */

#endif /* FSTRANSFORM_FEATURES_HH */
