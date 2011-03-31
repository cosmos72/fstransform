/*
 * gui/tty.hh
 *
 *  Created on: Mar 23, 2011
 *      Author: max
 */

#ifndef FSTRANSFORM_UI_UI_TTY_HH
#define FSTRANSFORM_UI_UI_TTY_HH

#include <cstdio>

#include "../fwd.hh"     // for ft_io
#include "../types.hh"   // for ft_uint
#include "ui.hh"         // for ft_ui

FT_UI_NAMESPACE_BEGIN

class ft_ui_tty : public ft_ui
{
private:
    typedef ft_ui super_type;

    struct ft_tty_window {
        ft_uoff len;
        ft_uint h0, h;

        ft_tty_window();
    };
    ft_tty_window this_dev, this_storage;
    ft_uint this_w, this_h; /*< tty width and height */
    FILE * this_file;
    bool need_clr;

    /** cannot call copy constructor */
    ft_ui_tty(const ft_ui_tty &);

    /** cannot call assignment operator */
    const ft_ui_tty & operator=(const ft_ui_tty &);

    void show_io_op(bool is_write, const ft_tty_window & window, ft_uoff offset, ft_uoff length);

public:
    /** default constructor */
    ft_ui_tty();

    /** destructor */
    virtual ~ft_ui_tty();

    int init(const char * tty_name);

    virtual int start(FT_IO_NS ft_io * io);

    virtual void show_io_read(ft_from from, ft_uoff offset, ft_uoff length);

    virtual void show_io_write(ft_to to, ft_uoff offset, ft_uoff length);

    virtual void show_io_copy(ft_dir dir, ft_uoff from_physical, ft_uoff to_physical, ft_uoff length);

    virtual void show_io_flush();
};


FT_UI_NAMESPACE_END

#endif /* FSTRANSFORM_UI_UI_TTY_HH */
