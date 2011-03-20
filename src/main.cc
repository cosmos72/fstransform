/*
 * main.cc
 *
 *  Created on: Mar 1, 2011
 *      Author: max
 */
#include "first.hh"

#include "transform.hh"    // for ft_transform

#include "map.hh"
#include "work.hh"


#if 0

FT_NAMESPACE_BEGIN
template<typename T>
static int test() {
	ft_map<T> map;
	map.insert(3, 3, 1, FC_DEFAULT_USER_DATA);
	map.insert(0, 0, 3, FC_DEFAULT_USER_DATA);
	ft_work<T>::show("test map", "", 1, map, FC_INFO);
	return 0;
}
FT_NAMESPACE_END

#endif /* 0 */

int main(int argc, char ** argv) {
	return FT_NS ft_transform::main(argc, argv);
	// return FT_NS test<ft_uint>();
}


