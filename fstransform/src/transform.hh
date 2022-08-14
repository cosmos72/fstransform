/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 2 of the License, or
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
 * move.hh
 *
 *  Created on: Feb 29, 2016
 *      Author: max
 */

#ifndef FSTRANSFORM_TRANSFORM_HH
#define FSTRANSFORM_TRANSFORM_HH

FT_NAMESPACE_BEGIN

/**
 * class doing the core of recursive move work.
 */
class ft_transform
{
private:
    /** true if usage() or version() was called. */
    bool quit_immediately;

    /** cannot call copy constructor */
    ft_transform(const ft_transform &);

    /** cannot call assignment operator */
    const ft_transform & operator=(const ft_transform &);

    /** display command-line usage to stdout and return 0 */
    int usage(const char * program_name);

    /** output version information and return 0 */
    int version();

    static int invalid_cmdline(const char * program_name, int err, const char * fmt, ...);

    /** return EISCONN if transformer is initialized, else call quit() and return 0 */
    int check_is_closed();

    /** return 0 if transformer is initialized, else call quit() and return ENOTCONN */
    int check_is_open();

    /** initialize transformer */
    int init();

    /** core of transformation algorithm */
    int transform();

    /** show progress status and E.T.A. */
    void show_progress();

public:
    /** default constructor */
    ft_transform();

    /** destructor. calls quit() */
    ~ft_transform();

    /**
     * high-level do-everything method. calls in sequence init(), run() and quit().
     * return 0 if success, else error.
     */
    static int main(int argc, char ** argv);

    bool is_initialized() const;

    /**
     * parse command line and initialize all subsystems (job, I/O, log...)
     * return 0 if success, else error.
     *
     * implementation: parse command line, fill a ft_args and call init(const ft_args &)
     */
    int init(int argc, char const* const* argv);

    /**
     * initialize all subsystems (log...) using specified arguments
     */
    int init(const ft_args & args);

    /**
     * transformation algorithm entry point. calls transform()
     */
    int run();

    /**
     * performs cleanup. called by destructor, you can also call it explicitly after (or instead of) run().
     */
    void quit();
};


FT_NAMESPACE_END


#endif /* FSTRANSFORM_TRANSFORM_HH */
