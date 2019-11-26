/**
 * @file    metadbsrvdefaultstore.cpp
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
 * metadbsrvdefaultstore.cpp
 *
 *  Created on: Nov 25, 2017
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
#include "metadbsrvdefaultstore.hpp"

using namespace std;
using namespace RcsdbFacade;

namespace Metadbsrv {

DefaultStore::DefaultStore():
    Store()
{}

void DefaultStore::Open(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::Close(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::Repair(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }

void DefaultStore::Put(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::Get(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::Delete(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::GetRange(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::Merge(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::Iterate(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }

void DefaultStore::CreateGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::DestroyGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::WriteGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }

void DefaultStore::StartTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::CommitTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::RollbackTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }

void DefaultStore::CreateSnapshotEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::CreateSnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::DestroySnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::ListSnapshots(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }

void DefaultStore::CreateBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::CreateBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::RestoreBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::DestroyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }

void DefaultStore::CreateIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }
void DefaultStore::DestroyIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("No such store", RcsdbFacade::Exception::eNoFunction); }

int GetState() { return 100000; }
uint32_t GetRefCount() { return 100000; }

}


