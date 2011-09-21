/*
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
