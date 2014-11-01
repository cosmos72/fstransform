/*
 * fsattr - modify file-system internal data structures
 *
 * Copyright (C) 2012 Massimiliano Ghilardi
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
 * main.cc
 *  Created on: Apr 15, 2012
 *      Author: max
 */
#include "first.hh"

# include "e4attr.hh"    // for e4attr_main()
# define FA_MAIN(argc, argv) FT_NS e4attr_main(argc, argv)

int main(int argc, char ** argv) {
    return FA_MAIN(argc, argv);
}
