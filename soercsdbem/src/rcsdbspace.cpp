/**
 * @file    rcsdbspace.hpp
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
 * rcsdbspace.cpp
 *
 *  Created on: Jan 27, 2017
 *      Author: Jan Lisowiec
 */

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include <list>
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
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/utilities/checkpoint.h>

#include "soe_bind_api.hpp"
#include "rcsdbargdefs.hpp"
#include "rcsdbfacadeapi.hpp"
#include "rcsdbfacadebase.hpp"
#include "rcsdbfunctorbase.hpp"
#include "rcsdbconfigbase.hpp"
#include "rcsdbconfig.hpp"
#include "rcsdbsnapshot.hpp"
#include "rcsdbstorebackup.hpp"
#include "rcsdbfilters.hpp"
#include "rcsdbfacade.hpp"
#include "rcsdbstoreiterator.hpp"
#include "rcsdbstore.hpp"
#include "rcsdbspace.hpp"
#include "rcsdbcluster.hpp"

using namespace std;
using namespace RcsdbFacade;

namespace RcsdbLocalFacade {

//
// Space
//
uint32_t Space::max_stores = 100000;
uint64_t Space::space_null_id = -1;  // max 64 bit number

Space::Space(const string &nm, uint64_t _id, bool ch_names, bool _debug):
    name(nm),
    id(_id),
    config(_debug),
    checking_names(ch_names),
    debug(_debug)
{
    RcsdbName::next_available = 0;

    stores.resize(static_cast<size_t>(Space::max_stores), 0);
    for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
        Space::stores[i] = 0;
    }
}

Space::~Space()
{
    for ( uint32_t i = 0 ; i < stores.size() ; i++ ) {
        if ( stores[i] ) {
            delete stores[i];
        }
    }
}

void Space::SetPath()
{
    assert(parent);
    path = dynamic_cast<RcsdbName *>(parent)->GetPath() + "/" + name;
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "path: " << path << " " << string(__FUNCTION__) << SoeLogger::LogDebug() << endl;
    }
}

const string &Space::GetPath()
{
    return path;
}

void Space::SetNextAvailable()
{
    for ( uint32_t i = 0 ; i < Space::stores.size() ; i++ ) {
        if ( !Space::stores[i] ) {
            RcsdbName::next_available = i;
            break;
        }
    }
}

Store *Space::GetStore(LocalArgsDescriptor &args,
        bool ch_names,
        bool _debug)  throw(RcsdbFacade::Exception)
{
    // check duplicate name
    for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
        if ( Space::stores[i] && Space::stores[i]->name == *args.store_name ) {
            return Space::stores[i];
        }
    }

    // check duplicate id
    if ( args.store_id != Store::store_invalid_id ) {
        for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
            if ( Space::stores[i] && Space::stores[i]->id == args.store_id ) {
                throw RcsdbFacade::Exception("Duplicate id for new name: " + *args.store_name + " id" + to_string(args.store_id),
                    RcsdbFacade::Exception::eInvalidArgument);
            }
        }
    }

    for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
        Store::eDBType database_type = Store::eSimpleDB;
        if ( args.transaction_db == true ) {
            database_type = Store::eTransactionDB;
        }
        if ( !Space::stores[i] ) {
            Space::stores[i] = new Store(*args.store_name,
                args.store_id == Store::store_invalid_id ? Store::store_start_id + i : args.store_id,
                database_type,
                args.bloom_bits,
                args.overwrite_dups,
                static_cast<Store::eSyncLevel>(args.sync_mode),
                _debug);
            Space::stores[i]->SetParent(this);
            Space::stores[i]->SetPath();
            Space::stores[i]->Create();
            Space::stores[i]->EnumSnapshots();
            Space::stores[i]->ValidateSnapshots();
            Space::stores[i]->EnumBackups();
            Space::stores[i]->ValidateBackups();
            try {
                Space::stores[i]->RegisterApiFunctions();
            } catch(const SoeApi::Exception &ex) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't register store functions: " <<
                    ex.what() << " " << string(__FUNCTION__) << SoeLogger::LogError() << endl;
            }
            return stores[i];
        }
    }

    throw RcsdbFacade::Exception("No more space slots available", Exception::eOsResourceExhausted);
}

void Space::DiscoverStores(const string &clu)
{
    string path = string(RcsdbName::db_root_path) + "/" + clu + "/" + name;
    DIR *dir = ::opendir(path.c_str());
    if ( !dir ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << string("Can't open path: ") << path.c_str() <<
            to_string(errno) << " "<< string(__FUNCTION__) << SoeLogger::LogError() << endl;
            return;
    }

    struct dirent *dir_entry;
    uint32_t cnt = 0;
    while ( (dir_entry = readdir(dir)) ) {
        if ( !dir_entry ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't read store dir: " << " errno: " <<
                to_string(errno) << " " << string(__FUNCTION__) << SoeLogger::LogError() << endl;
            return;
        }

        if ( string(dir_entry->d_name) == "." ||
            string(dir_entry->d_name) == ".." ||
            string(dir_entry->d_name) == RcsdbName::db_elem_filename ) {
            continue;
        }
        if ( debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Dir file: " << dir_entry->d_name << " " << string(__FUNCTION__) << SoeLogger::LogDebug() << endl;
        }

        string cfg = path + "/" + dir_entry->d_name + "/" + RcsdbName::db_elem_filename;
        string line;
        string all_lines;
        ifstream cfile(cfg.c_str());
        if ( cfile.is_open() != true ) {
            ::closedir(dir);
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't open config: " << cfg + " errno: " <<
                to_string(errno) << " " << string(__FUNCTION__) << SoeLogger::LogError() << endl;
                return;
        }
        while ( cfile.eof() != true ) {
            getline(cfile, line);
            //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "line: " << line;
            all_lines += line;
        }

        Space::stores[cnt] = new Store("zzzz",
            Store::store_null_id,
            Store::eSimpleDB,
            Store::bloom_bits_default,
            Store::overdups_default,
            Store::eSyncLevel::eSyncLevelDefault,
            false);
        Space::stores[cnt]->SetParent(this);
        Space::stores[cnt]->ParseConfig(all_lines, true);
        Space::stores[cnt]->SetPath();
        Space::stores[cnt]->EnumSnapshots();
        Space::stores[cnt]->ValidateSnapshots();
        Space::stores[cnt]->EnumBackups();
        Space::stores[cnt]->ValidateBackups();

        // We need to set options against even though they are set in constructor
        // because some options are saved in config.json
        Space::stores[cnt]->SetOpenOptions();
        Space::stores[cnt]->SetReadOptions();
        Space::stores[cnt]->SetWriteOptions();

        try {
            Space::stores[cnt]->RegisterApiFunctions();
        } catch(const SoeApi::Exception &ex) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't register store functions: " <<
                ex.what() << " " << string(__FUNCTION__) << SoeLogger::LogError() << endl;
        }

        cfile.close();
        if ( cnt++ >= Space::max_stores ) {
            break;
        }
    }
    ::closedir(dir);
}

//
// Find functions
//
Store *Space::FindStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
        if ( Space::stores[i] && Space::stores[i]->name == *args.store_name ) {
            return Space::stores[i];
        }
    }

    return 0;
}

//
// Create
//
uint64_t Space::CreateSpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    return Cluster::CreateSpace(args);
}

void Space::DestroySpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    return Cluster::DestroySpace(args);
}

uint64_t Space::GetSpaceByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    return Cluster::GetSpaceByName(args);
}

void Space::ParseConfig(const string &cfg, bool no_exc)
{
    config.Parse(cfg, no_exc);
    name = config.name;
    id = config.id;
}

string Space::CreateConfig()
{
    config.name = name;
    config.id = id;
    return config.Create();
}

string Space::ConfigAddSnapshot(const string &name, uint32_t id)
{
    return config.Create();
}

string Space::ConfigRemoveSnapshot(const string &name, uint32_t id)
{
    return config.Create();
}

string Space::ConfigAddBackup(const string &name, uint32_t id)
{
    return config.Create();
}

string Space::ConfigRemoveBackup(const string &name, uint32_t id)
{
    return config.Create();
}

void Space::PutPr(uint32_t store_idx, uint64_t store_id, const string &store_name,
    const char *data, size_t data_len,
    const char *key, size_t key_len) throw(RcsdbFacade::Exception)
{
    if ( !key || !data ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    if ( store_idx >= Space::max_stores ) {
        throw RcsdbFacade::Exception("Out-of-bounds store idx: " + to_string(store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( !stores[store_idx] ) {
        throw RcsdbFacade::Exception("Wrong store: " + to_string(store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( stores[store_idx]->id == store_id &&
            (checking_names && store_name == stores[store_idx]->name) ) {
        stores[store_idx]->PutPr(Slice(key, key_len), Slice(data, data_len));
        return;
    }
    throw RcsdbFacade::Exception("Store in wrong state idx: " + to_string(store_idx) + " " + string(__FUNCTION__), Exception::eInvalidStoreState);
}

void Space::GetPr(uint32_t store_idx, uint64_t store_id, const string &store_name,
    const char *key, size_t key_len,
    char *buf, size_t &buf_len) throw(RcsdbFacade::Exception)
{
    if ( !key ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    if ( store_idx >= Space::max_stores ) {
        throw RcsdbFacade::Exception("Out-of-bounds store idx: " + to_string(store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( !stores[store_idx] ) {
        throw RcsdbFacade::Exception("Wrong store: " + to_string(store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( stores[store_idx]->id == store_id &&
            (checking_names && store_name == stores[store_idx]->name) ) {
        string value;
        string *str_ptr = &value;
        stores[store_idx]->GetPr(Slice(key, key_len), str_ptr, buf_len);
        if ( str_ptr ) {
            size_t value_l = str_ptr->length();
            if ( value_l > buf_len ) {
                throw RcsdbFacade::Exception("Buffer too small: " + to_string(buf_len) + " value.length: " +
                    to_string(value_l) + " " + string(__FUNCTION__), Exception::eTruncatedValue);
            }

            buf_len = value_l;
            memcpy(buf, str_ptr->c_str(), buf_len);
        } else {
            buf_len = 0;
        }
        return;
    }
    throw RcsdbFacade::Exception("Store in wrong state idx: " + to_string(store_idx) + " " + string(__FUNCTION__), Exception::eInvalidStoreState);
}

void Space::DeletePr(uint32_t store_idx, uint64_t store_id,
    const string &store_name, const char *key, size_t key_len) throw(RcsdbFacade::Exception)
{
    if ( !key ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    if ( store_idx >= Space::max_stores ) {
        throw RcsdbFacade::Exception("Out-of-bounds store idx: " + to_string(store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( !stores[store_idx] ) {
        throw RcsdbFacade::Exception("Wrong store: " + to_string(store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if (  stores[store_idx]->id == store_id &&
            (checking_names && store_name == stores[store_idx]->name) ) {
        stores[store_idx]->DeletePr(Slice(key, key_len));
        return;
    }
    throw RcsdbFacade::Exception("Store in wrong state idx: " + to_string(store_idx) + " " + string(__FUNCTION__), Exception::eInvalidStoreState);
}

//
// Registration functions
//
void Space::RegisterApiStaticFunctions() throw(SoeApi::Exception)
{
    // bind functions to boost functors
    boost::function<uint64_t(LocalArgsDescriptor &)> SpaceCreate =
        boost::bind(&RcsdbLocalFacade::Cluster::CreateSpace, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceDestroy =
        boost::bind(&RcsdbLocalFacade::Cluster::DestroySpace, _1);

    boost::function<uint64_t(LocalArgsDescriptor &)> SpaceGetByName =
        boost::bind(&RcsdbLocalFacade::Cluster::GetSpaceByName, _1);


    // register via SoeApi
    new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
        SpaceCreate, RcsdbFacade::CreateLocalName(STATIC_CLUSTER, "", "", 0, FUNCTION_CREATE), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceDestroy, RcsdbFacade::CreateLocalName(STATIC_CLUSTER, "", "", 0, FUNCTION_DESTROY), Cluster::global_debug);

    new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
            SpaceGetByName, RcsdbFacade::CreateLocalName(STATIC_CLUSTER, "", "", 0, FUNCTION_GET_BY_NAME), Cluster::global_debug);
}

void Space::DeregisterApiStaticFunctions() throw(SoeApi::Exception)
{

}

void Space::RegisterApiFunctions() throw(SoeApi::Exception)
{
    // bind functions to boost functors
    boost::function<void(LocalArgsDescriptor &)> SpaceOpen =
        boost::bind(&RcsdbLocalFacade::Space::Open, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceClose =
        boost::bind(&RcsdbLocalFacade::Space::Close, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpacePut =
        boost::bind(&RcsdbLocalFacade::Space::Put, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceGet =
        boost::bind(&RcsdbLocalFacade::Space::Get, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceDelete =
        boost::bind(&RcsdbLocalFacade::Space::Delete, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceGetRange =
        boost::bind(&RcsdbLocalFacade::Space::GetRange, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceMerge =
        boost::bind(&RcsdbLocalFacade::Space::Merge, this, _1);


    // register via SoeApi
    assert(parent);
    string &cluster = dynamic_cast<Cluster *>(parent)->name;

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceOpen, RcsdbFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_OPEN), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
         SpaceClose, RcsdbFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_CLOSE), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpacePut, RcsdbFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_PUT), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceGet, RcsdbFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_GET), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceDelete, RcsdbFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_DELETE), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceGetRange, RcsdbFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_GET_RANGE), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceMerge, RcsdbFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_MERGE), Cluster::global_debug);
}

void Space::DeregisterApiFunctions() throw(SoeApi::Exception)
{

}

//
// Dynamic API functions
//
void Space::Open(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    DIR *dir = ::opendir(path.c_str());
    if ( !dir ) {
        throw RcsdbFacade::Exception("Can't open space name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + string(__FUNCTION__), Exception::eOsError);
    }

    string config = path + "/" + RcsdbName::db_elem_filename;
    string line;
    string all_lines;
    ifstream cfile(config.c_str());
    if ( cfile.is_open() != true ) {
        ::closedir(dir);
        throw RcsdbFacade::Exception("Can't open config, name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + string(__FUNCTION__), Exception::eOsError);
    }
    while ( cfile.eof() != true ) {
        getline(cfile, line);
        //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "line: " << line;
        all_lines += line;
    }

    ParseConfig(all_lines);
    cfile.close();
    ::closedir(dir);
}

void Space::Close(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("NOP " + string(__FUNCTION__));
}

void Space::Repair(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("NOP " + string(__FUNCTION__));
}

void Space::Create()
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "path: " << path << SoeLogger::LogDebug() << endl;
    }

    DIR *dir = ::opendir(path.c_str());
    if ( dir ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Path already exists, space name: " << name << " path: " <<
            path << " errno: " << to_string(errno) << " " << string(__FUNCTION__) << SoeLogger::LogError() << endl;
        return;
    }

    int ret = ::mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if ( ret ) {
        throw RcsdbFacade::Exception("Can't create path, space name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + string(__FUNCTION__), Exception::eOsError);
    }

    string all_lines = CreateConfig();

    string config = path + "/" + RcsdbName::db_elem_filename;
    ofstream cfile(config.c_str());
    cfile << all_lines;
    cfile.close();
}

void Space::Put(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.store_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    if ( args.store_idx >= Space::max_stores ) {
        throw RcsdbFacade::Exception("Out-of-bounds store idx: " + to_string(args.store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( !stores[args.store_idx] ) {
        throw RcsdbFacade::Exception("Wrong store: " + to_string(args.store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( stores[args.store_idx]->id == args.store_id &&
            (checking_names && *args.store_name == stores[args.store_idx]->name) ) {
        stores[args.store_idx]->Put(args);
        return;
    }
    throw RcsdbFacade::Exception("Store in wrong state idx: " + to_string(args.store_idx) + " " + string(__FUNCTION__), Exception::eInvalidStoreState);
}

void Space::Get(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.store_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    if ( args.store_idx >= Space::max_stores ) {
        throw RcsdbFacade::Exception("Out-of-bounds store idx: " + to_string(args.store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( !stores[args.store_idx] ) {
        throw RcsdbFacade::Exception("Wrong store: " + to_string(args.store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( stores[args.store_idx]->id == args.store_id &&
            (checking_names && *args.store_name == stores[args.store_idx]->name) ) {
        string value;
        stores[args.store_idx]->Get(args);
        return;
    }
    throw RcsdbFacade::Exception("Store in wrong state idx: " + to_string(args.store_idx) + " " + string(__FUNCTION__, Exception::eInvalidStoreState));
}

void Space::GetRange(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.store_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    if ( args.store_idx >= Space::max_stores ) {
        throw RcsdbFacade::Exception("Out-of-bounds store idx: " + to_string(args.store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( !stores[args.store_idx] ) {
        throw RcsdbFacade::Exception("Wrong store: " + to_string(args.store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( stores[args.store_idx]->id == args.store_id &&
            (checking_names && *args.store_name == stores[args.store_idx]->name) ) {
        string value;
        stores[args.store_idx]->GetRange(args);
        return;
    }
    throw RcsdbFacade::Exception("Store in wrong state idx: " + to_string(args.store_idx) + " " + string(__FUNCTION__), Exception::eInvalidStoreState);
}

void Space::Delete(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.store_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    if ( args.store_idx >= Space::max_stores ) {
        throw RcsdbFacade::Exception("Out-of-bounds store idx: " + to_string(args.store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( !stores[args.store_idx] ) {
        throw RcsdbFacade::Exception("Wrong store: " + to_string(args.store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if (  stores[args.store_idx]->id == args.store_id &&
            (checking_names && *args.store_name == stores[args.store_idx]->name) ) {
        stores[args.store_idx]->Delete(args);
        return;
    }
    throw RcsdbFacade::Exception("Store in wrong state idx: " + to_string(args.store_idx) + " " + string(__FUNCTION__), Exception::eInvalidStoreState);
}

void Space::StartTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__), Exception::eOpNotSupported);
}

void Space::CommitTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__), Exception::eOpNotSupported);
}

void Space::RollbackTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__), Exception::eOpNotSupported);
}

void Space::CreateSnapshotEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__), Exception::eOpNotSupported);
}

void Space::CreateSnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__), Exception::eOpNotSupported);
}

void Space::DestroySnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__), Exception::eOpNotSupported);
}

void Space::ListSnapshots(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__), Exception::eOpNotSupported);
}

void Space::Merge(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.store_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    if ( args.store_idx >= Space::max_stores ) {
        throw RcsdbFacade::Exception("Out-of-bounds store idx: " + to_string(args.store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( !stores[args.store_idx] ) {
        throw RcsdbFacade::Exception("Wrong store: " + to_string(args.store_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( stores[args.store_idx]->id == args.store_id &&
            (checking_names && *args.store_name == stores[args.store_idx]->name) ) {
        stores[args.store_idx]->Merge(args);
        return;
    }
    throw RcsdbFacade::Exception("Store in wrong state idx: " + to_string(args.store_idx) + " " + string(__FUNCTION__), Exception::eInvalidStoreState);

}

void Space::CreateGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__), Exception::eOpNotSupported);
}

void Space::DestroyGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__), Exception::eOpNotSupported);
}

void Space::WriteGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__), Exception::eOpNotSupported);
}



//
// Miscellaneous functions
//
bool Space::StopAndArchiveName(LocalArgsDescriptor &args)
{
    for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
        if ( Space::stores[i] && Space::stores[i]->name == *args.store_name &&
                Space::stores[i] && Space::stores[i]->id == args.store_id ) {
            bool ret = Space::stores[i]->StopAndArchiveName(args);
            if ( ret != true ) {
                return ret;
            }
            Space::stores[i] = 0;
        }
    }
    SetNextAvailable();
    state = eSpaceDestroying;

    Cluster::ArchiveName(this);
    return true;

}

bool Space::StopAndArchiveStore(LocalArgsDescriptor &args)
{
    bool ret = false;
#ifdef __DEBUG_DESTROY__
    strcat(args.buf, " stores: ");
#endif
    for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
#ifdef __DEBUG_DESTROY__
        if ( Space::stores[i] ) {
            char tttt[128];
            strcat(args.buf, Space::stores[i]->name.c_str());
            strcat(args.buf, " id ");
            sprintf(tttt, " %lu ", Space::stores[i]->id);
            strcat(args.buf, tttt);
            strcat(args.buf, " ");
        }
#endif
        if ( Space::stores[i] && Space::stores[i]->name == *args.store_name &&
                Space::stores[i]->id == args.store_id ) {
            ret = Space::stores[i]->StopAndArchiveName(args);
            if ( ret == true ) {
                Space::stores[i] = 0;
            }
            break;
        }
    }

    return ret;
}

bool Space::DeleteStore(RcsdbLocalFacade::Store *st)
{
#ifdef __DEBUG_DESTROY__
    char debug_buf[2048];
    strcat(debug_buf, " stores: ");
#endif
    for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
#ifdef __DEBUG_DESTROY__
        if ( Space::stores[i] ) {
            char tttt[128];
            strcat(debug_buf, Space::stores[i]->name.c_str());
            strcat(debug_buf, " id ");
            sprintf(tttt, " %lu ", Space::stores[i]->id);
            strcat(debug_buf, tttt);
            strcat(debug_buf, " ");
        }
#endif
        if ( Space::stores[i] && Space::stores[i]->name == st->name &&
                Space::stores[i]->id == st->id ) {
            Store *st = Space::stores[i];
            Space::stores[i] = 0;
            st->DeregisterApiFunctions();
            delete st;
            return true;
        }
    }

    return false;
}

void Space::AsJson(string &json) throw(RcsdbFacade::Exception)
{
    json += "{\n";
    json += string("\"containers\" : [\n");

    for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
        if ( Space::stores[i] ) {
            Space::stores[i]->AsJson(json);
            uint32_t j;
            for ( j = i + 1 ; j < Space::max_stores ; j++ ) {
                if ( Space::stores[j] ) {
                    json += ",\n";
                    break;
                }
            }
            if ( j >= Space::max_stores ) {
                json += "\n";
            }
        }
    }

    json += "],\n";
    json += "\"id\" : " + to_string(id) + ",\n";
    json += "\"name\" : \"" + name + "\"\n}";
}

void Space::CreateBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__), Exception::eOpNotSupported);
}

void Space::CreateBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__), Exception::eOpNotSupported);
}


void Space::RestoreBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Space::DestroyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Space::VerifyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Space::DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__), Exception::eOpNotSupported);
}

void Space::ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__), Exception::eOpNotSupported);
}

uint64_t Space::CreateMeta(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    return Space::CreateSpace(args);
}

void Space::DestroyMeta(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    Space::DestroySpace(args);
}

uint64_t Space::GetMetaByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    return Space::GetSpaceByName(args);
}

}
