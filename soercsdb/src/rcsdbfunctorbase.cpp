/*
 * rcsdbfunctorbase.cpp
 *
 *  Created on: Aug 3, 2017
 *      Author: Jan Lisowiec
 */


#include <sys/types.h>

#include <vector>
#include <map>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "soe_bind_api.hpp"
#include "rcsdbfunctorbase.hpp"

using namespace boost;

namespace RcsdbFacade {

FunctorBase::FunctorBase()
    :num_functors(0)
{
    for ( unsigned int i = 0 ; i < max_functors ; i++ ) {
        functors[i] = 0;
    }
}

FunctorBase::~FunctorBase()
{
    for ( ; num_functors ; num_functors-- ) {
        unsigned i = num_functors - 1;
        if ( functors[i] ) {
            delete functors[i];
            functors[i] = 0;
        }
    }
}

int FunctorBase::Register(ApiBasePtr fun)
{
    if ( num_functors < max_functors ) {
        functors[num_functors++] = fun;
        return 0;
    }

    return -1;
}

int FunctorBase::Deregister(ApiBasePtr fun)
{
    for ( unsigned int i = 0 ; i < num_functors ; i++ ) {
        if ( functors[i] == fun ) {
            delete functors[i];
            functors[i] = 0;
            return 0;
        }
    }

    return -1;
}

void FunctorBase::DeregisterAll()
{
    for ( ; num_functors ; num_functors-- ) {
        unsigned i = num_functors - 1;
        if ( functors[i] ) {
            delete functors[i];
            functors[i] = 0;
        }
    }
}

}


