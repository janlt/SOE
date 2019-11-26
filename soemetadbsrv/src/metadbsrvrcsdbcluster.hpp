/**
 * @file    rcsdbcluster.hpp
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
 * rcsdbcluster.hpp
 *
 *  Created on: Jan 27, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBSRV_SRC_METADBSRVRCSDBCLUSTER_HPP_
#define SOE_SOE_SOE_METADBSRV_SRC_METADBSRVRCSDBCLUSTER_HPP_

namespace Metadbsrv {

void StaticInitializerEnter2(void);
void StaticInitializerExit2(void);

//
// Cluster
//
class Cluster: public RcsdbName
{
    friend class Space;
    friend class Store;

public:
    typedef enum _eClusterState {
        eClusterOpened       = 0,
        eClusterDestroying   = 1,
        eClusterDestroyed    = 2,
        eClusterClosing      = 3,
        eClusterClosed       = 4
    } eClusterState;

private:
    std::vector<Metadbsrv::Space *>  spaces;
    static uint32_t                  max_spaces;

    static uint64_t                  cluster_null_id;
    const static uint64_t            cluster_invalid_id = -1;
    const static uint64_t            cluster_start_id = 10;

    static MonotonicId64Gener        id_gener;

    static uint32_t                  max_clusters;
    static Cluster                  *clusters[];
    static Lock                      global_lock;

    static bool                      global_debug;
    eClusterState                    state;

    static Lock                      archive_lock;
    static std::list<RcsdbName *>    archive;
    static boost::thread            *pruner_thread;

    static const char               *metadbsrv_name;

public:
    std::string                      name;
    NameBase                        *parent;
    uint64_t                         id;
    RcsdbFacade::ClusterConfig       config;
    bool                             checking_names;
    bool                             debug;
    std::string                      path;

    // cluster discovery is done on demand
    class Initializer
    {
    public:
        static Initializer *instance;
        static bool initialized;
        Initializer();
        ~Initializer();

        static void Initialize();
        static Initializer *CheckInstance();
    };

    // static functions must be registered upon module load
    class StaticInitializer
    {
    public:
        static bool static_initialized;
        StaticInitializer();
        ~StaticInitializer();
    };

    static StaticInitializer static_initializer;

public:
    Cluster();
    virtual ~Cluster();
    virtual void SetParent(NameBase *par) { parent = this; }

    static uint64_t CreateCluster(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void DestroyCluster(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t GetClusterByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    static void ListSubsets(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void ListStores(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void ListSpaces(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void ListClusters(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    static void ListBackupStores(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void ListBackupSpaces(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void ListBackupClusters(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    static Store *StaticFindStore(LocalArgsDescriptor &args);

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
    Cluster(const std::string &nm,
            uint64_t _id = 0,
            bool ch_names = true,
            bool _debug = false) throw(RcsdbFacade::Exception);
    virtual void SetNextAvailable();
    virtual void Create() throw(RcsdbFacade::Exception);
    virtual void SetPath();
    virtual const std::string &GetPath();
    virtual void ParseConfig(const std::string &cfg, bool no_exc = false);
    virtual std::string CreateConfig();
    virtual std::string ConfigAddSnapshot(const std::string &name, uint32_t id);
    virtual std::string ConfigRemoveSnapshot(const std::string &name, uint32_t id);
    virtual std::string ConfigAddBackup(const std::string &name, uint32_t id);
    virtual std::string ConfigRemoveBackup(const std::string &name, uint32_t id);
    virtual bool StopAndArchiveName(LocalArgsDescriptor &args);
    virtual void AsJson(std::string &json) throw(RcsdbFacade::Exception);

    static void ArchiveName(RcsdbName *name);
    static void PruneArchive();

    static void Discover();
    static void Destroy();
    void DiscoverSpaces();
    static Cluster *GetCluster(LocalArgsDescriptor &args,
            bool ch_names = true,
            bool db = false) throw(RcsdbFacade::Exception);
    Space *GetSpace(LocalArgsDescriptor &args,
            bool ch_names = true,
            bool db = false) throw(RcsdbFacade::Exception);
    Store *GetStore(LocalArgsDescriptor &args,
            bool ch_names = true,
            bool db = false) throw(RcsdbFacade::Exception);

    Space *FindSpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    Store *FindStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    bool StopAndArchiveSpace(LocalArgsDescriptor &args);
    bool StopAndArchiveStore(LocalArgsDescriptor &args);
    bool RepairOpenCloseStore(LocalArgsDescriptor &args);
    bool DeleteStore(Metadbsrv::Store *st);

    virtual void RegisterApiFunctions() throw(SoeApi::Exception);
    virtual void DeregisterApiFunctions() throw(SoeApi::Exception);
    static void RegisterApiStaticFunctions() throw(SoeApi::Exception);
    static void DeregisterApiStaticFunctions() throw(SoeApi::Exception);

    static uint64_t CreateSpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t CreateStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t RepairStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void DestroySpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void DestroyStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t GetSpaceByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t GetStoreByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    static void DeleteFromStores(Metadbsrv::Store *st);

    static std::string AsJson(void) throw(RcsdbFacade::Exception);

    void ListAllSpaces(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    void ListAllStores(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    void ListAllSubsets(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

public:
    virtual void BackupConfigFiles(const std::string &b_path);
    virtual void RestoreConfigFiles(const std::string &b_path);
    virtual void CreateConfigFiles(const std::string &to_dir, const std::string &from_dir, const std::string &cl_n, const std::string &sp_n, const std::string &s_name);
};

}

#endif /* SOE_SOE_SOE_METADBSRV_SRC_METADBSRVRCSDBCLUSTER_HPP_ */
