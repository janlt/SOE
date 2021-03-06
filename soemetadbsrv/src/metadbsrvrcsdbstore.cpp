/**
 * @file    rcsdbstore.cpp
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
 * rcsdbstore.cpp
 *
 *  Created on: Jan 27, 2017
 *      Author: Jan Lisowiec
 */

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

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
#include <chrono>
#include <boost/random.hpp>
#include <boost/filesystem.hpp>

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

using namespace rocksdb;

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
#include "metadbsrvrcsdbspace.hpp"
#include "metadbsrvrcsdbcluster.hpp"

using namespace std;
using namespace RcsdbFacade;

namespace Metadbsrv {

#define FILE_LINE (string(__FILE__) + " " + to_string(__LINE__) + " ")

uint64_t Store::store_null_id = -1;  // max 64 bit number
uint32_t Store::bloom_bits_default = 4;
bool Store::overdups_default = false;
const char *Store::snapshot_prefix_dir = "snapshots";
const char *Store::backup_prefix_dir = "backups";
MonotonicId64Gener Store::id_gener;

#define STRINGIFY_ENUM( name ) # name
string Store::pr_status[ePrStatus::eMax] =
    { string( STRINGIFY_ENUM( Store::ePrStatus::eOk ) ),
      string( STRINGIFY_ENUM( Store::ePrStatus::eResourceExhausted ) ),
      string( STRINGIFY_ENUM( Store::ePrStatus::eStoreInternalError ) ),
      string( STRINGIFY_ENUM( Store::ePrStatus::eAccessDenied ) ),
      string( STRINGIFY_ENUM( Store::ePrStatus::eResourceNotFound ) ),
      string( STRINGIFY_ENUM( Store::ePrStatus::eWrongDbType ) )
    };

RcsdbName::~RcsdbName() {}
RcsdbName::RcsdbName(): next_available(0) {}

string Store::PrStatusToString(Store::ePrStatus sts)
{
    if ( sts < Store::ePrStatus::eOk || sts >= Store::ePrStatus::eMax ) {
        return string("Invalid status code: ") + to_string(sts);
    }
    return pr_status[sts];
}

void Store::SetOpenOptions()
{
    open_options.IncreaseParallelism();
    open_options.create_if_missing = true;
    open_options.merge_operator.reset(new UpdateMerge);
    open_options.write_buffer_size = Store::sst_buffer_size;
    open_options.keep_log_file_num = 64;
    //open_options.compaction_filter = new UpdateFilter;
    switch ( sync_level ) {
    case eSyncLevelDefault:
        break;
    case eSyncLevelFsync:
        break;
    case eSyncLevelAsync:
        open_options.WAL_ttl_seconds = 4;
        break;
    default:
        break;
    }
    BlockBasedTableOptions table_options;
    table_options.block_cache = cache;
    open_options.table_factory.reset(NewBlockBasedTableFactory(table_options));
}

void Store::SetTransactionOptions()
{
}

void Store::SetReadOptions()
{
}

void Store::SetWriteOptions()
{
    switch ( sync_level ) {
    case eSyncLevelDefault:
        break;
    case eSyncLevelFsync:
        write_options.sync = true;
        break;
    case eSyncLevelAsync:
        break;
    default:
        break;
    }
}

Store::Store():
    cache(0),
    filter_policy(0),
    rcsdb(0),
    rcsdb_ref_count(0),
    in_archive_ttl(0),
    open_options(),
    read_options(),
    write_options(),
    state(Store::eStoreState::eStoreInit),
    batch_mode(false),
    bloom_bits(0),
    overwrite_duplicates(false),
    config(false),
    debug(false),
    name(""),
    parent(0),
    id(Store::store_invalid_id),
    db_type(Store::eDBType::eSimpleDB),
    sync_level(Store::eSyncLevel::eSyncLevelDefault)
{}

Store::Store(const string &nm,
        uint64_t _id,
        eDBType _db_type,
        uint32_t b_bits,
        bool over_dups,
        Store::eSyncLevel _sync_level,
        bool _debug):
    cache(NewLRUCache(Store::lru_cache_size, Store::lru_shard_bits)),
    filter_policy(NULL),
    rcsdb(0),
    rcsdb_ref_count(0),
    in_archive_ttl(0),
    open_options(),
    read_options(),
    write_options(),
    state(eStoreInit),
    batch_mode(false),
    bloom_bits(b_bits),
    overwrite_duplicates(over_dups),
    config(_debug),
    debug(_debug),
    name(nm),
    parent(0),
    id(_id),
    db_type(_db_type),
    sync_level(_sync_level)
{
    RcsdbName::next_available = 0;

    filter_policy = NULL; // For now we have no filter

    SetOpenOptions();
    SetReadOptions();
    SetWriteOptions();

    for ( uint32_t i = 0 ; i < Store::max_batches ; i++ ) {
        batches[i].reset();
    }

    for ( uint32_t i = 0 ; i < Store::max_transactions ; i++ ) {
        transactions[i].reset();
    }

    for ( uint32_t i = 0 ; i < Store::max_snapshots ; i++ ) {
        snapshots[i].reset();
    }

    for ( uint32_t i = 0 ; i < Store::max_backups ; i++ ) {
        backups[i].reset();
    }

    for ( uint32_t i = 0 ; i < Store::max_iterators ; i++ ) {
        iterators[i].reset();
    }
}

Store::~Store()
{
    // no throwing from destructor
    // rcsdb is deleted in Close()
    assert(rcsdb == 0);
}

//
// Config functions
//
void Store::SetPath()
{
    assert(parent);
    path = dynamic_cast<RcsdbName *>(parent)->GetPath() + "/" + name;
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "path: " << path << " " << SoeLogger::LogDebug() << endl;
    }
}

const string &Store::GetPath()
{
    return path;
}

int Store::ReadSnapshotsDir(DIR *dir)
{
    while ( !snapshot_dirs.empty() ) {
        snapshot_dirs.pop_back();
    }

    struct dirent *dir_entry;
    while ( (dir_entry = ::readdir(dir)) ) {
        if ( !dir_entry ) {
            return errno;
        }

        if ( string(dir_entry->d_name) == "." || string(dir_entry->d_name) == ".." ) {
            continue;
        }
        if ( debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Dir file: " << dir_entry->d_name << " " << SoeLogger::LogDebug() << endl;
        }

        snapshot_dirs.push_back(dir_entry->d_name);
    }

    return 0;
}

void Store::EnumSnapshots()
{
    string snapshots_path = path + "/" + Store::snapshot_prefix_dir;
    DIR *snapshot_dir = ::opendir(snapshots_path.c_str());
    if ( snapshot_dir ) {
        int ret = ReadSnapshotsDir(snapshot_dir);
        if ( ret ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Snapshots dir corrupt or not readable, store name: " << name << " path: " <<
                snapshots_path << " ret: " << to_string(ret) << " " << SoeLogger::LogError() << endl;
        }
        ::closedir(snapshot_dir);
    }
}

void Store::ValidateSnapshots()
{
    bool found = false;
    for ( auto it = config.snapshots.begin() ; it != config.snapshots.end() ; it++ ) {
        found = false;
        for ( auto it1 = snapshot_dirs.begin() ; it1 != snapshot_dirs.end() ; it1++ ) {
            if ( debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Snaphot, name: " << it->first << " dir: " << *it1 << " " << SoeLogger::LogDebug() << endl;
            }

            if ( it->first == *it1 ) {
                found = true;
                break;
            }
        }

        if ( found != true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Snaphot has no dir, name: " << it->first << SoeLogger::LogDebug() << endl;
        } else {
            for ( uint32_t i = 0 ; i < Store::max_snapshots ; i++ ) {
                if ( !Store::snapshots[i] ) {
                    snapshots[i].reset(new RcsdbSnapshot(new Checkpoint(), it->first, GetPath(), it->second));
                    break;
                }
            }
        }
    }
}

int Store::ReadBackupsDir(DIR *dir)
{
    while ( !backups_dirs.empty() ) {
        backups_dirs.pop_back();
    }

    struct dirent *dir_entry;
    while ( (dir_entry = ::readdir(dir)) ) {
        if ( !dir_entry ) {
            return errno;
        }

        if ( string(dir_entry->d_name) == "." || string(dir_entry->d_name) == ".." ) {
            continue;
        }
        if ( debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Dir file: " << dir_entry->d_name << " " << SoeLogger::LogDebug() << endl;
        }

        backups_dirs.push_back(dir_entry->d_name);
    }

    return 0;
}

void Store::EnumBackups()
{
    string backups_path = path + "/" + Store::backup_prefix_dir;
    DIR *backup_dir = ::opendir(backups_path.c_str());
    if ( backup_dir ) {
        int ret = ReadBackupsDir(backup_dir);
        if ( ret ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Backups dir corrupt or not readable, store name: " << name << " path: " <<
                backups_path << " ret: " << to_string(ret) << " " << SoeLogger::LogError() << endl;
        }
        ::closedir(backup_dir);
    }
}

void Store::ValidateBackups()
{
    bool found = false;
    for ( auto it = config.backups.begin() ; it != config.backups.end() ; it++ ) {
        found = false;
        for ( auto it1 = backups_dirs.begin() ; it1 != backups_dirs.end() ; it1++ ) {
            if ( debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Backup, name: " << it->first << " dir: " << *it1 << " " << SoeLogger::LogDebug() << endl;
            }

            if ( it->first == *it1 ) {
                found = true;
                break;
            }
        }

        if ( found != true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Backup has no dir, name: " << it->first << SoeLogger::LogDebug() << endl;
        } else {
            for ( uint32_t i = 0 ; i < Store::max_backups ; i++ ) {
                if ( !Store::backups[i] ) {
                    BackupEngine *backup_engine;
                    string backup_path = GetPath() + "/" + Store::backup_prefix_dir + "/" + it->first;
                    Status s = BackupEngine::Open(Env::Default(), BackupableDBOptions(backup_path.c_str()), &backup_engine);
                    if ( !s.ok() ) {
                        throw RcsdbFacade::Exception("Create failed " + it->first + " " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
                    }

                    backups[i].reset(new RcsdbBackup(backup_engine, 0, it->first, GetPath(), it->second));
                    break;
                }
            }
        }
    }
}


void Store::ParseConfig(const string &cfg, bool no_exc)
{
    config.Parse(cfg, no_exc);

    filter_policy = NULL; // TODO

    name = config.name;
    id = config.id;
    bloom_bits = config.bloom_bits;
    overwrite_duplicates = config.overwrite_duplicates;
    sync_level = static_cast<eSyncLevel>(config.sync_level);
    db_type = (config.transaction_db == true ? Store::eDBType::eTransactionDB : Store::eDBType::eSimpleDB);
}

string Store::CreateConfig()
{
    config.name = name;
    config.id = id;
    config.bloom_bits = bloom_bits;
    config.overwrite_duplicates = overwrite_duplicates;
    config.transaction_db = (db_type == Store::eDBType::eTransactionDB ? true : false);
    config.sync_level = static_cast<int>(sync_level);
    return config.Create();
}

string Store::ConfigAddSnapshot(const string &s_name, uint32_t id)
{
    config.snapshots.push_back(pair<string, uint32_t>(s_name, id));
    return config.Create();
}

string Store::ConfigRemoveSnapshot(const string &s_name, uint32_t id)
{
    for ( auto it = config.snapshots.begin() ; it != config.snapshots.end() ; it++ ) {
        if ( it->first == s_name && it->second == id ) {
            config.snapshots.erase(it);
            break;
        }
    }
    return config.Create();
}

string Store::ConfigAddBackup(const string &b_name, uint32_t id)
{
    config.backups.push_back(pair<string, uint32_t>(b_name, id));
    return config.Create();
}

string Store::ConfigRemoveBackup(const string &b_name, uint32_t id)
{
    for ( auto it = config.backups.begin() ; it != config.backups.end() ; it++ ) {
        if ( it->first == b_name && it->second == id ) {
            config.backups.erase(it);
            break;
        }
    }
    return config.Create();
}

void Store::SaveConfig(const string &all_lines, const string &config_file)
{
    ofstream cfile(config_file.c_str());
    cfile << all_lines;
    cfile.close();
}

void Store::Create()
{
    WriteLock w_lock(Store::open_close_lock);

    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "path: " << path << SoeLogger::LogDebug() << endl;
    }

    int sts = 1;
    int ret;

    DIR *store_dir = ::opendir(path.c_str());
    if ( store_dir ) {
        ::closedir(store_dir);
        store_dir = 0;
        if ( debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Path already exists, store name: " << name << " path: " <<
                path << " errno: " << to_string(errno) << " " << SoeLogger::LogDebug() << endl;
        }

        sts = 0;
    }

    if ( sts ) {
        ret = ::mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if ( ret ) {
            if ( store_dir ) {
                ::closedir(store_dir);
                store_dir = 0;
            }
            throw RcsdbFacade::Exception("Can't create path, store name: " + name + " path: " +
                path + " errno: " + to_string(errno) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOsError);
        }
        ret = ::chmod(path.c_str(), 0777);
        if ( ret ) {
            if ( store_dir ) {
                ::closedir(store_dir);
                store_dir = 0;
            }
            throw RcsdbFacade::Exception("Can't chmod path, store name: " + name + " path: " +
                path + " errno: " + to_string(errno) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOsError);
        }

    }
    if ( store_dir ) {
        ::closedir(store_dir);
    }

    sts = 1;
    string snapshots_path = path + "/" + Store::snapshot_prefix_dir;
    DIR *snapshot_dir = ::opendir(snapshots_path.c_str());
    if ( snapshot_dir ) {
        ret = ReadSnapshotsDir(snapshot_dir);
        if ( ret ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Snapshots dir corrupt or not readable, store name: " << name << " path: " <<
                snapshots_path << " ret: " << to_string(ret) << " " << SoeLogger::LogError() << endl;
        }

        ::closedir(snapshot_dir);
        snapshot_dir = 0;
        if ( debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Snapshot path already exists, store name: " << name << " path: " <<
                snapshots_path << " errno: " << to_string(errno) << " " << SoeLogger::LogDebug() << endl;
        }
        sts = 0;
    }

    if ( sts ) {
        ret = ::mkdir(snapshots_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if ( ret ) {
            if ( snapshot_dir ) {
                ::closedir(snapshot_dir);
                snapshot_dir = 0;
            }
            throw RcsdbFacade::Exception("Can't create snapshots_path, store name: " + name + " path: " +
                snapshots_path + " errno: " + to_string(errno) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOsError);
        }
    }

    sts = 1;
    string backups_path = path + "/" + Store::backup_prefix_dir;
    DIR *backup_dir = ::opendir(backups_path.c_str());
    if ( backup_dir ) {
        ret = ReadBackupsDir(backup_dir);
        if ( ret ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Backups dir corrupt or not readable, store name: " << name << " path: " <<
                backups_path << " ret: " << to_string(ret) << " " << SoeLogger::LogError() << endl;
        }

        ::closedir(backup_dir);
        backup_dir = 0;
        if ( debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Backup path already exists, store name: " << name << " path: " <<
                backups_path << " errno: " << to_string(errno) << " " << SoeLogger::LogDebug() << endl;
        }
        sts = 0;
    }

    if ( sts ) {
        ret = ::mkdir(backups_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if ( ret ) {
            if ( backup_dir ) {
                ::closedir(backup_dir);
                backup_dir = 0;
            }
            throw RcsdbFacade::Exception(FILE_LINE + "Can't create backups_path, store name: " + name + " path: " +
                backups_path + " errno: " + to_string(errno), RcsdbFacade::Exception::eOsError);
        }
    }

    string all_lines = CreateConfig();
    string config_file = path + "/" + RcsdbName::db_elem_filename;
    SaveConfig(all_lines, config_file);

    if ( snapshot_dir ) {
        ::closedir(snapshot_dir);
    }
    if ( backup_dir ) {
        ::closedir(backup_dir);
    }
}

void Store::DestroyPr() throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "path: " << path << SoeLogger::LogDebug() << endl;
    }

    int ret = RmTree(path.c_str());
    if ( debug ) {
        if ( ret ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "rm store files failed: " << ret << " errno: " << errno << SoeLogger::LogDebug() << endl;
        }
    }
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "RmTree error ret: " << ret << " errno: " << errno <<
            " ... proceeding regardless " << SoeLogger::LogError() << endl;

        if ( ret == -EBUSY ) {
            throw RcsdbFacade::Exception(FILE_LINE + "Can't unlink path: " + path.c_str(),
                RcsdbFacade::Exception::eStoreBusy);
        }
    }
}

int Store::RmTree(const char *tree_path)
{
    size_t path_len;
    DIR *dir;
    struct stat stat_path, stat_entry;
    struct dirent *entry;
    SoeLogger::Logger logger;
    char full_path[2048];

    //char c_path[128];
    //cout << "Store::" << __FUNCTION__ << "tree_path: " << tree_path << " cwd: " << getcwd(c_path, sizeof(c_path)) << endl;

    // stat for the path
    ::stat(tree_path, &stat_path);

    // path doesn't exists or is not a directory
    if ( !S_ISDIR(stat_path.st_mode) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " path: " << path << " is not directory" << SoeLogger::LogError() << endl;
        return -1;
    }

    // this user's access wrong
    dir = ::opendir(tree_path);
    if ( !dir ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " path: " << path << " can't open errno: " << to_string(errno) << SoeLogger::LogError() << endl;
        return -1;
    }

    // the length of the path
    path_len = ::strlen(tree_path);

    // iteration through entries in the directory
    for ( entry = ::readdir(dir) ; entry ; entry = ::readdir(dir) ) {
        // skip entries "." and ".."
        if ( !strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") ) {
            continue;
        }

        // determinate a full path of an entry
        ::strcpy(full_path, tree_path);
        ::strcat(full_path, "/");
        ::strcat(full_path, entry->d_name);

        // stat for the entry
        ::stat(full_path, &stat_entry);

        // recursively remove a nested directory
        if ( S_ISDIR(stat_entry.st_mode) ) {
            int ret = RmTree(full_path);
            if ( ret < 0 ) {
                return ret;
            }
            continue;
        }

        // remove a file object
        if ( !::unlink(full_path) ) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << " removed file: " << full_path << SoeLogger::LogDebug() << endl;
        } else {
            logger.Clear(CLEAR_DEFAULT_ARGS) << " can't remove file: " << full_path << " errno: " << to_string(errno) << SoeLogger::LogError() << endl;
            if ( errno == EBUSY ) {
                return -EBUSY;
            }
            return -1;
        }
    }

    // remove the devastated directory and close the object of it
    if ( !::rmdir(tree_path) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " removed directory: " << tree_path << SoeLogger::LogDebug() << endl;
    } else {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " can't remove directory: " << tree_path << " errno: " << to_string(errno) << SoeLogger::LogError() << endl;
        if ( errno == EBUSY ) {
            return -EBUSY;
        }
        return -1;
    }

    return ::closedir(dir);
}

//
// Store functions
//
void Store::SetNextAvailable()
{}

uint64_t Store::CreateStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    return Cluster::CreateStore(args);
}

uint64_t Store::RepairStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    return Cluster::RepairStore(args);
}

void Store::DestroyStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    Cluster::DestroyStore(args);
}

uint64_t Store::GetStoreByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    return Cluster::GetStoreByName(args);
}

void Store::OpenPr() throw(RcsdbFacade::Exception)
{
    //cout << __FUNCTION__ << " name=" << name << " rrc=" << InterlockedRefCountGet() << " state=" << state << endl;

    if ( state == eStoreOpened ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " in wrong state: " <<
            state << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception(FILE_LINE + "Store in wrong state: " + to_string(state), RcsdbFacade::Exception::eInvalidStoreState);
    }

    Status s;
    if ( state != eStoreOpened && !InterlockedRefCountGet() ) {
        switch ( db_type ) {
        case eSimpleDB:
            s = DB::Open(open_options, path, &rcsdb);
            break;
        case eTransactionDB:
        {
            TransactionDB *tr_db;
            s = TransactionDB::Open(open_options, transaction_options, path, &tr_db);
            rcsdb = dynamic_cast<DB *>(tr_db);
        }
            break;
        default:
            break;
        }

        if ( !s.ok() ) {
            if ( debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "error: " << s.ToString() << SoeLogger::LogDebug() << endl;
            }
            if ( s.code() == Status::kCorruption ) {
                state = eStoreBroken;
            }
            throw RcsdbFacade::Exception(FILE_LINE + "Open failed: " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
        }
    }

    if ( state == eStoreDestroyed ) {
        RegisterApiFunctions();
    }
    InterlockedRefCountIncrement();
    state = eStoreOpened;
}

void Store::ClosePr(bool delete_rcsdb) throw(RcsdbFacade::Exception)
{
    //cout << __FUNCTION__ << " name=" << name << " rrc=" << InterlockedRefCountGet() << " state=" << state << endl;

    if ( state != eStoreOpened || !InterlockedRefCountGet() ) {
        if ( state == eStoreClosed && !InterlockedRefCountGet() ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " already closed ... ignoring state: " <<
                to_string(state) << " ref_count: " << InterlockedRefCountGet() << SoeLogger::LogDebug() << endl;
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " in wrong state ... ignoring state: " <<
                to_string(state) << " ref_count: " << InterlockedRefCountGet() << SoeLogger::LogError() << endl;
        }
    }

    if ( state == eStoreDestroyed ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " in wrong state: " <<
            state << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception(FILE_LINE + "Store in wrong state: " +
            to_string(state) + " aborting ... " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidStoreState);
    }

    if ( state == eStoreClosed ) {
        return;
    }

    if ( !InterlockedRefCountDecrement() ) {
        state = eStoreClosed;
        if ( rcsdb ) {
            // Destroy iterators
            {
                WriteLock w_lock(Store::iterators_lock);

                for ( uint32_t i = 0 ; i < Store::max_iterators ; i++ ) {
                    if ( iterators[i].get() ) {
                        iterators[i].reset();
                    }
                }
            }

            if ( delete_rcsdb == true && rcsdb ) {
                delete rcsdb;
                rcsdb = 0;
            }
        }
    }
}

void Store::RepairPr() throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);

    int must_wait = 0;
    uint32_t opened_count = InterlockedRefCountGet();
    if ( opened_count > 1 ) {
        must_wait = 1;
    }

    // kick out all other sessions if necessary
    for ( ; opened_count ; opened_count-- ) {
        this->ClosePr(false);
    }

    // this will stall all other sessions (if present) only for this store
    // and quiesce i/os
    state = eStoreClosing;
    if ( must_wait ) {
        usleep(8000000);
    }

    // delete rocsksdb database
    if ( this->rcsdb ) {
        delete this->rcsdb;
        this->rcsdb = 0;
    }
    state = eStoreClosed;

    Status s = RepairDB(GetPath(), open_options);
    if ( s.ok() ) {
        throw RcsdbFacade::Exception(FILE_LINE + "Cannot repair store, name: " + name + to_string(state) + " status: " + string(s.getState() ? s.getState() : ""),
            RcsdbFacade::Exception::eFoobar);
    }
}

void Store::Crc32cPr() throw(RcsdbFacade::Exception)
{
    throw RcsdbFacade::Exception(FILE_LINE + "Don't call me just yet" + string(__FUNCTION__), RcsdbFacade::Exception::eOpNotSupported);
}

void Store::PutPr(const Slice& key, const Slice& value, bool over_dups) throw(RcsdbFacade::Exception)
{
    if ( state != eStoreOpened ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " in wrong state: " <<
            state << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception(FILE_LINE + "Store in wrong state: " + to_string(state) + " ref_cnt: " + to_string(InterlockedRefCountGet()),
            RcsdbFacade::Exception::eInvalidStoreState);
    }

    // Check for duplicate key
    if ( overwrite_duplicates == false ) {
        string ch_value;
        bool val_found;
        bool ret = rcsdb->KeyMayExist(read_options, key, &ch_value, &val_found);
        if ( ret == true || val_found == true ) {
            Status check_s = rcsdb->Get(read_options, key, &ch_value);
            if ( check_s.ok() ) {
                if ( over_dups == false ) {
                    throw RcsdbFacade::Exception(FILE_LINE + string("Duplicate found: ") + key.ToString(), RcsdbFacade::Exception::eDuplicateKey);
                } else {
                    throw RcsdbFacade::Exception(FILE_LINE + string("Duplicate found: ") + key.ToString(), TranslateRcsdbStatus(check_s));
                }
            }
        }
    }

    Status s;
    if ( batch_mode == true ) {
        WriteBatch batch;
        batch.Put(key, value);
        s = rcsdb->Write(write_options, &batch);
    } else {
        s = rcsdb->Put(write_options, key, value);
    }
    if ( !s.ok() ) {
        if ( debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "error: " << s.ToString() << SoeLogger::LogDebug() << endl;
        }
        throw RcsdbFacade::Exception(FILE_LINE + "Put failed: " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
    }
}

void Store::GetPr(const Slice& key, string *&value, size_t &buf_len) throw(RcsdbFacade::Exception)
{
    if ( state != eStoreOpened ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " in wrong state: " <<
            state << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception(FILE_LINE + "Store in wrong state: " + to_string(state), RcsdbFacade::Exception::eInvalidStoreState);
    }
    Status s = rcsdb->Get(read_options, key, value);
    if ( !s.ok() ) {
        if ( debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "error: " << s.ToString() << SoeLogger::LogDebug() << endl;
        }
        buf_len = 0;
        throw RcsdbFacade::Exception(FILE_LINE + "Get failed: " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
        value = 0; // not reached
    }
}

void Store::DeletePr(const Slice& key) throw(RcsdbFacade::Exception)
{
    if ( state != eStoreOpened ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " in wrong state: " <<
            state << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception(FILE_LINE + "Store in wrong state: " + to_string(state), RcsdbFacade::Exception::eInvalidStoreState);
    }

    string ch_value;
    bool val_found;
    bool ret = rcsdb->KeyMayExist(read_options, key, &ch_value, &val_found);
    if ( ret == true ) {
        Status check_s = rcsdb->Get(read_options, key, &ch_value);
        if ( check_s.code() == Status::kNotFound ) {
            throw RcsdbFacade::Exception(FILE_LINE + string("Key not found: ") + key.ToString(), RcsdbFacade::Exception::eNotFoundKey);
        }
    } else {
        throw RcsdbFacade::Exception(FILE_LINE + string("Key not found: ") + key.ToString(), RcsdbFacade::Exception::eNotFoundKey);
    }

    Status s = rcsdb->Delete(write_options, key);
    if ( !s.ok() ) {
        if ( debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "error: " << s.ToString() << SoeLogger::LogDebug() << endl;
        }
        throw RcsdbFacade::Exception(FILE_LINE + "Delete failed: " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
    }
}

void Store::GetRangePr(const Slice& start_key, const Slice& end_key, RangeCallback range_callback, bool dir_forward, char *arg) throw(RcsdbFacade::Exception)
{
    if ( state != eStoreOpened ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " in wrong state: " <<
            state << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception(FILE_LINE + "Store in wrong state: " + to_string(state), RcsdbFacade::Exception::eInvalidStoreState);
    }

    // Get a store iterator
    boost::shared_ptr<Iterator> iter(rcsdb->NewIterator(read_options));

    if ( dir_forward == true ) {
        if ( !start_key.data() ) {
            iter->SeekToFirst();
        } else {
            iter->Seek(start_key);
        }

        for ( iter->Seek(start_key) ; iter->Valid() && (!end_key.data() || iter->key().ToString() <= end_key.ToString()) ; iter->Next() ) {
            if ( iter->value().empty() != true ) {
                Slice key_d(iter->key());
                Slice data_d(iter->value());

                if ( range_callback ) {
                    bool ret = range_callback(key_d.data(), key_d.size(), data_d.data(), data_d.size(), arg);
                    if ( ret != true ) {
                        break;
                    }
                }
            }
        }
    } else {
        if ( !end_key.data() ) {
            iter->SeekToLast();
        } else {
            iter->Seek(end_key);
        }

        for ( iter->Seek(end_key) ; iter->Valid() && (!start_key.data() || iter->key().ToString() >= start_key.ToString()) ; iter->Prev() ) {
            if ( iter->value().empty() != true ) {
                Slice key_d(iter->key());
                Slice data_d(iter->value());

                if ( range_callback ) {
                    bool ret = range_callback(key_d.data(), key_d.size(), data_d.data(), data_d.size(), arg);
                    if ( ret != true ) {
                        break;
                    }
                }
            }
        }
    }
}

void Store::MergePr(const Slice& key, const Slice& value) throw(RcsdbFacade::Exception)
{
    if ( state != eStoreOpened ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " in wrong state: " <<
            state << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception(FILE_LINE + "Store in wrong state: " + to_string(state) + " ref_cnt: " + to_string(InterlockedRefCountGet()),
            RcsdbFacade::Exception::eInvalidStoreState);
    }
    Status s = rcsdb->Merge(write_options, key, value);
    if ( !s.ok() ) {
        if ( debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "error: " << s.ToString() << SoeLogger::LogDebug() << endl;
        }
        throw RcsdbFacade::Exception(FILE_LINE + "Merge failed: " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
    }
}

Store::ePrStatus Store::CreateGroupPr(int &idx) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::batches_lock);

    for ( uint32_t i = 0 ; i < Store::max_batches ; i++ ) {
        if ( !batches[i] ) {
            batches[i].reset(new WriteBatch());
            idx = i;
            return Store::ePrStatus::eOk;
        }
    }
    return Store::ePrStatus::eResourceExhausted;
}

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

void Store::IterateSetEnded(StoreIterator *it, bool &_valid, Slice &_key, Slice &_data)
{
    SOESTORE_DEBUG_ITER(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "*** ENDED" << SoeLogger::LogDebug() << endl);
    _valid = false;
    it->SetEnded();
    _key = Slice();
    _data = Slice();
}

void Store::IterateForwardSubsetPr(StoreIterator *it, Slice &key, Slice &data, uint32_t cmd, bool &valid) throw(RcsdbFacade::Exception)
{
    Slice s_key = it->StartKey();
    Slice e_key = it->EndKey();

    if ( cmd == LD_Iterator_First ) {
        // Seek to first and to key
        SOESTORE_DEBUG_ITER(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "*** 1" << SoeLogger::LogDebug() << endl);

        it->SeekForNext(s_key);
        Slice kkey = it->Key();
        if ( it->InRange(kkey) == false ) {
            IterateSetEnded(it, valid, key, data);
            return;
        }
    } else if ( cmd == LD_Iterator_Last ) {
        it->SeekForPrev(e_key);
    } else if ( cmd == LD_Iterator_Next ) {
        if ( it->EndKeyEmpty() == false ) {   // not empty e_key
            if ( key.compare(e_key) >= 0 ) {
                valid = false;
                it->SetEnded();
                return;
            }
        }

        it->Next();

        if ( it->EndKeyEmpty() == false ) {
            Slice kkey = it->Key();
            if ( e_key.compare(kkey) < 0 ) {
                IterateSetEnded(it, valid, key, data);
                return;
            }
        } else {
            Slice kkey = it->Key();
            if ( kkey.size() >= it->KeyPref().size() ) {  // check if different pref (past this subset's pref)
                Slice kkey_pref(kkey.data(), it->KeyPref().size());
                if ( kkey_pref.compare(it->KeyPref()) ) {
                    IterateSetEnded(it, valid, key, data);
                    return;
                }
            } else { // if key.size() < key_pref.size() we are past it
                IterateSetEnded(it, valid, key, data);
                return;
            }
        }
    } else if ( cmd == LD_Iterator_IsValid ) {
        if ( it->Ended() == true ) {
            valid = it->Ended();
        } else {
            valid = it->Valid();
        }
    } else {
        throw RcsdbFacade::Exception(FILE_LINE + "Iterator end arg: " + to_string(state), RcsdbFacade::Exception::eInvalidArgument);
    }
}

void Store::IterateBackwardSubsetPr(StoreIterator *it, Slice &key, Slice &data, uint32_t cmd, bool &valid) throw(RcsdbFacade::Exception)
{
    Slice s_key = it->StartKey();
    Slice e_key = it->EndKey();

    if ( cmd == LD_Iterator_First ) {
        if ( it->EndKeyEmpty() == false ) { // end_key not empty
            it->SeekForPrev(e_key);
            Slice kkey = it->Key();
            if ( it->InRange(kkey) == false ) {
                IterateSetEnded(it, valid, key, data);
                return;
            }
        } else { // end_key empty
            if ( key.empty() == true ) { // key empty
                it->SeekToLast();
                if ( it->StartKeyEmpty() == false ) { // start_key not empty
                    Slice kkey = it->Key();
                    if ( it->InRange(kkey) == false ) {
                        IterateSetEnded(it, valid, key, data);
                        return;
                    }
                }
            } else {
                if ( it->EndKeyEmpty() == true ) {
                    it->SeekToLast();
                    Slice kkey = it->Key(); // check boundary on kkey, it may not be the subset key
                    if ( it->InRange(kkey, true) == false ) {
                        it->SeekForPrev(kkey);
                    }
                } else { // key not empty, e_key not empty, Subset
                    it->SeekForPrev(e_key);
                }
                Slice kkey = it->Key(); // check boundary on s_key
                if ( it->InRange(kkey) == false ) {
                    IterateSetEnded(it, valid, key, data);
                    return;
                }
            }
        }
    } else if ( cmd == LD_Iterator_Last ) {
        // Seek to first and to key
        it->SeekForNext(s_key);
    } else if ( cmd == LD_Iterator_Next ) {
        if ( it->StartKeyEmpty() == false ) {
            if ( key.compare(s_key) <= 0 ) {
                valid = false;
                it->SetEnded();
                return;
            }
        }

        it->Prev();

        if ( it->StartKeyEmpty() == false ) {
            Slice kkey = it->Key();
            if ( s_key.compare(kkey) > 0 ) {
                IterateSetEnded(it, valid, key, data);
                return;
            }
        } else {
            Slice kkey = it->Key();
            if ( kkey.size() >= it->KeyPref().size() ) {  // check if different pref (past this subset's pref)
                Slice kkey_pref(kkey.data(), it->KeyPref().size());
                if ( kkey_pref.compare(it->KeyPref()) ) {
                    IterateSetEnded(it, valid, key, data);
                    return;
                }
            } else { // if key.size() < key_pref.size() we are past it
                IterateSetEnded(it, valid, key, data);
                return;
            }
        }
    } else if ( cmd == LD_Iterator_IsValid ) {
        if ( it->Ended() == true ) {
            valid = it->Ended();
        } else {
            valid = it->Valid();
        }
    } else {
        throw RcsdbFacade::Exception(FILE_LINE + "Iterator end arg: " + to_string(state), RcsdbFacade::Exception::eInvalidArgument);
    }
}


void Store::IterateForwardStorePr(StoreIterator *it, Slice &key, Slice &data, uint32_t cmd, bool &valid) throw(RcsdbFacade::Exception)
{
    Slice s_key = it->StartKey();
    Slice e_key = it->EndKey();

    if ( cmd == LD_Iterator_First ) {
        if ( key.empty() == true ) {
            SOESTORE_DEBUG_ITER(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "*** 5" << SoeLogger::LogDebug() << endl);
            it->SeekForNext(s_key);
        } else {   // key not empty
            SOESTORE_DEBUG_ITER(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "*** 5.1" << SoeLogger::LogDebug() << endl);
            if ( s_key.compare(key) >= 0 ) {
                SOESTORE_DEBUG_ITER(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "*** 7" << SoeLogger::LogDebug() << endl);
                it->SeekForNext(s_key);
            } else {
                SOESTORE_DEBUG_ITER(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "*** 7.1" << SoeLogger::LogDebug() << endl);
                it->SeekForNext(key);
            }
            Slice kkey = it->Key(); // check boundary on e_key
            if ( it->InRange(kkey) == false ) {
                IterateSetEnded(it, valid, key, data);
                return;
            }
        }
    } else if ( cmd == LD_Iterator_Last ) {
        if ( key.empty() ) {
            if ( it->EndKeyEmpty() == true ) {
                it->SeekToLast();
            } else {
                it->SeekForPrev(e_key);
            }
        } else {
            it->Seek(key);
        }
    } else if ( cmd == LD_Iterator_Next ) {
        if ( it->EndKeyEmpty() == false ) {
            if ( key.compare(e_key) >= 0 ) {
                valid = false;
                it->SetEnded();
                return;
            }
        }
        it->Next();
        if ( it->EndKeyEmpty() == false ) {
            Slice kkey = it->Key();
            if ( e_key.compare(kkey) < 0 ) {
                IterateSetEnded(it, valid, key, data);
                return;
            }
        }
    } else if ( cmd == LD_Iterator_IsValid ) {
        if ( it->Ended() == true ) {
            valid = it->Ended();
        } else {
            valid = it->Valid();
        }
    } else {
        throw RcsdbFacade::Exception(FILE_LINE + "Iterator end arg: " + to_string(state), RcsdbFacade::Exception::eInvalidArgument);
    }
}

void Store::IterateBackwardStorePr(StoreIterator *it, Slice &key, Slice &data, uint32_t cmd, bool &valid) throw(RcsdbFacade::Exception)
{
    Slice s_key = it->StartKey();
    Slice e_key = it->EndKey();

    if ( cmd == LD_Iterator_First ) {
        if ( key.empty() == true ) {
            SOESTORE_DEBUG_ITER(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "*** 5" << SoeLogger::LogDebug() << endl);
            if ( it->EndKeyEmpty() == false ) {
                it->SeekForPrev(e_key);
            } else {
                it->SeekToLast();
            }

            Slice kkey = it->Key(); // check boundary on s_key
            if ( it->InRange(kkey) == false ) {
                IterateSetEnded(it, valid, key, data);
                return;
            }
        } else { // key not empty
            SOESTORE_DEBUG_ITER(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "*** 5.1" << SoeLogger::LogDebug() << endl);

            if ( e_key.compare(key) < 0 ) {
                SOESTORE_DEBUG_ITER(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "*** 7" << SoeLogger::LogDebug() << endl);
                it->SeekForPrev(e_key);
            } else {
                SOESTORE_DEBUG_ITER(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "*** 7.1" << SoeLogger::LogDebug() << endl);
                it->SeekForPrev(key);
            }

            Slice kkey = it->Key(); // check boundary on s_key
            if ( it->InRange(kkey) == false ) {
                IterateSetEnded(it, valid, key, data);
                return;
            }
        }
    } else if ( cmd == LD_Iterator_Last ) {
        if ( key.empty() == true ) {
            if ( it->StartKeyEmpty() == true ) {
                it->SeekToFirst();
            } else {
                it->SeekForNext(s_key);
            }
        } else {
            it->Seek(key);
        }
    } else if ( cmd == LD_Iterator_Next ) {
        if ( it->StartKeyEmpty() == false ) {
            if ( key.compare(s_key) <= 0 ) {
                valid = false;
                it->SetEnded();
                return;
            }
        }
        it->Prev();
        if ( it->StartKeyEmpty() == false ) {
            Slice kkey = it->Key();
            if ( s_key.compare(kkey) > 0 ) {
                IterateSetEnded(it, valid, key, data);
                return;
            }
        }
    } else if ( cmd == LD_Iterator_IsValid ) {
        if ( it->Ended() == true ) {
            valid = it->Ended();
        } else {
            valid = it->Valid();
        }
    } else {
        throw RcsdbFacade::Exception(FILE_LINE + "Iterator end arg: " + to_string(state), RcsdbFacade::Exception::eInvalidArgument);
    }
}

void Store::IteratePr(uint32_t iterator_idx, Slice &key, Slice &data, uint32_t cmd, bool &valid) throw(RcsdbFacade::Exception)
{
    valid = false;

    if ( iterator_idx >= Store::max_iterators || !iterators[iterator_idx] || iterators[iterator_idx]->Idx() != iterator_idx ) {
        throw RcsdbFacade::Exception(FILE_LINE + "IteratePr invalid iterator: " + to_string(iterator_idx) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }

    StoreIterator *it = iterators[iterator_idx].get();
    bool dir_forward = it->DirForward();

    if ( state != eStoreOpened || (state == eStoreOpened && !rcsdb) ) {
        valid = false;
        it->Ended();
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " in wrong state: " <<
            state << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception(FILE_LINE + "Store in wrong state: " + to_string(state), RcsdbFacade::Exception::eInvalidStoreState);
    }

    if ( it->Initialized() == false ) {
        valid = true;
    } else {
        valid = it->Valid();
    }

    SOESTORE_DEBUG_ITER(SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "ITERATE_PR iter key: " << key.data() <<
        " s_key " << it->StartKey().data() <<
        " e_key " << it->EndKey().data() <<
        " k_pref " << it->KeyPref().data() <<
        " valid " << valid <<
        " cmd " << cmd <<
        " dir_forward " << dir_forward <<
        " ended " << it->Ended() << SoeLogger::LogDebug() << endl);

    if ( valid == false || it->Ended() == true ) {
        valid = false;
        key = Slice();
        data = Slice();
        return;
    }

    if ( cmd == LD_Iterator_IsValid ) {
        return;
    }

    if ( dir_forward == true ) {
        if ( it->Type() == StoreIterator::eType::eStoreIteratorTypeSubset ) {
            IterateForwardSubsetPr(it, key, data, cmd, valid);
        } else {
            IterateForwardStorePr(it, key, data, cmd, valid);
        }
    } else {
        if ( it->Type() == StoreIterator::eType::eStoreIteratorTypeSubset ) {
            IterateBackwardSubsetPr(it, key, data, cmd, valid);
        } else {
            IterateBackwardStorePr(it, key, data, cmd, valid);
        }
    }

    if ( it->Initialized() == false ) {
        it->SetInitialized();
    }

    if ( it->Valid() == true ) {
        key = it->Key();
        data = it->Value();
    } else {
        if ( it->Initialized() == false ) {
            valid = false;
            throw RcsdbFacade::Exception(FILE_LINE + "Iterator failed: " + to_string(state), RcsdbFacade::Exception::eInternalError);
        }
        valid = false;
    }
}

void Store::DestroyGroupPr(int group_idx) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::batches_lock);

    if ( batches[group_idx] ) {
        batches[group_idx].reset();
    } else {
        throw RcsdbFacade::Exception(FILE_LINE + "DestroyGroup invalid group: " + to_string(group_idx) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }
}
void Store::WriteGroupPr(int group_idx) throw(RcsdbFacade::Exception)
{
    if ( !batches[group_idx] ) {
        throw RcsdbFacade::Exception(FILE_LINE + "DestroyGroup invalid group: " + to_string(group_idx) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }

    Status s = rcsdb->Write(write_options, batches[group_idx].get());
    if ( !s.ok() ) {
        if ( debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "error: " << s.ToString() << SoeLogger::LogDebug() << endl;
        }
        throw RcsdbFacade::Exception(FILE_LINE + "Write failed: " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
    }
}

void Store::GroupPutPr(const Slice& key, const Slice& value, int group_idx) throw(RcsdbFacade::Exception)
{
    if ( !batches[group_idx] ) {
        throw RcsdbFacade::Exception(FILE_LINE + "GroupPutPr invalid group: " + to_string(group_idx) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }

    batches[group_idx]->Put(key, value);
}

void Store::GroupMergePr(const Slice& key, const Slice& value, int group_idx) throw(RcsdbFacade::Exception)
{
    if ( !batches[group_idx] ) {
        throw RcsdbFacade::Exception(FILE_LINE + "GroupMergePr invalid group: " + to_string(group_idx) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }

    batches[group_idx]->Merge(key, value);
}

void Store::GroupDeletePr(const Slice& key, int group_idx) throw(RcsdbFacade::Exception)
{
    if ( !batches[group_idx] ) {
        throw RcsdbFacade::Exception(FILE_LINE + "GroupDeletePr invalid group: " + to_string(group_idx) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }

    batches[group_idx]->Delete(key);
}

Store::ePrStatus Store::StartTransactionPr(int &idx) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::transactions_lock);

    for ( uint32_t i = 0 ; i < Store::max_transactions ; i++ ) {
        if ( !transactions[i] ) {
            try {
                TransactionDB *tdb = dynamic_cast<TransactionDB *>(rcsdb);
                if ( rcsdb ) {
                    if ( !tdb ) {
                        return Store::ePrStatus::eWrongDbType;
                    }
                } else {
                    return Store::ePrStatus::eStoreInternalError;
                }
                transactions[i].reset(tdb->BeginTransaction(write_options));
                idx = i;
                return Store::ePrStatus::eOk;
            } catch (const bad_cast & ex) {
                throw RcsdbFacade::Exception(FILE_LINE + "TransactionDB must be created: " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
            }
        }
    }

    return Store::ePrStatus::eResourceExhausted;
}

void Store::CommitTransactionPr(int transaction_idx) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::transactions_lock);

    if ( transactions[transaction_idx] ) {
        transactions[transaction_idx]->Commit();
    } else {
        throw RcsdbFacade::Exception(FILE_LINE + "CommitTransaction invalid transaction: " + to_string(transaction_idx) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }

    transactions[transaction_idx].reset();
}

void Store::RollbackTransactionPr(int transaction_idx) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::transactions_lock);

    if ( transactions[transaction_idx] ) {
        transactions[transaction_idx]->Rollback();
    } else {
        throw RcsdbFacade::Exception(FILE_LINE + "RollabckTransaction invalid transaction: " + to_string(transaction_idx) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }

    transactions[transaction_idx].reset();
}

Store::ePrStatus Store::CreateSnapshotEnginePr(const string &s_name, uint32_t &idx, uint32_t &id) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::snapshots_lock);

    for ( uint32_t i = 0 ; i < Store::max_snapshots ; i++ ) {
        if ( !snapshots[i] ) {
            Checkpoint *checkpoint;
            Status s = Checkpoint::Create(rcsdb, &checkpoint);
            if ( !s.ok() ) {
                throw RcsdbFacade::Exception(FILE_LINE + "Create failed " + s_name + " " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
            }
            RcsdbSnapshot *snapshot = new RcsdbSnapshot(checkpoint);
            snapshot->SetName(s_name);
            string abs_path = GetPath() + "/" + Store::snapshot_prefix_dir + "/" + s_name;
            snapshot->SetPath(abs_path);
            snapshot->SetId(i + Store::snapshot_start_id);
            idx = i;
            id = snapshot->GetId();
            snapshots[i].reset(snapshot);
            return Store::ePrStatus::eOk;
        }
    }

    return Store::ePrStatus::eResourceExhausted;
}

Store::ePrStatus Store::CreateSnapshotPr(const string &s_name, uint32_t snapshot_idx, uint32_t snapshot_id) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::snapshots_lock);

    for ( uint32_t i = 0 ; i < Store::max_snapshots ; i++ ) {
        if ( snapshots[i] ) {
            if ( snapshots[i]->GetName() == s_name && snapshots[i]->GetId() == snapshot_id && i == snapshot_idx ) {
                Status s1 = snapshots[i]->GetCheckpoint()->CreateCheckpoint(snapshots[i]->GetPath());
                if ( !s1.ok() ) {
                    throw RcsdbFacade::Exception(FILE_LINE + "CreateCheckpoint failed " + s_name + " " + s1.ToString() + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
                }
                return Store::ePrStatus::eOk;
            }
        }
    }

    return Store::ePrStatus::eResourceExhausted;
}

Store::ePrStatus Store::DestroySnapshotPr(const string &s_name, uint32_t snapshot_idx, uint32_t snapshot_id) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::snapshots_lock);

    for ( uint32_t i = 0 ; i < Store::max_snapshots ; i++ ) {
        if ( snapshots[i] ) {
            if ( snapshots[i]->GetName() == s_name && snapshots[i]->GetId() == snapshot_id && i == snapshot_idx ) {
                snapshots[i].reset();
                return Store::ePrStatus::eOk;
            }
        }
    }

    return Store::ePrStatus::eResourceNotFound;
}

Store::ePrStatus Store::ListSnapshotsPr(vector<pair<string, uint32_t> > &list) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::snapshots_lock);

    for ( uint32_t i = 0 ; i < Store::max_snapshots ; i++ ) {
        if ( snapshots[i] ) {
            list.push_back(pair<string, uint32_t>(snapshots[i]->GetName(), snapshots[i]->GetId()));
        }
    }

    return Store::ePrStatus::eOk;
}

int Store::SetupDir(const string &dir, mode_t mode)
{
    DIR *dir_s = ::opendir(dir.c_str());
    if ( dir_s ) {
        ::closedir(dir_s);
        return 0;
    }

    int ret = ::mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if ( ret ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Error creating dir " << dir << " errno: " << errno << SoeLogger::LogError() << endl;
        return ret;
    }

    ret = ::chmod(dir.c_str(), mode);
    if ( ret ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Error chmod dir " << dir << " errno: " << errno << SoeLogger::LogError() << endl;
    }

    return ret;
}

int Store::CreateRecursivePath(const string &path, mode_t mode)
{
    int ret = 0;

    string pp = path;

    size_t pos = 0;
    string token, parsed_path;
    while ( (pos = pp.find("/")) != string::npos ) {
        token = pp.substr(0, pos);
        if ( token.length() ) {
            parsed_path += "/" + token;
            if ( SetupDir(parsed_path, mode) ) {
                ret = -1;
                break;
            }
        }
        pp.erase(0, pos + string("/").length());
        //cout << "path: " << parsed_path << endl;
    }

    if ( pp.length() ) {
        //cout << pp << endl;
        parsed_path += "/" + pp;
        if ( SetupDir(parsed_path, mode) ) {
            ret = -1;
        }
    }

    return ret;
}

void Store::BackupConfigFiles(const string &b_path)
{
    string from_config = path + "/" + RcsdbName::db_elem_filename;
    string to_config = b_path + "/" + RcsdbName::db_elem_filename;

    if ( boost::filesystem::exists(to_config) == false && boost::filesystem::exists(from_config) == true ) {
        boost::filesystem::copy_file(from_config, to_config);
    }

    cout << "#### " << __LINE__ << " to_c: " << to_config << " from_c: " << from_config << endl << flush;

    // find last non-slash char in backup path and call parent
    string space_b_path = b_path;
    size_t last_nb = 0;
    for ( ; last_nb < space_b_path.length() - 1 ; ) {
        char last_char = space_b_path.at(space_b_path.length() - last_nb - 1);
        if ( last_char != '/' ) {
            break;
        }
        last_nb++;
    }

    space_b_path.erase(space_b_path.end() - last_nb - name.length(), space_b_path.end());
    cout << "#### " << __LINE__ << " s_b_p: " << space_b_path << endl << flush;

    cout << "#### " << __LINE__ << " s_b_p: " << space_b_path << endl << flush;
    parent->BackupConfigFiles(space_b_path);
}

void Store::RestoreConfigFiles(const string &b_path)
{
    string to_config = path + "/" + RcsdbName::db_elem_filename;
    string from_config = b_path + "/" + RcsdbName::db_elem_filename;

    if ( boost::filesystem::exists(to_config) == false && boost::filesystem::exists(from_config) == true ) {
        boost::filesystem::copy_file(from_config, to_config);
    }

    //cout << "#### " << b_path << endl << flush;
    // find last non-slash char in backup path and call parent
    string space_b_path = b_path;
    size_t last_nb = 0;
    for ( ; last_nb < space_b_path.length() - 1 ; ) {
        char last_char = space_b_path.at(space_b_path.length() - last_nb - 1);
        if ( last_char != '/' ) {
            break;
        }
        last_nb++;
    }

    //cout << "#### 1 " << space_b_path << endl << flush;
    space_b_path.erase(space_b_path.end() - last_nb - name.length(), space_b_path.end());
    //cout << "#### 2 " << space_b_path << endl << flush;
    parent->RestoreConfigFiles(space_b_path);
}

void Store::CreateConfigFiles(const string &to_dir, const string &from_dir, const string &cl_n, const string &sp_n, const string &s_name)
{
    string to_config = to_dir + "/" + RcsdbName::db_elem_filename;
    string from_config = from_dir + "/" + RcsdbName::db_elem_filename;

    if ( boost::filesystem::exists(to_config) == false && boost::filesystem::exists(from_config) == true ) {
        boost::filesystem::copy_file(from_config, to_config);
    }

    cout << "#### " << __LINE__ << " to_c: " << to_config << " from_c: " << from_config << endl << flush;

    // find last non-slash char in backup path and call parent
    string space_to_dir = to_dir;
    size_t last_nb = 0;
    for ( ; last_nb < space_to_dir.length() - 1 ; ) {
        char last_char = space_to_dir.at(space_to_dir.length() - last_nb - 1);
        if ( last_char != '/' ) {
            break;
        }
        last_nb++;
    }

    space_to_dir.erase(space_to_dir.end() - last_nb - s_name.length(), space_to_dir.end());
    cout << "#### " << __LINE__ << " " << space_to_dir << endl << flush;

    // find last non-slash char in backup path and call parent
    string space_from_dir = from_dir;
    last_nb = 0;
    for ( ; last_nb < space_from_dir.length() - 1 ; ) {
        char last_char = space_from_dir.at(space_from_dir.length() - last_nb - 1);
        if ( last_char != '/' ) {
            break;
        }
        last_nb++;
    }

    space_from_dir.erase(space_from_dir.end() - last_nb - s_name.length(), space_from_dir.end());
    cout << "#### " << __LINE__ << " " << space_from_dir << endl << flush;

    cout << "#### " << __LINE__ << " to: " << space_to_dir << " from: " << space_from_dir << endl << flush;
    parent->CreateConfigFiles(space_to_dir, space_from_dir, cl_n, sp_n, sp_n);
}

Store::ePrStatus Store::CreateBackupEnginePr(const string *cl_nm,
    const string *sp_nm,
    const string &b_path,
    bool  absol_path,
    const string *b_name,
    bool incremental,
    uint32_t &idx,
    uint32_t &id) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::backups_lock);

    for ( uint32_t i = 0 ; i < Store::max_backups ; i++ ) {
        if ( !backups[i] ) {
            BackupEngine *backup_engine = 0;
            BackupEngineReadOnly* backup_engine_ro = 0;

            string backup_path;
            if ( absol_path == false ) {
                backup_path = GetPath() + "/" + Store::backup_prefix_dir + "/" + b_path;
            } else {
                backup_path = b_path + "/" + *cl_nm + "/" + *sp_nm + "/" + name + "/";
            }

            int bc_sts = CreateRecursivePath(backup_path, 0777);
            if ( bc_sts ) {
                throw RcsdbFacade::Exception("Can't mkdir/chmod backup dir, store name: " + name + " path: " +
                    backup_path + " errno: " + to_string(errno) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOsError);
            }

            if ( !incremental == false ) {
                backup_path = b_path; //GetPath() + "/" + Store::backup_prefix_dir;
                BackupableDBOptions bcpable(backup_path.c_str());
                bcpable.destroy_old_data = false;

                Status s = BackupEngineReadOnly::Open(Env::Default(), bcpable, &backup_engine_ro);
                if ( !s.ok() ) {
                    throw RcsdbFacade::Exception(FILE_LINE + "Create backup engine read_only failed " + *b_name + " " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
                }
            } else {
                BackupableDBOptions bcpable(backup_path.c_str());
                bcpable.destroy_old_data = true;

                Status s = BackupEngine::Open(Env::Default(), bcpable, &backup_engine);
                if ( !s.ok() ) {
                    throw RcsdbFacade::Exception(FILE_LINE + "Create backup engine failed " + *b_name + " " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
                }

                // copy config files recursively
                BackupConfigFiles(backup_path);
            }

            RcsdbBackup *backup = new RcsdbBackup(backup_engine, backup_engine_ro);
            backup->SetName(*b_name);
            idx = i;
            backup->SetId(idx + Store::backup_start_id);
            id = idx + Store::backup_start_id;
            backup->SetPath(backup_path);

            backups[i].reset(backup);
            return Store::ePrStatus::eOk;
        }
    }

    return Store::ePrStatus::eResourceExhausted;
}

Store::ePrStatus Store::CreateBackupPr(const string &b_name, uint32_t backup_idx, uint32_t backup_id) throw(RcsdbFacade::Exception)
{
    for ( uint32_t i = 0 ; i < Store::max_backups ; i++ ) {
        if ( backups[i] ) {
            if ( backups[i]->GetName() == b_name && backups[i]->GetId() == backup_id && i == backup_idx ) {
                Status s = backups[i]->GetBackupEngine()->CreateNewBackup(rcsdb, true);
                if ( !s.ok() ) {
                    throw RcsdbFacade::Exception(FILE_LINE + "Create backup failed " + b_name + " " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
                }
                return Store::ePrStatus::eOk;
            }
        }
    }

    return Store::ePrStatus::eResourceNotFound;
}

Store::ePrStatus Store::DestroyBackupEnginePr(const string &b_name, uint32_t backup_idx, uint32_t backup_id) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::backups_lock);

    for ( uint32_t i = 0 ; i < Store::max_backups ; i++ ) {
        if ( backups[i] ) {
            if ( backups[i]->GetName() == b_name && backups[i]->GetId() == backup_id && i == backup_idx ) {
                backups[i].reset();
                return Store::ePrStatus::eOk;
            }
        }
    }

    return Store::ePrStatus::eResourceNotFound;
}

Store::ePrStatus Store::ListBackupsPr(vector<pair<string, uint32_t> > &list) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::backups_lock);

    for ( uint32_t i = 0 ; i < Store::max_backups ; i++ ) {
        if ( backups[i] ) {
            list.push_back(pair<string, uint32_t>(backups[i]->GetName(), backups[i]->GetId()));
        }
    }

    return Store::ePrStatus::eOk;
}

void Store::TransactionPutPr(const Slice& key, const Slice& value, int transaction_idx) throw(RcsdbFacade::Exception)
{
    if ( !transactions[transaction_idx] ) {
        throw RcsdbFacade::Exception(FILE_LINE + "TransactionPutPr invalid transaction: " + to_string(transaction_idx) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }

    Status s = transactions[transaction_idx]->Put(key, value);
    if ( !s.ok() ) {
        throw RcsdbFacade::Exception(FILE_LINE + name + " " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
    }
}

void Store::TransactionGetPr(const Slice& key, string *&value, int transaction_idx, size_t &buf_len) throw(RcsdbFacade::Exception)
{
    if ( !transactions[transaction_idx] ) {
        throw RcsdbFacade::Exception(FILE_LINE + "TransactionGetPr invalid transaction: " + to_string(transaction_idx) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }

    Status s = transactions[transaction_idx]->Get(read_options, key, value);
    if ( !s.ok() ) {
        buf_len = 0;
        throw RcsdbFacade::Exception(FILE_LINE + name + " " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
    }
}

void Store::TransactionMergePr(const Slice& key, const Slice& value, int transaction_idx) throw(RcsdbFacade::Exception)
{
    if ( !transactions[transaction_idx] ) {
        throw RcsdbFacade::Exception(FILE_LINE + "TransactionPutPr invalid transaction: " + to_string(transaction_idx) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }

    Status s = transactions[transaction_idx]->Merge(key, value);
    if ( !s.ok() ) {
        throw RcsdbFacade::Exception(FILE_LINE + name + " " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
    }
}

void Store::TransactionDeletePr(const Slice& key, int transaction_idx) throw(RcsdbFacade::Exception)
{
    if ( !transactions[transaction_idx] ) {
        throw RcsdbFacade::Exception(FILE_LINE + "TransactionPutPr invalid transaction: " + to_string(transaction_idx) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }

    Status s = transactions[transaction_idx]->Delete(key);
    if ( !s.ok() ) {
        throw RcsdbFacade::Exception(FILE_LINE + name + " " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
    }
}

Store::ePrStatus Store::CreateIteratorPr(const Slice& start_key, const Slice& end_key, const Slice& key_pref,
    bool dir_forward, uint32_t &idx, StoreIterator::eType it_type) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::iterators_lock);

    if ( state != eStoreOpened ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " in wrong state: " <<
            state << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception(FILE_LINE + name + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidStoreState);
    }

    for ( uint32_t i = 0 ; i < Store::max_iterators ; i++ ) {
        if ( !iterators[i].get() ) {
            StoreIterator *store_iter = new StoreIterator(*this,
                it_type,
                start_key,
                end_key,
                key_pref,
                dir_forward,
                read_options,
                i);

            iterators[i].reset(store_iter);
            idx = i;
            return Store::ePrStatus::eOk;
        }
    }

    return Store::ePrStatus::eResourceExhausted;
}

void Store::DestroyIteratorPr(uint32_t itarator_idx) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::iterators_lock);

    if ( state != eStoreOpened ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " in wrong state: " <<
            state << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception(FILE_LINE + name + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidStoreState);
    }

    if ( itarator_idx >= Store::max_iterators || !iterators[itarator_idx] ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " invalid iterator_idx: " <<
            itarator_idx << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception(FILE_LINE + name + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }

    if ( iterators[itarator_idx]->Idx() != itarator_idx ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " internal error iterator_idx: " <<
            itarator_idx << " iterators[itarator_idx]->Idx() " << iterators[itarator_idx]->Idx() << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception(FILE_LINE + name + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInternalError);
    }

    iterators[itarator_idx].reset();
}

bool Store::IsOpen()
{
    return state == eStoreOpened ? true : false;
}

//
// Soe API functions
//
void Store::RegisterApiStaticFunctions() throw(SoeApi::Exception)
{
}

void Store::DeregisterApiStaticFunctions() throw(SoeApi::Exception)
{
}

void Store::RegisterApiFunctions() throw(SoeApi::Exception)
{
    // register via SoeApi
    assert(parent);
    assert(dynamic_cast<Space *>(parent)->parent);
    Space *sp = dynamic_cast<Space *>(parent);

    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "store: " << name << " space: " << sp->name << flush;
    }

    Cluster *clu = dynamic_cast<Cluster *>(sp->parent);

    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " cluster: " << clu->name << flush;
    }

    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
}

void Store::DeregisterApiFunctions() throw(SoeApi::Exception)
{
}

uint32_t Store::InterlockedRefCountIncrement()
{
    uint32_t new_val;

    WriteLock w_lock(Store::rcsdb_ref_count_lock);

    //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "this: " << hex << this << dec << " re_count: " <<
    //    rcsdb_ref_count << " state: " << state << SoeLogger::LogDebug() << endl;

    rcsdb_ref_count++;
    new_val = rcsdb_ref_count;
    return new_val;
}

uint32_t Store::InterlockedRefCountDecrement()
{
    uint32_t new_val;

    WriteLock w_lock(Store::rcsdb_ref_count_lock);

    //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "this: " << hex << this << dec << " re_count: " <<
    //    rcsdb_ref_count << " state: " << state << SoeLogger::LogDebug() << endl;

    rcsdb_ref_count--;
    new_val = rcsdb_ref_count;
    return new_val;
}

uint32_t Store::InterlockedRefCountGet()
{
    uint32_t new_val;

    WriteLock w_lock(Store::rcsdb_ref_count_lock);

    //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "this: " << hex << this << dec << " re_count: " <<
    //    rcsdb_ref_count << " state: " << state << SoeLogger::LogDebug() << endl;

    new_val = rcsdb_ref_count;
    return new_val;
}

void Store::Open(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);

    if ( state == eStoreOpened ) {
        InterlockedRefCountIncrement();
        return;
    }

    if ( state == eStoreDestroyed ) {
        throw RcsdbFacade::Exception(FILE_LINE + "Store in wrong state: " +
            to_string(state) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eNoFunction);
    }

    DIR *dir = ::opendir(path.c_str());
    if ( !dir ) {
        throw RcsdbFacade::Exception(FILE_LINE + "Can't open store name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }

    string config = path + "/" + RcsdbName::db_elem_filename;
    string line;
    string all_lines;
    ifstream cfile(config.c_str());
    if ( cfile.is_open() != true ) {
        ::closedir(dir);
        throw RcsdbFacade::Exception(FILE_LINE + "Can't open config, name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInternalError);
    }
    while ( cfile.eof() != true ) {
        cfile >> line;
        //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "line: " << line;
        all_lines += line + "\n";
    }

    ParseConfig(all_lines);

    cfile.close();
    ::closedir(dir);

    EnumSnapshots();
    ValidateSnapshots();

    EnumBackups();
    ValidateBackups();

    args.store_id = id;

    assert(dynamic_cast<Space *>(parent)->parent);
    Space *sp = dynamic_cast<Space *>(parent);
    args.space_id = sp->id;
    Cluster *clu = dynamic_cast<Cluster *>(sp->parent);
    args.cluster_id = clu->id;

    this->OpenPr();

    //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store::Open() this: " << hex << this << dec << " re_count: " <<
        //rcsdb_ref_count << " state: " << state << SoeLogger::LogDebug() << endl;
}

void Store::Repair(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    this->RepairPr();
}

void Store::Close(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);
    //cout << "(0) Store::" << __FUNCTION__ << " name=" << name << " id=" << id << " state=" << state <<
        //" r_c=" << InterlockedRefCountGet() << endl;
    this->ClosePr();
    //cout << "(1) Store::" << __FUNCTION__ << " name=" << name << " id=" << id << " state=" << state <<
        //" r_c=" << InterlockedRefCountGet() << " rcsdb=" << rcsdb << endl;
    //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store::Close() this: " << hex << this << dec << " re_count: " <<
        //rcsdb_ref_count << " state: " << state << SoeLogger::LogDebug() << endl;
}

void Store::Put(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.key || !args.data ) {
        throw RcsdbFacade::Exception(FILE_LINE + "Bad argument " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }

    if ( args.group_idx != Store::invalid_group ) {
        GroupPutPr(Slice(args.key, args.key_len), Slice(args.data, args.data_len), args.group_idx);
    } else if ( args.transaction_idx != Store::invalid_transaction ) {
        TransactionPutPr(Slice(args.key, args.key_len), Slice(args.data, args.data_len), args.transaction_idx);
    } else {
        PutPr(Slice(args.key, args.key_len), Slice(args.data, args.data_len), args.overwrite_dups);
    }
    args.data_len = 0;
}

void Store::Get(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.key ) {
        throw RcsdbFacade::Exception(FILE_LINE + "Bad argument " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }

    string value;
    string *str_ptr = &value;
    if ( args.transaction_idx != Store::invalid_transaction ) {
        TransactionGetPr(Slice(args.key, args.key_len), str_ptr, args.transaction_idx, args.buf_len);
    } else {
        GetPr(Slice(args.key, args.key_len), str_ptr, args.buf_len);
    }
    if ( str_ptr ) {
        size_t value_l = str_ptr->length();

        // In case of a truncation we fill in the available portion of the buffer
        // and set the buf_len to the actual value size to indicate to the client how
        // big it is
        if ( value_l > args.buf_len ) {
            memcpy(args.buf, str_ptr->c_str(), args.buf_len);
            args.buf_len = value_l;
            throw RcsdbFacade::Exception(FILE_LINE + "Buffer too small: " + to_string(args.buf_len) + " value.length: " +
                to_string(value_l) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eTruncatedValue);
        }

        // TODO do better buffer handling
        args.buf_len = value_l;
        memcpy(args.buf, str_ptr->c_str(), args.buf_len);
        args.buf[args.buf_len] = '\0';  // really not needed
    } else {
        args.buf_len = 0;
    }
}

void Store::Delete(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.key ) {
        throw RcsdbFacade::Exception(FILE_LINE + "Bad argument " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }

    if ( args.group_idx != Store::invalid_group ) {
        GroupDeletePr(Slice(args.key, args.key_len), args.group_idx);
    } else if ( args.transaction_idx != Store::invalid_transaction ) {
        TransactionDeletePr(Slice(args.key, args.key_len), args.transaction_idx);
    } else {
        DeletePr(Slice(args.key, args.key_len));
    }
}

RangeContext *Store::GetRangeContext(RangeContext::eType _type, LocalArgsDescriptor &args)
{
    RangeContext *ctx = 0;

    WriteLock w_lock(Store::range_contexts_lock);

    if ( state != eStoreOpened ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Store " << name << " id: " << id << " in wrong state: " <<
            state << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception(FILE_LINE + name + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidStoreState);
    }

    for ( uint32_t i = 0 ; i < Store::max_range_contexts ; i++ ) {
        if ( !range_contexts[i].get() ) {
            switch ( _type ) {
            case RangeContext::eType::eRangeContextDeleteContainer:
            case RangeContext::eType::eRangeContextDeleteSubset:
                ctx = new DeleteRangeContext(*this,
                    _type,
                    Slice(args.key, args.key_len),
                    Slice(args.end_key, args.end_key_len),
                    Slice(args.data, args.data_len),
                    args.overwrite_dups,
                    i);
                break;
            case RangeContext::eType::eRangeContextPutContainer:
            case RangeContext::eType::eRangeContextPutSubset:
            case RangeContext::eType::eRangeContextGetContainer:
            case RangeContext::eType::eRangeContextGetSubset:
            case RangeContext::eType::eRangeContextMergeContainer:
            case RangeContext::eType::eRangeContextMergeSubset:
            default:
                return ctx;
                break;
            }

            range_contexts[i].reset(ctx);
            break;
        }
    }

    return ctx;
}

void Store::PutRangeContext(RangeContext *_ctx)
{
    WriteLock w_lock(Store::range_contexts_lock);

    if ( !_ctx || _ctx->Idx() >= Store::max_range_contexts ) {
        return;
    }
    if ( range_contexts[_ctx->Idx()].get() ) {
        range_contexts[_ctx->Idx()].reset();
    }
}

void Store::GetRange(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    // set up range context object
    switch ( args.iterator_idx ) {
    case Store_range_delete:
        args.range_callback = &DeleteRangeContext::DeleteStoreRangeCallback;
        args.range_callback_arg = reinterpret_cast<char *>(GetRangeContext(RangeContext::eType::eRangeContextDeleteContainer, args));
        break;
    case Subset_range_delete:
        args.range_callback = &DeleteRangeContext::DeleteSubsetRangeCallback;
        args.range_callback_arg = reinterpret_cast<char *>(GetRangeContext(RangeContext::eType::eRangeContextDeleteSubset, args));
        break;
    case Store_range_get:
        args.range_callback = &GetRangeContext::GetStoreRangeCallback;
        args.range_callback_arg = reinterpret_cast<char *>(GetRangeContext(RangeContext::eType::eRangeContextGetContainer, args));
        break;
    case Subset_range_get:
        args.range_callback = &GetRangeContext::GetSubsetRangeCallback;
        args.range_callback_arg = reinterpret_cast<char *>(GetRangeContext(RangeContext::eType::eRangeContextGetContainer, args));
        break;
    case Store_range_put:
        args.range_callback = &PutRangeContext::PutStoreRangeCallback;
        args.range_callback_arg = reinterpret_cast<char *>(GetRangeContext(RangeContext::eType::eRangeContextPutContainer, args));
        break;
    case Subset_range_put:
        args.range_callback = &PutRangeContext::PutSubsetRangeCallback;
        args.range_callback_arg = reinterpret_cast<char *>(GetRangeContext(RangeContext::eType::eRangeContextPutContainer, args));
        break;
    case Store_range_merge:
        args.range_callback = &MergeRangeContext::MergeStoreRangeCallback;
        args.range_callback_arg = reinterpret_cast<char *>(GetRangeContext(RangeContext::eType::eRangeContextMergeContainer, args));
        break;
    case Subset_range_merge:
        args.range_callback = &MergeRangeContext::MergeSubsetRangeCallback;
        args.range_callback_arg = reinterpret_cast<char *>(GetRangeContext(RangeContext::eType::eRangeContextMergeContainer, args));
        break;
    default:
        args.range_callback_arg = 0;
        break; // no change
    }

    if ( !args.range_callback_arg ) {
        throw RcsdbFacade::Exception(FILE_LINE + "No range context avalable " + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOsResourceExhausted);
    }

    // if key = 0 set the iter to the begin(), if end_key = 0 iterate to end()
    GetRangePr(Slice(args.key, args.key_len), Slice(args.end_key, args.data_len), args.range_callback, args.overwrite_dups, args.range_callback_arg);

    reinterpret_cast<RangeContext *>(args.range_callback_arg)->ExecuteRange(*this);
    PutRangeContext(reinterpret_cast<RangeContext *>(args.range_callback_arg));
}

void Store::Merge(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.key || !args.data ) {
        throw RcsdbFacade::Exception(FILE_LINE + "Bad argument " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }

    if ( args.group_idx != Store::invalid_group ) {
        GroupMergePr(Slice(args.key, args.key_len), Slice(args.data, args.data_len), args.group_idx);
    } else if ( args.transaction_idx != Store::invalid_transaction ) {
        TransactionMergePr(Slice(args.key, args.key_len), Slice(args.data, args.data_len), args.transaction_idx);
    } else {
        MergePr(Slice(args.key, args.key_len), Slice(args.data, args.data_len));
    }
}

void Store::Iterate(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    Slice key(const_cast<char *>(args.key), args.key_len);
    Slice data(args.buf, args.buf_len);
    IteratePr(args.iterator_idx, key, data, args.sync_mode, args.transaction_db);
    args.key = key.data();
    args.key_len = key.size();
    args.buf = const_cast<char *>(data.data());
    args.buf_len = data.size();
}

void Store::StartTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);

    int transaction_idx;
    Store::ePrStatus sts = StartTransactionPr(transaction_idx);
    if ( sts != Store::ePrStatus::eOk ) {
        throw RcsdbFacade::Exception(FILE_LINE + "No transaction avalable " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }

    args.transaction_idx = static_cast<uint32_t>(transaction_idx);
}

void Store::CommitTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);

    CommitTransactionPr(args.transaction_idx);
}

void Store::RollbackTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);

    RollbackTransactionPr(args.transaction_idx);
}

void Store::CreateSnapshotEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);

    uint32_t snapshot_idx;
    uint32_t snapshot_id;
    if ( !args.snapshot_name ) {
        throw RcsdbFacade::Exception(FILE_LINE + string("No snapshot name specified ") + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }
    Store::ePrStatus sts = CreateSnapshotEnginePr(*args.snapshot_name, snapshot_idx, snapshot_id);
    if (  sts != Store::ePrStatus::eOk ) {
        throw RcsdbFacade::Exception(FILE_LINE + "No snapshot avalable " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }

    args.snapshot_idx = snapshot_idx;
    args.snapshot_id = snapshot_id;
}

void Store::CreateSnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.snapshot_name || args.snapshot_idx == Store::invalid_snapshot || args.snapshot_id == Store::invalid_snapshot ) {
        throw RcsdbFacade::Exception(FILE_LINE + string("No snapshot engine name or invalid id specified ") + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }
    Store::ePrStatus sts = CreateSnapshotPr(*args.snapshot_name, args.snapshot_idx, args.snapshot_id);
    if (  sts != Store::ePrStatus::eOk ) {
        throw RcsdbFacade::Exception(FILE_LINE + "No snapshot avalable " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }

    string all_lines = ConfigAddSnapshot(*args.snapshot_name, args.snapshot_idx);
    string config_file = path + "/" + RcsdbName::db_elem_filename;
    SaveConfig(all_lines, config_file);
}

void Store::DestroySnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.snapshot_name || args.snapshot_idx == Store::invalid_snapshot || args.snapshot_id == Store::invalid_snapshot ) {
        throw RcsdbFacade::Exception(FILE_LINE + string("No valid snapshot name or id specified ") + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }
    Store::ePrStatus sts = DestroySnapshotPr(*args.snapshot_name, args.snapshot_idx, args.snapshot_id);
    if (  sts != Store::ePrStatus::eOk ) {
        throw RcsdbFacade::Exception(FILE_LINE + "No snapshot found " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }

    string all_lines = ConfigRemoveSnapshot(*args.snapshot_name, args.snapshot_idx);
    string config_file = path + "/" + RcsdbName::db_elem_filename;
    SaveConfig(all_lines, config_file);

    args.snapshot_idx = Store::invalid_snapshot;
    args.snapshot_id = Store::invalid_snapshot;
}

void Store::ListSnapshots(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.snapshots ) {
        throw RcsdbFacade::Exception(FILE_LINE + "Bad argument " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }

    Store::ePrStatus sts = ListSnapshotsPr(*args.snapshots);
    if (  sts != Store::ePrStatus::eOk ) {
        throw RcsdbFacade::Exception(FILE_LINE + "No snapshot avalable " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }
}

void Store::CreateBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);

    uint32_t backup_engine_idx;
    uint32_t backup_engine_id;

    const string *backup_path;
    bool absol_path;
    if ( args.snapshot_name ) {
        backup_path = args.snapshot_name;
        absol_path = true;
    } else {
        backup_path = args.backup_name;
        absol_path = false;
    }

    if ( (backup_path && backup_path->empty() == true) ) {
        throw RcsdbFacade::Exception(FILE_LINE + string("No backup engine name specified ") + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }

    if ( !args.cluster_name || !args.space_name || !args.store_name || *args.store_name != name || !args.backup_name ) {
        throw RcsdbFacade::Exception(FILE_LINE + string("Invalid argument ") + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }

    Store::ePrStatus sts = CreateBackupEnginePr(args.cluster_name,
        args.space_name,
        *backup_path,
        absol_path,
        args.backup_name,
        static_cast<bool>(args.sync_mode),
        backup_engine_idx,
        backup_engine_id);
    if (  sts != Store::ePrStatus::eOk ) {
        throw RcsdbFacade::Exception(FILE_LINE + "No backup engine avalable " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }

    args.backup_idx = backup_engine_idx;
    args.backup_id = backup_engine_id;
}

void Store::CreateBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.backup_name || args.backup_idx == Store::invalid_backup || args.backup_id == Store::invalid_backup ) {
        throw RcsdbFacade::Exception(FILE_LINE + string("No backup engine name or invalid id specified ") + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }
    Store::ePrStatus sts = CreateBackupPr(*args.backup_name, args.backup_idx, args.backup_id);
    if (  sts != Store::ePrStatus::eOk ) {
        throw RcsdbFacade::Exception(FILE_LINE + "Backup failed " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }

    string all_lines = ConfigAddBackup(*args.backup_name, args.backup_idx);
    string config_file = path + "/" + RcsdbName::db_elem_filename;
    SaveConfig(all_lines, config_file);
}

void Store::RestoreBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.backup_name || args.backup_idx == Store::invalid_backup || args.backup_id == Store::invalid_backup ) {
        throw RcsdbFacade::Exception(FILE_LINE + string("No backup engine name or invalid id specified ") + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }
    if ( !args.backups ) {
        Store::ePrStatus sts = RestoreBackupPr(*args.backup_name, args.backup_idx, args.backup_id);
        if (  sts != Store::ePrStatus::eOk ) {
            throw RcsdbFacade::Exception(FILE_LINE + "Restore failed " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
        }
    } else {
        if ( args.backups->size() == 3 ) {
            Store::ePrStatus sts = RestoreNonExistingBackupPr(*args.backup_name, args.backup_idx, args.backup_id, (*args.backups)[2].first, (*args.backups)[1].first, (*args.backups)[0].first);
            if (  sts != Store::ePrStatus::eOk ) {
                throw RcsdbFacade::Exception(FILE_LINE + "Restore failed " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
            }
        } else {
            throw RcsdbFacade::Exception(FILE_LINE + string("Non-existing store restore needs {cluster,space,store} ") + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
        }
    }
}

Store::ePrStatus Store::RestoreNonExistingBackupPr(const string &b_name, uint32_t backup_idx, uint32_t backup_id, const string &cl_n, const string &sp_n, const string &st_n) throw(RcsdbFacade::Exception)
{
    for ( uint32_t i = 0 ; i < Store::max_backups ; i++ ) {
        if ( backups[i] ) {
            if ( backups[i]->GetName() == b_name && backups[i]->GetId() == backup_id && i == backup_idx ) {
                if ( backups[i]->GetType() != RcsdbBackup::eBackupEngineType::eReadOnly ) {
                    throw RcsdbFacade::Exception(FILE_LINE + "Wrong or invalid backup engine " + b_name + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
                }
                string from_path = backups[i]->GetPath();
                string to_path = string(RcsdbName::db_root_path) + "/" + cl_n + "/" + sp_n + "/" + st_n + "/";

                int bc_sts = CreateRecursivePath(to_path, 0777);
                if ( bc_sts ) {
                    throw RcsdbFacade::Exception("Can't mkdir/chmod backup dir: " + to_path +
                        " errno: " + to_string(errno) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOsError);
                }

                Status s = backups[i]->GetBackupEngineReadOnly()->RestoreDBFromBackup(1, to_path.c_str(), to_path.c_str());
                if ( !s.ok() ) {
                    throw RcsdbFacade::Exception(FILE_LINE + "Restore backup failed " + b_name + " " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
                }

                // first create dir and copy config files recursively
                CreateConfigFiles(to_path, from_path, cl_n, sp_n, st_n);

                return Store::ePrStatus::eOk;
            }
        }
    }

    return Store::ePrStatus::eResourceNotFound;
}

Store::ePrStatus Store::RestoreBackupPr(const string &b_name, uint32_t backup_idx, uint32_t backup_id) throw(RcsdbFacade::Exception)
{
    for ( uint32_t i = 0 ; i < Store::max_backups ; i++ ) {
        if ( backups[i] ) {
            if ( backups[i]->GetName() == b_name && backups[i]->GetId() == backup_id && i == backup_idx ) {
                if ( backups[i]->GetType() != RcsdbBackup::eBackupEngineType::eReadOnly ) {
                    throw RcsdbFacade::Exception(FILE_LINE + "Wrong or invalid backup engine " + b_name + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
                }
                Status s = backups[i]->GetBackupEngineReadOnly()->RestoreDBFromBackup(1, path.c_str(), path.c_str());
                if ( !s.ok() ) {
                    throw RcsdbFacade::Exception(FILE_LINE + "Restore backup failed " + b_name + " " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
                }

                // copy config files recursively
                RestoreConfigFiles(backups[i]->GetPath());

                return Store::ePrStatus::eOk;
            }
        }
    }

    return Store::ePrStatus::eResourceNotFound;
}

void Store::DestroyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);

    if ( !args.backup_name || args.backup_idx == Store::invalid_backup || args.backup_id == Store::invalid_backup ) {
        throw RcsdbFacade::Exception(FILE_LINE + string("No backup engine name or invalid id specified ") + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }
    Store::ePrStatus sts = DestroyBackupPr(*args.backup_name, args.backup_idx, args.backup_id);
    if (  sts != Store::ePrStatus::eOk ) {
        throw RcsdbFacade::Exception(FILE_LINE + "Destroy failed " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }
}

Store::ePrStatus Store::DestroyBackupPr(const string &b_name, uint32_t backup_idx, uint32_t backup_id) throw(RcsdbFacade::Exception)
{
    DIR *backup_dir = ::opendir(b_name.c_str());
    if ( !backup_dir ) {
        throw RcsdbFacade::Exception(FILE_LINE + string("Backup does't exist: ") + b_name + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }
    ::closedir(backup_dir);
    string rm_cmd = "rm -rf " + b_name;
    int ret = ::system(rm_cmd.c_str());
    if ( ret ) {
        return Store::ePrStatus::eStoreInternalError;
    }
    return Store::ePrStatus::eOk;
}

void Store::VerifyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);

    if ( !args.backup_name || args.backup_idx == Store::invalid_backup || args.backup_id == Store::invalid_backup ) {
        throw RcsdbFacade::Exception(FILE_LINE + string("No backup engine name or invalid id specified ") + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }
    Store::ePrStatus sts = VerifyBackupPr(*args.backup_name, args.backup_idx, args.backup_id);
    if (  sts != Store::ePrStatus::eOk ) {
        throw RcsdbFacade::Exception(FILE_LINE + "Destroy failed " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }
}

Store::ePrStatus Store::VerifyBackupPr(const string &b_name, uint32_t backup_idx, uint32_t backup_id) throw(RcsdbFacade::Exception)
{
    for ( uint32_t i = 0 ; i < Store::max_backups ; i++ ) {
        if ( backups[i] ) {
            if ( backups[i]->GetName() == b_name && backups[i]->GetId() == backup_id && i == backup_idx ) {
                if ( backups[i]->GetType() != RcsdbBackup::eBackupEngineType::eReadOnly ) {
                    throw RcsdbFacade::Exception(FILE_LINE + "Wrong or invalid backup engine " + b_name + " " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
                }

                Status s = backups[i]->GetBackupEngineReadOnly()->VerifyBackup(1);
                if ( !s.ok() ) {
                    throw RcsdbFacade::Exception(FILE_LINE + "Restore backup failed " + b_name + " " + s.ToString() + " " + string(__FUNCTION__), TranslateRcsdbStatus(s));
                }

                return Store::ePrStatus::eOk;
            }
        }
    }

    return Store::ePrStatus::eResourceNotFound;
}

void Store::DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);

    if ( !args.backup_name || args.backup_idx == Store::invalid_backup || args.backup_id == Store::invalid_backup ) {
        throw RcsdbFacade::Exception(FILE_LINE + string("No valid backup engine name or id specified ") + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }
    Store::ePrStatus sts = DestroyBackupEnginePr(*args.backup_name, args.backup_idx, args.backup_id);
    if (  sts != Store::ePrStatus::eOk ) {
        throw RcsdbFacade::Exception(FILE_LINE + "No backup engine found " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }

    string all_lines = ConfigRemoveBackup(*args.backup_name, args.backup_idx);
    string config_file = path + "/" + RcsdbName::db_elem_filename;
    SaveConfig(all_lines, config_file);

    args.backup_idx = Store::invalid_backup;
}

void Store::ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( !args.backups ) {
        throw RcsdbFacade::Exception(FILE_LINE + "Bad argument " + string(__FUNCTION__), RcsdbFacade::Exception::eInvalidArgument);
    }

    Store::ePrStatus sts = ListBackupsPr(*args.backups);
    if (  sts != Store::ePrStatus::eOk ) {
        throw RcsdbFacade::Exception(FILE_LINE + "No backup avalable " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), RcsdbFacade::Exception::eOpAborted);
    }
}

void Store::CreateGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);

    int group_idx = Store::invalid_group;
    Store::ePrStatus sts = CreateGroupPr(group_idx);
    if ( sts != Store::ePrStatus::eOk ) {
        throw RcsdbFacade::Exception(FILE_LINE + "No group avalable " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), TranslateStoreStatus(sts));
    }

    args.group_idx = static_cast<uint32_t>(group_idx);
}

void Store::DestroyGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);

    DestroyGroupPr(args.group_idx);
}

void Store::WriteGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteGroupPr(args.group_idx);
}

void Store::CreateIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);

    uint32_t iterator_idx = Store::invalid_iterator;
    Slice start_key(args.key, args.key_len);
    Slice end_key(args.end_key, args.end_key_len);
    Slice key_pref(args.data, args.data_len);
    Store::ePrStatus sts = CreateIteratorPr(start_key, end_key, key_pref, args.overwrite_dups, iterator_idx, static_cast<StoreIterator::eType>(args.cluster_idx));
    if ( sts != Store::ePrStatus::eOk ) {
        throw RcsdbFacade::Exception(FILE_LINE + "No iterator available " + Store::PrStatusToString(sts) + " " + string(__FUNCTION__), TranslateStoreStatus(sts));
    }

    args.iterator_idx = iterator_idx;
}

void Store::DestroyIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(Store::open_close_lock);

    DestroyIteratorPr(args.iterator_idx);
}

RcsdbFacade::Exception::eStatus Store::TranslateRcsdbStatus(const Status &s)
{
    RcsdbFacade::Exception::eStatus sts = RcsdbFacade::Exception::eInternalError;
    switch ( s.code() ) {
    case Status::kOk:
        sts = RcsdbFacade::Exception::eOk;
        break;
    case Status::kNotFound:
        sts = RcsdbFacade::Exception::eNotFoundKey;
        break;
    case Status::kCorruption:
        sts = RcsdbFacade::Exception::eStoreCorruption;
        break;
    case Status::kNotSupported:
        sts = RcsdbFacade::Exception::eOpNotSupported;
        break;
    case Status::kInvalidArgument:
        sts = RcsdbFacade::Exception::eInvalidArgument;
        break;
    case Status::kIOError:
        sts = RcsdbFacade::Exception::eIOError;
        break;
    case Status::kMergeInProgress:
        sts = RcsdbFacade::Exception::eMergeInProgress;
        break;
    case Status::kIncomplete:
        sts = RcsdbFacade::Exception::eIncomplete;
        break;
    case Status::kShutdownInProgress:
        sts = RcsdbFacade::Exception::eShutdownInProgress;
        break;
    case Status::kTimedOut:
        sts = RcsdbFacade::Exception::eOpTimeout;
        break;
    case Status::kAborted:
        sts = RcsdbFacade::Exception::eOpAborted;
        break;
    case Status::kBusy:
        sts = RcsdbFacade::Exception::eStoreBusy;
        break;
    case Status::kExpired:
        sts = RcsdbFacade::Exception::eItemExpired;
        break;
    case Status::kTryAgain:
        sts = RcsdbFacade::Exception::eTryAgain;
        break;
    default:
        break;
    }
    return sts;
}

RcsdbFacade::Exception::eStatus Store::TranslateStoreStatus(Store::ePrStatus s)
{
    RcsdbFacade::Exception::eStatus sts = RcsdbFacade::Exception::eInternalError;
    switch ( s ) {
    case eOk:
        sts = RcsdbFacade::Exception::eOk;
        break;
    case eResourceExhausted:
        sts = RcsdbFacade::Exception::eOsResourceExhausted;
        break;
    case eStoreInternalError:
        sts = RcsdbFacade::Exception::eInternalError;
        break;
    case eAccessDenied:
        sts = RcsdbFacade::Exception::eOsError;
        break;
    case eResourceNotFound:
        sts = RcsdbFacade::Exception::eInvalidArgument;
        break;
    case eWrongDbType:
        sts = RcsdbFacade::Exception::eInvalidArgument;
        break;
    default:
        break;
    }
    return sts;
}

//
// Miscellaneous functions
//
bool Store::StopAndArchiveName(LocalArgsDescriptor &args)
{
    WriteLock w_lock(Store::open_close_lock);

    if ( state != eStoreOpened && state != eStoreClosed && args.transaction_db == false) {
        return false;
    }

    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "archiving name id: " << this->id << " name: " << this->name <<
            " state: " << state << SoeLogger::LogDebug() << endl;
    }

    if ( state != eStoreClosed ) {
        int must_wait = 0;
        uint32_t opened_count = InterlockedRefCountGet();
        if ( opened_count > 1 ) {
            must_wait = 1;
        }

        // kick out all other sessions if necessary
        for ( ; opened_count ; opened_count-- ) {
            this->ClosePr(false);
        }

        // this will stall all other sessions (if present) only for this store
        // and quiesce i/os
        state = eStoreClosing;
        if ( must_wait ) {
            usleep(8000000);
        }
    }

    // delete rocsksdb database
    if ( this->rcsdb ) {
        delete this->rcsdb;
        this->rcsdb = 0;
    }

    // proceed with destroying the store
    state = eStoreDestroyed;
    this->DestroyPr();
    Cluster::ArchiveName(this);
    return true;
}

void Store::ArchiveName()
{
    Cluster::ArchiveName(this);
}

void Store::DeregisterApi()
{
    DeregisterApiFunctions();
}

void Store::AsJson(string &json) throw(RcsdbFacade::Exception)
{

}

uint64_t Store::CreateMeta(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    return Store::CreateStore(args);
}

uint64_t Store::RepairMeta(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    return Store::RepairStore(args);
}

void Store::DestroyMeta(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    Store::DestroyStore(args);
}

uint64_t Store::GetMetaByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    return Store::GetStoreByName(args);
}

void Store::ListAllSubsets(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    LocalArgsDescriptor iter_args;
    ::memset(&iter_args, '\0', sizeof(iter_args));
    iter_args.group_idx = -1;
    iter_args.transaction_idx = -1;
    iter_args.iterator_idx = -1;

    // set the subset prefix for search and other fields
    char iter_buf[1024];
    iter_args.key = "1234567890_Nonsense_value_DisjointSubset0987654321_";
    iter_args.key_len = strlen("1234567890_Nonsense_value_DisjointSubset0987654321_");
    char end_key[1024];
    memset(end_key, 0xff, sizeof(end_key));
    memcpy(end_key, "1234567890_Nonsense_value_DisjointSubset0987654321_", strlen("1234567890_Nonsense_value_DisjointSubset0987654321_"));
    iter_args.end_key = end_key;
    iter_args.end_key_len = sizeof(end_key);
    iter_args.sync_mode = LD_Iterator_First;
    iter_args.buf = iter_buf;
    iter_args.buf_len = sizeof(iter_buf);
    iter_args.data = 0;
    iter_args.data_len = 0;
    iter_args.cluster_idx = StoreIterator::eType::eStoreIteratorTypeContainer;
    iter_args.overwrite_dups = true; // overloaded, true = IteratorForward

    SoeLogger::Logger logger;
    LocalArgsDescriptor open_args;
    ::memset(&open_args, '\0', sizeof(open_args));
    open_args = args;

    try {
        Open(open_args);
        CreateIterator(iter_args);

        uint64_t i = 0;
        for ( Iterate(iter_args) ; iter_args.transaction_db == true ; ) {
            //cout << "####### buf: " << iter_args.buf << " buf_len: " << iter_args.buf_len << endl;
            iter_args.buf[MIN(sizeof(iter_buf) - 1, iter_args.buf_len)] = '\0';
            args.list->push_back(std::pair<uint64_t, std::string>(i, iter_args.buf));
            iter_args.sync_mode = LD_Iterator_Next;
            i++;
            Iterate(iter_args);
        }
    } catch (const RcsdbFacade::Exception &ex) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "ListAllSubsets failed what: " << ex.what() <<
            " status: " << ex.status << " errno: " << ex.os_errno << SoeLogger::LogError() << endl;

    }

    DestroyIterator(iter_args);
    Close(open_args);
}

int Store::GetState()
{
    return static_cast<int>(state);
}

uint32_t Store::GetRefCount()
{
    return rcsdb_ref_count;
}

}
