/**
 * @file    metadbsrvrangecontext.cpp
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
 * metadbsrvrangecontext.cpp
 *
 *  Created on: Dec 28, 2017
 *      Author: Jan Lisowiec
 */

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <chrono>
#include <boost/random.hpp>

extern "C" {
#include "soeutil.h"
}

#include "soelogger.hpp"
using namespace SoeLogger;

#include <rocksdb/cache.h>
#include <rocksdb/compaction_filter.h>
#include <rocksdb/slice.h>
#include <rocksdb/table.h>
#include <rocksdb/utilities/options_util.h>
#include <rocksdb/db.h>
#include <rocksdb/utilities/backupable_db.h>
#include <rocksdb/merge_operator.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/stackable_db.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/utilities/checkpoint.h>

#include "soe_bind_api.hpp"
#include "rcsdbargdefs.hpp"
#include "rcsdbidgener.hpp"
#include "rcsdbfacadeapi.hpp"
#include "rcsdbfacadebase.hpp"
#include "rcsdbconfigbase.hpp"
#include "rcsdbconfig.hpp"
#include "rcsdbfilters.hpp"

#include "metadbsrvrcsdbname.hpp"
#include "metadbsrvrcsdbsnapshot.hpp"
#include "metadbsrvrcsdbstorebackup.hpp"
#include "metadbsrvrcsdbstoreiterator.hpp"
#include "metadbsrvrangecontext.hpp"
#include "metadbsrvrcsdbstore.hpp"

using namespace std;

namespace Metadbsrv {

RangeContext::RangeContext(RangeContext::eType _type)
    :type(_type)
{}

RangeContext::~RangeContext()
{}

RangeContext::eType RangeContext::Type()
{
    return type;
}

DeleteRangeContext::DeleteRangeContext(RangeContext::eType _type)
    :RangeContext(_type),
    dir_forward(true),
    start_key_empty(false),
    end_key_empty(false),
    idx(Store::invalid_iterator)
{
    ::memset(key_pref_buf, '\0', sizeof(key_pref_buf));
}

DeleteRangeContext::DeleteRangeContext(Store &_store,
    RangeContext::eType _type,
    const Slice &_start_key,
    const Slice &_end_key,
    const Slice &_key_pref,
    bool _dir_forward,
    uint32_t _i)
        :RangeContext(_type),
        dir_forward(_dir_forward),
        start_key_empty(false),
        end_key_empty(false),
        idx(_i)
{
    ::memset(key_pref_buf, '\0', sizeof(key_pref_buf));

    ::memcpy(start_key_buf, _start_key.data(), MIN(_start_key.size(), sizeof(start_key_buf) - 1));
    start_key_buf[MIN(_start_key.size(), sizeof(start_key_buf) - 1)] = '\0';
    start_key = Slice(start_key_buf, MIN(_start_key.size(), sizeof(start_key_buf) - 1));

    ::memcpy(end_key_buf, _end_key.data(), MIN(_end_key.size(), sizeof(end_key_buf) - 1));
    end_key_buf[MIN(_end_key.size(), sizeof(end_key_buf) - 1)] = '\0';
    end_key = Slice(end_key_buf, MIN(_end_key.size(), sizeof(end_key_buf)));

    ::memcpy(key_pref_buf, _key_pref.data(), MIN(_key_pref.size(), sizeof(key_pref_buf) - 1));
    key_pref_buf[MIN(_key_pref.size(), sizeof(key_pref_buf) - 1)] = '\0';
    key_pref = Slice(key_pref_buf, MIN(_key_pref.size(), sizeof(key_pref_buf) - 1));

    if ( _type == RangeContext::eType::eRangeContextDeleteSubset ) {
        if ( !key_pref.compare(start_key) ) {
            start_key_empty = true;
        }

        if ( !key_pref.compare(end_key) ) {
            end_key_empty = true;
        }
    }
}

DeleteRangeContext::~DeleteRangeContext()
{
    for ( uint32_t ii = 0 ; ii < keys.size() ; ii++ ) {
        delete [] keys[ii].data();
    }
}

bool DeleteRangeContext::DirForward()
{
    return dir_forward;
}

uint32_t DeleteRangeContext::Idx()
{
    return idx;
}

bool DeleteRangeContext::InRange(Slice &key, bool check_validity)
{
    if ( check_validity == true && ValidKey(key) == false ) {
        return false;
    }
    if ( (StartKeyEmpty() == false && start_key.compare(key) > 0) || (EndKeyEmpty() == false && end_key.compare(key) < 0) ) {
        SOESTORE_DEBUG_RANGE_CONTEXT(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " #### 1 key " << key.data() << " s_k_empty " << start_key_empty << " e_k_empty " << end_key_empty << SoeLogger::LogDebug() << endl);
        return false;
    }
    SOESTORE_DEBUG_RANGE_CONTEXT(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " #### 2 key " << key.data() << " s_k_empty " << start_key_empty << " e_k_empty " << end_key_empty << SoeLogger::LogDebug() << endl);
    return true;
}

bool DeleteRangeContext::StartKeyEmpty()
{
    if ( type == RangeContext::eType::eRangeContextDeleteSubset ) {
        return start_key_empty;
    }
    return start_key.empty();
}

bool DeleteRangeContext::EndKeyEmpty()
{
    if ( type == RangeContext::eType::eRangeContextDeleteSubset ) {
        return end_key_empty;
    }
    return end_key.empty();
}

bool DeleteRangeContext::ValidKey(Slice &key)
{
    if ( type == RangeContext::eRangeContextDeleteSubset ) {
        if ( key.size() < key_pref.size() ) {
            return false;
        }
        Slice key_pref_part(key.data(), key_pref.size());
        if ( key_pref.compare(key_pref_part) ) {
            return false;
        }
    }
    return true;
}

void DeleteRangeContext::AddKey(const Slice &key)
{
    keys.push_back(key);
}

void DeleteRangeContext::AddValue(const Slice &value)
{}

void DeleteRangeContext::ExecuteRange(Store &_store)
{
    for ( uint32_t ii = 0 ; ii < keys.size() ; ii++ ) {
        _store.DeletePr(keys[ii]);
    }
}



GetRangeContext::GetRangeContext(RangeContext::eType _type)
    :RangeContext(_type)
{}

uint32_t GetRangeContext::Idx()
{
    return Store::invalid_range_context;
}

void GetRangeContext::ExecuteRange(Store &_store)
{}


PutRangeContext::PutRangeContext(RangeContext::eType _type)
    :RangeContext(_type)
{}

uint32_t PutRangeContext::Idx()
{
    return Store::invalid_range_context;
}

void PutRangeContext::ExecuteRange(Store &_store)
{}



MergeRangeContext::MergeRangeContext(RangeContext::eType _type)
    :RangeContext(_type)
{}

uint32_t MergeRangeContext::Idx()
{
    return Store::invalid_range_context;
}

void MergeRangeContext::ExecuteRange(Store &_store)
{}


//
// range callbacks
//
bool PutRangeContext::PutStoreRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context)
{
    return false;
}

bool GetRangeContext::GetStoreRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context)
{
    return false;
}

bool DeleteRangeContext::DeleteStoreRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context)
{
    if ( !context ) {
        return false;
    }
    DeleteRangeContext *del_ctx = reinterpret_cast<DeleteRangeContext *>(context);
    if ( !del_ctx || del_ctx->Type() != RangeContext::eType::eRangeContextDeleteContainer ) {
        return false;
    }

    if ( Slice(key, key_len).compare(del_ctx->StartKey()) >= 0  && Slice(key, key_len).compare(del_ctx->EndKey()) <= 0 ) {
        char *new_key = new char[key_len + 1];
        ::memcpy(new_key, key, key_len);
        new_key[key_len] = '\0';
        del_ctx->AddKey(Slice(new_key, key_len));
    }
    return true;
}

bool MergeRangeContext::MergeStoreRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context)
{
    return false;
}



bool PutRangeContext::PutSubsetRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context)
{
    return false;
}

bool GetRangeContext::GetSubsetRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context)
{
    return false;
}

bool DeleteRangeContext::DeleteSubsetRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context)
{
    if ( !context ) {
        return false;
    }
    DeleteRangeContext *del_ctx = reinterpret_cast<DeleteRangeContext *>(context);
    if ( !del_ctx || del_ctx->Type() != RangeContext::eType::eRangeContextDeleteSubset ) {
        return false;
    }

    if ( del_ctx->StartKeyEmpty() == true || del_ctx->EndKeyEmpty() == true ) {
        if ( key_len >= del_ctx->KeyPref().size() && !memcmp(key, del_ctx->KeyPref().data(), del_ctx->KeyPref().size()) ) {
            char *new_key = new char[key_len + 1];
            ::memcpy(new_key, key, key_len);
            new_key[key_len] = '\0';
            del_ctx->AddKey(Slice(new_key, key_len));
        }
    } else {
        if ( key_len >= del_ctx->KeyPref().size() && !memcmp(key, del_ctx->KeyPref().data(), del_ctx->KeyPref().size()) &&
            Slice(key + del_ctx->KeyPref().size(), key_len - del_ctx->KeyPref().size()).compare(del_ctx->StartKey()) >= 0 &&
            Slice(key + del_ctx->KeyPref().size(), key_len - del_ctx->KeyPref().size()).compare(del_ctx->EndKey()) <= 0 ) {
            char *new_key = new char[key_len + 1];
            ::memcpy(new_key, key, key_len);
            new_key[key_len] = '\0';
            del_ctx->AddKey(Slice(new_key, key_len));
        }
    }
    return true;
}

bool MergeRangeContext::MergeSubsetRangeCallback(const char *key, size_t key_len, const char *data, size_t data_len, void *context)
{
    return false;
}

}



