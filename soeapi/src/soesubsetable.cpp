/**
 * @file    soesubsetable.cpp
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
 * soesubsetbase.cpp
 *
 *  Created on: Jun 8, 2017
 *      Author: Jan Lisowiec
 */

#include <stdint.h>
#include <string.h>
#include <dlfcn.h>
#include <stdio.h>

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

namespace Soe {

Subsetable::Subsetable(shared_ptr<Session> _sess, Subsetable::eSubsetType _type) throw(runtime_error)
    :type(_type),
    sess(_sess),
    last_status(Session::Status::eOk)
{
    this_subs = shared_ptr<Subsetable>(this, Subsetable::Deleter());
}

Subsetable::~Subsetable()
{}

int Subsetable::MakeKey(const char *pref,
    size_t pref_len,
    const char *key,
    size_t key_len,
    char *key_buf,
    size_t key_buf_len,
    size_t *key_ret_len)
{
    if ( pref_len + key_len >= key_buf_len ) {
        return -1;
    }
    ::memcpy(key_buf, pref, pref_len);
    ::memcpy(&key_buf[pref_len], key, key_len);
    key_buf[pref_len + key_len] = '\0';
    if ( key_ret_len ) {
        *key_ret_len = pref_len + key_len;
    }
    return 0;
}

}
