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
 * main.cc
 *  Created on: Aug 18, 2011
 *      Author: max
 */
#include "first.hh"

#undef FM_TEST_

#if defined(FM_TEST_)

#include "rope/rope_test.hh"

/* main for test programs here... */
int FM_MAIN(int argc, char ** argv) {
	return FT_NS rope_test() ? 0 : 1;
}

#else /* actual fsmove program */

# include "move.hh"    // for fm_move
# define FM_MAIN(argc, argv) FT_NS fm_move::main(argc, argv)

#endif /* defined(FT_TEST_) */




int main(int argc, char ** argv) {
    return FM_MAIN(argc, argv);
}
