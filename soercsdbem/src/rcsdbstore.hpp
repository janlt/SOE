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
 * rcsdbstore.hpp
 *
 *  Created on: Jan 27, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_RCSDBEM_SRC_RCSDBSTORE_HPP_
#define SOE_SOE_SOE_RCSDBEM_SRC_RCSDBSTORE_HPP_

namespace RcsdbLocalFacade {

//
// Store
//
class Store: public RcsdbName
{
    friend class Space;
    friend class Cluster;
    friend class StoreIterator;

public:
    typedef enum _eStoreState {
        eStoreInit        = 0,
        eStoreOpened      = 1,
        eStoreBroken      = 2,
        eStoreClosing     = 3,
        eStoreDestroying  = 4,
        eStoreDestroyed   = 5,
        eStoreClosed      = 6
    } eStoreState;

    typedef enum _eDBType {
        eSimpleDB         = 0,
        eTransactionDB    = 1
    } eDBType;

    typedef enum _ePrStatus {
        eOk                 = 0,
        eResourceExhausted  = 1,
        eStoreInternalError = 2,
        eAccessDenied       = 3,
        eResourceNotFound   = 4,
        eWrongDbType        = 5,
        eMax                = 6
    } ePrStatus;

    typedef enum _eSyncLevel {
        eSyncLevelDefault   = 0,
        eSyncLevelFsync     = 1,
        eSyncLevelAsync     = 2
    } eSyncLevel;

    static std::string              pr_status[ePrStatus::eMax];

private:
    std::shared_ptr<Cache>          cache;
    const FilterPolicy             *filter_policy;
    DB                             *rcsdb;
    uint32_t                        rcsdb_ref_count;
    uint32_t                        in_archive_ttl;
    Options                         open_options;
    TransactionDBOptions            transaction_options;
    ReadOptions                     read_options;
    WriteOptions                    write_options;
    eStoreState                     state;
    bool                            batch_mode;
    uint32_t                        bloom_bits;
    bool                            overwrite_duplicates;
    RcsdbFacade::StoreConfig        config;
    bool                            debug;
    Lock                            rcsdb_ref_count_lock;
    Lock                            open_close_lock;

    const static uint32_t           invalid_group = -1;
    const static uint32_t           max_batches = 32;
    std::shared_ptr<WriteBatch>     batches[Store::max_batches];
    Lock                            batches_lock;

    const static uint32_t           invalid_transaction = -1;
    const static uint32_t           max_transactions = 32;
    std::shared_ptr<Transaction>    transactions[Store::max_transactions];
    Lock                            transactions_lock;

    const static uint32_t           invalid_snapshot = -1;
    const static uint32_t           snapshot_start_id = 1000000;
    const static uint32_t           max_snapshots = 4096;
    const static char              *snapshot_prefix_dir;
    std::shared_ptr<RcsdbSnapshot>  snapshots[Store::max_snapshots];
    std::vector<std::string>        snapshot_dirs;
    Lock                            snapshots_lock;

    const static uint32_t           invalid_backup = -1;
    const static uint32_t           backup_start_id = 4000000;
    const static uint32_t           max_backups = 128;
    const static char              *backup_prefix_dir;
    std::shared_ptr<RcsdbBackup>    backups[Store::max_backups];
    std::vector<std::string>        backups_dirs;
    Lock                            backups_lock;


public:
    const static uint32_t           invalid_iterator = -1;
    const static uint32_t           iterator_start_id = 8000000;
    const static uint32_t           max_iterators = 128;

private:
    std::shared_ptr<StoreIterator>  iterators[Store::max_iterators];
    Lock                            iterators_lock;

public:
    std::string                     name;
    NameBase                       *parent;
    uint64_t                        id;
    std::string                     path;
    eDBType                         db_type;
    eSyncLevel                      sync_level;

    static uint64_t                 store_null_id;
    static uint32_t                 bloom_bits_default;
    static bool                     overdups_default;

    const static uint64_t           store_invalid_id = -1;
    const static uint64_t           store_start_id = 100000;

    //const static size_t             lru_cache_size = 1024 * 1024 * 1024LL;
    const static size_t             lru_cache_size = 150 * 1024 * 1024LL;
    const static int                lru_shard_bits = 8;

    const static size_t             sst_buffer_size = 128 * 1024 * 1024LL;

protected:
    void SetOpenOptions();
    void SetTransactionOptions();
    void SetReadOptions();
    void SetWriteOptions();

    Store(const std::string &nm,
        uint64_t _id,
        eDBType _db_type = eSimpleDB,
        uint32_t b_bits = Store::bloom_bits_default,
        bool over_dup = false,
        Store::eSyncLevel _sync_level = Store::eSyncLevel::eSyncLevelDefault,
        bool _debug = false);
    virtual void Create();
    virtual void SetPath();
    virtual const std::string &GetPath();
    virtual void ParseConfig(const std::string &cfg, bool no_exc = false);
    virtual std::string CreateConfig();
    virtual std::string ConfigAddSnapshot(const std::string &name, uint32_t id);
    virtual std::string ConfigRemoveSnapshot(const std::string &name, uint32_t id);
    virtual std::string ConfigAddBackup(const std::string &name, uint32_t id);
    virtual std::string ConfigRemoveBackup(const std::string &name, uint32_t id);
    virtual bool StopAndArchiveName(LocalArgsDescriptor &args);
    virtual void SetParent(NameBase *par) { parent = par; }

    void ArchiveName();

    void SetDbType(eDBType _type) { db_type = _type; }
    static std::string PrStatusToString(Store::ePrStatus sts);

    int ReadSnapshotsDir(DIR *dir);
    void ValidateSnapshots();
    int ReadBackupsDir(DIR *dir);
    void ValidateBackups();
    void EnumSnapshots();
    void EnumBackups();
    void SaveConfig(const std::string &all_lines, const std::string &config_file);

    uint32_t InterlockedRefCountIncrement();
    uint32_t InterlockedRefCountDecrement();
    uint32_t InterlockedRefCountGet();

public:
    virtual ~Store();

    static uint64_t CreateStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void DestroyStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t GetStoreByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

    static uint64_t CreateMeta(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static void DestroyMeta(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);
    static uint64_t GetMetaByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception);

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

    bool IsOpen();

    void DeregisterApi();

protected:
    void OpenPr() throw(RcsdbFacade::Exception);
    void ClosePr(bool delete_rcsdb = true) throw(RcsdbFacade::Exception);
    void RepairPr() throw(RcsdbFacade::Exception);
    void DestroyPr() throw(RcsdbFacade::Exception);
    void Crc32cPr() throw(RcsdbFacade::Exception);
    int RmTree(const char *tree_path);

    void PutPr(const Slice& key, const Slice& value) throw(RcsdbFacade::Exception);
    void GetPr(const Slice& key, std::string *&value, size_t &buf_len) throw(RcsdbFacade::Exception);
    void DeletePr(const Slice& key) throw(RcsdbFacade::Exception);
    void GetRangePr(const Slice& start_key, const Slice& end_key, RangeCallback range_callback, bool dir_forward, char *arg) throw(RcsdbFacade::Exception);
    void MergePr(const Slice& key, const Slice& value) throw(RcsdbFacade::Exception);

    void IteratePr(uint32_t iterator_idx, Slice &key, Slice &data, uint32_t cmd, bool &valid) throw(RcsdbFacade::Exception);

    ePrStatus CreateGroupPr(int &idx) throw(RcsdbFacade::Exception);
    void DestroyGroupPr(int group_idx) throw(RcsdbFacade::Exception);
    void WriteGroupPr(int group_idx) throw(RcsdbFacade::Exception);

    ePrStatus StartTransactionPr(int &idx) throw(RcsdbFacade::Exception);
    void CommitTransactionPr(int transaction_idx) throw(RcsdbFacade::Exception);
    void RollbackTransactionPr(int transaction_idx) throw(RcsdbFacade::Exception);
    ePrStatus EndTransactionPr(int idx) throw(RcsdbFacade::Exception);

    ePrStatus CreateSnapshotEnginePr(const std::string &name, uint32_t &idx, uint32_t &id) throw(RcsdbFacade::Exception);
    ePrStatus CreateSnapshotPr(const std::string &name, uint32_t snapshot_idx, uint32_t snapshot_id) throw(RcsdbFacade::Exception);
    ePrStatus DestroySnapshotPr(const std::string &name, uint32_t snapshot_idx, uint32_t snapshot_id) throw(RcsdbFacade::Exception);
    ePrStatus ListSnapshotsPr(std::vector<std::pair<std::string, uint32_t> > &list) throw(RcsdbFacade::Exception);

    ePrStatus CreateBackupEnginePr(const std::string &name, uint32_t &idx, uint32_t &id) throw(RcsdbFacade::Exception);
    ePrStatus CreateBackupPr(const std::string &name, uint32_t backup_idx, uint32_t backup_id) throw(RcsdbFacade::Exception);
    ePrStatus DestroyBackupEnginePr(const std::string &name, uint32_t backup_idx, uint32_t backup_id) throw(RcsdbFacade::Exception);
    ePrStatus ListBackupsPr(std::vector<std::pair<std::string, uint32_t> > &list) throw(RcsdbFacade::Exception);

    ePrStatus CreateIteratorPr(const Slice& start_key, const Slice& end_key, const Slice& key_pref,
        bool dir_forward, uint32_t &idx, StoreIterator::eType it_type) throw(RcsdbFacade::Exception);
    void DestroyIteratorPr(uint32_t iterator_idx) throw(RcsdbFacade::Exception);

    void GroupPutPr(const Slice& key, const Slice& value, int group_idx) throw(RcsdbFacade::Exception);
    void GroupMergePr(const Slice& key, const Slice& value, int group_idx) throw(RcsdbFacade::Exception);
    void GroupDeletePr(const Slice& key, int group_idx) throw(RcsdbFacade::Exception);

    void TransactionPutPr(const Slice& key, const Slice& value, int transaction_idx) throw(RcsdbFacade::Exception);
    void TransactionGetPr(const Slice& key, std::string *&value, int transaction_idx, size_t &buf_len) throw(RcsdbFacade::Exception);
    void TransactionMergePr(const Slice& key, const Slice& value, int transaction_idx) throw(RcsdbFacade::Exception);
    void TransactionDeletePr(const Slice& key, int transaction_idx) throw(RcsdbFacade::Exception);

    virtual void RegisterApiFunctions() throw(SoeApi::Exception);
    virtual void DeregisterApiFunctions() throw(SoeApi::Exception);
    static void RegisterApiStaticFunctions() throw(SoeApi::Exception);
    static void DeregisterApiStaticFunctions() throw(SoeApi::Exception);

    RcsdbFacade::Exception::eStatus TranslateRcsdbStatus(const Status &s);
    RcsdbFacade::Exception::eStatus TranslateStoreStatus(Store::ePrStatus s);
    virtual void SetNextAvailable();
    virtual void AsJson(std::string &json) throw(RcsdbFacade::Exception);

    void IterateForwardStorePr(StoreIterator *it, Slice &key, Slice &data, uint32_t cmd, bool &valid) throw(RcsdbFacade::Exception);
    void IterateBackwardStorePr(StoreIterator *it, Slice &key, Slice &data, uint32_t cmd, bool &valid) throw(RcsdbFacade::Exception);
    void IterateForwardSubsetPr(StoreIterator *it, Slice &key, Slice &data, uint32_t cmd, bool &valid) throw(RcsdbFacade::Exception);
    void IterateBackwardSubsetPr(StoreIterator *it, Slice &key, Slice &data, uint32_t cmd, bool &valid) throw(RcsdbFacade::Exception);
    void IterateSetEnded(StoreIterator *it, bool &_valid, Slice &_key, Slice &_data);
};

}

#endif /* SOE_SOE_SOE_RCSDBEM_SRC_RCSDBSTORE_HPP_ */
