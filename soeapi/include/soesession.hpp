/**
 * @file    soesession.hpp
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
 * soesession.hpp
 *
 *  Created on: Feb 3, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOE_INCLUDE_SOESESSION_HPP_
#define SOE_SOE_SOE_SOE_INCLUDE_SOESESSION_HPP_

namespace Soe {

class Group;
class Transaction;
class BackupEngine;
class SnapshotEngine;
class SessionIterator;
class SubsetIterator;
class Subsetable;
class DisjointSubset;
class UnionSubset;
class ComplementSubset;
class NotComplementSubset;
class CartesianSubset;
class Futurable;
class SimpleFuture;
class PutFuture;
class GetFuture;
class DeleteFuture;
class MergeFuture;
class GetSetFuture;
class PutSetFuture;
class DeleteSetFuture;
class MergeSetFuture;

/**
 * @brief Session class
 *
 * This class provides access to underlying Soe names via Open(), Create(), Destroy() methods.
 * An object if this class is instantiated as a four-way tuple {"Cluster", "Space", "Store", "Provider"}
 *
 * When user desires to open a session by specifying {"Cluster", "Space", "Store", "Provider"}
 * and any one of the names specified doesn't exist, an error will be returned.
 * When user desires to create a new session and any one of the four names specified doesn't exist,
 * a new set of names will be created in Soe's address space. Otherwise, if all the names
 * specified already exist, an session to an existing set of names will be created.
 *
 * Provider defaults to Provider::default_provider, which currently is configure to
 * be "METADBCLI"
 *
 * Users can create multiple instances of Session within the same address space and use
 * them concurrently, i.e. sessions are MT safe.
 *
 * Depending on the application needs, users can create transaction sessions or non-transaction
 * ones. A transaction session, and the underlying Store, are transaction aware and are
 * capable of resolving update conflicts resulting from multiple transactions trying to update
 * the same key(s). Transaction session is created with transaction_db argument set to "true"
 *
 */
class Session: public Sessionable
{
public:
    typedef enum _eSyncMode {
        SOE_SESSION_SYNC_MODE
    } eSyncMode;

    typedef enum _eSubsetType {
        SOE_SESSION_SUBSET_TYPE
    } eSubsetType;

    class Deleter
    {
    public:
        void operator()(Session *p) {}
    };

private:
    friend class Group;
    friend class Transaction;
    friend class BackupEngine;
    friend class SnapshotEngine;
    friend class SessionIterator;
    friend class SubsetIterator;
    friend class Subsetable;
    friend class DisjointSubset;
    friend class UnionSubset;
    friend class ComplementSubset;
    friend class NotComplementSubset;
    friend class CartesianSubset;
    friend class Futurable;
    friend class SimpleFuture;
    friend class SetFuture;
    friend class Manager;

    Provider            provider;
    std::string         cluster_name;
    std::string         space_name;
    std::string         container_name;
    uint64_t            cluster_id;
    uint64_t            space_id;
    uint64_t            container_id;
    uint32_t            cr_thread_id;
    SoeAccessors        acs;
    bool                transaction_db;
    bool                no_throw;
    bool                debug;
    Session::eSyncMode  sync_mode;
    CreateLocalNameFun  create_name_fun;

    SoeAccessors        cluster_acs;
    SoeAccessors        space_acs;

    // for async API
    std::shared_ptr<Session>   this_sess;
    std::list<Futurable *>     garbage_futures;
    Lock                       garbage_futures_lock;
    boost::atomic_uint32_t     pending_num_futures;

    static std::list<Futurable *>     global_garbage_futures;
    static Lock                       global_garbage_futures_lock;

    const static uint32_t      invalid_id = -1;
    const static uint64_t      invalid_64_id = -1;

public:

    Session(const std::string &c_n = "",
        const std::string &s_n = "",
        const std::string &cn_n = "",
        const std::string &pr = Provider::default_provider,
        bool _transation_db = false,
        bool _debug = false,
        bool _no_throw = true,
        Session::eSyncMode s_m = Session::eSyncMode::eSyncDefault) throw(std::runtime_error);
    virtual ~Session();

    typedef enum _eStatus {
        SOE_SESSION_STATUS
    } Status;

    static const SoeSessionStatusText sts_texts[];

    static const char *StatusToText(Status sts);

    bool               open_status;
    Status             last_status;

    Status GetLastStatus() { return last_status; }
    void SetLastStatus(Status _sts) { last_status = _sts; }
    void SetCLastStatus(Status _sts) const { const_cast<Session *>(this)->last_status = _sts; }

    Status Open(const std::string &c_n,
        const std::string &s_n,
        const std::string &cn_n,
        const std::string &pr = Provider::default_provider,
        bool _transation_db = false,
        bool _debug = false,
        bool no_throw = true,
        Session::eSyncMode s_m = Session::eSyncMode::eSyncDefault) throw(std::runtime_error);
    bool ok(Status *sts = 0) const;
    Status Create() throw(std::runtime_error);
    Status DestroyStore() throw(std::runtime_error);

    //
    // API to list clusters, spaces, stores
    //
    Status ListClusters(std::vector<std::pair<uint64_t, std::string> > &clusters, std::string backup_path = "") throw(std::runtime_error);
    Status ListSpaces(const std::string &c_n, uint64_t c_id, std::vector<std::pair<uint64_t, std::string> > &spaces, std::string backup_path = "") throw(std::runtime_error);
    Status ListStores(const std::string &c_n, uint64_t c_id, const std::string &s_n, uint64_t s_id, std::vector<std::pair<uint64_t, std::string> > &stores, std::string backup_path = "") throw(std::runtime_error);
    Status ListSubsets(const std::string &c_n, uint64_t c_id, const std::string &s_n, uint64_t s_id, const std::string &t_n, uint64_t t_id, std::vector<std::pair<uint64_t, std::string> > &subsets) throw(std::runtime_error);

    //
    // Sync message API
    //
    Status Put(const char *key, size_t key_size, const char *data, size_t data_size, bool over_dups = false) throw(std::runtime_error);
    Status Get(const char *key, size_t key_size, char *buf, size_t *buf_size) throw(std::runtime_error);
    Status Delete(const char *key, size_t key_size) throw(std::runtime_error);
    Status Merge(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error);

    Status GetRange(const char *start_key,
            size_t start_key_size,
            const char *end_key,
            size_t end_key_size,
            char *buf,
            size_t *buf_size,
            RangeCallback r_cb = NULL,
            void *ctx = 0,
            Iterator::eIteratorDir dir = Iterator::eIteratorDir::eIteratorDirForward) throw(std::runtime_error);

    //
    // Vectored sync API
    //
    Status GetSet(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error);
    Status PutSet(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_dup = true, size_t *fail_pos = 0) throw(std::runtime_error);
    Status DeleteSet(std::vector<Duple> &keys, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error);
    Status MergeSet(std::vector<std::pair<Duple, Duple>> &items) throw(std::runtime_error);

    //
    // Transaction sync API
    //
    Transaction *StartTransaction() throw(std::runtime_error);
    Status CommitTransaction(const Transaction *tr) const throw(std::runtime_error);
    Status RollbackTransaction(const Transaction *tr) const throw(std::runtime_error);

    //
    // Snapshot engine sync API
    //
    SnapshotEngine *CreateSnapshotEngine(const char *name) throw(std::runtime_error);
    Status DestroySnapshotEngine(const SnapshotEngine *en) const throw(std::runtime_error);
    Status ListSnapshots(std::vector<std::pair<std::string, uint32_t> > &list) throw(std::runtime_error);

    //
    // Backup engine sync API
    //
    BackupEngine *CreateBackupEngine(const char *eng_name, const char *bckp_path = 0, bool incremental = true) throw(std::runtime_error);
    Status DestroyBackupEngine(const BackupEngine *eng_name) const throw(std::runtime_error);
    Status ListBackups(std::vector<std::pair<std::string, uint32_t> > &list) throw(std::runtime_error);

    //
    // Group sync API
    //
    Group *CreateGroup() throw(std::runtime_error);
    Status DestroyGroup(Group *gr) const throw(std::runtime_error);
    Status Write(const Group *gr) const throw(std::runtime_error);

    //
    // Iterator sync API
    //
    Iterator *CreateIterator(Iterator::eIteratorDir dir = Iterator::eIteratorDir::eIteratorDirForward,
        const char *start_key = 0,
        size_t start_key_size = 0,
        const char *end_key = 0,
        size_t end_key_size = 0) throw(std::runtime_error);
    Status DestroyIterator(Iterator *it) const throw(std::runtime_error);

    //
    // Subset sync API
    //
    Subsetable *CreateSubset(const char *name,
        size_t name_len,
        Session::eSubsetType sb_type = Session::eSubsetType::eSubsetTypeDisjoint) throw(std::runtime_error);
    Subsetable *OpenSubset(const char *name,
        size_t name_len,
        Session::eSubsetType sb_type = Session::eSubsetType::eSubsetTypeDisjoint) throw(std::runtime_error);
    Status DestroySubset(Subsetable *sb) const throw(std::runtime_error);
    Status DeleteSubset(Subsetable *sb) const throw(std::runtime_error);

    void Close();
    Status Repair() throw(std::runtime_error);

    //
    // Async message API
    //
    PutFuture *PutAsync(const char *key, size_t key_size, const char *data, size_t data_size, uint64_t *pos = 0, Futurable *_par = 0) throw(std::runtime_error);
    GetFuture *GetAsync(const char *key, size_t key_size, char *buf, size_t *buf_size, uint64_t *pos = 0, Futurable *_par = 0) throw(std::runtime_error);
    DeleteFuture *DeleteAsync(const char *key, size_t key_size, uint64_t *pos = 0, Futurable *_par = 0) throw(std::runtime_error);
    MergeFuture *MergeAsync(const char *key, size_t key_size, const char *data, size_t data_size, uint64_t *pos = 0, Futurable *_par = 0) throw(std::runtime_error);

    //
    // Async vectored API
    //
    GetSetFuture *GetSetAsync(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error);
    PutSetFuture *PutSetAsync(std::vector<std::pair<Duple, Duple>> &items, bool fail_on_first_dup = true, size_t *fail_pos = 0) throw(std::runtime_error);
    DeleteSetFuture *DeleteSetAsync(std::vector<Duple> &keys, bool fail_on_first_nfound = true, size_t *fail_pos = 0) throw(std::runtime_error);
    MergeSetFuture *MergeSetAsync(std::vector<std::pair<Duple, Duple>> &items) throw(std::runtime_error);

    //
    // Future managed destroy
    //
    void DestroyFuture(Futurable *fut);

protected:
    size_t DestroySetFuture(Futurable *set_f, bool no_lock = false);
    void DecPendingNumFutures();
    void IncPendingNumFutures();

protected:
    Status OpenSession() throw(std::runtime_error);

    void CreateStore(bool create_con = true) throw(std::runtime_error);
    void CreateCluster() throw(std::runtime_error);
    void CreateSpace() throw(std::runtime_error);
    void InitDesc(LocalArgsDescriptor &args) const;
    void InitAdmDesc(LocalAdmArgsDescriptor &args) const;

    Status CreateSnapshot(const SnapshotEngine *en) const throw(std::runtime_error);
    Status DestroySnapshot(const SnapshotEngine *en) const throw(std::runtime_error);
    Status CreateBackup(const BackupEngine *en) const throw(std::runtime_error);
    Status RestoreBackup(const BackupEngine *en,
        const std::string &cl_n,
        const std::string &sp_n,
        const std::string &st_n,
        bool _overwrite = true) const throw(std::runtime_error);
    Status DestroyBackup(const BackupEngine *en) const throw(std::runtime_error);
    Status VerifyBackup(const BackupEngine *en,
        const std::string &cl_n,
        const std::string &sp_n,
        const std::string &st_n) const throw(std::runtime_error);

    Status Put(LocalArgsDescriptor &args) throw(std::runtime_error);
    Status Get(LocalArgsDescriptor &args) throw(std::runtime_error);
    Status Delete(LocalArgsDescriptor &args) throw(std::runtime_error);
    Status Merge(LocalArgsDescriptor &args) throw(std::runtime_error);
    Status GetRange(LocalArgsDescriptor &args) throw(std::runtime_error);

    Status GetSet(LocalArgsDescriptor &args, std::vector<std::pair<Duple, Duple>> &items) throw(std::runtime_error);
    Status PutSet(LocalArgsDescriptor &args, std::vector<std::pair<Duple, Duple>> &items) throw(std::runtime_error);
    Status DeleteSet(LocalArgsDescriptor &args, std::vector<Duple> &keys) throw(std::runtime_error);
    Status MergeSet(LocalArgsDescriptor &args, std::vector<std::pair<Duple, Duple>> &items) throw(std::runtime_error);

    Status GetSetIteratively(std::vector<std::pair<Duple, Duple>> &items) throw(std::runtime_error);
    Status PutSetIteratively(std::vector<std::pair<Duple, Duple>> &items) throw(std::runtime_error);
    Status DeleteSetIteratively(std::vector<Duple> &keys) throw(std::runtime_error);
    Status MergeSetIteratively(std::vector<std::pair<Duple, Duple>> &items) throw(std::runtime_error);

    Status Iterate(LocalArgsDescriptor &args) throw(std::runtime_error);

    Iterator *CreateSubsetIterator(Iterator::eIteratorDir dir,
        Subsetable *subs,
        const char *start_key,
        size_t start_key_size,
        const char *end_key,
        size_t end_key_size) throw(std::runtime_error);

    void logDbgMsg(const char *msg) const;
    void logErrMsg(const char *msg) const;

    Session::Status TranslateExceptionStatus(const RcsdbFacade::Exception &ex);
    Session::Status TranslateConstExceptionStatus(const RcsdbFacade::Exception &ex) const;
};

}

#endif /* SOE_SOE_SOE_SOE_INCLUDE_SOESESSION_HPP_ */
