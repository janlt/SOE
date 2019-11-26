/**
 * @file    soefuturable.cpp
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
 * futurable.cpp
 *
 *  Created on: Dec 8, 2017
 *      Author: Jan Lisowiec
 */

#include <stdint.h>

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

namespace Soe {


#ifdef __DEBUG_FUTURES__
Lock Futurable::debug_io_lock;
Lock Futurable::futurables_lock;
std::vector<uint64_t> Futurable::futurables;
#endif

void SignalFuture(RcsdbFacade::FutureSignal *sig)
{
    Futurable *fut = reinterpret_cast<Futurable *>(sig->future);
#ifdef __DEBUG_FUTURES__
    {
        WriteLock io_w_lock(Futurable::futurables_lock);
        uint32_t f_found = 0;
        for ( size_t i = 0 ; i < Futurable::futurables.size() ; i++ ) {
            if ( Futurable::futurables[i] == reinterpret_cast<uint64_t>(fut) ) {
                f_found++;
            }
        }
        if ( f_found != 1 ) {
            std::cerr << __FUNCTION__ << f_found << std::endl;
        }
    }
#endif
    if ( fut->extra_check == Futurable::extra_check_value ) {
        fut->Signal(sig->args);
    }
}

}
