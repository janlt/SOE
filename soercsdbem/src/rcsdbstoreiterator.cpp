/**
 * @file    rcsdbstoreiterator.cpp
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
 * rcsdbstoreiterator.cpp
 *
 *  Created on: Jul 13, 2017
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
#include "rcsdbfacadeapi.hpp"
#include "rcsdbfacadebase.hpp"
#include "rcsdbfunctorbase.hpp"
#include "rcsdbconfigbase.hpp"
#include "rcsdbconfig.hpp"
#include "rcsdbfilters.hpp"
#include "rcsdbfacade.hpp"
#include "rcsdbsnapshot.hpp"
#include "rcsdbstorebackup.hpp"
#include "rcsdbstoreiterator.hpp"
#include "rcsdbstore.hpp"

using namespace std;
using namespace RcsdbFacade;

namespace RcsdbLocalFacade {

StoreIterator::StoreIterator(StoreIterator::eType _type)
    :rcsdb_iter(0),
     dir_forward(true),
     start_key_empty(false),
     end_key_empty(false),
     id(Store::invalid_iterator),
     idx(Store::invalid_iterator),
     initialized(false),
     ended(false),
     type(_type)
{
    ::memset(key_pref_buf, '\0', sizeof(key_pref_buf));
}

StoreIterator::StoreIterator(Store &_store,
    StoreIterator::eType _type,
    const Slice &_start_key,
    const Slice &_end_key,
    const Slice &_key_pref,
    bool _dir_forward,
    ReadOptions &_read_options,
    uint32_t _i)
        :rcsdb_iter(0),
         dir_forward(true),
         start_key_empty(false),
         end_key_empty(false),
         id(Store::invalid_iterator),
         idx(Store::invalid_iterator),
         initialized(false),
         ended(false),
         type(_type)
{
    ::memset(key_pref_buf, '\0', sizeof(key_pref_buf));

    rcsdb_iter.reset(_store.rcsdb->NewIterator(_read_options));

    ::memcpy(start_key_buf, _start_key.data(), _start_key.size());
    start_key_buf[_start_key.size()] = '\0';
    start_key = Slice(start_key_buf, _start_key.size());

    ::memcpy(end_key_buf, _end_key.data(), _end_key.size());
    end_key_buf[_end_key.size()] = '\0';
    end_key = Slice(end_key_buf, _end_key.size());

    ::memcpy(key_pref_buf, _key_pref.data(), _key_pref.size());
    key_pref_buf[_key_pref.size()] = '\0';
    key_pref = Slice(key_pref_buf, _key_pref.size());

    dir_forward = _dir_forward;
    idx = _i;
    id = Store::iterator_start_id + _i;

    if ( _type == StoreIterator::eType::eStoreIteratorTypeSubset ) {
        if ( !key_pref.compare(start_key) ) {
            start_key_empty = true;
        }

        if ( !key_pref.compare(end_key) ) {
            end_key_empty = true;
        }
    }
}

StoreIterator::~StoreIterator()
{
    rcsdb_iter.reset();
}

void StoreIterator::SeekForNext(Slice &_key)
{
    rcsdb_iter->SeekForPrev(_key);
    if ( rcsdb_iter->Valid() == false ) {
        rcsdb_iter->SeekToFirst();
    }
    for ( ; rcsdb_iter->Valid() == true &&
        rcsdb_iter->key().compare(_key) < 0 ;
        rcsdb_iter->Next() ) {
        SOESTORE_DEBUG_ITER(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " v: " << rcsdb_iter->Valid() << " key " << rcsdb_iter->key().data() << " v " << rcsdb_iter->Valid() << SoeLogger::LogDebug() << endl);
    }
}

void StoreIterator::SeekForPrev(Slice &_key)
{
    rcsdb_iter->SeekForPrev(_key);
}

void StoreIterator::SeekToFirst()
{
    rcsdb_iter->SeekToFirst();
}

void StoreIterator::SeekToLast()
{
    rcsdb_iter->SeekToLast();
}

void StoreIterator::Seek(Slice &_key)
{
    rcsdb_iter->Seek(_key);
}

void StoreIterator::Next()
{
    rcsdb_iter->Next();
}

void StoreIterator::Prev()
{
    rcsdb_iter->Prev();
}

bool StoreIterator::Valid()
{
    return rcsdb_iter->Valid();
}

Slice StoreIterator::Key()
{
    return rcsdb_iter->key();
}

Slice StoreIterator::Value()
{
    return rcsdb_iter->value();
}

bool StoreIterator::Ended()
{
    return ended;
}

void StoreIterator::SetEnded()
{
    ended = true;
}

StoreIterator::eType StoreIterator::Type()
{
    return type;
}

bool StoreIterator::DirForward()
{
    return dir_forward;
}

uint32_t StoreIterator::Idx()
{
    return idx;
}

bool StoreIterator::Initialized()
{
    return initialized;
}

void StoreIterator::SetInitialized()
{
    initialized = true;
}

bool StoreIterator::InRange(Slice &key, bool check_validity)
{
    if ( check_validity == true && ValidKey(key) == false ) {
        return false;
    }
    if ( (StartKeyEmpty() == false && start_key.compare(key) > 0) || (EndKeyEmpty() == false && end_key.compare(key) < 0) ) {
        SOESTORE_DEBUG_ITER(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " #### 1 key " << key.data() << " s_k_empty " << start_key_empty << " e_k_empty " << end_key_empty << SoeLogger::LogDebug() << endl);
        return false;
    }
    SOESTORE_DEBUG_ITER(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " #### 2 key " << key.data() << " s_k_empty " << start_key_empty << " e_k_empty " << end_key_empty << SoeLogger::LogDebug() << endl);
    return true;
}

bool StoreIterator::StartKeyEmpty()
{
    if ( type == StoreIterator::eType::eStoreIteratorTypeSubset ) {
        return start_key_empty;
    }
    return start_key.empty();
}

bool StoreIterator::EndKeyEmpty()
{
    if ( type == StoreIterator::eType::eStoreIteratorTypeSubset ) {
        return end_key_empty;
    }
    return end_key.empty();
}

bool StoreIterator::ValidKey(Slice &key)
{
    if ( type == StoreIterator::eStoreIteratorTypeSubset ) {
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

}
