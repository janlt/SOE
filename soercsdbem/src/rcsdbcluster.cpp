/**
 * @file    rcsdbcluster.cpp
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
 * rcsdbcluster.cpp
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
#include <boost/thread.hpp>
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
#include "rcsdbfilters.hpp"
#include "rcsdbfacade.hpp"
#include "rcsdbsnapshot.hpp"
#include "rcsdbstorebackup.hpp"
#include "rcsdbstoreiterator.hpp"
#include "rcsdbstore.hpp"
#include "rcsdbspace.hpp"
#include "rcsdbcluster.hpp"

extern char *__progname;

using namespace std;
using namespace RcsdbFacade;

void __attribute__ ((constructor)) RcsdbLocalFacade::StaticInitializerEnter(void);
void __attribute__ ((destructor)) RcsdbLocalFacade::StaticInitializerExit(void);

namespace RcsdbLocalFacade {

//
// Cluster
//
uint32_t Cluster::max_spaces = 32;
uint32_t Cluster::max_clusters = 64;

uint64_t Cluster::cluster_null_id = -1;  // max 64 bit number

Cluster *Cluster::clusters[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
Lock     Cluster::global_lock;

bool Cluster::global_debug = false;
Lock     Cluster::archive_lock;
list<RcsdbFacade::NameBase *>  Cluster::archive;
const char *Cluster::metadbsrv_name = "soemetadbsrv";


//
// Initializer
//
Cluster::Initializer *Cluster::Initializer::instance = 0;
bool Cluster::Initializer::initialized = false;

Cluster::Initializer *Cluster::Initializer::CheckInstance()
{
    if ( !Cluster::Initializer::instance ) {
        Cluster::Initializer::Initialize();
    }
    return Cluster::Initializer::instance;
}

void Cluster::Initializer::Initialize()
{
    WriteLock w_lock(Cluster::global_lock);

    if ( !Initializer::instance ) {
        Initializer::instance = new Cluster::Initializer();
    }
}

Cluster::Initializer::Initializer()
{
    //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << __progname << SoeLogger::LogError() << endl;
    if ( string(Cluster::metadbsrv_name)  == __progname ) {
        RcsdbName::db_root_path = RcsdbName::db_root_path_srv;
    }

    DIR *dir = ::opendir(RcsdbName::db_root_path);
    if ( !dir ) {
        int ret = ::mkdir(RcsdbName::db_root_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if ( ret ) {
            ::closedir(dir);
            throw RcsdbFacade::Exception(string("Can't create root dir: ") +
                    RcsdbName::db_root_path + " errno: " + to_string(errno) + " " + string(__FUNCTION__));
        }

        dir = ::opendir(RcsdbName::db_root_path);
        if ( !dir ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << string("Can't create root: ") << RcsdbName::db_root_path <<
                to_string(errno) << " "<< string(__FUNCTION__) << SoeLogger::LogError() << endl;
            return;
        }
    }
    ::closedir(dir);

    if ( Initializer::initialized == false ) {
        if ( Cluster::global_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Dir: " << RcsdbName::db_root_path << SoeLogger::LogDebug() << endl;
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "config: " << RcsdbName::db_elem_filename << SoeLogger::LogDebug() << endl;
        }
        Cluster::Discover();
    }
    Initializer::initialized = true;
}

Cluster::Initializer::~Initializer()
{
    WriteLock w_lock(Cluster::global_lock);

    if ( Initializer::initialized == true ) {
        if ( Cluster::global_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
        }
        Cluster::Destroy();
    }
    Initializer::initialized = false;
}


//
// Static initalizer
//
Cluster::StaticInitializer *static_initializer_ptr = 0;
Cluster::StaticInitializer static_initializer;

void StaticInitializerEnter(void)
{
    static_initializer_ptr = &static_initializer;
}

void StaticInitializerExit(void)
{
    static_initializer_ptr = 0;
}

bool Cluster::StaticInitializer::static_initialized = false;

boost::thread *Cluster::pruner_thread = 0;

Cluster::StaticInitializer::StaticInitializer()
{
    WriteLock w_lock(Cluster::global_lock);

    if ( StaticInitializer::static_initialized == false ) {
        try {
            Cluster::RegisterApiStaticFunctions();
            Space::RegisterApiStaticFunctions();
            Store::RegisterApiStaticFunctions();

            if ( string(Cluster::metadbsrv_name)  == __progname ) {
                pruner_thread = new boost::thread(&Cluster::PruneArchive);
            }
        } catch (const SoeApi::Exception &ex) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Register static functions failed: " << SoeLogger::LogError() << endl;
        }
    }
    StaticInitializer::static_initialized = true;
}

Cluster::StaticInitializer::~StaticInitializer()
{
    WriteLock w_lock(Cluster::global_lock);

    if ( StaticInitializer::static_initialized == true ) {
        try {
            Cluster::DeregisterApiStaticFunctions();
            Space::DeregisterApiStaticFunctions();
            Store::DeregisterApiStaticFunctions();

            if ( Cluster::pruner_thread ) {
                delete Cluster::pruner_thread;
            }
        } catch (const SoeApi::Exception &ex) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Deregister static functions failed: " << SoeLogger::LogError() << endl;
        }
    }
    StaticInitializer::static_initialized = false;
}



//
// Cluster
//
void Cluster::Destroy()
{
    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] ) {
            delete Cluster::clusters[i];
            Cluster::clusters[i] = 0;
        }
    }
}

void Cluster::Discover()
{
    DIR *dir = ::opendir(RcsdbName::db_root_path);
    if ( !dir ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << string("Can't open root: ") << RcsdbName::db_root_path <<
            to_string(errno) << " "<< string(__FUNCTION__) << SoeLogger::LogError() << endl;
        return;
    }

    struct dirent *dir_entry;
    uint32_t cnt = 0;
    while ( (dir_entry = readdir(dir)) ) {
        if ( !dir_entry ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't read space dir: " << " errno: " <<
                to_string(errno) << " " << string(__FUNCTION__) << SoeLogger::LogError() << endl;
            return;
        }

        if ( string(dir_entry->d_name) == "." || string(dir_entry->d_name) == ".." ) {
            continue;
        }
        if ( Cluster::global_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Dir file: " << dir_entry->d_name << " " << string(__FUNCTION__) << SoeLogger::LogDebug() << endl;
        }

        string cfg = string(RcsdbName::db_root_path) + "/" + dir_entry->d_name + "/" + RcsdbName::db_elem_filename;
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
            cfile >> line;
            //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "line: " << line;
            all_lines += line;
        }

        Cluster::clusters[cnt] = new Cluster("xxxx", Cluster::cluster_null_id, true, false);
        Cluster::clusters[cnt]->SetParent(Cluster::clusters[cnt]);
        Cluster::clusters[cnt]->ParseConfig(all_lines, true);
        Cluster::clusters[cnt]->SetPath();
        try {
            Cluster::clusters[cnt]->RegisterApiFunctions();
        } catch(const SoeApi::Exception &ex) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't register space functions: " <<
                ex.what() << " " << string(__FUNCTION__) << SoeLogger::LogError() << endl;
        }
        Cluster::clusters[cnt]->state = Cluster::eClusterOpened;
        cfile.close();
        if ( cnt++ >= Cluster::max_clusters ) {
            break;
        }
    }
    ::closedir(dir);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] ) {
            Cluster::clusters[i]->DiscoverSpaces();
        }
    }
}

void Cluster::DiscoverSpaces()
{
    string path = string(RcsdbName::db_root_path) + "/" + name;
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
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't read space dir: " << " errno: " <<
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
            cfile >> line;
            //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "line: " << line;
            all_lines += line;
        }

        Cluster::spaces[cnt] = new Space("yyyy", Space::space_null_id, true, false);
        Cluster::spaces[cnt]->SetParent(this);
        Cluster::spaces[cnt]->ParseConfig(all_lines, true);
        Cluster::spaces[cnt]->SetPath();
        try {
            Cluster::spaces[cnt]->RegisterApiFunctions();
        } catch(const SoeApi::Exception &ex) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't register space functions: " <<
                ex.what() << " " << string(__FUNCTION__) << SoeLogger::LogError() << endl;
        }
        Cluster::spaces[cnt]->state = Space::eSpaceOpened;
        cfile.close();
        if ( cnt++ >= Cluster::max_spaces ) {
            break;
        }
    }
    ::closedir(dir);

    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] ) {
            Cluster::spaces[i]->DiscoverStores(name);
        }
    }
}


//
// Non static iterator like functions
//
Cluster *Cluster::GetCluster(LocalArgsDescriptor &args,
        bool ch_names,
        bool _debug) throw(RcsdbFacade::Exception)
{
    // check if name already exist
    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] && Cluster::clusters[i]->name == *args.cluster_name ) {
            return Cluster::clusters[i];
        }
    }

    // check if id is a duplicate
    if ( args.cluster_id != Cluster::cluster_invalid_id ) {
        for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
            if ( Cluster::clusters[i] && Cluster::clusters[i]->id == args.cluster_id ) {
                throw RcsdbFacade::Exception("Duplicate id for new name: " + *args.cluster_name + " id" + to_string(args.cluster_id),
                    RcsdbFacade::Exception::eInvalidArgument);
            }
        }
    }

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( !Cluster::clusters[i] ) {
            Cluster::clusters[i] = new Cluster(*args.cluster_name,
                args.cluster_id == Cluster::cluster_invalid_id ? Cluster::cluster_start_id + i : args.cluster_id,
                ch_names,
                _debug);
            Cluster::clusters[i]->SetParent(Cluster::clusters[i]);
            Cluster::clusters[i]->SetPath();
            Cluster::clusters[i]->Create();
            try {
                Cluster::clusters[i]->RegisterApiFunctions();
            } catch(const SoeApi::Exception &ex) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't register cluster functions: " <<
                    ex.what() << " " << string(__FUNCTION__) << SoeLogger::LogError() << endl;
            }
            Cluster::clusters[i]->state = Cluster::eClusterOpened;
            return Cluster::clusters[i];
        }
    }

    throw RcsdbFacade::Exception("No more cluster slots available", Exception::eOsResourceExhausted);
}

Space *Cluster::GetSpace(LocalArgsDescriptor &args,
        bool ch_names,
        bool _debug) throw(RcsdbFacade::Exception)
{
    // check if the name already exists
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] && Cluster::spaces[i]->name == *args.space_name ) {
            return Cluster::spaces[i];
        }
    }

    // check if id is a duplicate
    if ( args.space_id != Space::space_invalid_id ) {
        for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
            if ( Cluster::spaces[i] && Cluster::spaces[i]->id == args.space_id ) {
                throw RcsdbFacade::Exception("Duplicate id for new name: " + *args.space_name + " id" + to_string(args.space_id),
                    RcsdbFacade::Exception::eInvalidArgument);
            }
        }
    }

    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( !Cluster::spaces[i] ) {
            Cluster::spaces[i] = new Space(*args.space_name,
                args.space_id == Space::space_invalid_id ? Space::space_start_id + i : args.space_id,
                ch_names,
                _debug);
            Cluster::spaces[i]->SetParent(this);
            Cluster::spaces[i]->SetPath();
            Cluster::spaces[i]->Create();
            try {
                Cluster::spaces[i]->RegisterApiFunctions();
            } catch(const SoeApi::Exception &ex) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't register space functions: " <<
                    ex.what() << " " << string(__FUNCTION__) << SoeLogger::LogError() << endl;
            }
            Cluster::spaces[i]->state = Space::eSpaceOpened;
            return Cluster::spaces[i];
        }
    }

    throw RcsdbFacade::Exception("No more cluster slots available", Exception::eOsResourceExhausted);
}

Store *Cluster::GetStore(LocalArgsDescriptor &args,
        bool ch_names,
        bool _debug) throw(RcsdbFacade::Exception)
{
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] && Cluster::spaces[i]->name == *args.space_name &&
            Cluster::spaces[i]->id == args.space_id ) {
            return Cluster::spaces[i]->GetStore(args, ch_names, _debug);
        }
    }

    throw RcsdbFacade::Exception("Space name: " + *args.space_name + " not found " + string(__FUNCTION__), Exception::eInvalidArgument);
}


//
// Create functions
//
uint64_t Cluster::CreateCluster(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.cluster_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    (void )Cluster::Initializer::CheckInstance();

    WriteLock w_lock(Cluster::global_lock);

    Cluster::global_debug = static_cast<bool>(args.status);

    Cluster *clu = Cluster::GetCluster(args, true, false);
    if ( clu ) {
        return clu->id;
    }
    return Cluster::cluster_null_id;
}

uint64_t Cluster::CreateSpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.cluster_name || !args.space_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    (void )Cluster::Initializer::CheckInstance();

    WriteLock w_lock(Cluster::global_lock);

    Cluster::global_debug = static_cast<bool>(args.status);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] &&
            Cluster::clusters[i]->name == *args.cluster_name &&
            Cluster::clusters[i]->id == args.cluster_id ) {
            Space *sp = Cluster::clusters[i]->GetSpace(args, true, false);
            if ( sp ) {
                return sp->id;
            }
            break;
        }
    }

    throw RcsdbFacade::Exception("Cluster name: " + *args.cluster_name + " not found " + string(__FUNCTION__));
}

uint64_t Cluster::CreateStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.cluster_name || !args.space_name || !args.store_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    (void )Cluster::Initializer::CheckInstance();

    WriteLock w_lock(Cluster::global_lock);

    Cluster::global_debug = static_cast<bool>(args.status);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] &&
            Cluster::clusters[i]->name == *args.cluster_name &&
            Cluster::clusters[i]->id == args.cluster_id ) {
            Store *st = Cluster::clusters[i]->GetStore(args, true, Cluster::global_debug);
            if ( st ) {
                args.store_id = st->id;
                return st->id;
            }
            break;
        }
    }

    throw RcsdbFacade::Exception("Cluster name: " + *args.cluster_name + " not found " + string(__FUNCTION__), Exception::eInvalidArgument);
}

//
// Destroy functions
// The general approach is that because operations may be in progress we need to:
//   1. Put the object on the list to be destroyed
//   2. Together, tint it so that new i/os won't get started (throw exception)
//   3. Quiesce all the i/os
//   4. Once there are no i/os and clients hold no references destroy it, i.e.
//      remove the contents on disk and delete the object in memory
//
void Cluster::DestroyCluster(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.cluster_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    (void )Cluster::Initializer::CheckInstance();

    WriteLock w_lock(Cluster::global_lock);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] && Cluster::clusters[i]->name == *args.cluster_name &&
            Cluster::clusters[i]->id == args.cluster_id ) {
            bool ret = Cluster::clusters[i]->StopAndArchiveName(args);
            if ( ret != true ) {
                throw RcsdbFacade::Exception("Can't destroy cluster: " + *args.cluster_name + " id: " +
                    to_string(args.cluster_id) + " " + string(__FUNCTION__), Exception::eStoreBusy);
            }
            Cluster::clusters[i] = 0;
        }
    }

    throw RcsdbFacade::Exception("Cluster not found: " + *args.cluster_name + " id: " +
        to_string(args.cluster_id) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
}

void Cluster::DestroySpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.cluster_name || !args.space_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    (void )Cluster::Initializer::CheckInstance();

    WriteLock w_lock(Cluster::global_lock);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] && Cluster::clusters[i]->name == *args.cluster_name &&
            Cluster::clusters[i]->id == args.cluster_id ) {
            bool ret = Cluster::clusters[i]->StopAndArchiveSpace(args);
            if ( ret != true ) {
                throw RcsdbFacade::Exception("Can't destroy space, cluster: " + *args.cluster_name + " id: " +
                    to_string(args.cluster_id) +
                    " space: " + *args.space_name + " space_id: " + to_string(args.space_id) +
                    " " + string(__FUNCTION__), Exception::eStoreBusy);
            }
            return;
        }
    }

    throw RcsdbFacade::Exception("Space not found, cluster: " + *args.cluster_name + " id: " +
        to_string(args.cluster_id) +
        " space: " + *args.space_name + " space_id: " + to_string(args.space_id) +
        " " + string(__FUNCTION__), Exception::eInvalidArgument);
}

void Cluster::DestroyStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.cluster_name || !args.space_name || !args.store_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    (void )Cluster::Initializer::CheckInstance();

#ifdef __DEBUG_DESTROY__
    char bbbb[10000];
    memset(bbbb, '\0', sizeof(bbbb));
    args.buf = bbbb;
#endif
    WriteLock w_lock(Cluster::global_lock);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] && Cluster::clusters[i]->name == *args.cluster_name &&
            Cluster::clusters[i]->id == args.cluster_id ) {
            bool ret = Cluster::clusters[i]->StopAndArchiveStore(args);
            if ( ret != true ) {
#ifdef __DEBUG_DESTROY__
                throw RcsdbFacade::Exception("Can't destroy store, cluster: " + *args.cluster_name + " id: " +
                    to_string(args.cluster_id) + " buf " + args.buf +
                    " space: " + *args.space_name + " space_id: " + to_string(args.space_id) +
                    " store: " + *args.store_name + " store_id: " + to_string(args.store_id) +
                    " " + string(__FUNCTION__));
#else
                throw RcsdbFacade::Exception("Can't destroy store, cluster: " + *args.cluster_name + " id: " +
                    to_string(args.cluster_id) +
                    " space: " + *args.space_name + " space_id: " + to_string(args.space_id) +
                    " store: " + *args.store_name + " store_id: " + to_string(args.store_id) +
                    " " + string(__FUNCTION__), Exception::eStoreBusy);
#endif
            }
            return;
        }
    }

    throw RcsdbFacade::Exception("Store not found, cluster: " + *args.cluster_name + " id: " +
        to_string(args.cluster_id) +
        " space: " + *args.space_name + " space_id: " + to_string(args.space_id) +
        " store: " + *args.store_name + " store_id: " + to_string(args.store_id) +
        " " + string(__FUNCTION__));
}

void Cluster::DeleteFromStores(RcsdbLocalFacade::Store *st)
{
#ifdef __DEBUG_DESTROY__
    char bbbb[10000];
    memset(bbbb, '\0', sizeof(bbbb));
    args.buf = bbbb;
#endif
    WriteLock w_lock(Cluster::global_lock);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] ) {
            bool ret = Cluster::clusters[i]->DeleteStore(st);
            if ( ret == true ) {
                break;
            }
        }
    }
}


//
// GetByName functions
//
uint64_t Cluster::GetClusterByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.cluster_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    (void )Cluster::Initializer::CheckInstance();

    ReadLock r_lock(Cluster::global_lock);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] && Cluster::clusters[i]->name == *args.cluster_name ) {
            return Cluster::clusters[i]->id;
        }
    }
    return Cluster::cluster_null_id;
}

uint64_t Cluster::GetSpaceByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.cluster_name || !args.space_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    (void )Cluster::Initializer::CheckInstance();

    ReadLock r_lock(Cluster::global_lock);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] && Cluster::clusters[i]->name == *args.cluster_name &&
            Cluster::clusters[i]->id == args.cluster_id ) {
            Space *sp = Cluster::clusters[i]->FindSpace(args);
            if ( sp ) {
                return sp->id;
            }
            break;
        }
    }
    return Space::space_null_id;
}

uint64_t Cluster::GetStoreByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.cluster_name || !args.space_name || !args.store_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    (void )Cluster::Initializer::CheckInstance();

    ReadLock r_lock(Cluster::global_lock);

    bool no_id_check = args.range_callback_arg == (char *)LD_GetByName;
    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] && Cluster::clusters[i]->name == *args.cluster_name &&
            (((no_id_check == false) && Cluster::clusters[i]->id == args.cluster_id) ||
            ((no_id_check == true) && args.cluster_id == Store::store_null_id)) ) {
            if ( no_id_check == true ) {
                args.cluster_id = Cluster::clusters[i]->id;
            }

            Store *st = Cluster::clusters[i]->FindStore(args);
            if ( st ) {
                return st->id;
            }
            break;
        }
    }
    return Store::store_null_id;
}

//
// Find functions
//
Space *Cluster::FindSpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] && Cluster::spaces[i]->name == *args.space_name ) {
            return Cluster::spaces[i];
        }
    }
    return 0;
}

Store *Cluster::FindStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    bool no_id_check = args.range_callback_arg == (char *)LD_GetByName;
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] && Cluster::spaces[i]->name == *args.space_name &&
            (((no_id_check == false) && Cluster::spaces[i]->id == args.space_id) ||
            ((no_id_check == true) && args.space_id == Space::space_null_id)) ) {
            if ( no_id_check == true ) {
                args.space_id = Cluster::spaces[i]->id;
            }

            return Cluster::spaces[i]->FindStore(args);
        }
    }
    return 0;
}

void Cluster::AsJson(string &json) throw(RcsdbFacade::Exception)
{
    json += string("{\n") + "\"cluster_name\" : " + "\"" + name + "\",\n";
    json += string("\"largest_container_id\" : ") + to_string(Cluster::cluster_null_id) + ",\n";
    json += string("\"namespaces\" : [\n");
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] ) {
            Cluster::spaces[i]->AsJson(json);
            uint32_t j;
            for ( j = i + 1 ; j < Cluster::max_spaces ; j++ ) {
                if ( Cluster::spaces[j] ) {
                    json += ",\n";
                    break;
                }
            }
            if ( j >= Cluster::max_spaces ) {
                json += "\n";
            }
        }
    }
    json += "],\n";
    json += "\"version\" : 4\n";
    json += "}\n";
}

string Cluster::AsJson(void) throw(RcsdbFacade::Exception)
{
    string json;
    ReadLock r_lock(Cluster::global_lock);

    json += string("{\n") + "\"clusters\" : \n[\n";

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
         if ( Cluster::clusters[i] ) {
             Cluster::clusters[i]->AsJson(json);
             uint32_t j;
             for ( j = i + 1 ; j < Cluster::max_clusters ; j++ ) {
                 if ( Cluster::clusters[j] ) {
                     json += ",\n";
                     break;
                 }
             }
             if ( j >= Cluster::max_clusters ) {
                 json += "\n";
             }
         }
    }

    json += string("]") + "\n}";
    return json;
}
//
// Register API functions
//
void Cluster::RegisterApiStaticFunctions() throw(SoeApi::Exception)
{
    // bind functions to boost functors
    boost::function<uint64_t(LocalArgsDescriptor &)> ClusterLocalCreate =
        boost::bind(&RcsdbLocalFacade::Cluster::CreateCluster, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalDestroy =
        boost::bind(&RcsdbLocalFacade::Cluster::DestroyCluster, _1);

    boost::function<uint64_t(LocalArgsDescriptor &)> ClusterLocalGetByName =
        boost::bind(&RcsdbLocalFacade::Cluster::GetClusterByName, _1);

    boost::function<string(void)> AsJson =
        boost::bind(&RcsdbLocalFacade::Cluster::AsJson);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalListClusters =
        boost::bind(&RcsdbLocalFacade::Cluster::ListClusters, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalListSpaces =
        boost::bind(&RcsdbLocalFacade::Cluster::ListSpaces, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalListStores =
        boost::bind(&RcsdbLocalFacade::Cluster::ListStores, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalListSubsets =
        boost::bind(&RcsdbLocalFacade::Cluster::ListSubsets, _1);

    // register via SoeApi
    new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
        ClusterLocalCreate, RcsdbFacade::CreateLocalName("", "", "", 0, FUNCTION_CREATE), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalDestroy, RcsdbFacade::CreateLocalName("", "", "", 0, FUNCTION_DESTROY), Cluster::global_debug);

    new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
        ClusterLocalGetByName, RcsdbFacade::CreateLocalName("", "", "", 0, FUNCTION_GET_BY_NAME), Cluster::global_debug);

    new SoeApi::Api<string(void)>(
        AsJson, RcsdbFacade::CreateLocalName("", "", "", 0, AS_JSON), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalListClusters, RcsdbFacade::CreateLocalName("", "", "", 0, FUNCTION_LIST_CLUSTERS), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalListSpaces, RcsdbFacade::CreateLocalName("", "", "", 0, FUNCTION_LIST_SPACES), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalListStores, RcsdbFacade::CreateLocalName("", "", "", 0, FUNCTION_LIST_STORES), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalListStores, RcsdbFacade::CreateLocalName("", "", "", 0, FUNCTION_LIST_SUBSETS), Cluster::global_debug);
}

//
// Deregister API functions
//
void Cluster::DeregisterApiStaticFunctions() throw(SoeApi::Exception)
{

}

void Cluster::RegisterApiFunctions() throw(SoeApi::Exception)
{
    // bind functions to boost functors
    boost::function<void(LocalArgsDescriptor &)> ClusterLocalOpen =
        boost::bind(&RcsdbLocalFacade::Cluster::Open, this, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalClose =
        boost::bind(&RcsdbLocalFacade::Cluster::Close, this, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalPut =
        boost::bind(&RcsdbLocalFacade::Cluster::Put, this, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalGet =
        boost::bind(&RcsdbLocalFacade::Cluster::Get, this, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalDelete =
        boost::bind(&RcsdbLocalFacade::Cluster::Delete, this, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalGetRange =
        boost::bind(&RcsdbLocalFacade::Cluster::GetRange, this, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalMerge =
        boost::bind(&RcsdbLocalFacade::Cluster::Merge, this, _1);


    // register via SoeApi
    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalOpen, RcsdbFacade::CreateLocalName(name, "", "", 0, FUNCTION_OPEN), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalClose, RcsdbFacade::CreateLocalName(name, "", "", 0, FUNCTION_CLOSE), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalPut, RcsdbFacade::CreateLocalName(name, "", "", 0, FUNCTION_PUT), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalGet, RcsdbFacade::CreateLocalName(name, "", "", 0, FUNCTION_GET), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalDelete, RcsdbFacade::CreateLocalName(name, "", "", 0, FUNCTION_DELETE), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalGetRange, RcsdbFacade::CreateLocalName(name, "", "", 0, FUNCTION_GET_RANGE), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalMerge, RcsdbFacade::CreateLocalName(name, "", "", 0, FUNCTION_MERGE), Cluster::global_debug);

}

void Cluster::DeregisterApiFunctions() throw(SoeApi::Exception)
{

}

//
// Ctor/Dest functions
//
Cluster::Cluster(const string &nm,
        uint64_t _id,
        bool ch_names,
        bool _debug) throw(RcsdbFacade::Exception):
    name(nm),
    id(_id),
    config(_debug),
    checking_names(ch_names),
    debug(_debug)
{
    RcsdbName::next_available = 0;
    Cluster::global_debug = debug;

    spaces.resize(static_cast<size_t>(Cluster::max_spaces), 0);
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        Cluster::spaces[i] = 0;
    }
}

Cluster::~Cluster()
{
    for ( uint32_t i = 0 ; i < spaces.size() ; i++ ) {
        if ( spaces[i] ) {
            delete spaces[i];
        }
    }
}

void Cluster::SetNextAvailable()
{
    for ( uint32_t i = 0 ; i < spaces.size() ; i++ ) {
        if ( !spaces[i] ) {
            RcsdbName::next_available = i;
            break;
        }
    }
}

void Cluster::SetPath()
{
    path = string(RcsdbName::db_root_path) + "/" + name;
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "path: " << path << " " << string(__FUNCTION__) << SoeLogger::LogDebug() << endl;
    }
}

const string &Cluster::GetPath()
{
    return path;
}

void Cluster::ParseConfig(const string &cfg, bool no_exc)
{
    config.Parse(cfg, no_exc);
    name = config.name;
    id = config.id;
}

string Cluster::CreateConfig()
{
    config.name = name;
    config.id = id;
    return config.Create();
}

string Cluster::ConfigAddSnapshot(const string &name, uint32_t id)
{
    return config.Create();
}

string Cluster::ConfigRemoveSnapshot(const string &name, uint32_t id)
{
    return config.Create();
}

string Cluster::ConfigAddBackup(const string &name, uint32_t id)
{
    return config.Create();
}

string Cluster::ConfigRemoveBackup(const string &name, uint32_t id)
{
    return config.Create();
}

void Cluster::Create()  throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "path: " << path << SoeLogger::LogDebug() << endl;
    }

    DIR *dir = ::opendir(path.c_str());
    if ( dir ) {
        ::closedir(dir);
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Path already exists, cluster name: " << name << " path: " <<
            path << " errno: " << to_string(errno) << " " << string(__FUNCTION__) << SoeLogger::LogError() << endl;
        return;
    }

    int ret = ::mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if ( ret ) {
        throw RcsdbFacade::Exception("Can't create path, cluster name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + string(__FUNCTION__), Exception::eOsError);
    }

    string all_lines = CreateConfig();

    string config = path + "/" + RcsdbName::db_elem_filename;
    ofstream cfile(config.c_str());
    cfile << all_lines;
    cfile.close();
}


//
// Miscellaneous functions
// WARNING: This operation destroys the entire cluster
//
bool Cluster::StopAndArchiveName(LocalArgsDescriptor &args)
{
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] && Cluster::spaces[i]->name == *args.space_name &&
                Cluster::spaces[i] && Cluster::spaces[i]->id == args.space_id ) {
            bool ret = Cluster::spaces[i]->StopAndArchiveName(args);
            if ( ret != true ) {
                return ret;
            }
            Cluster::spaces[i] = 0;
        }
    }
    SetNextAvailable();
    state = eClusterDestroying;
    Cluster::ArchiveName(this);

    return true;
}

bool Cluster::StopAndArchiveSpace(LocalArgsDescriptor &args)
{
    bool ret = false;
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] && Cluster::spaces[i]->name == *args.space_name &&
                Cluster::spaces[i]->id == args.space_id ) {
            ret = Cluster::spaces[i]->StopAndArchiveName(args);
            if ( ret == true ) {
                Cluster::spaces[i] = 0;
            }
            break;
        }
    }

    return ret;
}

bool Cluster::StopAndArchiveStore(LocalArgsDescriptor &args)
{
#ifdef __DEBUG_DESTROY__
    strcat(args.buf, "spaces: ");
#endif
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
#ifdef __DEBUG_DESTROY__
        if ( Cluster::spaces[i] ) {
            char tttt[128];
            strcat(args.buf, Cluster::spaces[i]->name.c_str());
            strcat(args.buf, " id ");
            sprintf(tttt, " %lu ", Cluster::spaces[i]->id);
            strcat(args.buf, tttt);
            strcat(args.buf, " ");
        }
#endif
        if ( Cluster::spaces[i] && Cluster::spaces[i]->name == *args.space_name &&
                Cluster::spaces[i]->id == args.space_id ) {
            return Cluster::spaces[i]->StopAndArchiveStore(args);
        }
    }

    return false;
}

bool Cluster::DeleteStore(RcsdbLocalFacade::Store *st)
{
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] ) {
            bool ret = Cluster::spaces[i]->DeleteStore(st);
            if ( ret == true ) {
                break;
            }
        }
    }

    return false;
}

void Cluster::ArchiveName(RcsdbFacade::NameBase *name)
{
    RcsdbLocalFacade::Store *st = dynamic_cast<RcsdbLocalFacade::Store *>(name);

    WriteLock w_lock(Cluster::archive_lock);
    Cluster::archive.push_back(st);
}

void Cluster::PruneArchive()
{
//    for ( ;; ) {
//        sleep(10);
//        cout << "Cluster::" << __FUNCTION__ << " size: " << Cluster::archive.size() << endl;
//    }
//    return;

    for ( ;; ) {
        sleep(2);
        vector<RcsdbLocalFacade::Store *> to_delete;
        //cout << "Cluster::" << __FUNCTION__ << " size: " << Cluster::archive.size() << endl;
        {
            WriteLock w_lock(Cluster::archive_lock);

            bool found_some = true;
            while ( found_some == true ) {
                found_some = false;
                for ( list<RcsdbFacade::NameBase *>::iterator it = Cluster::archive.begin() ; it != Cluster::archive.end() ; it++ ) {
                    RcsdbLocalFacade::Store *st = dynamic_cast<RcsdbLocalFacade::Store *>(*it);
                    if ( st->in_archive_ttl++ > 4 ) {
                        if ( st->state == RcsdbLocalFacade::Store::eStoreState::eStoreDestroyed ) {
                            //cout << __FUNCTION__ << " name: " << st->name << " state: " << st->state << " rcsdb_ref_count: " << st->rcsdb_ref_count << endl;

                            // remove from list and add to to_delete
                            Cluster::archive.erase(it);
                            to_delete.push_back(dynamic_cast<RcsdbLocalFacade::Store *>(*it));
                            found_some = true;
                            break;
                        }
                    }
                }
            }
        }

        // delete outside of Cluster::archive_lock to avoid deadlock, using the to_delte vector
        if ( to_delete.size() > 0 ) {
            cout << "Cluster::" << __FUNCTION__ << " to_delete size: " << to_delete.size() << endl;
        }
        for ( uint32_t i = 0 ; i < to_delete.size() ; i++ ) {
            to_delete[i]->DeregisterApi();
            delete to_delete[i];
            to_delete[i] = 0;
        }
        to_delete.clear();
    }
}

//
// Dynamic API functions
//
void Cluster::Open(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    DIR *dir = ::opendir(path.c_str());
    if ( !dir ) {
        ::closedir(dir);
        throw RcsdbFacade::Exception("Can't open cluster name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + string(__FUNCTION__), Exception::eOsError);
    }

    string config = path + "/" + RcsdbName::db_elem_filename;
    string line;
    string all_lines;
    ifstream cfile(config.c_str());
    if ( cfile.is_open() != true ) {
        ::closedir(dir);
        throw RcsdbFacade::Exception("Can't open config, name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + string(__FUNCTION__));
    }
    while ( cfile.eof() != true ) {
        cfile >> line;
        //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "line: " << line;
        all_lines += line;
    }

    ParseConfig(all_lines);
    cfile.close();
    ::closedir(dir);
}

void Cluster::Close(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("NOP " + string(__FUNCTION__));
}

void Cluster::Repair(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("NOP " + string(__FUNCTION__));
}

void Cluster::Put(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.space_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    ReadLock r_lock(Cluster::global_lock);

    if ( args.space_idx >= Cluster::max_spaces  || !spaces[args.space_idx] ) {
        throw RcsdbFacade::Exception("Wrong space_idx: " + to_string(args.space_idx) + " " + string(__FUNCTION__));
    }
    if ( spaces[args.space_idx]->id != args.space_id ||
            (checking_names == true && spaces[args.space_idx]->name != *args.space_name) ) {
        throw RcsdbFacade::Exception("Wrong space: " + *args.space_name + " id: " + to_string(args.space_id) +
            " []->name: " + spaces[args.space_idx]->name + " []->id: " +
            to_string(spaces[args.space_idx]->id) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    spaces[args.space_idx]->Put(args);
}

void Cluster::Get(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.space_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    ReadLock r_lock(Cluster::global_lock);

    if ( args.space_idx >= Cluster::max_spaces  || !spaces[args.space_idx] ) {
        throw RcsdbFacade::Exception("Wrong space_idx: " + to_string(args.space_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( spaces[args.space_idx]->id != args.space_id ||
            (checking_names == true && spaces[args.space_idx]->name != *args.space_name) ) {
        throw RcsdbFacade::Exception("Wrong space: " + *args.space_name + " id: " + to_string(args.space_id) +
            " []->name: " + spaces[args.space_idx]->name + " []->id: " +
            to_string(spaces[args.space_idx]->id) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    spaces[args.space_idx]->Get(args);
}

void Cluster::Delete(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.space_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    ReadLock r_lock(Cluster::global_lock);

    if ( args.space_idx >= Cluster::max_spaces  || !spaces[args.space_idx] ) {
        throw RcsdbFacade::Exception("Wrong space_idx: " + to_string(args.space_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( spaces[args.space_idx]->id != args.space_id ||
            (checking_names == true && spaces[args.space_idx]->name != *args.space_name) ) {
        throw RcsdbFacade::Exception("Wrong space: " + *args.space_name + " id: " + to_string(args.space_id) +
            " []->name: " + spaces[args.space_idx]->name + " []->id: " +
            to_string(spaces[args.space_idx]->id) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    spaces[args.space_idx]->Delete(args);
}

void Cluster::GetRange(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Call GetRange on individual Store instead " + string(__FUNCTION__));
}

void Cluster::StartTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::CommitTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::RollbackTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::CreateSnapshotEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::CreateSnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::DestroySnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::ListSnapshots(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::Merge(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.space_name ) {
        throw RcsdbFacade::Exception("Bad argument " + string(__FUNCTION__), Exception::eInvalidArgument);
    }

    ReadLock r_lock(Cluster::global_lock);

    if ( args.space_idx >= Cluster::max_spaces  || !spaces[args.space_idx] ) {
        throw RcsdbFacade::Exception("Wrong space_idx: " + to_string(args.space_idx) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    if ( spaces[args.space_idx]->id != args.space_id ||
            (checking_names == true && spaces[args.space_idx]->name != *args.space_name) ) {
        throw RcsdbFacade::Exception("Wrong space: " + *args.space_name + " id: " + to_string(args.space_id) +
            " []->name: " + spaces[args.space_idx]->name + " []->id: " +
            to_string(spaces[args.space_idx]->id) + " " + string(__FUNCTION__), Exception::eInvalidArgument);
    }
    spaces[args.space_idx]->Merge(args);

}

void Cluster::CreateGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::DestroyGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::WriteGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::CreateBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::CreateBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::RestoreBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::DestroyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::VerifyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

void Cluster::ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception("Function not supported " + string(__FUNCTION__));
}

uint64_t Cluster::CreateMeta(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    return Cluster::CreateCluster(args);
}

void Cluster::DestroyMeta(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    Cluster::DestroyCluster(args);
}

uint64_t Cluster::GetMetaByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    return Cluster::GetClusterByName(args);
}

#define _METADBSRV_TEST_LIST_

void Cluster::ListSubsets(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
#ifdef _METADBSRV_TEST_LIST_
    for ( uint32_t i = 0 ; i < 8 ; i++ ) {
        args.list->push_back(std::pair<uint64_t, std::string>(i + 2223456, string("LOCAL_SUBSET_SRUBSET_") + to_string(i)));
    }
#endif
}

void Cluster::ListStores(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
#ifdef _METADBSRV_TEST_LIST_
    for ( uint32_t i = 0 ; i < 100 ; i++ ) {
        args.list->push_back(std::pair<uint64_t, std::string>(i + 3323456, string("LOCAL_STORE_SRORE_") + to_string(i)));
    }
#endif
}

void Cluster::ListSpaces(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
#ifdef _METADBSRV_TEST_LIST_
    for ( uint32_t i = 0 ; i < 4 ; i++ ) {
        args.list->push_back(std::pair<uint64_t, std::string>(i + 4423456, string("LOCAL_SPACE_SRACE_") + to_string(i)));
    }
#endif
}

void Cluster::ListClusters(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
#ifdef _METADBSRV_TEST_LIST_
    for ( uint32_t i = 0 ; i < 64 ; i++ ) {
        args.list->push_back(std::pair<uint64_t, std::string>(i + 5523456, string("LOCAL_CLUSTER_SRUSTER_") + to_string(i)));
    }
#endif
}

}
