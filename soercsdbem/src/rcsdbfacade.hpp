/**
 * @file    rcsdbfacade.hpp
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
 * RCSDBfacade.hpp
 *
 *  Created on: Nov 29, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_RCSDB_SRC_RCSDBFACADE_HPP_
#define SOE_SOE_SOE_RCSDB_SRC_RCSDBFACADE_HPP_

using namespace rocksdb;

typedef boost::mutex PlainLock;
typedef boost::shared_mutex Lock;
typedef boost::unique_lock<Lock> WriteLock;
typedef boost::shared_lock<Lock> ReadLock;

namespace RcsdbLocalFacade {

class Store;
class Space;
class Cluster;
class StoreConfig;
class SpaceConfig;
class ClusterConfig;

class RcsdbName: public RcsdbFacade::NameBase
{
friend class Cluster;
friend class Space;
friend class Store;

public:
    uint32_t next_available;

    static const char               *db_root_path_local;
    static const char               *db_root_path_srv;

    static const char               *db_root_path;
    static const char               *db_elem_filename;

public:
    virtual ~RcsdbName();

protected:
    RcsdbName();
    virtual void Create() {}
    virtual void SetParent(RcsdbFacade::NameBase *par) {}
    virtual void SetPath() {}
    virtual const std::string &GetPath() = 0;
    virtual void ParseConfig(const std::string &cfg, bool no_exc = false) {}
    virtual std::string CreateConfig() = 0;
    virtual std::string ConfigAddSnapshot(const std::string &name, uint32_t id) = 0;
    virtual std::string ConfigRemoveSnapshot(const std::string &name, uint32_t id) = 0;
    virtual bool StopAndArchiveName(LocalArgsDescriptor &args) = 0;

    virtual void RegisterApiFunctions() throw(SoeApi::Exception) {}
    virtual void DeregisterApiFunctions() throw(SoeApi::Exception) {}

    virtual void Open(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void Close(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void Repair(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}

    virtual void Put(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void Get(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void Delete(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void GetRange(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void Merge(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void Iterate(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}

    virtual void CreateGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void DestroyGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void WriteGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}

    virtual void StartTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void CommitTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void RollbackTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) {}

    virtual void CreateSnapshotEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void CreateSnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void DestroySnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void ListSnapshots(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) {}

    virtual void CreateBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void CreateBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void ListBackupEngines(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) {}

    virtual void CreateIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}
    virtual void DestroyIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) {}

    virtual void SetNextAvailable() {}
    virtual void AsJson(std::string &json) throw(RcsdbFacade::Exception) {}

    RcsdbFacade::FunctorBase          ftors;
    static RcsdbFacade::FunctorBase   static_ftors;
};

}


#endif /* SOE_SOE_SOE_RCSDB_SRC_RCSDBFACADE_HPP_ */
