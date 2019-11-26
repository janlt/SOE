/**
 * @file    soesessiongroup.cpp
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
 * soesessiongroup.cpp
 *
 *  Created on: Feb 3, 2017
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

#include "soe_soe.hpp"

using namespace std;
using namespace KvsFacade;
using namespace RcsdbFacade;

namespace Soe {

//
// Group
//
Group::Group(shared_ptr<Session> _sess, uint32_t _gr)
    :group_no(_gr)
    ,sess(_sess)
{
    sess->InitDesc(args);
}

Group::~Group()
{
    group_no = Group::invalid_group;
}

Session::Status Group::Put(const char *key, size_t key_size, const char *data, size_t data_size) throw(runtime_error)
{
    args.key = key;
    args.key_len = key_size;
    args.data = data;
    args.data_len = data_size;
    args.group_idx = group_no;

    return sess->Put(args);
}

Session::Status Group::Merge(const char *key, size_t key_size, const char *data, size_t data_size) throw(runtime_error)
{
    args.key = key;
    args.key_len = key_size;
    args.data = data;
    args.data_len = data_size;
    args.group_idx = group_no;

    return sess->Merge(args);
}

Session::Status Group::Delete(const char *key, size_t key_size) throw(runtime_error)
{
    args.key = key;
    args.key_len = key_size;
    args.group_idx = group_no;

    return sess->Delete(args);
}

}
