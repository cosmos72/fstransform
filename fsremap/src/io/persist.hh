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
 * io/persist.hh
 *
 *  Created on: Mar 6, 2011
 *      Author: max
 */

#ifndef FSREMAP_PERSIST_HH
#define FSREMAP_PERSIST_HH

#include "../types.hh"   // for ft_uoff

FT_IO_NAMESPACE_BEGIN

class ft_persist
{
private:
    /** cannot call copy constructor */
    ft_persist(const ft_persist &);

    /** cannot call assignment operator */
    const ft_persist & operator=(const ft_persist &);

public:
    /** default constructor */
    ft_persist();

    /** destructor */
    ~ft_persist();
};


FT_IO_NAMESPACE_END

#endif /* FSREMAP_PERSIST_HH */
