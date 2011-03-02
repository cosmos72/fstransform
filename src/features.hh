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

#  ifdef FT_HAVE_LONG_LONG
#    define FT_EXTERN_TEMPLATE_LIST(ft_extern_template_prefix, ft_class_or_function_macro) \
       ft_extern_template_prefix ft_class_or_function_macro(unsigned short) \
       ft_extern_template_prefix ft_class_or_function_macro(unsigned int) \
       ft_extern_template_prefix ft_class_or_function_macro(unsigned long) \
       ft_extern_template_prefix ft_class_or_function_macro(unsigned long long)
#  else
#    define FT_EXTERN_TEMPLATE_LIST(ft_extern_template_prefix, ft_class_or_function_macro) \
       ft_extern_template_prefix ft_class_or_function_macro(unsigned short) \
       ft_extern_template_prefix ft_class_or_function_macro(unsigned int) \
       ft_extern_template_prefix ft_class_or_function_macro(unsigned long)
#  endif

#  define FT_EXTERN_TEMPLATE_DECLARE(ft_class_or_function_macro_list)     ft_class_or_function_macro_list(extern template, FT_EXTERN_TEMPLATE_LIST)
#  define FT_EXTERN_TEMPLATE_INSTANTIATE(ft_class_or_function_macro_list) ft_class_or_function_macro_list(       template, FT_EXTERN_TEMPLATE_LIST)

#else /* !defined(FT_HAVE_EXTERN_TEMPLATE) */
#  define FT_EXTERN_TEMPLATE_LIST(ft_extern_template_prefix, ft_extern_template_macro)
#  define FT_EXTERN_TEMPLATE_DECLARE(ft_class_or_function_macro)
#  define FT_EXTERN_TEMPLATE_INSTANTIATE(ft_class_or_function_macro)
#endif /* FT_HAVE_EXTERN_TEMPLATE */

#endif /* FSTRANSFORM_FEATURES_HH */
