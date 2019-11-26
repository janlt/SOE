/**
 * @file    soesessioncapi.cpp
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
 * soesessioncapi.cpp
 *
 *  Created on: Jun 23, 2017
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
 * Create Session
 *
 * @param[in]  c_n            Cluster name
 * @param[in]  s_n            Space name
 * @param[in]  cn_n           Container (store) name
 * @param[in]  pr             Provider name
 * @param[in]  transaction_db Transaction store ( "true" - transactions supported, "false" - transactions not supported)
 * @param[in]  debug          Print debug info
 * @param[in]  s_m            Sync mode (see soesession.hpp)
 * @param[out] hndl           Handle (see soecapi.hpp)
 */
SessionHND SessionNew(const char *c_n,
        const char *s_n,
        const char *cn_n,
        const char *pr,
        SoeBool _transation_db,
        SoeBool _debug,
        eSessionSyncMode s_m)
{
    std::string prov;
    if ( !pr || !*pr ) {
        prov = Soe::Provider::default_provider;
    } else {
        prov = pr;
    }

    try {
        return reinterpret_cast<SessionHND>(new Soe::Session(c_n,
            s_n,
            cn_n,
            prov,
            static_cast<bool>(_transation_db),
            static_cast<bool>(_debug),
            true,
            static_cast<Soe::Session::eSyncMode>(Translate_C_CPP_SessionSyncMode(s_m))));
    } catch ( ... ) {
        return 0;
    }
}

/**
 * @brief C API
 *
 * Open Session
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[in]  c_n            Cluster name
 * @param[in]  s_n            Space name
 * @param[in]  cn_n           Container (store) name
 * @param[in]  pr             Provider name
 * @param[in]  transaction_db Transaction store ( "true" - transactions supported, "false" - transactions not supported)
 * @param[in]  debug          Print debug info
 * @param[in]  s_m            Sync mode (see soesession.hpp)
 * @param[out] status         Status (see soecapi.hpp)
 */
SessionStatus SessionOpen(SessionHND s_hnd,
    const char *c_n,
    const char *s_n,
    const char *cn_n,
    const char *pr,
    SoeBool _transation_db,
    SoeBool _debug,
    eSessionSyncMode s_m)
{
    std::string prov;
    if ( !pr || !*pr ) {
        prov = Soe::Provider::default_provider;
    } else {
        prov = pr;
    }

    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->Open(c_n,
            s_n,
            cn_n,
            prov,
            static_cast<bool>(_transation_db),
            static_cast<bool>(_debug),
            true,
            static_cast<Soe::Session::eSyncMode>(Translate_C_CPP_SessionSyncMode(s_m)))));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get Session Status
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[out] status         Return status of the most recent operation (see soecapi.hpp)
 */
SessionStatus SessionLastStatus(SessionHND s_hnd)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    return static_cast<SessionStatus>(obj->GetLastStatus());
}

/**
 * @brief C API
 *
 * Check Session
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[in]  sts            Session status pointer
 * @param[out] status         Return check code (see soecapi.hpp)
 */
SoeBool SessionOk(SessionHND s_hnd, SessionStatus *sts)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return false;
    }
    return static_cast<SoeBool>(obj->ok());
}

/**
 * @brief C API
 *
 * Create Session from a handle, will create physical store
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus SessionCreate(SessionHND s_hnd)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->Create()));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Destroy Session using a handle along with the underlying Store
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus SessionDestroyStore(SessionHND s_hnd)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->DestroyStore()));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * List all the clusters for a given provider
 *
 * @param[in]      s_hnd          Session handle (see soecapi.hpp)
 * @param[in/out]  clusters       Clusters that will be filled on return
 * @param[out]     status         Return status (see soecapi.hpp)
 */
SessionStatus SessionListClusters(SessionHND s_hnd, CDStringUin64Vector *clusters)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !clusters ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        std::vector<std::pair<uint64_t, std::string> > cpp_clusters;
        SessionStatus sts = static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->ListClusters(cpp_clusters)));

        clusters->size = cpp_clusters.size();
        clusters->vector = reinterpret_cast<CStringUint64 *>(::malloc(clusters->size*sizeof(CStringUint64)));

        for ( size_t idx = 0 ; idx < cpp_clusters.size() ; idx++ ) {
            clusters->vector[idx].num = cpp_clusters[idx].first;
            char *c_name = reinterpret_cast<char *>(::malloc(cpp_clusters[idx].second.size() + 1));
            strcpy(c_name, cpp_clusters[idx].second.c_str());
            c_name[cpp_clusters[idx].second.size()] = 0;
            CDuple c_item;
            c_item.data_ptr = c_name;
            c_item.data_size = cpp_clusters[idx].second.size();
            clusters->vector[idx].string = c_item;
        }

        return sts;
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * List all the spaces for a given cluster
 *
 * @param[in]      s_hnd          Session handle (see soecapi.hpp)
 * @param[in]      c_n            Cluster name
 * @param[in]      c_id           Cluster id
 * @param[in/out]  spaces         Spaces that will be filled on return
 * @param[out]     status         Return status (see soecapi.hpp)
 */
SessionStatus SessionListSpaces(SessionHND s_hnd, const char *c_n, uint64_t c_id, CDStringUin64Vector *spaces)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !spaces ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        std::vector<std::pair<uint64_t, std::string> > cpp_spaces;
        SessionStatus sts = static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->ListSpaces(c_n, c_id, cpp_spaces)));

        spaces->size = cpp_spaces.size();
        spaces->vector = reinterpret_cast<CStringUint64 *>(::malloc(spaces->size*sizeof(CStringUint64)));

        for ( size_t idx = 0 ; idx < cpp_spaces.size() ; idx++ ) {
            spaces->vector[idx].num = cpp_spaces[idx].first;
            char *c_name = reinterpret_cast<char *>(::malloc(cpp_spaces[idx].second.size() + 1));
            strcpy(c_name, cpp_spaces[idx].second.c_str());
            c_name[cpp_spaces[idx].second.size()] = 0;
            CDuple c_item;
            c_item.data_ptr = c_name;
            c_item.data_size = cpp_spaces[idx].second.size();
            spaces->vector[idx].string = c_item;
        }

        return sts;
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * List all the stores for a given cluster and space
 *
 * @param[in]      s_hnd          Session handle (see soecapi.hpp)
 * @param[in]      c_n            Cluster name
 * @param[in]      c_id           Cluster id
 * @param[in]      s_n            Space name
 * @param[in]      s_id           Space id
 * @param[in/out]  stores         Stores that will be filled on return
 * @param[out]     status         Return status (see soecapi.hpp)
 */
SessionStatus SessionListStores(SessionHND s_hnd, const char *c_n, uint64_t c_id, const char *s_n, uint64_t s_id, CDStringUin64Vector *stores)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !stores ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        std::vector<std::pair<uint64_t, std::string> > cpp_stores;
        SessionStatus sts = static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->ListStores(c_n, c_id, s_n, s_id, cpp_stores)));

        stores->size = cpp_stores.size();
        stores->vector = reinterpret_cast<CStringUint64 *>(::malloc(stores->size*sizeof(CStringUint64)));

        for ( size_t idx = 0 ; idx < cpp_stores.size() ; idx++ ) {
            stores->vector[idx].num = cpp_stores[idx].first;
            char *c_name = reinterpret_cast<char *>(::malloc(cpp_stores[idx].second.size() + 1));
            strcpy(c_name, cpp_stores[idx].second.c_str());
            c_name[cpp_stores[idx].second.size()] = 0;
            CDuple c_item;
            c_item.data_ptr = c_name;
            c_item.data_size = cpp_stores[idx].second.size();
            stores->vector[idx].string = c_item;
        }

        return sts;
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * List all the subsets for a given cluster, space and store
 *
 * @param[in]      s_hnd          Session handle (see soecapi.hpp)
 * @param[in]      c_n            Cluster name
 * @param[in]      c_id           Cluster id
 * @param[in]      s_n            Space name
 * @param[in]      s_id           Space id
 * @param[in]      t_n            Store name
 * @param[in]      t_id           Store id
 * @param[in/out]  stores         Stores that will be filled on return
 * @param[out]     status         Return status (see soecapi.hpp)
 */
SessionStatus SessionListSubsets(SessionHND s_hnd, const char *c_n, uint64_t c_id, const char *s_n, uint64_t s_id, const char *t_n, uint64_t t_id, CDStringUin64Vector *subsets)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !subsets ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        std::vector<std::pair<uint64_t, std::string> > cpp_subsets;
        SessionStatus sts = static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->ListSubsets(c_n, c_id, s_n, s_id, t_n, t_id, cpp_subsets)));

        subsets->size = cpp_subsets.size();
        subsets->vector = reinterpret_cast<CStringUint64 *>(::malloc(subsets->size*sizeof(CStringUint64)));

        for ( size_t idx = 0 ; idx < cpp_subsets.size() ; idx++ ) {
            subsets->vector[idx].num = cpp_subsets[idx].first;
            char *c_name = reinterpret_cast<char *>(::malloc(cpp_subsets[idx].second.size() + 1));
            strcpy(c_name, cpp_subsets[idx].second.c_str());
            c_name[cpp_subsets[idx].second.size()] = 0;
            CDuple c_item;
            c_item.data_ptr = c_name;
            c_item.data_size = cpp_subsets[idx].second.size();
            subsets->vector[idx].string = c_item;
        }

        return sts;
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Put KV pair (item) in store associated with session
 * If a key doesn't exist it'll be created, if it does it'll be updated
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[in]  data           Data
 * @param[in]  data_size      Data length
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus SessionPut(SessionHND s_hnd, const char *key, size_t key_size, const char *data, size_t data_size)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
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
 * Get KV pair (item) from store associated with session
 * If a key doesn't exist an error will be returned
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[in]  buf            Buffer for data, in case of eTruncatedValue return status will
 *                            be filled in with portion of the value since some applications can work with that
 * @param[in]  buf_size       Buffer size pointer
 *                            It'll be set the the length of the value when the return status is either eOk or eTruncatedValue
 *                            The caller has to check the return status and decide if it' ok to proceed with truncated value
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus SessionGet(SessionHND s_hnd, const char *key, size_t key_size, char *buf, size_t *buf_size)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
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
 * Delete KV pair (item) from a store associated with session
 * If a key doesn't exist an error will be returned
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus SessionDelete(SessionHND s_hnd, const char *key, size_t key_size)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
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
 * Merge KV pair (item) in store associated with session
 * If a key doesn't exist it'll be created, if it does it'll be merged
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[in]  data           Data
 * @param[in]  data_size      Data length
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus SessionMerge(SessionHND s_hnd, const char *key, size_t key_size, const char *data, size_t data_size)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
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
 * Get a range of KV pairs (item) from store associated with session
 * CapiRangeCallback will be called on each key falling with the range.
 * If the callback returns "true" the processing continues, if "false"
 * the processing will be aborted.
 * An error will be returned upon data truncation, i.e. if the buffer is too small.
 *
 * @param[in]  s_hnd           Session handle (see soecapi.hpp)
 * @param[in]  start_key       Start key
 * @param[in]  start_key_size  Start key length
 * @param[in]  end_key         End key
 * @param[in]  end_key_size    End key length
 * @param[in]  buf             Buffer for the data for each callback invocation
 * @param[in]  buf_size        Buffer size
 * @param[in]  r_cb            Range callback
 * @param[in]  ctx             Caller's context to be passed around, will be passed to each callback invocation
 * @param[in]  dir             Traversal direction, i.e. first-to-last or last-to-first
 * @param[out] status          Return status (see soecapi.hpp)
 *
 */
SessionStatus SessionGetRange(SessionHND s_hnd,
        const char *start_key,
        size_t start_key_size,
        const char *end_key,
        size_t end_key_size,
        char *buf,
        size_t *buf_size,
        CapiRangeCallback r_cb,
        void *ctx,
        eIteratorDir dir)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->GetRange(start_key,
            start_key_size,
            end_key,
            end_key_size,
            buf,
            buf_size,
            reinterpret_cast<RangeCallback>(r_cb),
            ctx,
            static_cast<Soe::Iterator::eIteratorDir>(Translate_C_CPP_IteratorDir(dir)))));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get a set of KV pairs (items) whose keys are passed in as a vector
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_nfound  Specifies whether or not to fail the function on the first
 *                                   not found key
 * @param[in,out]  fail_pos          Position within items with not found key that caused a failure
 * @param[out] status                Return status (see soesession.hpp)
 *                                   If items.size() == 0 the return status is eInvalidArgument
 *                                   If fail_on_first_nfound == false and items.size() != 0 the return status is eOk
 *                                   if fail_on_first_nfound == true and items.size() != 0 and a key in keys is not found
 *                                   the return status is eNotFoundKey and fail_pos is set to first failed index
 */
SessionStatus SessionGetSet(SessionHND s_hnd, CDPairVector *items, SoeBool fail_on_first_nfound, size_t *fail_pos)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
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
 * Set a set of KV pairs (items) whose keys are passed in as a vector
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_dup     Specifies whether or not to fail the function on the first
 *                                   duplicate key
 * @param[in,out]  fail_pos          Position within items with not found key that caused a failure
 * @param[out] status                Return status (see soesession.hpp)
 *                                   If items.size() == 0 the return status is eInvalidArgument
 *                                   If fail_on_first_nfound == false and items.size() != 0 the return status is eOk
 *                                   if fail_on_first_nfound == true and items.size() != 0 and a key in keys is not found
 *                                   the return status is eDuplicateKey and fail_pos is set to first failed index
 */
SessionStatus SessionPutSet(SessionHND s_hnd, CDPairVector *items, SoeBool fail_on_first_dup, size_t *fail_pos)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
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
 * Delete a set of KV pairs (items) whose keys are passed in as a vector
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_nfound  Specifies whether or not to fail the function on the first
 *                                   not found key
 * @param[in,out]  fail_pos          Position within items with not found key that caused a failure
 * @param[out] status                Return status (see soesession.hpp)
 *                                   If keys.size() == 0 the return status is eInvalidArgument
 *                                   If fail_on_first_nfound == false and keys.size() != 0 the return status is eOk
 *                                   if fail_on_first_nfound == true and keys.size() != 0 and a key in keys is not found
 *                                   the return status is eNotFoundKey and fail_pos is set to first failed index
 */
SessionStatus SessionDeleteSet(SessionHND s_hnd, CDVector *keys, SoeBool fail_on_first_nfound, size_t *fail_pos)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
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
 * Merge a set of KV pairs (items) whose keys are passed in as a vector
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[out] status                Return status (see soesession.hpp)
 *                                   If keys.size() == 0 the return status is eInvalidArgument
 */
SessionStatus SessionMergeSet(SessionHND s_hnd, CDPairVector *items)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
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
 * Start a transaction
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[out] TransactionHND        Return transaction handle (see soesession.hpp)
 */
TransactionHND SessionStartTransaction(SessionHND s_hnd)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return 0;
    }
    try {
        return reinterpret_cast<TransactionHND>(obj->StartTransaction());
    } catch ( ... ) {
        return 0;
    }
}

/**
 * @brief C API
 *
 * Commit transaction
 * Transaction handle will be deleted and it can't be used after a transaction has been
 * committed
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  tr                    Transaction handle
 * @param[out] TransactionHND        Return transaction handle (see soesession.hpp)
 */
SessionStatus SessionCommitTransaction(SessionHND s_hnd, const TransactionHND tr)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !tr ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->CommitTransaction(reinterpret_cast<Soe::Transaction *>(tr))));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Roll back transaction
 * Transaction handle will be deleted and it can't be used after a transaction has been
 * committed
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  tr                    Transaction handle
 * @param[out] TransactionHND        Return transaction handle (see soesession.hpp)
 */
SessionStatus SessionRollbackTransaction(SessionHND s_hnd, const TransactionHND tr)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !tr ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->RollbackTransaction(reinterpret_cast<Soe::Transaction *>(tr))));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Create snapshot engine
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[out] SnapshotEngineHND     Return snapshot engine handle (see soesession.hpp)
 */
SnapshotEngineHND SessionCreateSnapshotEngine(SessionHND s_hnd, const char *name)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return 0;
    }
    try {
        return reinterpret_cast<SnapshotEngineHND>(obj->CreateSnapshotEngine(name));
    } catch ( ... ) {
        return 0;
    }
}

/**
 * @brief C API
 *
 * Destroy snapshot engine
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  en                    Snapshot engine handle
 * @param[out] status                Return status (see soesession.hpp)
 */
SessionStatus SessionDestroySnapshotEngine(SessionHND s_hnd, const SnapshotEngineHND en)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !en ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->DestroySnapshotEngine(reinterpret_cast<Soe::SnapshotEngine *>(en))));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * List snapshots in store
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in,out] list               List of snapshots filled in by the function
 * @param[out] status                Return status (see soesession.hpp)
 */
SessionStatus SessionListSnapshots(SessionHND s_hnd, CDStringUin32Vector *list)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !list ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    std::vector<std::pair<std::string, uint32_t> > cpp_list;
    // TODO

    try {
        Soe::Session::Status sts = obj->ListSnapshots(cpp_list);

        // TODO

        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(sts));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Create backup engine
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  name                  Backup engine's name
 * @param[out] BackupEngineHND       Return backup engine handle (see soesession.hpp)
 */
BackupEngineHND SessionCreateBackupEngine(SessionHND s_hnd, const char *name)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return 0;
    }
    try {
        return reinterpret_cast<BackupEngineHND>(obj->CreateBackupEngine(name));
    } catch ( ... ) {
        return 0;
    }
}

/**
 * @brief C API
 *
 * Destroy backup engine
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  en                    Backup engine handle
 * @param[out] status                Return status (see soesession.hpp)
 */
SessionStatus SessionDestroyBackupEngine(SessionHND s_hnd, const BackupEngineHND en)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !en ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->DestroyBackupEngine(reinterpret_cast<Soe::BackupEngine *>(en))));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * List backups for session
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in,out] list               List of backups filled in by the function
 * @param[out] status                Return status (see soesession.hpp)
 */
SessionStatus SessionListBackups(SessionHND s_hnd, CDStringUin32Vector *list)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !list ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    std::vector<std::pair<std::string, uint32_t> > cpp_list;
    // TODO

    try {
        Soe::Session::Status sts = obj->ListBackups(cpp_list);

        // TODO

        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(sts));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Create group for batching requests
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[out] GroupHND              Return group handle (see soesession.hpp)
 */
GroupHND SessionCreateGroup(SessionHND s_hnd)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return 0;
    }
    try {
        return reinterpret_cast<GroupHND>(obj->CreateGroup());
    } catch ( ... ) {
        return 0;
    }
}

/**
 * @brief C API
 *
 * Destroy group, deletes physically all items in a group from the store
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  gr                    Group handle (see soecapi.hpp)
 * @param[out] status                Return status (see soesession.hpp)
 */
SessionStatus SessionDestroyGroup(SessionHND s_hnd, GroupHND gr)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !gr ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->DestroyGroup(reinterpret_cast<Soe::Group *>(gr))));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Write group, writes all items in a group to the store
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  gr                    Group handle (see soecapi.hpp)
 * @param[out] status                Return status (see soesession.hpp)
 */
SessionStatus SessionWrite(SessionHND s_hnd, const GroupHND gr)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !gr ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->Write(reinterpret_cast<Soe::Group *>(gr))));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Create iterator for KV pair traversal
 * Iterators are not writable or updateable
 *
 * @param[in]  s_hnd           Session handle (see soecapi.hpp)
 * @param[in]  dir             Traversal direction, i.e. first-to-last or last-to-first
 * @param[in]  start_key       Start key
 * @param[in]  start_key_size  Start key length
 * @param[in]  end_key         End key
 * @param[in]  end_key_size    End key length
 * @param[out] IteratorHND     Return iterator handle (see soecapi.hpp)
 *
 */
IteratorHND SessionCreateIterator(SessionHND s_hnd,
    eIteratorDir dir,
    const char *start_key,
    size_t start_key_size,
    const char *end_key,
    size_t end_key_size)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
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
 * Destroy iterator
 *
 * @param[in]  s_hnd           Session handle (see soecapi.hpp)
 * @param[in]  it              Iterator handle
 * @param[out] status          Return status (see soesession.hpp)
 *
 */
SessionStatus SessionDestroyIterator(SessionHND s_hnd, IteratorHND it)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !it ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->DestroyIterator(reinterpret_cast<Soe::Iterator *>(it))));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Create subset
 * If subset exists a error will be returned (duplicate)
 *
 * @param[in]  s_hnd           Session handle (see soecapi.hpp)
 * @param[in]  name            Subset name
 * @param[in]  name_len        Subset name length
 * @param[in]  sb_type         Subset type (see soecapi.hpp)
 * @param[out] SubsetableHND   Return subset handle (see soecapi.hpp)
 *
 */
SubsetableHND SessionCreateSubset(SessionHND s_hnd,
    const char *name,
    size_t name_len,
    eSubsetableSubsetType sb_type)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return 0;
    }
    try {
        return reinterpret_cast<SubsetableHND>(obj->CreateSubset(name,
            name_len,
            static_cast<Soe::Session::eSubsetType>(Translate_C_CPP_SessionSubsetType(sb_type))));
    } catch ( ... ) {
        return 0;
    }
}

/**
 * @brief C API
 *
 * Open subset
 *
 * @param[in]  s_hnd           Session handle (see soecapi.hpp)
 * @param[in]  name            Subset name
 * @param[in]  name_len        Subset name length
 * @param[in]  sb_type         Subset type (see soecapi.hpp)
 * @param[out] SubsetableHND   Return subset handle (see soecapi.hpp)
 *
 */
SubsetableHND SessionOpenSubset(SessionHND s_hnd,
    const char *name,
    size_t name_len,
    eSubsetableSubsetType sb_type)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return 0;
    }
    try {
        return reinterpret_cast<SubsetableHND>(obj->OpenSubset(name,
            name_len,
            static_cast<Soe::Session::eSubsetType>(Translate_C_CPP_SessionSubsetType(sb_type))));
    } catch ( ... ) {
        return 0;
    }
}

/**
 * @brief C API
 *
 * Destroy subset, will physically destroy subset and all its data
 *
 * @param[in]  s_hnd           Session handle (see soecapi.hpp)
 * @param[in]  sb              Subset handle
 * @param[in]  name_len        Subset name length
 * @param[out] status          Return status (see soesession.hpp)
 *
 */
SessionStatus SessionDestroySubset(SessionHND s_hnd, SubsetableHND sb)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !sb ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->DestroySubset(reinterpret_cast<Soe::Subsetable *>(sb))));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Delete subset, will delete subset handle without destroying subset's data
 * NOTE: Subset data can destroyed via Subset API, i.e. without destroying the subset itself
 *
 * @param[in]  s_hnd           Session handle (see soecapi.hpp)
 * @param[in]  sb              Subset handle
 * @param[in]  name_len        Subset name length
 * @param[out] status          Return status (see soesession.hpp)
 *
 */
SessionStatus SessionDeleteSubset(SessionHND s_hnd, SubsetableHND sb)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !sb ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->DeleteSubset(reinterpret_cast<Soe::Subsetable *>(sb))));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Close session
 *
 * @param[in]  s_hnd           Session handle (see soecapi.hpp)
 *
 */
void SessionClose(SessionHND s_hnd)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return;
    }
    delete obj;
}

/**
 * @brief C API
 *
 * Repair store associated with session
 *
 * @param[in]  s_hnd           Session handle (see soecapi.hpp)
 * @param[out] status          Return status (see soesession.hpp)
 *
 */
SessionStatus SessionRepair(SessionHND s_hnd)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->Repair()));
}


SOE_EXTERNC_END
