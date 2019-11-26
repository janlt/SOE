/*
 * test_providers.cpp
 *
 *  Created on: Feb 3, 2017
 *      Author: Jan Lisowiec
 */

#include <string>
#include <vector>

#include <string.h>
#include <dlfcn.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <new>

#include "soe_soe.hpp"

using namespace std;
using namespace Soe;

int main(int argc, char **argv)
{
    std::vector<std::pair<std::string, ProviderCapabilities> > provs;
    Soe::Provider::EnumProviders(provs);

    std::cout << std::endl << std::endl << std::flush;

    for ( auto it = provs.begin() ; it != provs.end() ; it++ ) {
        std::cout << "service_name: " << it->first  << std::endl << std::flush;
        for ( auto it1 = it->second.capabilities.begin() ; it1 != it->second.capabilities.end() ; it1++ ) {
            std::cout << "    Capability: " << *it1 << std::endl << std::flush;
        }
        std::cout << "ProviderCapabilities: " << it->second.create_name_fun("TESTROOT", "TEST_CLUSTER", "TEST_SPACE", 0, "TEST_CONTAINER");
        std::cout << std::endl << std::flush;
    }

    return 0;
}

