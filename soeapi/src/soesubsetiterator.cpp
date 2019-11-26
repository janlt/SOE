/**
 * @file    soesubsetiterator.cpp
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
 * soesubsetiterator.cpp
 *
 *  Created on: Jun 12, 2017
 *      Author: sysprog
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

SubsetIterator::SubsetIterator(shared_ptr<Session> _sess,
        shared_ptr<Subsetable> _subs,
        uint32_t _it,
        Iterator::eIteratorDir _dir)
    :Iterator(_it, Iterator::eIteratorTypeSubset),
    sess(_sess),
    subset(_subs),
    dir(_dir)
{
    sess->InitDesc(args);
    args.iterator_idx = iterator_no;
    args.overwrite_dups = (dir == Iterator::eIteratorDir::eIteratorDirForward ? LD_Iterator_Forward : LD_Iterator_Backward);
    args.cluster_idx = static_cast<uint32_t>(type);   // overloaded field

    sess->InitDesc(status_args);
    status_args.iterator_idx = iterator_no;
    status_args.overwrite_dups = (dir == Iterator::eIteratorDir::eIteratorDirForward ? LD_Iterator_Forward : LD_Iterator_Backward);
    status_args.cluster_idx = static_cast<uint32_t>(type);   // overloaded field
}

SubsetIterator::~SubsetIterator()
{
    iterator_no = Iterator::invalid_iterator;
}

Iterator::eStatus SubsetIterator::TranslateStatus(Session::Status s_sts)
{
    Iterator::eStatus i_sts = Iterator::eStatus::eAborted;
    session_status = static_cast<int>(s_sts);

    switch ( s_sts ) {
    case Session::eOk:
        i_sts = Iterator::eStatus::eOk;
        break;
    case Session::eDuplicateKey:
        break;
    case Session::eNotFoundKey:
        i_sts = Iterator::eStatus::eEndKey;
        break;
    case Session::eTruncatedValue:
        i_sts = Iterator::eStatus::eTruncatedValue;
        break;
    case Session::eStoreCorruption:
    case Session::eOpNotSupported:
    case Session::eInvalidArgument:
        i_sts = Iterator::eStatus::eInvalidArg;
        break;
    case Session::eIOError:
    case Session::eMergeInProgress:
    case Session::eIncomplete:
    case Session::eShutdownInProgress:
    case Session::eOpTimeout:
    case Session::eOpAborted:
    case Session::eStoreBusy:
    case Session::eItemExpired:
    case Session::eTryAgain:
    case Session::eInternalError:
        i_sts = Iterator::eStatus::eInvalidIter;
        break;
    case Session::eOsError:
    case Session::eOsResourceExhausted:
    case Session::eProviderResourceExhausted:
    case Session::eFoobar:
    case Session::eInvalidStoreState:
    case Session::eProviderUnreachable:
    case Session::eChannelError:
    case Session::eNetworkError:
    case Session::eNoFunction:
    case Session::eInvalidProvider:
        break;
    default:
        break;
    }

    return i_sts;
}

/**
 * @brief SessionIterator First
 *
 * Set the iterator at first position from [start_key - end_key] range, or [key - end_key] range,
 * See Session::CreateIteartor for return status
 * Iterator may get invalidated if a value is written or updated that falls within itearator's range
 * User sall always check the validity of iterator via Valid() function
 *
 * @param[in]  key            Key (default - 0)
 * @param[in]  key_size       Key length (default - 0)
 * @param[out] status         return status (see soesessioniterator.hpp)
 */
Iterator::eStatus SubsetIterator::First(const char *key, size_t key_size) throw(runtime_error)
{
    char it_key[1024];
    size_t it_key_size = 0;

    int ret = subset->CreateKey(key, key_size, it_key, sizeof(it_key), &it_key_size);
    if ( ret ) {
        return TranslateStatus(Session::Status::eTruncatedValue);
    }

    args.key = it_key;
    args.key_len = it_key_size;
    args.sync_mode = LD_Iterator_First; // overloaded field
    args.buf = 0;
    args.buf_len = 0;
    args.data = 0;
    args.data_len = 0;

    return TranslateStatus(sess->Iterate(args));
}

/**
 * @brief SessionIterator Next
 *
 * Advance to next position
 * See Session::CreateIteartor for return status
 * Iterator may get invalidated if a value is written or updated that falls within itearator's range
 * User sall always check the validity of iterator via Valid() function
 *
 * @param[out] status         return status (see soesessioniterator.hpp)
 */
Iterator::eStatus SubsetIterator::Next() throw(runtime_error)
{
    args.sync_mode = LD_Iterator_Next; // overloaded field
    args.buf = 0;
    args.buf_len = 0;
    args.data = 0;
    args.data_len = 0;

    return TranslateStatus(sess->Iterate(args));
}

/**
 * @brief SessionIterator Last
 *
 * Set the iterator at last position
 * Iterator may get invalidated if a value is written or updated that falls within itearator's range
 * User sall always check the validity of iterator via Valid() function
 *
 * @param[out] status         return status (see soesessioniterator.hpp)
 */
Iterator::eStatus SubsetIterator::Last() throw(runtime_error)
{
    args.sync_mode = LD_Iterator_Last; // overloaded field
    args.buf = 0;
    args.buf_len = 0;
    args.data = 0;
    args.data_len = 0;

    return TranslateStatus(sess->Iterate(args));
}

/**
 * @brief SessionIterator Valid
 *
 * Get iterator's validity
 * Iterator may get invalidated if a value is written or updated that falls within itearator's range
 * User sall always check the validity of iterator via Valid() function
 *
 * @param[out] bool         return value (invalid - false)
 */
bool SubsetIterator::Valid() throw(runtime_error)
{
    status_args.sync_mode = LD_Iterator_IsValid; // overloaded field
    status_args.data = 0;
    status_args.data_len = 0;
    status_args.buf = 0;
    status_args.buf_len = 0;
    sess->Iterate(status_args);

    return status_args.transaction_db; // overloaded field
}

/**
 * @brief SessionIterator Key
 *
 * Get key duple at current position
 * Iterator may get invalidated if a value is written or updated that falls within itearator's range
 * User sall always check the validity of iterator via Valid() function
 *
 * @param[out] Duple         return key
 */
Duple SubsetIterator::Key() const
{
    size_t key_n_size;
    const char *key_n = subset->GetKeyName(key_n_size);
    if ( args.key_len < key_n_size || memcmp(args.key, key_n, key_n_size) ) {
        return Duple(0, 0);
    }
    return Duple(args.key + key_n_size, args.key_len - key_n_size); // overloaded fields
}

/**
 * @brief SessionIterator Value
 *
 * Get value duple at current position
 * Iterator may get invalidated if a value is written or updated that falls within itearator's range
 * User sall always check the validity of iterator via Valid() function
 *
 * @param[out] Duple         return value
 */
Duple SubsetIterator::Value() const
{
    return Duple(args.buf, args.buf_len);
}

}
