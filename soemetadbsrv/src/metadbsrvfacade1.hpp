/**
 * @file    metadbsrvfacade1.hpp
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
 * metadbsrvfacade1.hpp
 *
 *  Created on: Nov 24, 2017
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_METADBSRV_SRC_METADBSRVFACADE1_HPP_
#define RENTCONTROL_SOE_SOE_SOE_METADBSRV_SRC_METADBSRVFACADE1_HPP_

namespace Metadbsrv {

void StaticInitializerEnter1(void);
void StaticInitializerExit1(void);

struct ProxyMapKey1
{
    uint64_t      i_val;
    uint32_t      t_tid;
    std::string   s_val;

    ProxyMapKey1(uint64_t _i = 0, uint32_t _pid_t = 0, const std::string &_s = "")
        :i_val(_i),
         t_tid(_pid_t),
         s_val(_s)
    {}
    ~ProxyMapKey1() {}

    bool operator == (const ProxyMapKey1 &r)
    {
        return r.i_val == i_val && r.t_tid == t_tid && r.s_val == s_val;
    }

    bool operator != (const ProxyMapKey1 &r)
    {
        return !(r.i_val == i_val && r.t_tid == t_tid && r.s_val == s_val);
    }

    bool operator < (const ProxyMapKey1 &r) const
    {
        return i_val < r.i_val ||
            (i_val == r.i_val &&
                ((t_tid == r.t_tid && s_val < r.s_val) ||
                (t_tid < r.t_tid && s_val == r.s_val) ||
                (t_tid < r.t_tid && s_val < r.s_val) ||
                (t_tid < r.t_tid && s_val < r.s_val)));
    }

    bool operator > (const ProxyMapKey1 &r) const
    {
        return i_val > r.i_val ||
            (i_val == r.i_val &&
                ((t_tid == r.t_tid && s_val > r.s_val) ||
                (t_tid > r.t_tid && s_val == r.s_val) ||
                (t_tid > r.t_tid && s_val > r.s_val) ||
                (t_tid > r.t_tid && s_val > r.s_val)));
    }
};

class SpaceProxy1;
class ClusterProxy1;

typedef boost::shared_mutex Lock;
typedef boost::unique_lock<Lock> WriteLock;
typedef boost::shared_lock<Lock> ReadLock;

typedef const std::string (*CreateMetadbsrvNameFun)(const std::string &cluster,
    const std::string &space,
    const std::string &store,
    uint32_t token,
    const std::string &fun);

//#define _GARBAGE_COLLECT_STORES_

class StoreProxy1: public RcsdbFacade::ProxyNameBase
{
    friend class SpaceProxy1;
    friend class ClusterProxy1;

    std::string                  name;
    std::string                  space;
    std::string                  cluster;
    uint64_t                     id;
    uint64_t                     space_id;
    uint64_t                     cluster_id;
    uint32_t                     thread_id;
    static RcsdbAccessors1<Metadbsrv::Store, Metadbsrv::DefaultStore>       global_acs;
    RcsdbAccessors1<Metadbsrv::Store, Metadbsrv::DefaultStore>              acs;
    bool                         debug;
    RcsdbFacade::ProxyNameBase  *parent;
#ifdef _GARBAGE_COLLECT_STORES_
    uint32_t                     in_archive_ttl;
#endif

#ifdef __DEBUG_ASYNC_IO__
    static Lock                  io_debug_lock;
#endif

public:
    AsyncIOSession              *io_session;
    AsyncIOSession              *io_session_async;
    uint64_t                     adm_session_id;

    typedef enum Stet_ {
        eCreated    = 0,
        eOpened     = 1,
        eClosed     = 2,
        eDeleted    = 3
    } State;
    State                        state;

public:
    const static uint64_t        invalid_id = -1;
    const static uint32_t        invalid_32_id = -1;

    StoreProxy1(const std::string &cl,
        const std::string &sp,
        const std::string &nm,
        uint64_t _cluster_id,
        uint64_t _space_id,
        uint64_t _id,
        uint32_t _thread_id,
        bool _debug = false,
        uint64_t _adm_s_id = StoreProxy1::invalid_id)
            :name(nm),
            space(sp),
            cluster(cl),
            id(_id),
            space_id(_space_id),
            cluster_id(_cluster_id),
            thread_id(_thread_id),
            debug(_debug),
            parent(0),
#ifdef _GARBAGE_COLLECT_STORES_
            in_archive_ttl(0),
#endif
            io_session(0),
            io_session_async(0),
            adm_session_id(_adm_s_id),
            state(StoreProxy1::eCreated)
    {}
    virtual ~StoreProxy1()
    {
        state = StoreProxy1::eDeleted;
    }
    void ResetAcs();
    void BindApiFunctions() throw(RcsdbFacade::Exception);
    static void BindStaticApiFunctions() throw(RcsdbFacade::Exception);

    std::string GetName() { return name; }
    uint64_t GetId() { return id; }
    bool GetDebug() { return debug; }
    std::string GetSpaceName() { return space; }
    uint64_t GetSpaceId() { return space_id; }
    std::string GetClusterName() { return cluster; }
    uint64_t GetClusterId() { return cluster_id; }
    uint32_t GetThreadId() { return thread_id; }
    void CloseStores();

    uint32_t processAsyncInboundOp(LocalArgsDescriptor &args, uint32_t req_type, RcsdbFacade::Exception::eStatus &sts) throw(RcsdbFacade::Exception);
    uint32_t processSyncInboundOp(LocalArgsDescriptor &args, uint32_t req_type, RcsdbFacade::Exception::eStatus &sts) throw(RcsdbFacade::Exception);

public:
    static void GarbageCollectStore(StoreProxy1 *&st_pr, uint64_t t_id, LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void GarbageCollectStoreNoLock(StoreProxy1 *&st_pr, uint64_t t_id, LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t CreateStore(LocalArgsDescriptor &args, const IPv4Address &from_addr, const MetaNet::Point *pt, uint64_t _adm_s_id) throw(RcsdbFacade::Exception);
    static void DestroyStore(LocalArgsDescriptor &args, const IPv4Address &from_addr) throw(RcsdbFacade::Exception);
    static uint64_t GetStoreByName(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception);
    static uint64_t RepairStore(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception);

    static uint32_t StaticCreateBackup(LocalAdmArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception);
    static uint32_t StaticRestoreBackup(LocalAdmArgsDescriptor &args, const IPv4Address &from_addr) throw(RcsdbFacade::Exception);
    static uint32_t StaticDestroyBackup(LocalAdmArgsDescriptor &args, const IPv4Address &from_addr) throw(RcsdbFacade::Exception);
    static uint32_t StaticVerifyBackup(LocalAdmArgsDescriptor &args, const IPv4Address &from_addr) throw(RcsdbFacade::Exception);
    static uint32_t StaticCreateSnapshot(LocalAdmArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception);
    static uint32_t StaticRestoreSnapshot(LocalAdmArgsDescriptor &args, const IPv4Address &from_addr) throw(RcsdbFacade::Exception);
    static uint32_t StaticDestroySnapshot(LocalAdmArgsDescriptor &args, const IPv4Address &from_addr) throw(RcsdbFacade::Exception);

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
    virtual void DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);

    virtual void CreateIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void DestroyIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    int PrepareSendIov(const LocalArgsDescriptor &args, IovMsgBase *iov);
    int PrepareRecvDesc(LocalArgsDescriptor &args, IovMsgBase *iov);

    void MakeAdmArgsFromStdArgs(LocalAdmArgsDescriptor &adm_args, const LocalArgsDescriptor &args);
    void OnDisconnectClose() throw(RcsdbFacade::Exception);
    RcsdbAccessors1<Metadbsrv::Store, Metadbsrv::DefaultStore> &GetAcs() { return acs; }

protected:
    void DecodeGroupTransactionIdx(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
};

class SpaceProxy1: public RcsdbFacade::ProxyNameBase
{
    friend class ClusterProxy1;

    std::string                  name;
    std::string                  space;
    uint64_t                     id;
    uint64_t                     space_id;
    static RcsdbAccessors1<Metadbsrv::Space, Metadbsrv::Space>        global_acs;
    RcsdbAccessors1<Metadbsrv::Space, Metadbsrv::Space>               acs;
    bool                         debug;
    RcsdbFacade::ProxyNameBase  *parent;

    std::map<ProxyMapKey1, StoreProxy1 *>  stores;
    Lock                                 map_lock;

public:
    const static uint64_t        invalid_id = -1;

    SpaceProxy1(const std::string &sp,
        const std::string &nm,
        uint64_t _space_id,
        uint64_t _id,
        bool _debug = false)
            :name(nm),
             space(sp),
            id(_id),
            space_id(_space_id),
            debug(_debug),
            parent(0)
    {}
    virtual ~SpaceProxy1() {}
    void BindApiFunctions()  throw(RcsdbFacade::Exception) {}
    static void BindStaticApiFunctions() throw(RcsdbFacade::Exception);

    std::string GetName() { return name; }
    uint64_t GetId() { return id; }
    bool GetDebug() { return debug; }
    std::string GetSpaceName() { return space; }
    uint64_t GetSpaceId() { return space_id; }

protected:
    bool AddStoreProxy(StoreProxy1 &st);
    StoreProxy1 *RemoveStoreProxy(const ProxyMapKey1 &key);  // delete from map and return without deleting the object
    bool DeleteStoreProxy(const ProxyMapKey1 &key);         // delete from map and delete the object
    StoreProxy1 *FindStoreProxy(const ProxyMapKey1 &key);
    bool FindAllStoreProxies(const ProxyMapKey1 &st_key, std::list<StoreProxy1 *> &removed_proxies);

public:
    static uint64_t CreateSpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void DestroySpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t GetSpaceByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    virtual void Open(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void Close(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void Repair(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }

    virtual void Put(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void Get(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void Delete(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void GetRange(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void Merge(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void Iterate(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }

    virtual void CreateGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void DestroyGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void WriteGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }

    virtual void StartTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void CommitTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void RollbackTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }

    virtual void CreateSnapshotEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void CreateSnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void DestroySnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void ListSnapshots(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }

    virtual void CreateBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void CreateBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }

    virtual void CreateIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void DestroyIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
};

class ClusterProxy1: public RcsdbFacade::ProxyNameBase
{
    friend class StoreProxy1;
    friend class SpaceProxy1;

    std::string            name;
    uint64_t               id;
    static RcsdbAccessors1<Metadbsrv::Cluster, Metadbsrv::Cluster>  global_acs;
    RcsdbAccessors1<Metadbsrv::Cluster, Metadbsrv::Cluster>         acs;
    bool                   debug;
    static Lock            global_lock;
    static Lock            static_op_lock;

public:
    static const uint32_t                         no_thread_id = 77777777;

private:
    std::map<ProxyMapKey1, SpaceProxy1 &>           spaces;
    Lock                                          map_lock;

    static std::map<ProxyMapKey1, ClusterProxy1 &>  clusters;
    static Lock                                   clusters_map_lock;

    static std::list<StoreProxy1 *>                archive;

public:
    class Initializer {
    public:
        static bool initialized;
        Initializer();
        ~Initializer();
    };
    static Initializer     initalized1;

    static bool            global_debug;
    static bool            msg_debug;
    static uint64_t        adm_msg_counter;
    const static uint64_t  invalid_id = -1;

public:
    ClusterProxy1(const std::string &nm,
        uint64_t _id,
        bool _debug = false)
            :name(nm),
            id(_id),
            debug(_debug)
    {}
    virtual ~ClusterProxy1() {}
    void BindApiFunctions()  throw(RcsdbFacade::Exception) {}
    static void BindStaticApiFunctions() throw(RcsdbFacade::Exception);

    std::string GetName() { return name; }
    uint64_t GetId() { return id; }
    bool GetDebug() { return debug; }

protected:
    bool AddSpaceProxy(SpaceProxy1 &sp);
    bool DeleteSpaceProxy(const ProxyMapKey1 &key);
    SpaceProxy1 *FindSpaceProxy(const ProxyMapKey1 &key);

    static bool StaticAddClusterProxy1NoLock(ClusterProxy1 &cp);
    static bool StaticDeleteClusterProxy1NoLock(const ProxyMapKey1 &key);
    static ClusterProxy1 *StaticFindClusterProxy1NoLock(const ProxyMapKey1 &key);

public:
    static bool StaticAddClusterProxy(ClusterProxy1 &cl, bool lock_clusters_map = true);
    static bool StaticDeleteClusterProxy(const ProxyMapKey1 &key, bool lock_clusters_map = true);
    static ClusterProxy1 *StaticFindClusterProxy(const ProxyMapKey1 &key, bool lock_clusters_map = true);

    static bool StaticAddSpaceProxy(const ProxyMapKey1 &cl_key, SpaceProxy1 &sp);
    static bool StaticDeleteSpaceProxy(const ProxyMapKey1 &cl_key, const ProxyMapKey1 &sp_key);
    static SpaceProxy1 *StaticFindSpaceProxy(const ProxyMapKey1 &cl_key, const ProxyMapKey1 &sp_key);

    static bool StaticAddStoreProxy(const ProxyMapKey1 &cl_key, const ProxyMapKey1 &sp_key, StoreProxy1 &st);
    static StoreProxy1 *StaticRemoveStoreProxy(const ProxyMapKey1 &cl_key, const ProxyMapKey1 &sp_key, const ProxyMapKey1 &st_key);
    static bool StaticDeleteStoreProxy(const ProxyMapKey1 &cl_key, const ProxyMapKey1 &sp_key, const ProxyMapKey1 &st_key);
    static StoreProxy1 *StaticFindStoreProxy(const ProxyMapKey1 &cl_key, const ProxyMapKey1 &sp_key, const ProxyMapKey1 &st_key);
    static bool StaticFindAllStoreProxies(const ProxyMapKey1 &cl_key, const ProxyMapKey1 &sp_key, const ProxyMapKey1 &st_key,
        std::list<StoreProxy1 *> &removed_proxies);

    static void ArchiveStoreProxy(const StoreProxy1 &sp);
    static void PruneArchive();

public:
    static uint64_t CreateCluster(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void DestroyCluster(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t GetClusterByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    static void ListSubsets(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception);
    static void ListStores(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception);
    static void ListSpaces(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception);
    static void ListClusters(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception);

    static void ListBackupStores(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception);
    static void ListBackupSpaces(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception);
    static void ListBackupClusters(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception);

    virtual void Open(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void Close(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void Repair(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }

    virtual void Put(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void Get(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void Delete(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void GetRange(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void Merge(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void Iterate(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }

    virtual void CreateGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void DestroyGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void WriteGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }

    virtual void StartTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void CommitTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void RollbackTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }

    virtual void CreateSnapshotEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void CreateSnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void DestroySnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void ListSnapshots(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }

    virtual void CreateBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void CreateBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }

    virtual void CreateIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void DestroyIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
};

}

#endif /* RENTCONTROL_SOE_SOE_SOE_METADBSRV_SRC_METADBSRVFACADE1_HPP_ */
