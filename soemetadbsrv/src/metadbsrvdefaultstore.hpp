/**
 * @file    metadbsrvdefaultstore.hpp
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
 * metadbsrvdefaultstore.hpp
 *
 *  Created on: Nov 25, 2017
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_METADBSRV_SRC_METADBSRVDEFAULTSTORE_HPP_
#define RENTCONTROL_SOE_SOE_SOE_METADBSRV_SRC_METADBSRVDEFAULTSTORE_HPP_

namespace Metadbsrv {

//
// DefaultStore
//
class DefaultStore: public Store
{
    string name;

public:

    DefaultStore();
    virtual void Create() {}
    virtual void SetPath() {}
    virtual const std::string &GetPath() { return name; }
    virtual void ParseConfig(const std::string &cfg, bool no_exc = false) {}
    virtual std::string CreateConfig() { return name; }
    virtual std::string ConfigAddSnapshot(const std::string &name, uint32_t id) { return name; }
    virtual std::string ConfigRemoveSnapshot(const std::string &name, uint32_t id) { return name; }
    virtual std::string ConfigAddBackup(const std::string &name, uint32_t id) { return name; }
    virtual std::string ConfigRemoveBackup(const std::string &name, uint32_t id) { return name; }
    virtual bool StopAndArchiveName(LocalArgsDescriptor &args) { return false; }
    virtual void SetParent(NameBase *par) {}

public:
    virtual ~DefaultStore() {}
    virtual void Open(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void Close(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void Repair(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    virtual void Put(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void Get(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void Delete(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void GetRange(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void Merge(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void Iterate(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    virtual void CreateGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void DestroyGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void WriteGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    virtual void StartTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void CommitTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void RollbackTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);

    virtual void CreateSnapshotEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void CreateSnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void DestroySnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void ListSnapshots(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);

    virtual void CreateBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void CreateBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void RestoreBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void DestroyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);

    virtual void CreateIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void DestroyIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    int GetState();
    uint32_t GetRefCount();

protected:
    virtual void RegisterApiFunctions() throw(SoeApi::Exception) {}
    virtual void DeregisterApiFunctions() throw(SoeApi::Exception) {}
    virtual void SetNextAvailable() {}
    virtual void AsJson(std::string &json) throw(RcsdbFacade::Exception) {}
};

}

#endif /* RENTCONTROL_SOE_SOE_SOE_METADBSRV_SRC_METADBSRVDEFAULTSTORE_HPP_ */
