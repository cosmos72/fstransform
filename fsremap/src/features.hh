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
 * features.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_FEATURES_HH
#define FSTRANSFORM_FEATURES_HH

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

#  define FT_ARCH_NAMESPACE_BEGIN FT_NAMESPACE_BEGIN namespace arch {
#  define FT_ARCH_NAMESPACE_END   } FT_NAMESPACE_END

#  define FT_IO_NAMESPACE_BEGIN FT_NAMESPACE_BEGIN namespace io {
#  define FT_IO_NAMESPACE_END   } FT_NAMESPACE_END

#  define FT_UI_NAMESPACE_BEGIN FT_NAMESPACE_BEGIN namespace ui {
#  define FT_UI_NAMESPACE_END   } FT_NAMESPACE_END


#  define FT_NS              ft::
#  define FT_ARCH_NS         FT_NS arch::
#  define FT_IO_NS           FT_NS io::
#  define FT_UI_NS           FT_NS ui::

#else

#  define FT_NAMESPACE_BEGIN
#  define FT_NAMESPACE_END

#  define FT_ARCH_NAMESPACE_BEGIN
#  define FT_ARCH_NAMESPACE_END

#  define FT_IO_NAMESPACE_BEGIN
#  define FT_IO_NAMESPACE_END

#  define FT_UI_NAMESPACE_BEGIN
#  define FT_UI_NAMESPACE_END



#  define FT_NS
#  define FT_ARCH_NS
#  define FT_IO_NS
#  define FT_UI_NS

#endif /* FT_HAVE_NAMESPACE */


// list of types we want to instantiate fr_work<T> with
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
