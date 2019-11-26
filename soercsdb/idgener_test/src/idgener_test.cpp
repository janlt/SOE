/*
 * idgener_test.cpp
 *
 *  Created on: Apr 19, 2018
 *      Author: Jan Lisowiec
 */


#include <chrono>
#include <fstream>
#include <iostream>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/random.hpp>

#include "soeexternc.h"
#include <rcsdbidgener.hpp>

using namespace RcsdbFacade;
using namespace std;
using namespace std::chrono;

/* to make the linker hapy */
extern "C" LZCFG *cfg_open(const char *config_file) { return 0; }
extern "C" void cfg_close(LZCFG *cfg_struct) {}

int main()
{
    system_clock::time_point start = system_clock::now();
    MonotonicId64Gener m64gen(start.time_since_epoch().count());

    for ( int i = 0 ; i < 100 ; i++ ) {
        cout << "next 64 id " << m64gen.GetId() << endl;
    }

    RandomUniformInt64Gener mIntgen(static_cast<int64_t>(start.time_since_epoch().count()), 1, 100);

    for ( int i = 0 ; i < 100 ; i++ ) {
        cout << "next rand id " << mIntgen.GetId() << endl;
    }

    return 0;
}
