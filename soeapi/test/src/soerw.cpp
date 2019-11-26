/**
 * @file    soerw.cpp
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
 * soerw.cpp
 *
 *  Created on: Jan 24, 2017
 *      Author: Jan Lisowiec
 */


#include <string>
#include <vector>

#ifndef EPOCH_SOE
#define EPOCH_SOE
#endif

#ifdef EPOCH_SOE

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

#include "mainbase.h"

using namespace std;
using namespace Soe;

//char *spo_namespace = default_spo_namespace;
//char *test_cluster_name = default_test_cluster_name;

Soe::Session *session = 0;

struct geometry_t {
    uint32_t blksize;
    uint64_t nblocks;
};

std::string GetProvider(eDbProvider prov)
{
    std::string provider = Provider::default_provider;

    switch ( prov ) {
    case eKvsProvider:
        provider = KvsFacade::GetProvider();
        break;
    case eRcsdbemProvider:
        provider = RcsdbFacade::GetProvider();
        break;
    case eMetadbcliProvider:
        provider = MetadbcliFacade::GetProvider();
        break;
    case ePostgresProvider:
        provider = Provider::default_provider;
        break;
    default:
        provider = Provider::default_provider;
        break;
    }

    return provider;
}
/*
 * ----------------------------------------------------------------------
 */
extern "C" void *soeTestConnect(const char *hostname,
    int port,
    const char *cluster,
    const char *space,
    const char *store,
    eDbProvider prov,
    eSpoTestType con_type,
    bool create,
    int sync_type,
    bool debug)
{
    if ( !hostname ) {
        hostname = "127.0.0.1";
    }

    Session::eSyncMode sync_mode = Session::eSyncMode::eSyncDefault;
    switch ( sync_type ) {
    case 0:
        break;
    case 1:
        sync_mode = Session::eSyncMode::eSyncFsync;
        break;
    case 2:
        sync_mode = Session::eSyncMode::eSyncAsync;
        break;
    default:
        break;
    }

    bool transaction_db_type = false;
    if ( con_type == eSpoConTran || con_type == eSpoCreaTran ) {
        transaction_db_type = true;
    }
    void *retval = 0;

    if ( create != true ) {
        // Try open it
        try {
            if ( session ) {
                delete session;
            }
            session = new Session(cluster,
                space,
                store,
                GetProvider(prov),
                transaction_db_type,
                debug,
                true,
                sync_mode);

            Soe::Session::Status sts = session->Open(cluster,
                space,
                store,
                GetProvider(prov),
                transaction_db_type,
                debug,
                true,
                sync_mode);
            if ( sts == Soe::Session::Status::eOk ) {
                retval = &session;
                LZDBD(DBG, "%s Open cluster name: %s OK\n", __FUNCTION__, cluster);LZFLUSH;
            } else if ( sts == Soe::Session::Status::eStoreCorruption ) {
                retval = session;
                LZDBD(DBG, "%s Open cluster name: %s Corrupted, try to run Recover\n", __FUNCTION__, cluster);LZFLUSH;
            } else {
                retval = 0;
            }
        } catch (const std::runtime_error &ex) {
            retval = 0;
        }
    }

    // If not opened create it
    if ( !retval ) {
        try {
            if ( session ) {
                delete session;
            }
            session = new Session(cluster,
                space,
                store,
                GetProvider(prov),
                transaction_db_type,
                debug,
                true,
                sync_mode);

            Soe::Session::Status sts = session->Create();
            if ( sts == Soe::Session::Status::eOk ) {
                retval = session;
                LZDBD(DBG, "%s Cretae cluster name: %s OK\n", __FUNCTION__, cluster);LZFLUSH;
            } else {
                retval = 0;
            }
        } catch (const std::runtime_error &ex) {
            LZDBD(DBG, "%s Create cluster name: %s failed(%s)\n", __FUNCTION__, cluster, ex.what());LZFLUSH;
            retval = 0;
        }
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" void soeTestDisconnect(void *connection)
{
    if ( session ) {
        session->Close();
    }
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestWriteKVBlock(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t block,
    const unsigned char *buf,
    uint32_t length)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    uint64_t beblockno = htobe64(block);

    try {
        Session::Status sts = session->Merge(reinterpret_cast<const char *>(&beblockno),
            sizeof(beblockno),
            reinterpret_cast<const char *>(buf),
            length);
        if ( sts == Session::Status::eOk ) {
            retval = true;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s cluster name: %s failed(%s)\n", __FUNCTION__, cluster, ex.what());LZFLUSH;
        retval = 0;
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestReadKVBlock(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t block,
    unsigned char *buf,
    uint32_t *length)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    uint64_t beblockno = htobe64(block);

    try {
        Session::Status sts = session->Get(reinterpret_cast<const char *>(&beblockno),
            sizeof(beblockno),
            reinterpret_cast<char *>(buf),
            reinterpret_cast<size_t *>(length));
        if ( sts == Session::Status::eOk ) {
            retval = true;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s cluster name: %s failed(%s)\n", __FUNCTION__, cluster, ex.what());LZFLUSH;
        retval = 0;
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestInitialiseKVDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint32_t blksize,
    uint64_t nblocks)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    geometry_t geometry;

    geometry.blksize = htobe32(blksize);
    geometry.nblocks = htobe64(nblocks);

    try {
        retval = soeTestWriteKVBlock(connection,
            cluster,
            space,
            store,
            -1LL,
            reinterpret_cast<unsigned char *>(&geometry),
            sizeof(geometry));
    } catch (const exception &ex) {
        LZDBD(DBG, "Initialisation error: device(%s): %s\n", store, ex.what());LZFLUSH;
        retval = false;
    }
    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestQueryKVDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint32_t *blksize,
    uint64_t *nblocks)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    geometry_t geometry;
    uint32_t size = sizeof(geometry_t);

    try {
        retval = soeTestReadKVBlock(connection,
            cluster,
            space,
            store,
            -1LL,
            reinterpret_cast<unsigned char *>(&geometry),
            &size);
        if(retval && !size)
            retval = false;
        else
        if(retval && size != sizeof(geometry_t)) {
            LZDBD(DBG, "Device geometry invalid structure size(%u) expected(%lu)\n", size, sizeof(geometry_t));LZFLUSH;
            retval = false;
        }
    } catch (const exception &ex) {
        LZDBD(DBG, "Query error: device(%s): %s\n", store, ex.what());LZFLUSH;
        retval = false;
    }

    if(retval) {
        if(blksize)
            *blksize = be32toh(geometry.blksize);
        if(nblocks)
            *nblocks = be64toh(geometry.nblocks);
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestDeleteKVBlock(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t block)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    uint64_t beblockno = htobe64(block);

    try {
        Session::Status sts = session->Delete(reinterpret_cast<const char *>(&beblockno),
            sizeof(beblockno));
        if ( sts == Session::Status::eOk ) {
            retval = true;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s cluster name: %s failed(%s)\n", __FUNCTION__, cluster, ex.what());LZFLUSH;
        retval = 0;
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestWriteMultipleKVBlocks(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t start,
    uint32_t nblocks,
    const unsigned char *buf,
    uint32_t len)
{
    bool retval = false;

    if (session->ok() != true) {
        return retval;
    }

    Group *group = session->CreateGroup();
    if ( !group ) {
        LZDBD(DBG, "%s CreateGroup failed: device(%s) length(%u)\n", __FUNCTION__, store, len);LZFLUSH;
        return false;
    }

    uint64_t blkaddr[nblocks];
    auto count = 0;
    try {
        for(auto block = start; block < start + nblocks; ++block) {
            blkaddr[block-start] = htobe64(block);
            Session::Status sts = group->Merge(reinterpret_cast<const char *>(&(blkaddr[block-start])),
                sizeof(block),
                reinterpret_cast<const char *>(buf+(len*(block-start))),
                len);
            if ( sts == Session::Status::eOk ) {
                retval = true;
                ++count;
            } else {
                retval = false;
                LZDBD(DBG, "%s Group::Merge failed: device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
                break;
            }
        }
        Session::Status sts = session->Write(group);
        if ( sts == Session::Status::eOk ) {
            retval = true;
        } else {
            LZDBD(DBG, "%s Session::Write failed: device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
            retval = false;
        }
        sts = session->DestroyGroup(group);
        if ( sts == Session::Status::eOk ) {
            retval = true;
        } else {
            LZDBD(DBG, "%s Session::DestroyGroup failed: device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
            retval = false;
        }
    } catch (const exception &ex) {
        LZDBD(DBG, "%s Send error: %s\n", __FUNCTION__, ex.what());LZFLUSH;
        retval = false;
    }

    return retval;
}

typedef struct _ArgStruct {
    unsigned char *buf;
    uint32_t       buf_len;
    uint32_t       nblocks;
    uint32_t       counter;
    uint64_t       blkaddr[10000];
} ArgStruct;

extern "C" bool MultipleKVBlockCalback(const char *key,
    size_t key_len,
    const char *data,
    size_t data_len,
    void *arg)
{
    if ( key_len < sizeof(uint64_t) || !key || !arg ) {
        LZDBD(DBG, "Invalid key or arg %s", __FUNCTION__);
        return false;
    }
    LZDBD(DBG, "%s key(%lu) key_len(%lu) data_len(%lu)\n",
        __FUNCTION__, *reinterpret_cast<const uint64_t *>(key), key_len, data_len);LZFLUSH;

    ArgStruct *arg_struct = reinterpret_cast<ArgStruct *>(arg);
    if ( arg_struct->counter >= sizeof(arg_struct->nblocks) ) {
        return true;
    }

    ::memcpy(&arg_struct->buf[arg_struct->counter*data_len], data, data_len);
    arg_struct->blkaddr[arg_struct->counter] = *reinterpret_cast<const uint64_t *>(key);
    return true;
}


/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestReadMultipleKVBlocks(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    int use_iter,
    uint64_t start,
    uint32_t nblocks,
    unsigned char *buf,
    uint32_t len)
{
    bool retval = false;

    if (session->ok()) {
        if ( !use_iter ) {
            uint64_t blkaddr[nblocks];
            auto count = 0;
            try {
                for(auto block = start; block < start + nblocks; ++block) {
                    blkaddr[block-start] = htobe64(block);
                    size_t get_len = static_cast<size_t>(len);
                    Session::Status sts = session->Get(reinterpret_cast<const char *>(&(blkaddr[block-start])), sizeof(block),
                        reinterpret_cast<char *>(buf+(len*(block-start))), &get_len);
                    if ( sts == Session::Status::eOk ) {
                        retval = true;
                        ++count;
                    } else {
                        retval = false;
                        LZDBD(DBG, "%s Get failed: device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
                        break;
                    }
                }
            } catch (const exception &ex) {
                LZDBD(DBG, "%s Send error: %s", __FUNCTION__, ex.what());
                retval = false;
            }
        } else {
            ArgStruct arg_struct;
            unsigned int i;

            memset(&arg_struct, '\0', sizeof(arg_struct));
            arg_struct.buf = buf;
            arg_struct.buf_len = len;
            arg_struct.nblocks = nblocks;

            try {
                uint64_t b_start = htobe64(start);
                uint64_t b_end = -1;
                size_t get_len = static_cast<size_t>(len);

                Session::Status sts = session->GetRange(reinterpret_cast<const char *>(&b_start),
                    static_cast<size_t>(sizeof(b_start)),
                    reinterpret_cast<char *>(&b_end),
                    static_cast<size_t>(sizeof(b_start)),
                    reinterpret_cast<char *>(buf),
                    &get_len,
                    MultipleKVBlockCalback,
                    &arg_struct,
                    Iterator::eIteratorDir::eIteratorDirForward);
                LZDBD(DBG, "%s GetRange device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
                if ( sts != Session::Status::eOk ) {
                    retval = false;
                    LZDBD(DBG, "%s GetRange failed: device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
                } else {
                    retval = true;
                }
            } catch (const exception &ex) {
                LZDBD(DBG, "%s GetRange error: %s", __FUNCTION__, ex.what());
                retval = false;
            }

            if ( retval == true ) {
                LZDBD(DBG, "arg_struct.counter(%u)\n", arg_struct.counter);LZFLUSH;
                for ( i = 0 ; i < arg_struct.counter ; i++ ) {
                    LZDBD(DBG, "block(%lu)\n", arg_struct.blkaddr[i]);LZFLUSH;
                }
            }
        }
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestDeleteMultipleKVBlocks(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t start,
    uint32_t nblocks)
{
    bool retval = false;

    if (session->ok()) {
        uint64_t blkaddr[nblocks];
        auto count = 0;
        try {
            for(auto block = start; block < start + nblocks; ++block) {
                blkaddr[block-start] = htobe64(block);
                Session::Status sts = session->Delete(reinterpret_cast<const char *>(&(blkaddr[block-start])), sizeof(block));
                if ( sts == Session::Status::eOk ) {
                    retval = true;
                    ++count;
                } else {
                    retval = false;
                    LZDBD(DBG, "%s Delete failed: device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
                    break;
                }
            }
        } catch (const exception &ex) {
            LZDBD(DBG, "%s Send error: %s\n", __FUNCTION__, ex.what());LZFLUSH;
            retval = false;
        }
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" void *soeTestCreateSGDevice(void *retval_c,
    const char *cluster,
    const char *space,
    const char *store,
    const bool create)
{
    void *retval = &session;

    try { // check for container
        try {
            Soe::Session::Status sts = session->Open(cluster, space, store);
            if ( sts == Soe::Session::Status::eOk ) {
                retval = &session;
                throw std::runtime_error("");
            } else {
                retval = 0;
            }
        } catch (const std::runtime_error &ex) {
            retval = 0;
            throw std::runtime_error("");
        }
    } catch(const exception &ex) {
        if ( create ) {
            try {
                Soe::Session::Status sts = session->Create();
                if ( sts == Soe::Session::Status::eOk ) {
                    retval = &session;
                } else {
                    retval = 0;
                }
            } catch (const std::runtime_error &ex) {
                retval = 0;
            }
        } else {
            LZDBD(DBG, "%s Error: container(%s) does not exist\n", __FUNCTION__, store);LZFLUSH;
            retval = 0;
        }
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestQuerySGDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint32_t *blksize,
    uint64_t *nblocks)
{
    bool retval = false;

    geometry_t geometry;
    uint32_t size = sizeof(geometry_t);

    try {
        retval = soeTestReadSGBlock(connection,
            cluster,
            space,
            store,
            -1LL,
            reinterpret_cast<unsigned char *>(&geometry),
            &size);
        if (retval == false || !size) {
            retval = false;
        } else {
            if(retval && size != sizeof(geometry_t)) {
                LZDBD(DBG, "%s Device geometry invalid structure size(%u) expected(%lu)\n", __FUNCTION__, size, sizeof(geometry_t));LZFLUSH;
                retval = false;
            }
        }
    } catch (const exception &ex) {
        LZDBD(DBG, "%s Query error: device(%s): %s\n", __FUNCTION__, store, ex.what());LZFLUSH;
        retval = false;
    }

    if(retval) {
        if(blksize)
            *blksize = be32toh(geometry.blksize);
        if(nblocks)
            *nblocks = be64toh(geometry.nblocks);
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestReadSGBlock(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t block,
    unsigned char *buf,
    uint32_t *length)
{
    bool retval = false;
    uint64_t beblockno = htobe64(block);

    if (session->ok()) {
        try {
            Session::Status sts = session->Get(reinterpret_cast<const char *>(&beblockno), sizeof(beblockno),
                reinterpret_cast<char *>(buf), reinterpret_cast<size_t *>(length));
            if ( sts == Session::Status::eOk ) {
                retval = true;
            } else {
                retval = false;
                LZDBD(DBG, "%s Read error: device(%s) block(%lu) sts(%d)\n", __FUNCTION__, store, block, sts);LZFLUSH;
            }
        } catch (const exception &ex) {
            LZDBD(DBG, "%s Read error: device(%s) block(%lu) error(%s)\n", __FUNCTION__, store, block, ex.what());LZFLUSH;
            *length = 0;
            retval = false;
        }
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestWriteSGBlock(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t block,
    const unsigned char *buf,
    uint32_t length)
{
    bool retval = false;
    uint64_t beblockno = htobe64(block);

    if (session->ok()) {
        try {
            size_t len = static_cast<size_t>(length);
            Session::Status sts = session->Merge(reinterpret_cast<const char *>(&beblockno), sizeof(beblockno),
                reinterpret_cast<const char *>(buf), len);
            if ( sts == Session::Status::eOk ) {
                retval = true;
            } else {
                retval = false;
                LZDBD(DBG, "%s Write error: device(%s) block(%lu) length(%u) sts(%d)\n", __FUNCTION__, store, block, length, sts);LZFLUSH;
            }
        } catch (const exception &ex) {
            LZDBD(DBG, "%s Write error: device(%s) block(%lu) length(%u) error(%s)\n", __FUNCTION__, store, block, length, ex.what());LZFLUSH;
            retval = false;
        }
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestWriteMultipleSGBlocks(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t start,
    uint32_t nblocks,
    const unsigned char *buf,
    uint32_t len)
{
    bool retval = false;

    if (session->ok() != true) {
        return retval;
    }

    Group *group = session->CreateGroup();
    if ( !group ) {
        LZDBD(DBG, "%s CreateGroup failed: device(%s) length(%u)\n", __FUNCTION__, store, len);LZFLUSH;
        return false;
    }

    uint64_t blkaddr[nblocks];
    auto count = 0;
    try {
        for(auto block = start; block < start + nblocks; ++block) {
            blkaddr[block-start] = htobe64(block);
            Session::Status sts = group->Merge(reinterpret_cast<const char *>(&(blkaddr[block-start])),
                sizeof(block),
                reinterpret_cast<const char *>(buf+(len*(block-start))),
                len);
            if ( sts == Session::Status::eOk ) {
                retval = true;
                ++count;
            } else {
                retval = false;
                LZDBD(DBG, "%s Group::Merge failed: device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
                break;
            }
        }
        Session::Status sts = session->Write(group);
        if ( sts == Session::Status::eOk ) {
            retval = true;
        } else {
            LZDBD(DBG, "%s Session::Write failed: device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
            retval = false;
        }
        sts = session->DestroyGroup(group);
        if ( sts == Session::Status::eOk ) {
            retval = true;
        } else {
            LZDBD(DBG, "%s Session::DestroyGroup failed: device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
            retval = false;
        }
    } catch (const exception &ex) {
        LZDBD(DBG, "%s Send error: %s\n", __FUNCTION__, ex.what());LZFLUSH;
        retval = false;
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestReadMultipleSGBlocks(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    int use_iter,
    uint64_t start,
    uint32_t nblocks,
    unsigned char *buf,
    uint32_t len)
{
    bool retval = false;

    if (session->ok()) {
        uint64_t blkaddr[nblocks];
        auto count = 0;
        try {
            for(auto block = start; block < start + nblocks; ++block) {
                blkaddr[block-start] = htobe64(block);
                size_t get_len = static_cast<size_t>(len);
                Session::Status sts = session->Get(reinterpret_cast<const char *>(&(blkaddr[block-start])), sizeof(block),
                    reinterpret_cast<char *>(buf+(len*(block-start))), &get_len);
                if ( sts == Session::Status::eOk ) {
                    retval = true;
                    ++count;
                } else {
                    retval = false;
                    LZDBD(DBG, "%s Get failed: device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
                    break;
                }
            }
        } catch (const exception &ex) {
            LZDBD(DBG, "%s Send error: %s", __FUNCTION__, ex.what());
            retval = false;
        }
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestDeleteMultipleSGBlocks(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t start,
    uint32_t nblocks)
{
    bool retval = false;

    if (session->ok()) {
        uint64_t blkaddr[nblocks];
        auto count = 0;
        try {
            for(auto block = start; block < start + nblocks; ++block) {
                blkaddr[block-start] = htobe64(block);
                Session::Status sts = session->Delete(reinterpret_cast<const char *>(&(blkaddr[block-start])), sizeof(block));
                if ( sts == Session::Status::eOk ) {
                    retval = true;
                    ++count;
                } else {
                    retval = false;
                    LZDBD(DBG, "%s Delete failed: device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
                    break;
                }
            }
        } catch (const exception &ex) {
            LZDBD(DBG, "%s Send error: %s\n", __FUNCTION__, ex.what());LZFLUSH;
            retval = false;
        }
    }

    return retval;
}


extern "C" bool soeTestWriteTransactionMultipleKVBlocks(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t start,
    uint32_t nblocks,
    const unsigned char *buf,
    uint32_t len,
    bool fail)
{
    bool retval = false;

    if (session->ok() != true) {
        return retval;
    }

    uint64_t blkaddr[nblocks];
    auto count = 0;
    try {
        Transaction *tr = session->StartTransaction();
        if ( !tr ) {
            LZDBD(DBG, "%s StartTransaction store name: %s failed\n", __FUNCTION__, store);LZFLUSH;
            return false;
        }

        for(auto block = start; block < start + nblocks; ++block) {
            blkaddr[block-start] = htobe64(block);
            Session::Status sts = tr->Merge(reinterpret_cast<const char *>(&(blkaddr[block-start])),
                sizeof(block),
                reinterpret_cast<const char *>(buf+(len*(block-start))),
                len);
            if ( sts == Session::Status::eOk ) {
                retval = true;
                ++count;
            } else {
                retval = false;
                LZDBD(DBG, "%s Transaction::Merge failed: device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
                break;
            }
        }

        Session::Status sts;
        if ( fail == true ) {
            sts = session->RollbackTransaction(tr);
        } else {
            sts = session->CommitTransaction(tr);
        }
        if ( sts == Session::Status::eOk ) {
            retval = true;
        } else {
            LZDBD(DBG, "%s Commit/Rollback Transaction failed: device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
            retval = false;
        }
    } catch (const exception &ex) {
        LZDBD(DBG, "%s Send error: %s\n", __FUNCTION__, ex.what());LZFLUSH;
        retval = false;
    }

    return retval;
}

extern "C" bool soeTestWriteTransactionKVBlock(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t block,
    const unsigned char *buf,
    uint32_t length,
    bool fail)
{
    bool retval = false;
    uint64_t beblockno = htobe64(block);

    if ( session->ok() != true ) {
        return retval;
    }

    try {
        Transaction *tr = session->StartTransaction();
        if ( !tr ) {
            LZDBD(DBG, "%s StartTransaction store name: %s failed\n", __FUNCTION__, store);LZFLUSH;
            return false;
        }

        Session::Status sts = tr->Merge(reinterpret_cast<const char *>(&beblockno),
        //Session::Status sts = tr->Put(reinterpret_cast<const char *>(&beblockno),
            sizeof(beblockno),
            reinterpret_cast<const char *>(buf),
            length);
        if ( sts != Session::Status::eOk ) {
            LZDBD(DBG, "%s Merge store name: %s failed\n", __FUNCTION__, store);LZFLUSH;
            retval = false;
        }

        if ( fail == true ) {
            sts = session->RollbackTransaction(tr);
        } else {
            sts = session->CommitTransaction(tr);
        }
        if ( sts == Session::Status::eOk ) {
            retval = true;
        } else {
            LZDBD(DBG, "%s Commit/Rollback Transaction failed: device(%s) sts(%d)\n", __FUNCTION__, store, sts);LZFLUSH;
            retval = false;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s cluster name: %s failed(%s)\n", __FUNCTION__, store, ex.what());LZFLUSH;
        retval = 0;
    }

    return retval;
}

SnapshotEngine *snapshot_engine = 0;

extern "C" bool soeTestCreateSnapshot(const char *cluster,
    const char *space,
    const char *store,
    const char *snap_name)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    try {
        snapshot_engine = session->CreateSnapshotEngine(snap_name);
        if ( !snapshot_engine ) {
            LZDBD(DBG, "%s cluster name: %s device: %s create snapshot engine failed\n", __FUNCTION__, cluster, store);LZFLUSH;
            return false;
        }

        Session::Status sts = snapshot_engine->CreateSnapshot();
        if ( sts == Session::Status::eOk ) {
            retval = true;
        } else {
            LZDBD(DBG, "%s cluster name: %s device: %s create snapshot failed(%d)\n", __FUNCTION__, cluster, store, sts);LZFLUSH;
            return false;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s cluster name: %s device: %s failed(%s)\n", __FUNCTION__, cluster, store, ex.what());LZFLUSH;
        retval = false;
    }

    return retval;
}

extern "C" bool soeTestDeleteSnapshot(const char *cluster,
    const char *space,
    const char *store,
    const char *snap_name,
    uint32_t snap_id)
{
    bool retval = false;

    try {
        Session::Status sts = Session::Status::eOpNotSupported; //session->DestroySnapshot(snapshot_engine);
        if ( sts == Session::Status::eOk ) {
            retval = true;
        } else {
            LZDBD(DBG, "%s cluster name: %s device: %s delete snasphot failed(%d)\n", __FUNCTION__, cluster, store, sts);LZFLUSH;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s cluster name: %s device: %s failed(%s)\n", __FUNCTION__, cluster, store, ex.what());LZFLUSH;
        retval = false;
    }

    return retval;
}

extern "C" bool soeTestDeleteSnapshotEngine(const char *cluster,
    const char *space,
    const char *store,
    const char *snap_name,
    uint32_t snap_id)
{
    bool retval = false;

    try {
        Session::Status sts = session->DestroySnapshotEngine(snapshot_engine);
        if ( sts == Session::Status::eOk ) {
            retval = true;
            snapshot_engine = 0;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s cluster name: %s device: %s failed(%s)\n", __FUNCTION__, cluster, store, ex.what());LZFLUSH;
        retval = 0;
    }

    return retval;
}

extern "C" bool soeTestListSnapshots(const char *cluster,
    const char *space,
    const char *store)
{
    bool retval = false;
    std::vector<std::pair<std::string, uint32_t> > list;

    try {
        Session::Status sts = session->ListSnapshots(list);
        if ( sts == Session::Status::eOk ) {
            retval = true;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s cluster name: %s device: %s failed(%s)\n", __FUNCTION__, cluster, store, ex.what());LZFLUSH;
        retval = 0;
    }

    for ( auto it = list.begin() ; it != list.end() ; it++ ) {
        std::cout << "Snapshot name: " << it->first << " id: " << it->second << std::endl << std::flush;
    }

    return retval;
}

BackupEngine *backup_engine = 0;
uint32_t back_engine_id = -1;

extern "C" bool soeTestCreateBackupEngine(const char *cluster,
    const char *space,
    const char *store,
    const char *back_name)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    try {
        backup_engine = session->CreateBackupEngine(back_name);
        if ( backup_engine ) {
            retval = true;
            back_engine_id = backup_engine->GetId();
        } else {
            LZDBD(DBG, "%s cluster name: %s device: %s creating backup engine failed\n", __FUNCTION__, cluster, store);LZFLUSH;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s cluster name: %s device: %s failed(%s)\n", __FUNCTION__, cluster, store, ex.what());LZFLUSH;
        retval = false;
    }

    return retval;
}

extern "C" bool soeTestCreateBackup(const char *cluster,
    const char *space,
    const char *store,
    const char *back_name,
    uint32_t back_id)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    try {
        if ( !backup_engine ) {
            LZDBD(DBG, "%s cluster name: %s device: %s No backup engine, create one first\n", __FUNCTION__, cluster, store);LZFLUSH;
            return false;
        }
        Session::Status sts = backup_engine->CreateBackup();
        if ( sts == Session::Status::eOk ) {
            retval = true;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s cluster name: %s device: %s failed(%s)\n", __FUNCTION__, cluster, store, ex.what());LZFLUSH;
        retval = false;
    }

    return retval;
}

extern "C" bool soeTestDeleteBackupEngine(const char *cluster,
    const char *space,
    const char *store,
    const char *back_name,
    uint32_t back_id)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    try {
        if ( !backup_engine ) {
            LZDBD(DBG, "%s cluster name: %s device: %s No backup engine, create one first\n", __FUNCTION__, cluster, store);LZFLUSH;
            return false;
        }
        Session::Status sts = session->DestroyBackupEngine(backup_engine);
        if ( sts == Session::Status::eOk ) {
            retval = true;
            backup_engine = 0;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s cluster name: %s device: %s failed(%s)\n", __FUNCTION__, cluster, store, ex.what());LZFLUSH;
        retval = 0;
    }

    return retval;
}

extern "C" bool soeTestListBackups(const char *cluster,
    const char *space,
    const char *store)
{
    bool retval = false;
    std::vector<std::pair<std::string, uint32_t> > list;

    try {
        Session::Status sts = session->ListBackups(list);
        if ( sts == Session::Status::eOk ) {
            retval = true;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s cluster name: %s device: %s failed(%s)\n", __FUNCTION__, cluster, store, ex.what());LZFLUSH;
        retval = 0;
    }

    for ( auto it = list.begin() ; it != list.end() ; it++ ) {
        std::cout << "Backup name: " << it->first << " id: " << it->second << std::endl << std::flush;
    }

    return retval;
}

extern "C" bool soeTestContainerRepair(const char *cluster,
    const char *space,
    const char *store)
{
    bool retval = false;

    try {
        Session::Status sts = session->Repair();
        if ( sts == Session::Status::eOk ) {
            retval = true;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s cluster name: %s device: %s failed(%s)\n", __FUNCTION__, cluster, store, ex.what());LZFLUSH;
        retval = 0;
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
extern "C" bool soeTestContainerDelete(void *connection,
    const char *cluster,
    const char *space,
    const char *store)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    try {
        Session::Status sts = session->DestroyStore();
        if ( sts == Session::Status::eOk ) {
            retval = true;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s cluster name: %s failed(%s)\n", __FUNCTION__, cluster, ex.what());LZFLUSH;
        retval = 0;
    }

    return retval;
}

/*
 * ----------------------------------------------------------------------
 */
Soe::DisjointSubset *disjoint_subset = 0;
char subset_name[128];

extern "C" bool soeTestCreateSubsetDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    const char *subset,
    bool create)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    if ( disjoint_subset && !strcmp(subset, subset_name) ) {
        LZDBD(DBG, "%s  subset name: %s exist\n", __FUNCTION__, subset);LZFLUSH;
        return retval;
    }

    try {
        Soe::Subsetable *subs = session->CreateSubset(subset, strlen(subset), Soe::Session::eSubsetType::eSubsetTypeDisjoint);
        if ( !subs ) {
            LZDBD(DBG, "%s CreateSubset subset name: %s failed\n", __FUNCTION__, subset);LZFLUSH;
            return retval;
        }
        disjoint_subset = dynamic_cast<Soe::DisjointSubset *>(subs);
        retval = true;
        strcpy(subset_name, subset);
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s CreateSubset subset name: %s failed(%s)\n", __FUNCTION__, subset, ex.what());LZFLUSH;
        retval = 0;
    }

    return retval;
}

extern "C" bool soeTestDeleteSubsetDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    const char *subset)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    if ( !disjoint_subset ) {
        try {
            Soe::Subsetable *subs = session->OpenSubset(subset, strlen(subset), Soe::Session::eSubsetType::eSubsetTypeDisjoint);
            if ( !subs ) {
                LZDBD(DBG, "%s CreateSubset subset name: %s failed\n", __FUNCTION__, subset);LZFLUSH;
                return retval;
            }
            disjoint_subset = dynamic_cast<DisjointSubset *>(subs);
            strcpy(subset_name, subset);
        } catch (const std::runtime_error &ex) {
            LZDBD(DBG, "%s DestroySubset subset name: %s failed(%s)\n", __FUNCTION__, subset, ex.what());LZFLUSH;
            retval = 0;
        }
    }

    if ( !disjoint_subset || strcmp(subset, subset_name) ) {
        LZDBD(DBG, "%s no subset or name doesn't match: %s\n", __FUNCTION__, subset);LZFLUSH;
        return retval;
    }

    try {
        Soe::Subsetable *subs = session->OpenSubset(subset, strlen(subset), Soe::Session::eSubsetType::eSubsetTypeDisjoint);
        if ( !subs ) {
            LZDBD(DBG, "%s CreateSubset subset name: %s failed\n", __FUNCTION__, subset);LZFLUSH;
            return retval;
        }
        Session::Status sts = session->DestroySubset(subs);
        if ( sts != Session::Status::eOk ) {
            LZDBD(DBG, "%s DestroySubset subset name: %s failed(%u)\n", __FUNCTION__, subset, sts);LZFLUSH;
            return retval;
        }
        sts = session->DeleteSubset(subs);
        if ( sts != Session::Status::eOk ) {
            LZDBD(DBG, "%s DestroySubset subset name: %s failed(%u)\n", __FUNCTION__, subset, sts);LZFLUSH;
            return retval;
        }
        retval = true;
        subs = 0;
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s DestroySubset subset name: %s failed(%s)\n", __FUNCTION__, subset, ex.what());LZFLUSH;
        retval = 0;
    }

    return retval;
}

extern "C" bool soeTestQuerySubsetDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    const char *subset,
    uint32_t *blksize,
    uint64_t *nblocks,
    int dump_hex)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    if ( !disjoint_subset ) {
        try {
            Soe::Subsetable *subs = session->OpenSubset(subset, strlen(subset), Soe::Session::eSubsetType::eSubsetTypeDisjoint);
            if ( !subs ) {
                LZDBD(DBG, "%s CreateSubset subset name: %s failed\n", __FUNCTION__, subset);LZFLUSH;
                return retval;
            }
            disjoint_subset = dynamic_cast<DisjointSubset *>(subs);
            strcpy(subset_name, subset);
        } catch (const std::runtime_error &ex) {
            LZDBD(DBG, "%s DestroySubset subset name: %s failed(%s)\n", __FUNCTION__, subset, ex.what());LZFLUSH;
            retval = 0;
        }
    }

    if ( !disjoint_subset || strcmp(subset, subset_name) ) {
        LZDBD(DBG, "%s no subset or name doesn't match: %s exist\n", __FUNCTION__, subset);LZFLUSH;
        return retval;
    }

    try {
        char start_key[128];
        char end_key[128];
        ::memset(start_key, '\0', sizeof(start_key));
        ::memset(end_key, 0xff, sizeof(end_key));

        Soe::Iterator *ss_it = disjoint_subset->CreateIterator(Iterator::eIteratorDir::eIteratorDirForward,
            start_key,
            0,
            end_key,
            sizeof(end_key));
        if ( !ss_it ) {
            LZDBD(DBG, "%s CreateIterator subset name: %s failed\n", __FUNCTION__, subset);LZFLUSH;
            return retval;
        }

        Iterator::eStatus it_sts = Iterator::eStatus::eOk;
        for ( it_sts = ss_it->First(start_key, 0) ;
                it_sts == Iterator::eStatus::eOk && ss_it->Valid() == true /*&& ss_it->Key().compare(Duple(end_key, sizeof(end_key))) <= 0*/ ;
                it_sts = ss_it->Next() ) {
            if ( dump_hex ) {
                fprintf(stdout, "Iterate returned Key()/Value()\n");
                debugPrintAsciiHexBuf(stdout, ss_it->Key().data(), static_cast<unsigned int>(ss_it->Key().size()));
                debugPrintHexBuf(stdout, ss_it->Value().data(), static_cast<unsigned int>(ss_it->Value().size()));
            }
        }
        if ( it_sts != Iterator::eStatus::eOk ) {
            LZDBD(DBG, "%s Iterator failed(%u)\n", __FUNCTION__, it_sts);LZFLUSH;
        } else {
            retval = true;
        }

        Session::Status s_sts = disjoint_subset->DestroyIterator(ss_it);
        if ( s_sts != Session::Status::eOk ) {
            retval = false;
            LZDBD(DBG, "%s DestroyIterator name: %s failed(%u)\n", __FUNCTION__, subset, s_sts);LZFLUSH;
        } else {
            retval = true;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s QuerySubset subset name: %s failed(%s)\n", __FUNCTION__, subset, ex.what());LZFLUSH;
        retval = 0;
    }

    return retval;
}

extern "C" bool soeTestReadSubsetDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    const char *subset,
    uint64_t block,
    unsigned char *buf,
    uint32_t *length)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    if ( !disjoint_subset ) {
        try {
            Soe::Subsetable *subs = session->OpenSubset(subset, strlen(subset), Soe::Session::eSubsetType::eSubsetTypeDisjoint);
            if ( !subs ) {
                LZDBD(DBG, "%s CreateSubset subset name: %s failed\n", __FUNCTION__, subset);LZFLUSH;
                return retval;
            }
            disjoint_subset = dynamic_cast<DisjointSubset *>(subs);
            strcpy(subset_name, subset);
        } catch (const std::runtime_error &ex) {
            LZDBD(DBG, "%s DestroySubset subset name: %s failed(%s)\n", __FUNCTION__, subset, ex.what());LZFLUSH;
            retval = 0;
        }
    }

    if ( !disjoint_subset || strcmp(subset, subset_name) ) {
        LZDBD(DBG, "%s no subset or name doesn't match: %s exist\n", __FUNCTION__, subset);LZFLUSH;
        return retval;
    }

    try {
        Session::Status sts = disjoint_subset->Get(reinterpret_cast<const char *>(&block),
            sizeof(block),
            reinterpret_cast<char *>(buf),
            reinterpret_cast<size_t *>(length));
        if ( sts != Session::Status::eOk ) {
            LZDBD(DBG, "%s subset Read name: %s failed(%u)\n", __FUNCTION__, subset, sts);LZFLUSH;
            return retval;
        }

        retval = true;
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s subset Read name: %s failed(%s)\n", __FUNCTION__, subset, ex.what());LZFLUSH;
        retval = 0;
    }

    return retval;
}

extern "C" bool soeTestReadMultipleSubsetDevice(void *conn,
    const char *cluster,
    const char *space,
    const char *store,
    const char *subset,
    const uint64_t,
    uint32_t,
    unsigned char *,
    uint32_t )
{
    return false;
}

extern "C" bool soeTestWriteMultipleSubsetDevice(void *conn,
    const char *cluster,
    const char *space,
    const char *store,
    const char *subset,
    const uint64_t start,
    uint32_t nblocks,
    const unsigned char *buf,
    uint32_t len)
{
    bool retval = false;
    uint64_t blkaddr[nblocks];
    std::vector<std::pair<Duple, Duple>> items;

    if ( session->ok() != true ) {
        return retval;
    }

    if ( !disjoint_subset ) {
        try {
            Soe::Subsetable *subs = session->OpenSubset(subset, strlen(subset), Soe::Session::eSubsetType::eSubsetTypeDisjoint);
            if ( !subs ) {
                LZDBD(DBG, "%s CreateSubset subset name: %s failed\n", __FUNCTION__, subset);LZFLUSH;
                return retval;
            }
            disjoint_subset = dynamic_cast<DisjointSubset *>(subs);
            strcpy(subset_name, subset);
        } catch (const std::runtime_error &ex) {
            LZDBD(DBG, "%s DestroySubset subset name: %s failed(%s)\n", __FUNCTION__, subset, ex.what());LZFLUSH;
            retval = 0;
        }
    }

    if ( !disjoint_subset || strcmp(subset, subset_name) ) {
        LZDBD(DBG, "%s no subset or name doesn't match: %s exist\n", __FUNCTION__, subset);LZFLUSH;
        return retval;
    }

    for( auto block = start ; block < start + nblocks ; ++block ) {
        blkaddr[block - start] = htobe64(block);

        Duple key(reinterpret_cast<const char *>(&blkaddr[block - start]), sizeof(block));
        Duple data(reinterpret_cast<const char *>(buf+(len*(block-start))), static_cast<size_t>(len));

        items.push_back(std::pair<Duple, Duple>(key, data));
    }

    Session::Status sts = disjoint_subset->MergeSet(items);
    if ( sts == Session::Status::eOk ) {
        retval = true;
        LZDBD(DBG, "%s done %s\n", __FUNCTION__, subset);LZFLUSH;
    } else {
        LZDBD(DBG, "%s failed(%u) %s\n", __FUNCTION__, sts, subset);LZFLUSH;
    }

    return retval;
}

extern "C" bool soeTestWriteSubsetDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    const char *subset,
    uint64_t block,
    const unsigned char *buf,
    uint32_t length)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    if ( !disjoint_subset ) {
        try {
            Soe::Subsetable *subs = session->OpenSubset(subset, strlen(subset), Soe::Session::eSubsetType::eSubsetTypeDisjoint);
            if ( !subs ) {
                LZDBD(DBG, "%s CreateSubset subset name: %s failed\n", __FUNCTION__, subset);LZFLUSH;
                return retval;
            }
            disjoint_subset = dynamic_cast<DisjointSubset *>(subs);
            strcpy(subset_name, subset);
        } catch (const std::runtime_error &ex) {
            LZDBD(DBG, "%s DestroySubset subset name: %s failed(%s)\n", __FUNCTION__, subset, ex.what());LZFLUSH;
            retval = 0;
        }
    }

    if ( !disjoint_subset || strcmp(subset, subset_name) ) {
        LZDBD(DBG, "%s no subset or name doesn't match: %s exist\n", __FUNCTION__, subset);LZFLUSH;
        return retval;
    }

    try {
        Session::Status sts = disjoint_subset->Merge(reinterpret_cast<const char *>(&block),
            sizeof(block),
            reinterpret_cast<const char *>(buf),
            static_cast<size_t >(length));
        if ( sts != Session::Status::eOk ) {
            LZDBD(DBG, "%s subset Merge name: %s failed(%u)\n", __FUNCTION__, subset, sts);LZFLUSH;
            return retval;
        }

        retval = true;

    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s subset Merge name: %s failed(%s)\n", __FUNCTION__, subset, ex.what());LZFLUSH;
        retval = 0;
    }

    return retval;
}

extern "C" bool soeTestQueryContainer(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    int dump_hex)
{
    bool retval = false;

    if ( session->ok() != true ) {
        return retval;
    }

    try {
        char start_key[1024];
        char end_key[1024];

        ::memset(start_key, '\0', sizeof(start_key));
        ::memset(end_key, 0xff, sizeof(end_key));

        Soe::Iterator *ss_it = session->CreateIterator(Iterator::eIteratorDir::eIteratorDirForward,
            start_key,
            0,
            end_key,
            sizeof(end_key));
        if ( !ss_it ) {
            LZDBD(DBG, "%s CreateIterator failed\n", __FUNCTION__);LZFLUSH;
            return retval;
        }
        LZDBD(DBG, "%s CreateIterator done\n", __FUNCTION__);LZFLUSH;

        Iterator::eStatus it_sts = Iterator::eStatus::eOk;
        for ( it_sts = ss_it->First(start_key, 0) ;
                ss_it->Valid() == true ;
                it_sts = ss_it->Next() ) {
            if ( dump_hex ) {
                fprintf(stdout, "Iterate returned Key()/Value()\n");
                debugPrintAsciiHexBuf(stdout, ss_it->Key().data(), static_cast<unsigned int>(ss_it->Key().size()));
                debugPrintHexBuf(stdout, ss_it->Value().data(), static_cast<unsigned int>(ss_it->Value().size()));
            }
        }
        if ( it_sts != Iterator::eStatus::eOk ) {
            LZDBD(DBG, "%s Iterator failed(%u)\n", __FUNCTION__, it_sts);LZFLUSH;
        }

        Session::Status s_sts = session->DestroyIterator(ss_it);
        if ( s_sts != Session::Status::eOk ) {
            LZDBD(DBG, "%s DestroyIterator failed(%u)\n", __FUNCTION__, it_sts);LZFLUSH;
        } else {
            retval = true;
            LZDBD(DBG, "%s DestroyIterator done(%u)\n", __FUNCTION__, it_sts);LZFLUSH;
        }
    } catch (const std::runtime_error &ex) {
        LZDBD(DBG, "%s Query Container subset failed(%s)\n", __FUNCTION__, ex.what());LZFLUSH;
        retval = 0;
    }

    return retval;
}

#endif // #ifdef EPOCH_SOE


