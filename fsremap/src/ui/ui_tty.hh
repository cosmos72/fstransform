/*
 * gui/tty.hh
 *
 *  Created on: Mar 23, 2011
 *      Author: max
 */

#ifndef FSREMAP_UI_UI_TTY_HH
#define FSREMAP_UI_UI_TTY_HH

#include <cstdio>

#include "../fwd.hh"     // for fr_io
#include "../types.hh"   // for ft_uint
#include "ui.hh"         // for fr_ui

FT_UI_NAMESPACE_BEGIN

class fr_ui_tty : public fr_ui
{
private:
    typedef fr_ui super_type;

    struct fr_tty_window {
        ft_uoff len;
        ft_uint h0, h;

        fr_tty_window();
    };
    fr_tty_window this_dev, this_storage;
    ft_uint this_w, this_h; /*< tty width and height */
    FILE * this_file;
    bool need_clr;

    /** cannot call copy constructor */
    fr_ui_tty(const fr_ui_tty &);

    /** cannot call assignment operator */
    const fr_ui_tty & operator=(const fr_ui_tty &);

    void show_io_op(bool is_write, const fr_tty_window & window, ft_uoff offset, ft_uoff length);

public:
    /** default constructor */
    fr_ui_tty();

    /** destructor */
    virtual ~fr_ui_tty();

    int init(const char * tty_name);

    virtual int start(FT_IO_NS fr_io * io);

    virtual void show_io_read(fr_from from, ft_uoff offset, ft_uoff length);

    virtual void show_io_write(fr_to to, ft_uoff offset, ft_uoff length);

    virtual void show_io_copy(fr_dir dir, ft_uoff from_physical, ft_uoff to_physical, ft_uoff length);

    virtual void show_io_flush();
};


FT_UI_NAMESPACE_END

#endif /* FSREMAP_UI_UI_TTY_HH */
