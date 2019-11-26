/**
 * @file    soesubsetscapi.cpp
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
 * soesubsetscapi.cpp
 *
 *  Created on: Jun 25, 2017
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

SOE_EXTERNC_BEGIN

/**
 * @brief C API
 *
 * Put KV pair (item) in subset associated with session/store
 * If a key doesn't exist it'll be created, if it does it'll be updated
 *
 * @param[in]  s_hnd          Subset handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[in]  data           Data
 * @param[in]  data_size      Data length
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus DisjointSubsetPut(SubsetableHND s_hnd, const char *key, size_t key_size, const char *data, size_t data_size)
{
    Soe::DisjointSubset *obj = reinterpret_cast<Soe::DisjointSubset *>(s_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->Put(key,
            key_size,
            data,
            data_size)));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get KV pair (item) from subset associated with session/store
 * If a key doesn't exist an error will be returned
 *
 * @param[in]  s_hnd          Subset handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus DisjointSubsetGet(SubsetableHND s_hnd, const char *key, size_t key_size, char *buf, size_t *buf_size)
{
    Soe::DisjointSubset *obj = reinterpret_cast<Soe::DisjointSubset *>(s_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->Get(key,
            key_size,
            buf,
            buf_size)));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Merge KV pair (item) in subset associated with session/store
 * If a key doesn't exist it'll be created, if it does it'll be merged
 *
 * @param[in]  s_hnd          Subset handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[in]  data           Data
 * @param[in]  data_size      Data length
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus DisjointSubsetMerge(SubsetableHND s_hnd, const char *key, size_t key_size, const char *data, size_t data_size)
{
    Soe::DisjointSubset *obj = reinterpret_cast<Soe::DisjointSubset *>(s_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->Merge(key,
            key_size,
            data,
            data_size)));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Delete KV pair (item) from a subset associated with session/store
 * If a key doesn't exist an error will be returned
 *
 * @param[in]  s_hnd          Subset handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus DisjointSubsetDelete(SubsetableHND s_hnd, const char *key, size_t key_size)
{
    Soe::DisjointSubset *obj = reinterpret_cast<Soe::DisjointSubset *>(s_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->Delete(key,
            key_size)));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get a set of KV pairs (items) from subset whose keys are passed in as a vector
 *
 * @param[in]  s_hnd                 Subset handle (see soecapi.hpp)
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_nfound  Specifies whether or not to fail the function on the first
 *                                   not found key
 * @param[in,out]  fail_pos          Position within items with not found key that caused a failure
 * @param[out] status                Return status (see soesession.hpp)
 */
SessionStatus DisjointSubsetGetSet(SubsetableHND s_hnd, CDPairVector *items, SoeBool fail_on_first_nfound, size_t *fail_pos)
{
    Soe::DisjointSubset *obj = reinterpret_cast<Soe::DisjointSubset *>(s_hnd);
    if ( !obj || !items ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    std::vector<std::pair<Soe::Duple, Soe::Duple>> cpp_items;
    int ret = Soe::CDPVectorToCPPDPVector(items, &cpp_items);
    if ( ret ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        Soe::Session::Status sts = obj->GetSet(cpp_items,
            static_cast<bool>(fail_on_first_nfound),
            fail_pos);

        int ret = Soe::CPPDPVectorToCDPVector(&cpp_items, items, false);
        if ( ret ) {
            return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
        }

        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(sts));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Set a set of KV pairs (items) in subset whose keys are passed in as a vector
 *
 * @param[in]  s_hnd                 Subset handle (see soecapi.hpp)
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_dup     Specifies whether or not to fail the function on the first
 *                                   duplicate key
 * @param[in,out]  fail_pos          Position within items with not found key that caused a failure
 * @param[out] status                Return status (see soesession.hpp)
 */
SessionStatus DisjointSubsetPutSet(SubsetableHND s_hnd, CDPairVector *items, SoeBool fail_on_first_dup, size_t *fail_pos)
{
    Soe::DisjointSubset *obj = reinterpret_cast<Soe::DisjointSubset *>(s_hnd);
    if ( !obj || !items ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    std::vector<std::pair<Soe::Duple, Soe::Duple>> cpp_items;
    int ret = Soe::CDPVectorToCPPDPVector(items, &cpp_items);
    if ( ret ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        Soe::Session::Status sts = obj->PutSet(cpp_items,
            static_cast<bool>(fail_on_first_dup),
            fail_pos);

        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(sts));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Delete a set of KV pairs (items) from subset whose keys are passed in as a vector
 *
 * @param[in]  s_hnd                 Subset handle (see soecapi.hpp)
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_nfound  Specifies whether or not to fail the function on the first
 *                                   not found key
 * @param[in,out]  fail_pos          Position within items with not found key that caused a failure
 * @param[out] status                Return status (see soesession.hpp)
 */
SessionStatus DisjointSubsetDeleteSet(SubsetableHND s_hnd, CDVector *keys, SoeBool fail_on_first_nfound, size_t *fail_pos)
{
    Soe::DisjointSubset *obj = reinterpret_cast<Soe::DisjointSubset *>(s_hnd);
    if ( !obj || !keys ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    std::vector<Soe::Duple> cpp_keys;
    int ret = Soe::CDVectorToCPPDVector(keys, &cpp_keys);
    if ( ret ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        Soe::Session::Status sts = obj->DeleteSet(cpp_keys,
            static_cast<bool>(fail_on_first_nfound),
            fail_pos);

        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(sts));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Merge a set of KV pairs (items) in subset whose keys are passed in as a vector
 *
 * @param[in]  s_hnd                 Subset handle (see soecapi.hpp)
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[out] status                Return status (see soesession.hpp)
 */
SessionStatus DisjointSubsetMergeSet(SubsetableHND s_hnd, CDPairVector *items)
{
    Soe::DisjointSubset *obj = reinterpret_cast<Soe::DisjointSubset *>(s_hnd);
    if ( !obj || !items ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    std::vector<std::pair<Soe::Duple, Soe::Duple>> cpp_items;
    int ret = Soe::CDPVectorToCPPDPVector(items, &cpp_items);
    if ( ret ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        Soe::Session::Status sts = obj->MergeSet(cpp_items);

        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(sts));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Create subset iterator for KV pair traversal
 * Iterators are not writable or updateable
 *
 * @param[in]  s_hnd           Subset handle (see soecapi.hpp)
 * @param[in]  dir             Traversal direction, i.e. first-to-last or last-to-first
 * @param[in]  start_key       Start key
 * @param[in]  start_key_size  Start key length
 * @param[in]  end_key         End key
 * @param[in]  end_key_size    End key length
 * @param[out] IteratorHND     Return iterator handle (see soecapi.hpp)
 *
 */
IteratorHND DisjointSubsetCreateIterator(SubsetableHND s_hnd,
    eIteratorDir dir,
    const char *start_key,
    size_t start_key_size,
    const char *end_key,
    size_t end_key_size)
{
    Soe::Subsetable *obj = reinterpret_cast<Soe::Subsetable *>(s_hnd);
    if ( !obj ) {
        return 0;
    }
    try {
        return reinterpret_cast<IteratorHND>(obj->CreateIterator(static_cast<Soe::Iterator::eIteratorDir>(Translate_C_CPP_IteratorDir(dir)),
            start_key,
            start_key_size,
            end_key,
            end_key_size));
    } catch ( ... ) {
        return 0;
    }
}

/**
 * @brief C API
 *
 * Destroy subset iterator for KV pair traversal
 * Iterators are not writable or updateable
 *
 * @param[in]  s_hnd           Subset handle (see soecapi.hpp)
 * @param[in]  dir             Traversal direction, i.e. first-to-last or last-to-first
 * @param[in]  start_key       Start key
 * @param[in]  start_key_size  Start key length
 * @param[in]  end_key         End key
 * @param[in]  end_key_size    End key length
 * @param[out] IteratorHND     Return iterator handle (see soecapi.hpp)
 *
 */
SessionStatus DisjointSubsetDestroyIterator(SubsetableHND s_hnd, IteratorHND it)
{
    Soe::DisjointSubset *obj = reinterpret_cast<Soe::DisjointSubset *>(s_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->DestroyIterator(reinterpret_cast<Soe::Iterator *>(it))));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

SOE_EXTERNC_END
