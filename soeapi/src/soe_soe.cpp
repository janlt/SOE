/**
 * @file    soe_soe.cpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe Session API
 *
 *
 * @section Copyright
 *
 *   Copyright Notice:
 *
 *    Copyright 2014-2019 Jan Lisowiec jlisowiec@gmail.com,
 *                   
 *
 *    This product is free software; you can redistribute it and/or,
 *    modify it under the terms of the GNU Library General Public
 *    License as published by the Free Software Foundation; either
 *    version 2 of the License, or (at your option) any later version.
 *    This software is distributed in the hope that it will be useful.
 *
 *
 * @section Licence
 *
 *   GPL 2 or later
 *
 *
 * @section Description
 *
 *   soe_soe
 *
 * @section Source
 *
 *    
 *    
 *
 */

/*
 * soe_soe.cpp
 *
 *  Created on: Jan 16, 2017
 *      Author: Jan Lisowiec
 */

#include <stdint.h>
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

#include "soecapi.hpp"
#include "soe_soe.hpp"

using namespace std;
using namespace KvsFacade;
using namespace RcsdbFacade;

namespace Soe {

/*
 * @brief Convert C vector of CDuples to C++ vector
 */
int CDPVectorToCPPDPVector(const CDPairVector *c_items, vector<pair<Soe::Duple, Soe::Duple>> *cpp_items)
{
    if ( !c_items || !cpp_items ) {
        return -1;
    }

    for ( size_t i = 0 ; i < c_items->size ; i++ ) {
        CDuple *cd_key = &c_items->vector[i].pair[0];
        CDuple *cd_value = &c_items->vector[i].pair[1];

        Soe::Duple k_d(cd_key->data_ptr, cd_key->data_size);
        Soe::Duple v_d(cd_value->data_ptr, cd_value->data_size);

        cpp_items->push_back(pair<Soe::Duple, Soe::Duple>(k_d, v_d));
    }

    return 0;
}

/*
 * @brief Convert C++ vector of Duples to C vector
 */
int CPPDPVectorToCDPVector(const vector<pair<Soe::Duple, Soe::Duple>> *cpp_items, CDPairVector *c_items, bool alloc_c_vector)
{
    if ( !c_items || !cpp_items ) {
        return -1;
    }

    if ( alloc_c_vector == true ) {
        CDuplePair *pair_vector = new CDuplePair[cpp_items->size()];
        c_items->vector = pair_vector;
        c_items->size = cpp_items->size();
    } else {
        if ( !c_items->vector || !c_items->size ) {
            return -1;
        }
    }

    size_t i = 0;

    for ( vector<pair<Soe::Duple, Soe::Duple>>::const_iterator it = cpp_items->begin() ; it != cpp_items->end() ; ++it ) {
        CDuple *cd_key = &c_items->vector[i].pair[0];
        CDuple *cd_value = &c_items->vector[i].pair[1];

        cd_key->data_ptr = (*it).first.data();
        cd_key->data_size = (*it).first.size();
        cd_value->data_ptr = (*it).second.data();
        cd_value->data_size = (*it).second.size();

        i++;
    }

    return 0;
}

int CDVectorToCPPDVector(const CDVector *c_items, vector<Soe::Duple> *cpp_items)
{
    if ( !c_items || !cpp_items ) {
        return -1;
    }

    for ( size_t i = 0 ; i < c_items->size ; i++ ) {
        CDuple *cd_key = &c_items->vector[i];

        Soe::Duple k_d(cd_key->data_ptr, cd_key->data_size);

        cpp_items->push_back(Soe::Duple(k_d));
    }

    return 0;
}

int CPDPVectorToCDVector(const vector<Soe::Duple> *cpp_items, CDVector *c_items, bool alloc_c_vector)
{
    if ( !c_items || !cpp_items ) {
        return -1;
    }

    if ( alloc_c_vector == true ) {
        CDuple *duple_vector = new CDuple[cpp_items->size()];
        c_items->vector = duple_vector;
        c_items->size = cpp_items->size();
    } else {
        if ( !c_items->vector || !c_items->size ) {
            return -1;
        }
    }

    size_t i = 0;

    for ( vector<Soe::Duple>::const_iterator it = cpp_items->begin() ; it != cpp_items->end() ; ++it ) {
        CDuple *cd_key = &c_items->vector[i];

        cd_key->data_ptr = (*it).data();
        cd_key->data_size = (*it).size();

        i++;
    }

    return 0;
}



}
