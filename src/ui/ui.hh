/*
 * ui/ui.hh
 *
 *  Created on: Mar 23, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_UI_UI_HH
#define FSTRANSFORM_UI_UI_HH

#include "../types.hh"   // for ft_uoff
#include "../fwd.hh"     // for ft_vector<T> forward declaration
#include "../extent.hh"  // for ft_dir

FT_UI_NAMESPACE_BEGIN

class ft_ui
{
private:
    /** cannot call copy constructor */
    ft_ui(const ft_ui &);

    /** cannot call assignment operator */
    const ft_ui & operator=(const ft_ui &);

protected:
    /** default constructor */
    ft_ui();

public:
    /** destructor */
    virtual ~ft_ui();

    virtual int start(FT_IO_NS ft_io * io) = 0;

    virtual void show_io_read(ft_from from, ft_uoff offset, ft_uoff length) = 0;

    virtual void show_io_write(ft_to to, ft_uoff offset, ft_uoff length) = 0;

    virtual void show_io_copy(ft_dir dir, ft_uoff from_physical, ft_uoff to_physical, ft_uoff length) = 0;

    virtual void show_io_flush() = 0;
};


FT_UI_NAMESPACE_END

#endif /* FSTRANSFORM_UI_TTY_HH */
