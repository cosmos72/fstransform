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
 * ui/ui.hh
 *
 *  Created on: Mar 23, 2011
 *      Author: max
 */

#ifndef FSREMAP_UI_UI_HH
#define FSREMAP_UI_UI_HH

#include "../types.hh"   // for ft_uoff
#include "../fwd.hh"     // for fr_vector<T> forward declaration
#include "../extent.hh"  // for fr_dir

FT_UI_NAMESPACE_BEGIN

class fr_ui
{
private:
    /** cannot call copy constructor */
    fr_ui(const fr_ui &);

    /** cannot call assignment operator */
    const fr_ui & operator=(const fr_ui &);

protected:
    /** default constructor */
    fr_ui();

public:
    /** destructor */
    virtual ~fr_ui();

    virtual int start(FT_IO_NS fr_io * io) = 0;

    virtual void show_io_read(fr_from from, ft_uoff offset, ft_uoff length) = 0;

    virtual void show_io_write(fr_to to, ft_uoff offset, ft_uoff length) = 0;

    virtual void show_io_copy(fr_dir dir, ft_uoff from_physical, ft_uoff to_physical, ft_uoff length) = 0;

    virtual void show_io_flush() = 0;
};


FT_UI_NAMESPACE_END

#endif /* FSREMAP_UI_TTY_HH */
