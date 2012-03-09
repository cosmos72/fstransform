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
 * move.hh
 *
 *  Created on: Aug 18, 2011
 *      Author: max
 */

#ifndef FSMOVE_MOVE_HH
#define FSMOVE_MOVE_HH

#include "args.hh"        // for fm_args
#include "fwd.hh"         // for fm_io forward declaration
#include "log.hh"         // for FC_TRACE


FT_NAMESPACE_BEGIN

/**
 * class doing the core of recursive move work.
 */
class fm_move
{
private:
    FT_IO_NS fm_io * this_io;

    /** cannot call copy constructor */
    fm_move(const fm_move &);

    /** cannot call assignment operator */
    const fm_move & operator=(const fm_move &);

    static int invalid_cmdline(const char * program_name, int err, const char * fmt, ...);

    /** return EISCONN if remapper is initialized, else call quit_io() and return 0 */
    int check_is_closed();

    /** return 0 if remapper is initialized, else call quit_io() and return ENOTCONN */
    int check_is_open();

    /**
     * checks that I/O is open.
     * if success, stores a reference to I/O object.
     */
    int init(FT_IO_NS fm_io & io);

    /** core of recursive move algorithm, actually moves the whole source tree into target */
    int move();

    /** show progress status and E.T.A. */
    void show_progress();

public:
    /** default constructor */
    fm_move();

    /** destructor. calls quit() */
    ~fm_move();

    /**
     * high-level do-everything method. calls in sequence init(), run() and cleanup().
     * return 0 if success, else error.
     */
    static int main(int argc, char ** argv);

    /** print command-line usage to stdout and return 0 */
    int usage(const char * program_name);

    bool is_initialized() const;

    /**
     * parse command line and initialize all subsystems (job, I/O, log...)
     * return 0 if success, else error.
     *
     * implementation: parse command line, fill a fr_args and call init(const fr_args &)
     */
    int init(int argc, char const* const* argv);

    /**
     * initialize all subsystems (job, I/O, log...) using specified arguments
     * return 0 if success, else error.
     */
    int init(const fm_args & args);

    /**
     * main recursive move algorithm.
     * calls in sequence analyze() and move()
     */
    int run();

    /**
     * performs cleanup. called by destructor, you can also call it explicitly after (or instead of) run().
     * closes configured I/O and delete it
     */
    void quit();
};


FT_NAMESPACE_END


#endif /* FSMOVE_MOVE_HH */
