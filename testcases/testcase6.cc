#include "cache.h"
#include <iostream>
#include <stdlib.h>
#include <sstream>

#define KB 1024

using namespace std;

/* Test case for cache simulator */

/* DO NOT MODIFY */

int main(int argc, char **argv){

    cache *mycache = NULL;

    for (unsigned n=16; n<=256; n=n*2){

        cout << "ASSOCIATIVITY = " << dec << n << endl;
        cout << "===================";//<< endl << endl;

        mycache = new cache(     n*KB,			//size
                                 1,			//associativity
                                 64,			//cache line size
                                 WRITE_THROUGH,		//write hit policy
                                 NO_WRITE_ALLOCATE, 	//write miss policy
                                 5, 			//hit time
                                 100, 			//miss penalty
                                 48    		//address width
        );

        mycache->print_configuration();

        mycache->load_trace("traces/GCC.t");

        mycache->run();

        cout << endl;

        mycache->print_statistics();

        cout << endl;

        delete mycache;

    }

}
