/**
 * @file    metadbsrvaccessors1.hpp
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
 * metadbsrvaccessors1.hpp
 *
 *  Created on: Nov 24, 2017
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_METADBSRV_SRC_METADBSRVACCESSORS1_HPP_
#define RENTCONTROL_SOE_SOE_SOE_METADBSRV_SRC_METADBSRVACCESSORS1_HPP_

namespace Metadbsrv {

class Store;

template <class T, class Z>
class RcsdbAccessors1 {
    T *obj;
    static Z default_obj;

public:
    static uint64_t create_fun(LocalArgsDescriptor &_desc) { return T::CreateMeta(_desc); }
    static void destroy_fun(LocalArgsDescriptor &_desc) { T::DestroyMeta(_desc); }
    static uint64_t get_by_name_fun(LocalArgsDescriptor &_desc) { return T::GetMetaByName(_desc); }
    static uint64_t repair_fun(LocalArgsDescriptor &_desc) { return T::RepairMeta(_desc); }

    void open(LocalArgsDescriptor &_desc) { obj->Open(_desc); }
    void close(LocalArgsDescriptor &_desc) { obj->Close(_desc); }
    void repair(LocalArgsDescriptor &_desc) { obj->Repair(_desc); }
    void put(LocalArgsDescriptor &_desc) { obj->Put(_desc); }
    void get(LocalArgsDescriptor &_desc) { obj->Get(_desc); }
    void del(LocalArgsDescriptor &_desc) { obj->Delete(_desc); }
    void merge(LocalArgsDescriptor &_desc) { obj->Merge(_desc); }
    void iterate(LocalArgsDescriptor &_desc) { obj->Iterate(_desc); }
    void get_range(LocalArgsDescriptor &_desc) { obj->GetRange(_desc); }
    void put_range(LocalArgsDescriptor &_desc) { obj->PutRange(_desc); }
    void delete_range(LocalArgsDescriptor &_desc) { obj->DeleteRange(_desc); }
    void merge_range(LocalArgsDescriptor &_desc) { obj->MergeRange(_desc); }

    void start_transaction(LocalAdmArgsDescriptor &_desc) { obj->StartTransaction(_desc); }
    void commit_transaction(LocalAdmArgsDescriptor &_desc) { obj->CommitTransaction(_desc); }
    void rollback_transaction(LocalAdmArgsDescriptor &_desc) { obj->RollbackTransaction(_desc); }

    void create_snapshot_engine(LocalAdmArgsDescriptor &_desc) { obj->CreateSnapshotEngine(_desc); }
    void create_snapshot(LocalAdmArgsDescriptor &_desc) { obj->CreateSnapshot(_desc); }
    void destroy_snapshot(LocalAdmArgsDescriptor &_desc) { obj->DestroySnapshot(_desc); }
    void list_snapshots(LocalAdmArgsDescriptor &_desc) { obj->ListSnapshots(_desc); }

    void create_backup_engine(LocalAdmArgsDescriptor &_desc) { obj->CreateBackupEngine(_desc); }
    void create_backup(LocalAdmArgsDescriptor &_desc) { obj->CreateBackup(_desc); }
    void restore_backup(LocalAdmArgsDescriptor &_desc) { obj->RestoreBackup(_desc); }
    void destroy_backup(LocalAdmArgsDescriptor &_desc) { obj->DestroyBackup(_desc); }
    void verify_backup(LocalAdmArgsDescriptor &_desc) { obj->VerifyBackup(_desc); }
    void destroy_backup_engine(LocalAdmArgsDescriptor &_desc) { obj->DestroyBackupEngine(_desc); }
    void list_backups(LocalAdmArgsDescriptor &_desc) { obj->ListBackups(_desc); }

    void create_group(LocalArgsDescriptor &_desc) { obj->CreateGroup(_desc); }
    void destroy_group(LocalArgsDescriptor &_desc) { obj->DestroyGroup(_desc); }
    void write_group(LocalArgsDescriptor &_desc) { obj->WriteGroup(_desc); }

    void create_iterator(LocalArgsDescriptor &_desc) { obj->CreateIterator(_desc); }
    void destroy_iterator(LocalArgsDescriptor &_desc) { obj->DestroyIterator(_desc); }

    int get_state() { return obj->GetState(); }
    uint32_t get_ref_count() { return obj->GetRefCount(); }

public:
    RcsdbAccessors1():
        obj(&default_obj)
    {}
    virtual ~RcsdbAccessors1() {}
    void SetObj(T *_obj) { obj = _obj; }
    void ResetObj() { obj = &default_obj; }
};

template <class T, class Z> Z RcsdbAccessors1<T, Z>::default_obj;

}

#endif /* RENTCONTROL_SOE_SOE_SOE_METADBSRV_SRC_METADBSRVACCESSORS1_HPP_ */
