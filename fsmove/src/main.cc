/*
 * main.cc
 *
 *  Created on: Aug 18, 2011
 *      Author: max
 */
#include "first.hh"


#ifdef FM_TEST_

/* main for test programs here... */

#else /* actual fsmove program */

# include "move.hh"    // for fm_move
# define FM_MAIN(argc, argv) FT_NS fm_move::main(argc, argv)

#endif /* defined(FT_TEST_) */




int main(int argc, char ** argv) {
    return FM_MAIN(argc, argv);
}
