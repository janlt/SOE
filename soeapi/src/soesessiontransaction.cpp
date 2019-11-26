/**
 * @file    soesessiontransaction.cpp
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
 * soesessiontransaction.cpp
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
// Transaction
//
Transaction::Transaction(shared_ptr<Session> _sess, uint32_t _tr)
    :transaction_no(_tr)
    ,sess(_sess)
{}

Transaction::~Transaction()
{
    transaction_no = Transaction::invalid_transaction;
}

Session::Status Transaction::Put(const char *key, size_t key_size, const char *data, size_t data_size) throw(runtime_error)
{
    LocalArgsDescriptor args;
    sess->InitDesc(args);
    args.key = key;
    args.key_len = key_size;
    args.data = data;
    args.data_len = data_size;
    args.transaction_idx = transaction_no;

    return sess->Put(args);
}

Session::Status Transaction::Get(const char *key, size_t key_size) throw(runtime_error)
{
    LocalArgsDescriptor args;
    sess->InitDesc(args);
    args.key = key;
    args.key_len = key_size;
    args.transaction_idx = transaction_no;

    return sess->Get(args);
}

Session::Status Transaction::Merge(const char *key, size_t key_size, const char *data, size_t data_size) throw(runtime_error)
{
    LocalArgsDescriptor args;
    sess->InitDesc(args);
    args.key = key;
    args.key_len = key_size;
    args.data = data;
    args.data_len = data_size;
    args.transaction_idx = transaction_no;

    return sess->Merge(args);
}

Session::Status Transaction::Delete(const char *key, size_t key_size) throw(runtime_error)
{
    LocalArgsDescriptor args;
    sess->InitDesc(args);
    args.key = key;
    args.key_len = key_size;
    args.transaction_idx = transaction_no;

    return sess->Delete(args);
}

}
