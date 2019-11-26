/**
 * @file    metadbclifacade.hpp
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
 * metadbclifacade.hpp
 *
 *  Created on: Feb 4, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBCLI_SRC_METADBCLIFACADE_HPP_
#define SOE_SOE_SOE_METADBCLI_SRC_METADBCLIFACADE_HPP_

namespace MetadbcliFacade {

//#define __DEBUG_ASYNC_IO__

void StaticInitializerEnter(void);
void StaticInitializerExit(void);

struct ProxyMapKey
{
    uint64_t      i_val;
    uint32_t      t_tid;
    std::string   s_val;

    ProxyMapKey(uint64_t _i = 0, uint32_t _pid_t = 0, const std::string &_s = "")
        :i_val(_i),
         t_tid(_pid_t),
         s_val(_s)
    {}
    ~ProxyMapKey() {}

    bool operator == (const ProxyMapKey &r)
    {
        return r.i_val == i_val && r.t_tid == t_tid && r.s_val == s_val;
    }

    bool operator != (const ProxyMapKey &r)
    {
        return !(r.i_val == i_val && r.t_tid == t_tid && r.s_val == s_val);
    }

    bool operator < (const ProxyMapKey &r) const
    {
        return i_val < r.i_val ||
            (i_val == r.i_val &&
                ((t_tid == r.t_tid && s_val < r.s_val) ||
                (t_tid < r.t_tid && s_val == r.s_val) ||
                (t_tid < r.t_tid && s_val < r.s_val)));
    }

    bool operator > (const ProxyMapKey &r) const
    {
        return i_val > r.i_val ||
            (i_val == r.i_val &&
                ((t_tid == r.t_tid && s_val > r.s_val) ||
                (t_tid > r.t_tid && s_val == r.s_val) ||
                (t_tid > r.t_tid && s_val > r.s_val)));
    }
};

class SpaceProxy;
class ClusterProxy;
class AsyncIOSession;

typedef boost::shared_mutex Lock;
typedef boost::unique_lock<Lock> WriteLock;
typedef boost::shared_lock<Lock> ReadLock;

typedef void (*FutureSignaller)(RcsdbFacade::FutureSignal *sig);

class StoreProxy: public RcsdbFacade::NameBase
{
    friend class SpaceProxy;
    friend class ClusterProxy;

    std::string             name;
    std::string             space;
    std::string             cluster;
    uint64_t                id;
    uint64_t                space_id;
    uint64_t                cluster_id;
    uint32_t                thread_id;
    bool                    debug;
    static std::string      default_arg;
    static StoreProxy      *reg_api_instance;
    RcsdbFacade::NameBase  *parent;
    AsyncIOSession         *io_session;
    AsyncIOSession         *io_session_async;
    static Lock             adm_op_lock;

    static const uint32_t   max_iters = 1024;
    static const uint32_t   invalid_iter_id = -1;
    ProxyIterator          *iterators[max_iters];
    Lock                    iters_lock;
    Lock                    iov_send_recv_op_lock;
    Lock                    iov_send_recv_op_lock_async;
    FutureSignaller         future_signaller;

    uint64_t                put_op_counter;
    uint64_t                get_op_counter;
    uint64_t                delete_op_counter;
    uint64_t                merge_op_counter;

    uint64_t                in_bytes_counter;
    uint64_t                out_bytes_counter;

    uint32_t                ref_count;
    Lock                    ref_count_lock;

    typedef enum Stet_ {
        eCreated    = 0,
        eOpened     = 1,
        eClosed     = 2,
        eDeleted    = 3
    } State;
    State                   state;

    RcsdbFacade::FunctorBase          ftors;
    static RcsdbFacade::FunctorBase   static_ftors;

    bool                    async_in_loop_started;
    boost::thread          *async_in_thread;
    uint32_t                ttl_in_defunct;

    static std::list<StoreProxy *>  garbage;
    static Lock                     garbage_lock;

#ifdef __DEBUG_ASYNC_IO__
    static Lock             io_debug_lock;
#endif

public:
    const static uint64_t   invalid_id = -1;

    StoreProxy(const std::string &cl,
        const std::string &sp,
        const std::string &nm,
        uint64_t _cl_id,
        uint64_t _sp_id,
        uint64_t _id,
        uint32_t _thread_id,
        bool _debug = false)
            :name(nm),
            space(sp),
            cluster(cl),
            id(_id),
            space_id(_sp_id),
            cluster_id(_cl_id),
            thread_id(_thread_id),
            debug(_debug),
            parent(0),
            io_session(0),
            io_session_async(0),
            future_signaller(0),
            put_op_counter(0),
            get_op_counter(0),
            delete_op_counter(0),
            merge_op_counter(0),
            in_bytes_counter(0),
            out_bytes_counter(0),
            ref_count(0),
            state(StoreProxy::State::eCreated),
            async_in_loop_started(false),
            async_in_thread(0),
            ttl_in_defunct(0)
    {
        ::memset(iterators, '\0', sizeof(iterators));
    }
    virtual ~StoreProxy()
    {
        DeregisterApiFunctions();

        if ( io_session ) {
            io_session->deinitialise(true);
            delete io_session;
        }
        if ( io_session_async ) {
            io_session_async->deinitialise(true);
            delete io_session_async;
        }
        if ( async_in_thread ) {
            async_in_thread->interrupt();
            async_in_thread->join();
            delete async_in_thread;
        }
    }
    void RegisterApiFunctions() throw(SoeApi::Exception);
    void DeregisterApiFunctions() throw(SoeApi::Exception);
    static void RegisterStaticApiFunctions() throw(SoeApi::Exception);
    static void DeregisterStaticApiFunctions() throw(SoeApi::Exception);

    std::string GetName() { return name; }
    uint64_t GetId() { return id; }
    bool GetDebug() { return debug; }
    std::string GetSpaceName() { return space; }
    std::string GetClusterName() { return cluster; }

    uint32_t GetIter(uint32_t store_iter);
    uint32_t PutIter(uint32_t proxy_iter);

    virtual void Create() {}
    virtual void SetPath() {}
    virtual const std::string &GetPath() { return StoreProxy::default_arg; }
    virtual void ParseConfig(const std::string &cfg, bool no_exc = false) {}
    virtual std::string CreateConfig() { return StoreProxy::default_arg; }
    virtual std::string ConfigAddSnapshot(const std::string &name, uint32_t id) { return StoreProxy::default_arg; }
    virtual std::string ConfigRemoveSnapshot(const std::string &name, uint32_t id) { return StoreProxy::default_arg; }
    virtual std::string ConfigAddBackup(const std::string &name, uint32_t id) { return StoreProxy::default_arg; }
    virtual std::string ConfigRemoveBackup(const std::string &name, uint32_t id) { return StoreProxy::default_arg; }
    virtual bool StopAndArchiveName(LocalArgsDescriptor &args) { return false; }
    virtual void SetParent(NameBase *par) {}
    virtual void SetNextAvailable() {}

    uint32_t SendOutboundAsyncOp(Point &event_point, IoEvent *events[], uint32_t ev_count);

protected:
    int PrepareSendIov(const LocalArgsDescriptor &args, IovMsgBase *iov);
    int PrepareRecvIov(LocalArgsDescriptor &args, IovMsgBase *iov);
    int PrepareSendAsyncIov(const LocalArgsDescriptor &args, IovMsgBase *iov);
    int PrepareRecvAsyncIov(LocalArgsDescriptor &args, IovMsgBase *iov);

    void MakeArgsFromAdmArgs(const LocalAdmArgsDescriptor &args, LocalArgsDescriptor &std_args);

    uint32_t RunOp(LocalArgsDescriptor &args, eMsgType op_type);
    uint32_t RunAsyncOp(LocalArgsDescriptor &args, eMsgType op_type);
    uint32_t RecvAsyncOp(LocalArgsDescriptor &args, eMsgType op_type);
    uint32_t RunAdmOp(LocalAdmArgsDescriptor &args, eMsgType op_type);

    void RecvInLoop();

    void FinalizeAsyncRsp(LocalArgsDescriptor &args, uint32_t rsp_type);
    void InitDesc(LocalArgsDescriptor &args) const;

    static uint64_t AddAndRegisterStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void RemoveAndDeregisterStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void Archive(StoreProxy *_pr);

    void CloseIOSession();
    void IncRefCount();

public:
    static uint64_t CreateStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void DestroyStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t GetStoreByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t RepairStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

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
    virtual void VerifyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception);

    virtual void CreateIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    virtual void DestroyIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
};

class SpaceProxy: public RcsdbFacade::NameBase
{
    friend class StoreProxy;
    friend class ClusterProxy;

    std::string             name;
    std::string             cluster;
    uint64_t                id;
    bool                    debug;
    static SpaceProxy      *reg_api_instance;
    RcsdbFacade::NameBase  *parent;
    static Lock             adm_op_lock;

    std::map<ProxyMapKey, StoreProxy &>  stores;
    Lock                                 map_lock;

    RcsdbFacade::FunctorBase          ftors;
    static RcsdbFacade::FunctorBase   static_ftors;

public:
    const static uint64_t   invalid_id = -1;

    SpaceProxy(const std::string &cl,
        const std::string &nm,
        uint64_t _id,
        bool _debug = false)
            :name(nm),
             cluster(cl),
            id(_id),
            debug(_debug),
            parent(0)
    {}
    virtual ~SpaceProxy()
    {
        DeregisterApiFunctions();
    }
    void RegisterApiFunctions()  throw(SoeApi::Exception);
    void DeregisterApiFunctions() throw(SoeApi::Exception);
    static void RegisterStaticApiFunctions() throw(SoeApi::Exception);
    static void DeregisterStaticApiFunctions() throw(SoeApi::Exception);

    std::string GetName() { return name; }
    uint64_t GetId() { return id; }
    bool GetDebug() { return debug; }
    std::string GetClusterName() { return cluster; }

    virtual void Create() {}
    virtual void SetPath() {}
    virtual const std::string &GetPath() { return StoreProxy::default_arg; }
    virtual void ParseConfig(const std::string &cfg, bool no_exc = false) {}
    virtual std::string CreateConfig() { return StoreProxy::default_arg; }
    virtual std::string ConfigAddSnapshot(const std::string &name, uint32_t id) { return StoreProxy::default_arg; }
    virtual std::string ConfigRemoveSnapshot(const std::string &name, uint32_t id) { return StoreProxy::default_arg; }
    virtual std::string ConfigAddBackup(const std::string &name, uint32_t id) { return StoreProxy::default_arg; }
    virtual std::string ConfigRemoveBackup(const std::string &name, uint32_t id) { return StoreProxy::default_arg; }
    virtual bool StopAndArchiveName(LocalArgsDescriptor &args) { return false; }
    virtual void SetParent(NameBase *par) {}
    virtual void SetNextAvailable() {}

protected:
    bool AddStoreProxy(StoreProxy &st);
    bool DeleteStoreProxy(const ProxyMapKey &key);
    StoreProxy *FindStoreProxy(const ProxyMapKey &key);

    static uint64_t AddAndRegisterSpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

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
    virtual void RestoreBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void DestroyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void VerifyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }

    virtual void CreateIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void DestroyIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
};

class ClusterProxy: public RcsdbFacade::NameBase
{
    friend class StoreProxy;
    friend class SpaceProxy;

    std::string           name;
    uint64_t              id;
    bool                  debug;
    static Lock           global_lock;
    static ClusterProxy  *reg_api_instance;
    static Lock           adm_op_lock;

    static const uint32_t                no_thread_id = 77777777;
    std::map<ProxyMapKey, SpaceProxy &>  spaces;
    Lock                                 map_lock;

    static std::map<ProxyMapKey, ClusterProxy &>  clusters;
    static Lock                                   clusters_map_lock;

    RcsdbFacade::FunctorBase          ftors;
    static RcsdbFacade::FunctorBase   static_ftors;

public:
    class Initializer {
    public:
        static bool initialized;
        Initializer();
        ~Initializer();
    };
    static Initializer    initalized;

    static bool           global_debug;
    static bool           msg_debug;
    static uint64_t       adm_msg_counter;
    const static uint64_t invalid_id = -1;

public:
    ClusterProxy(const std::string &nm,
        uint64_t _id,
        bool _debug = false)
            :name(nm),
            id(_id),
            debug(_debug)
    {}
    virtual ~ClusterProxy()
    {
        DeregisterApiFunctions();
    }
    void RegisterApiFunctions()  throw(SoeApi::Exception);
    void DeregisterApiFunctions()  throw(SoeApi::Exception);
    static void RegisterStaticApiFunctions() throw(SoeApi::Exception);
    static void DeregisterStaticApiFunctions() throw(SoeApi::Exception);

    std::string GetName() { return name; }
    uint64_t GetId() { return id; }
    bool GetDebug() { return debug; }

    virtual void Create() {}
    virtual void SetPath() {}
    virtual const std::string &GetPath() { return StoreProxy::default_arg; }
    virtual void ParseConfig(const std::string &cfg, bool no_exc = false) {}
    virtual std::string CreateConfig() { return StoreProxy::default_arg; }
    virtual std::string ConfigAddSnapshot(const std::string &name, uint32_t id) { return StoreProxy::default_arg; }
    virtual std::string ConfigRemoveSnapshot(const std::string &name, uint32_t id) { return StoreProxy::default_arg; }
    virtual std::string ConfigAddBackup(const std::string &name, uint32_t id) { return StoreProxy::default_arg; }
    virtual std::string ConfigRemoveBackup(const std::string &name, uint32_t id) { return StoreProxy::default_arg; }
    virtual bool StopAndArchiveName(LocalArgsDescriptor &args) { return false; }
    virtual void SetParent(NameBase *par) {}
    virtual void SetNextAvailable() {}

protected:
    bool AddSpaceProxy(SpaceProxy &sp);
    bool DeleteSpaceProxy(const ProxyMapKey &key);
    SpaceProxy *FindSpaceProxy(const ProxyMapKey &key);

    static bool StaticAddClusterProxyNoLock(ClusterProxy &cp);
    static bool StaticDeleteClusterProxyNoLock(const ProxyMapKey &key);
    static ClusterProxy *StaticFindClusterProxyNoLock(const ProxyMapKey &key);

    static uint64_t AddAndRegisterCluster(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void SwapNameList(std::vector<std::pair<uint64_t, std::string> > &from_list, std::vector<std::pair<uint64_t, std::string> > &to_list);

public:
    static bool StaticAddClusterProxy(ClusterProxy &cl, bool lock_clusters_map = true);
    static bool StaticDeleteClusterProxy(const ProxyMapKey &key, bool lock_clusters_map = true);
    static ClusterProxy *StaticFindClusterProxy(const ProxyMapKey &key, bool lock_clusters_map = true);

    static bool StaticAddSpaceProxy(const ProxyMapKey &cl_key, SpaceProxy &sp);
    static bool StaticDeleteSpaceProxy(const ProxyMapKey &cl_key, const ProxyMapKey &sp_key);
    static SpaceProxy *StaticFindSpaceProxy(const ProxyMapKey &cl_key, const ProxyMapKey &sp_key);

    static bool StaticAddStoreProxy(const ProxyMapKey &cl_key, const ProxyMapKey &sp_key, StoreProxy &st);
    static bool StaticDeleteStoreProxy(const ProxyMapKey &cl_key, const ProxyMapKey &sp_key, const ProxyMapKey &st_key);
    static StoreProxy *StaticFindStoreProxy(const ProxyMapKey &cl_key, const ProxyMapKey &sp_key, const ProxyMapKey &st_key);

public:
    static uint64_t CreateCluster(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void DestroyCluster(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t GetClusterByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    static void ListSubsets(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void ListStores(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void ListSpaces(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void ListClusters(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

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
    virtual void RestoreBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void DestroyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void VerifyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }

    virtual void CreateIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
    virtual void DestroyIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception) { throw RcsdbFacade::Exception("Not supported", RcsdbFacade::Exception::eOpNotSupported); }
};

}

#endif /* SOE_SOE_SOE_METADBCLI_SRC_METADBCLIFACADE_HPP_ */
