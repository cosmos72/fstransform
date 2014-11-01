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
 * traits.hh
 *
 *  Created on: Feb 27, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_TRAITS_HH
#define FSTRANSFORM_TRAITS_HH

#include "check.hh"

FT_NAMESPACE_BEGIN

template<typename T> struct ft_type_traits;
/**
 * whether char is signed or unsigned is implementation dependent.
 * in any case, the compiler treats (char), (unsigned char) and (signed char) as different types
 */
template<> struct ft_type_traits<char>               { typedef unsigned char      unsigned_type; typedef signed char signed_type; };
template<> struct ft_type_traits<signed char>        { typedef unsigned char      unsigned_type; typedef signed char signed_type; };
template<> struct ft_type_traits<unsigned char>      { typedef unsigned char      unsigned_type; typedef signed char signed_type; };
template<> struct ft_type_traits<short>              { typedef unsigned short     unsigned_type; typedef short       signed_type; };
template<> struct ft_type_traits<unsigned short>     { typedef unsigned short     unsigned_type; typedef short       signed_type; };
template<> struct ft_type_traits<int>                { typedef unsigned int       unsigned_type; typedef int         signed_type; };
template<> struct ft_type_traits<unsigned int>       { typedef unsigned int       unsigned_type; typedef int         signed_type; };
template<> struct ft_type_traits<long>               { typedef unsigned long      unsigned_type; typedef long        signed_type; };
template<> struct ft_type_traits<unsigned long>      { typedef unsigned long      unsigned_type; typedef long        signed_type; };

#ifdef FT_HAVE_LONG_LONG
template<> struct ft_type_traits<long long>          { typedef unsigned long long unsigned_type; typedef long long   signed_type; };
template<> struct ft_type_traits<unsigned long long> { typedef unsigned long long unsigned_type; typedef long long   signed_type; };
#endif /* FT_HAVE_LONG_LONG */

#define FT_TYPE_TO_UNSIGNED(T) FT_NS ft_type_traits< T >::unsigned_type
#define FT_TYPE_TO_SIGNED(T)   FT_NS ft_type_traits< T >::signed_type

FT_NAMESPACE_END

#endif /* FSTRANSFORM_TRAITS_HH */
