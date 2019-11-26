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
 * rcsdbspace.hpp
 *
 *  Created on: Jan 27, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBSRV_SRC_METADBSRVRCSDBSPACE_HPP_
#define SOE_SOE_SOE_METADBSRV_SRC_METADBSRVRCSDBSPACE_HPP_

namespace Metadbsrv {

//
// Space
//
class Space: public RcsdbName
{
    friend class Cluster;
    friend class Store;

public:
    typedef enum _eSpaceState {
        eSpaceOpened       = 0,
        eSpaceDestroying   = 1,
        eSpaceDestroyed    = 2,
        eSpaceClosing      = 3,
        eSpaceClosed       = 4
    } eSpaceState;

private:
    std::vector<Metadbsrv::Store *>  stores;
    static uint32_t                  max_stores;
    Lock                             get_store_lock;

public:
    std::string                      name;
    NameBase                        *parent;
    uint64_t                         id;
    RcsdbFacade::SpaceConfig         config;
    bool                             checking_names;
    bool                             debug;
    std::string                      path;
    eSpaceState                      state;

    static uint64_t                  space_null_id;
    const static uint64_t            space_invalid_id = -1;
    const static uint64_t            space_start_id = 1000;

    static MonotonicId64Gener        id_gener;

protected:
    Space(const std::string &nm, uint64_t _id = 0, bool ch_names = true, bool _debug = false);
    virtual void Create();
    virtual void SetParent(NameBase *par) { parent = par; }
    virtual void SetPath();
    virtual const std::string &GetPath();
    virtual void ParseConfig(const std::string &cfg, bool no_exc = false);
    virtual std::string CreateConfig();
    virtual std::string ConfigAddSnapshot(const std::string &name, uint32_t id);
    virtual std::string ConfigRemoveSnapshot(const std::string &name, uint32_t id);
    virtual std::string ConfigAddBackup(const std::string &name, uint32_t id);
    virtual std::string ConfigRemoveBackup(const std::string &name, uint32_t id);
    virtual bool StopAndArchiveName(LocalArgsDescriptor &args);

    void DiscoverStores(const std::string &clu);

public:
    virtual void BackupConfigFiles(const std::string &b_path);
    virtual void RestoreConfigFiles(const std::string &b_path);
    virtual void CreateConfigFiles(const std::string &to_dir, const std::string &from_dir, const std::string &cl_n, const std::string &sp_n, const std::string &s_name);

public:
    Space();
    virtual ~Space();

    static uint64_t CreateSpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void DestroySpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t GetSpaceByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    static uint64_t CreateMeta(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void DestroyMeta(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t GetMetaByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t RepairMeta(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    virtual void Open(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void Close(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void Repair(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    virtual void Put(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void Get(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void Delete(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void GetRange(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void Merge(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

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
    virtual void VerifyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);

protected:
    void PutPr(uint32_t store_idx, uint64_t store_id, const std::string &store_name,
        const char *data, size_t data_len,
        const char *key, size_t key_len) throw(RcsdbFacade::Exception);

    void GetPr(uint32_t store_idx, uint64_t store_id, const std::string &store_name,
        const char *key, size_t key_len,
        char *buf, size_t &buf_len) throw(RcsdbFacade::Exception);

    void DeletePr(uint32_t store_idx, uint64_t store_id, const std::string &store_name,
        const char *key, size_t key_len) throw(RcsdbFacade::Exception);

    Store *GetStore(LocalArgsDescriptor &args,
            bool ch_names = true,
            bool db = false)  throw(RcsdbFacade::Exception);

    Store *FindStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    bool StopAndArchiveStore(LocalArgsDescriptor &args);
    bool DeleteStore(Metadbsrv::Store *st);
    bool RepairOpenCloseStore(LocalArgsDescriptor &args);

    virtual void RegisterApiFunctions() throw(SoeApi::Exception);
    virtual void DeregisterApiFunctions() throw(SoeApi::Exception);
    static void RegisterApiStaticFunctions() throw(SoeApi::Exception);
    static void DeregisterApiStaticFunctions() throw(SoeApi::Exception);

    virtual void SetNextAvailable();
    virtual void AsJson(std::string &json) throw(RcsdbFacade::Exception);

    void ListAllStores(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    void ListAllSubsets(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
};

}

#endif /* SOE_SOE_SOE_METADBSRV_SRC_METADBSRVRCSDBSPACE_HPP_ */
