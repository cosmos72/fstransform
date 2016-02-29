/*
 * fstransform - modify file-system internal data structures
 *
 * Copyright (C) 2016 Massimiliano Ghilardi
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
 *  Created on: Feb 29, 2016
 *      Author: max
 */
#include "first.hh"
#include "transform.hh" // for ft_transform

#define FT_MAIN(argc, argv) FT_NS ft_transform::main(argc, argv)

int main(int argc, char ** argv) {
    return FT_MAIN(argc, argv);
}
