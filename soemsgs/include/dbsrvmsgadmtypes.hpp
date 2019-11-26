/**
 * @file    dbsrvmsgadmtypes.hpp
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
 * dbsrvmsgadmtypes.hpp
 *
 *  Created on: Feb 7, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGADMTYPES_HPP_
#define SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGADMTYPES_HPP_

namespace MetaMsgs {

class AdmMsgPoolInitializer
{
public:
    static bool initialized;
    AdmMsgPoolInitializer();
    ~AdmMsgPoolInitializer() {}
};


// -----------------------------------------------------------------------------------------

class AdmMsgAssignMMChannelReq: public AdmMsgReq
{
public:
    uint32_t   pid;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgAssignMMChannelReq, MemMgmt::ClassAlloc<AdmMsgAssignMMChannelReq> > poolT;

public:
    AdmMsgAssignMMChannelReq();
    virtual ~AdmMsgAssignMMChannelReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgAssignMMChannelReq *get();
    static void put(AdmMsgAssignMMChannelReq *obj);
    static void registerPool();
};


class AdmMsgAssignMMChannelRsp: public AdmMsgRsp
{
public:
    std::string   path;
    uint32_t      pid;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgAssignMMChannelRsp, MemMgmt::ClassAlloc<AdmMsgAssignMMChannelRsp> > poolT;

public:
    AdmMsgAssignMMChannelRsp();
    virtual ~AdmMsgAssignMMChannelRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgAssignMMChannelRsp *get();
    static void put(AdmMsgAssignMMChannelRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgCreateStoreReq: public AdmMsgReq
{
public:
    typedef struct _Fields {
        uint32_t  transactional : 1;
        uint32_t  overwrite_dups : 1;
        uint32_t  sync_mode : 6;
        uint32_t  bloom_bits : 8;
        uint32_t  unused : 16;
    } Fields;

    typedef union _BitFields {
        uint32_t  fields;
        Fields    bit_fields;
    } BitFields;

    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint32_t      cr_thread_id;
    uint32_t      only_get_by_name;
    BitFields     flags;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCreateStoreReq, MemMgmt::ClassAlloc<AdmMsgCreateStoreReq> > poolT;

public:
    AdmMsgCreateStoreReq();
    virtual ~AdmMsgCreateStoreReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCreateStoreReq *get();
    static void put(AdmMsgCreateStoreReq *obj);
    static void registerPool();
};


class AdmMsgCreateStoreRsp: public AdmMsgRsp
{
public:
    uint64_t   cluster_id;
    uint64_t   space_id;
    uint64_t   store_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCreateStoreRsp, MemMgmt::ClassAlloc<AdmMsgCreateStoreRsp> > poolT;

public:
    AdmMsgCreateStoreRsp();
    virtual ~AdmMsgCreateStoreRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCreateStoreRsp *get();
    static void put(AdmMsgCreateStoreRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgRepairStoreReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;
    uint32_t      cr_thread_id;
    uint32_t      only_get_by_name;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgRepairStoreReq, MemMgmt::ClassAlloc<AdmMsgRepairStoreReq> > poolT;

public:
    AdmMsgRepairStoreReq();
    virtual ~AdmMsgRepairStoreReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgRepairStoreReq *get();
    static void put(AdmMsgRepairStoreReq *obj);
    static void registerPool();
};


class AdmMsgRepairStoreRsp: public AdmMsgRsp
{
public:
    uint64_t   cluster_id;
    uint64_t   space_id;
    uint64_t   store_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgRepairStoreRsp, MemMgmt::ClassAlloc<AdmMsgRepairStoreRsp> > poolT;

public:
    AdmMsgRepairStoreRsp();
    virtual ~AdmMsgRepairStoreRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgRepairStoreRsp *get();
    static void put(AdmMsgRepairStoreRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgCreateSpaceReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    uint64_t      cluster_id;
    uint32_t      only_get_by_name;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCreateSpaceReq, MemMgmt::ClassAlloc<AdmMsgCreateSpaceReq> > poolT;

public:
    AdmMsgCreateSpaceReq();
    virtual ~AdmMsgCreateSpaceReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCreateSpaceReq *get();
    static void put(AdmMsgCreateSpaceReq *obj);
    static void registerPool();
};


class AdmMsgCreateSpaceRsp: public AdmMsgRsp
{
public:
    uint64_t      cluster_id;
    uint64_t      space_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCreateSpaceRsp, MemMgmt::ClassAlloc<AdmMsgCreateSpaceRsp> > poolT;

public:
    AdmMsgCreateSpaceRsp();
    virtual ~AdmMsgCreateSpaceRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCreateSpaceRsp *get();
    static void put(AdmMsgCreateSpaceRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgCreateClusterReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    uint32_t      only_get_by_name;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCreateClusterReq, MemMgmt::ClassAlloc<AdmMsgCreateClusterReq> > poolT;

public:
    AdmMsgCreateClusterReq();
    virtual ~AdmMsgCreateClusterReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCreateClusterReq *get();
    static void put(AdmMsgCreateClusterReq *obj);
    static void registerPool();
};


class AdmMsgCreateClusterRsp: public AdmMsgRsp
{
public:
    uint64_t   cluster_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCreateClusterRsp, MemMgmt::ClassAlloc<AdmMsgCreateClusterRsp> > poolT;

public:
    AdmMsgCreateClusterRsp();
    virtual ~AdmMsgCreateClusterRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCreateClusterRsp *get();
    static void put(AdmMsgCreateClusterRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgDeleteStoreReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;
    uint32_t      cr_thread_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgDeleteStoreReq, MemMgmt::ClassAlloc<AdmMsgDeleteStoreReq> > poolT;

public:
    AdmMsgDeleteStoreReq();
    ~AdmMsgDeleteStoreReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgDeleteStoreReq *get();
    static void put(AdmMsgDeleteStoreReq *obj);
    static void registerPool();
};


class AdmMsgDeleteStoreRsp: public AdmMsgRsp
{
public:
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgDeleteStoreRsp, MemMgmt::ClassAlloc<AdmMsgDeleteStoreRsp> > poolT;

public:
    AdmMsgDeleteStoreRsp();
    virtual ~AdmMsgDeleteStoreRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgDeleteStoreRsp *get();
    static void put(AdmMsgDeleteStoreRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgDeleteSpaceReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    uint64_t      cluster_id;
    uint64_t      space_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgDeleteSpaceReq, MemMgmt::ClassAlloc<AdmMsgDeleteSpaceReq> > poolT;

public:
    AdmMsgDeleteSpaceReq();
    virtual ~AdmMsgDeleteSpaceReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgDeleteSpaceReq *get();
    static void put(AdmMsgDeleteSpaceReq *obj);
    static void registerPool();
};


class AdmMsgDeleteSpaceRsp: public AdmMsgRsp
{
public:
    uint64_t   cluster_id;
    uint64_t   space_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgDeleteSpaceRsp, MemMgmt::ClassAlloc<AdmMsgDeleteSpaceRsp> > poolT;

public:
    AdmMsgDeleteSpaceRsp();
    virtual ~AdmMsgDeleteSpaceRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgDeleteSpaceRsp *get();
    static void put(AdmMsgDeleteSpaceRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgDeleteClusterReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    uint64_t      cluster_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgDeleteClusterReq, MemMgmt::ClassAlloc<AdmMsgDeleteClusterReq> > poolT;

public:
    AdmMsgDeleteClusterReq();
    virtual ~AdmMsgDeleteClusterReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgDeleteClusterReq *get();
    static void put(AdmMsgDeleteClusterReq *obj);
    static void registerPool();
};


class AdmMsgDeleteClusterRsp: public AdmMsgRsp
{
public:
    uint64_t      cluster_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgDeleteClusterRsp, MemMgmt::ClassAlloc<AdmMsgDeleteClusterRsp> > poolT;

public:
    AdmMsgDeleteClusterRsp();
    virtual ~AdmMsgDeleteClusterRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgDeleteClusterRsp *get();
    static void put(AdmMsgDeleteClusterRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgCreateBackupReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;
    uint32_t      sess_thread_id;
    std::string   backup_engine_name;
    uint32_t      backup_engine_idx;
    uint32_t      backup_engine_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCreateBackupReq, MemMgmt::ClassAlloc<AdmMsgCreateBackupReq> > poolT;

public:
    AdmMsgCreateBackupReq();
    virtual ~AdmMsgCreateBackupReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCreateBackupReq *get();
    static void put(AdmMsgCreateBackupReq *obj);
    static void registerPool();
};


class AdmMsgCreateBackupRsp: public AdmMsgRsp
{
public:
    uint32_t      backup_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCreateBackupRsp, MemMgmt::ClassAlloc<AdmMsgCreateBackupRsp> > poolT;

public:
    AdmMsgCreateBackupRsp();
    virtual ~AdmMsgCreateBackupRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCreateBackupRsp *get();
    static void put(AdmMsgCreateBackupRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgDeleteBackupReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    std::string   backup_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;
    uint32_t      sess_thread_id;
    uint64_t      backup_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgDeleteBackupReq, MemMgmt::ClassAlloc<AdmMsgDeleteBackupReq> > poolT;

public:
    AdmMsgDeleteBackupReq();
    virtual ~AdmMsgDeleteBackupReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgDeleteBackupReq *get();
    static void put(AdmMsgDeleteBackupReq *obj);
    static void registerPool();
};


class AdmMsgDeleteBackupRsp: public AdmMsgRsp
{
public:
    uint64_t      backup_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgDeleteBackupRsp, MemMgmt::ClassAlloc<AdmMsgDeleteBackupRsp> > poolT;

public:
    AdmMsgDeleteBackupRsp();
    virtual ~AdmMsgDeleteBackupRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgDeleteBackupRsp *get();
    static void put(AdmMsgDeleteBackupRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgVerifyBackupReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    std::string   backup_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;
    uint32_t      sess_thread_id;
    uint64_t      backup_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgVerifyBackupReq, MemMgmt::ClassAlloc<AdmMsgVerifyBackupReq> > poolT;

public:
    AdmMsgVerifyBackupReq();
    virtual ~AdmMsgVerifyBackupReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgVerifyBackupReq *get();
    static void put(AdmMsgVerifyBackupReq *obj);
    static void registerPool();
};


class AdmMsgVerifyBackupRsp: public AdmMsgRsp
{
public:
    uint64_t      backup_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgVerifyBackupRsp, MemMgmt::ClassAlloc<AdmMsgVerifyBackupRsp> > poolT;

public:
    AdmMsgVerifyBackupRsp();
    virtual ~AdmMsgVerifyBackupRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgVerifyBackupRsp *get();
    static void put(AdmMsgVerifyBackupRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgListBackupsReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgListBackupsReq, MemMgmt::ClassAlloc<AdmMsgListBackupsReq> > poolT;

public:
    AdmMsgListBackupsReq();
    virtual ~AdmMsgListBackupsReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgListBackupsReq *get();
    static void put(AdmMsgListBackupsReq *obj);
    static void registerPool();
};


class AdmMsgListBackupsRsp: public AdmMsgRsp
{
public:
    uint32_t        num_backups;
    std::vector<std::pair<uint64_t, std::string> >   backups;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgListBackupsRsp, MemMgmt::ClassAlloc<AdmMsgListBackupsRsp> > poolT;

public:
    AdmMsgListBackupsRsp();
    virtual ~AdmMsgListBackupsRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgListBackupsRsp *get();
    static void put(AdmMsgListBackupsRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgCreateSnapshotReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;
    uint32_t      sess_thread_id;
    std::string   snapshot_engine_name;
    uint32_t      snapshot_engine_idx;
    uint32_t      snapshot_engine_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCreateSnapshotReq, MemMgmt::ClassAlloc<AdmMsgCreateSnapshotReq> > poolT;

public:
    AdmMsgCreateSnapshotReq();
    virtual ~AdmMsgCreateSnapshotReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCreateSnapshotReq *get();
    static void put(AdmMsgCreateSnapshotReq *obj);
    static void registerPool();
};


class AdmMsgCreateSnapshotRsp: public AdmMsgRsp
{
public:
    uint32_t      snapshot_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCreateSnapshotRsp, MemMgmt::ClassAlloc<AdmMsgCreateSnapshotRsp> > poolT;

public:
    AdmMsgCreateSnapshotRsp();
    ~AdmMsgCreateSnapshotRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCreateSnapshotRsp *get();
    static void put(AdmMsgCreateSnapshotRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgDeleteSnapshotReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    std::string   snapshot_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;
    uint32_t      sess_thread_id;
    uint64_t      snapshot_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgDeleteSnapshotReq, MemMgmt::ClassAlloc<AdmMsgDeleteSnapshotReq> > poolT;

public:
    AdmMsgDeleteSnapshotReq();
    virtual ~AdmMsgDeleteSnapshotReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgDeleteSnapshotReq *get();
    static void put(AdmMsgDeleteSnapshotReq *obj);
    static void registerPool();
};


class AdmMsgDeleteSnapshotRsp: public AdmMsgRsp
{
public:
    uint64_t      snapshot_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgDeleteSnapshotRsp, MemMgmt::ClassAlloc<AdmMsgDeleteSnapshotRsp> > poolT;

public:
    AdmMsgDeleteSnapshotRsp();
    virtual ~AdmMsgDeleteSnapshotRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgDeleteSnapshotRsp *get();
    static void put(AdmMsgDeleteSnapshotRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgListSnapshotsReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgListSnapshotsReq, MemMgmt::ClassAlloc<AdmMsgListSnapshotsReq> > poolT;

public:
    AdmMsgListSnapshotsReq();
    virtual ~AdmMsgListSnapshotsReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgListSnapshotsReq *get();
    static void put(AdmMsgListSnapshotsReq *obj);
    static void registerPool();
};


class AdmMsgListSnapshotsRsp: public AdmMsgRsp
{
public:
    uint32_t      num_snapshots;
    std::vector<std::pair<uint64_t, std::string> >   snapshots;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgListSnapshotsRsp, MemMgmt::ClassAlloc<AdmMsgListSnapshotsRsp> > poolT;

public:
    AdmMsgListSnapshotsRsp();
    virtual ~AdmMsgListSnapshotsRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgListSnapshotsRsp *get();
    static void put(AdmMsgListSnapshotsRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgOpenStoreReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgOpenStoreReq, MemMgmt::ClassAlloc<AdmMsgOpenStoreReq> > poolT;

public:
    AdmMsgOpenStoreReq();
    virtual ~AdmMsgOpenStoreReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgOpenStoreReq *get();
    static void put(AdmMsgOpenStoreReq *obj);
    static void registerPool();
};


class AdmMsgOpenStoreRsp: public AdmMsgRsp
{
public:
    uint64_t      store_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgOpenStoreRsp, MemMgmt::ClassAlloc<AdmMsgOpenStoreRsp> > poolT;

public:
    AdmMsgOpenStoreRsp();
    virtual ~AdmMsgOpenStoreRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgOpenStoreRsp *get();
    static void put(AdmMsgOpenStoreRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgCloseStoreReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCloseStoreReq, MemMgmt::ClassAlloc<AdmMsgCloseStoreReq> > poolT;

public:
    AdmMsgCloseStoreReq();
    virtual ~AdmMsgCloseStoreReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCloseStoreReq *get();
    static void put(AdmMsgCloseStoreReq *obj);
    static void registerPool();
};


class AdmMsgCloseStoreRsp: public AdmMsgRsp
{
public:
    uint64_t      store_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCloseStoreRsp, MemMgmt::ClassAlloc<AdmMsgCloseStoreRsp> > poolT;

public:
    AdmMsgCloseStoreRsp();
    virtual ~AdmMsgCloseStoreRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCloseStoreRsp *get();
    static void put(AdmMsgCloseStoreRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgListClustersReq: public AdmMsgReq
{
public:
    std::string   backup_path;
    uint32_t      backup_req;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgListClustersReq, MemMgmt::ClassAlloc<AdmMsgListClustersReq> > poolT;

public:
    AdmMsgListClustersReq();
    virtual ~AdmMsgListClustersReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgListClustersReq *get();
    static void put(AdmMsgListClustersReq *obj);
    static void registerPool();
};


class AdmMsgListClustersRsp: public AdmMsgRsp
{
public:
    uint32_t      num_clusters;
    std::vector<std::pair<uint64_t, std::string> >   clusters;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgListClustersRsp, MemMgmt::ClassAlloc<AdmMsgListClustersRsp> > poolT;

public:
    AdmMsgListClustersRsp();
    virtual ~AdmMsgListClustersRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgListClustersRsp *get();
    static void put(AdmMsgListClustersRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgListSpacesReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    uint64_t      cluster_id;
    std::string   backup_path;
    uint32_t      backup_req;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgListSpacesReq, MemMgmt::ClassAlloc<AdmMsgListSpacesReq> > poolT;

public:
    AdmMsgListSpacesReq();
    virtual ~AdmMsgListSpacesReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgListSpacesReq *get();
    static void put(AdmMsgListSpacesReq *obj);
    static void registerPool();
};


class AdmMsgListSpacesRsp: public AdmMsgRsp
{
public:
    uint32_t      num_spaces;
    std::vector<std::pair<uint64_t, std::string> >   spaces;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgListSpacesRsp, MemMgmt::ClassAlloc<AdmMsgListSpacesRsp> > poolT;

public:
    AdmMsgListSpacesRsp();
    virtual ~AdmMsgListSpacesRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgListSpacesRsp *get();
    static void put(AdmMsgListSpacesRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgListStoresReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    std::string   backup_path;
    uint32_t      backup_req;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgListStoresReq, MemMgmt::ClassAlloc<AdmMsgListStoresReq> > poolT;

public:
    AdmMsgListStoresReq();
    virtual ~AdmMsgListStoresReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgListStoresReq *get();
    static void put(AdmMsgListStoresReq *obj);
    static void registerPool();
};


class AdmMsgListStoresRsp: public AdmMsgRsp
{
public:
    uint32_t      num_stores;
    std::vector<std::pair<uint64_t, std::string> >   stores;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgListStoresRsp, MemMgmt::ClassAlloc<AdmMsgListStoresRsp> > poolT;

public:
    AdmMsgListStoresRsp();
    virtual ~AdmMsgListStoresRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgListStoresRsp *get();
    static void put(AdmMsgListStoresRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgListSubsetsReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgListSubsetsReq, MemMgmt::ClassAlloc<AdmMsgListSubsetsReq> > poolT;

public:
    AdmMsgListSubsetsReq();
    virtual ~AdmMsgListSubsetsReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgListSubsetsReq *get();
    static void put(AdmMsgListSubsetsReq *obj);
    static void registerPool();
};


class AdmMsgListSubsetsRsp: public AdmMsgRsp
{
public:
    uint32_t      num_subsets;
    std::vector<std::pair<uint64_t, std::string> >   subsets;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgListSubsetsRsp, MemMgmt::ClassAlloc<AdmMsgListSubsetsRsp> > poolT;

public:
    AdmMsgListSubsetsRsp();
    virtual ~AdmMsgListSubsetsRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgListSubsetsRsp *get();
    static void put(AdmMsgListSubsetsRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgCreateBackupEngineReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;
    std::string   backup_engine_name;
    std::string   backup_cluster_name;
    std::string   backup_space_name;
    std::string   backup_store_name;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCreateBackupEngineReq, MemMgmt::ClassAlloc<AdmMsgCreateBackupEngineReq> > poolT;

public:
    AdmMsgCreateBackupEngineReq();
    virtual ~AdmMsgCreateBackupEngineReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCreateBackupEngineReq *get();
    static void put(AdmMsgCreateBackupEngineReq *obj);
    static void registerPool();
};


class AdmMsgCreateBackupEngineRsp: public AdmMsgRsp
{
public:
    uint32_t      backup_engine_id;
    uint32_t      backup_engine_idx;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCreateBackupEngineRsp, MemMgmt::ClassAlloc<AdmMsgCreateBackupEngineRsp> > poolT;

public:
    AdmMsgCreateBackupEngineRsp();
    virtual ~AdmMsgCreateBackupEngineRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCreateBackupEngineRsp *get();
    static void put(AdmMsgCreateBackupEngineRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgDeleteBackupEngineReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    std::string   backup_engine_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;
    uint32_t      backup_engine_id;
    uint32_t      backup_engine_idx;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgDeleteBackupEngineReq, MemMgmt::ClassAlloc<AdmMsgDeleteBackupEngineReq> > poolT;

public:
    AdmMsgDeleteBackupEngineReq();
    virtual ~AdmMsgDeleteBackupEngineReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgDeleteBackupEngineReq *get();
    static void put(AdmMsgDeleteBackupEngineReq *obj);
    static void registerPool();
};


class AdmMsgDeleteBackupEngineRsp: public AdmMsgRsp
{
public:
    uint32_t      backup_engine_id;
    uint32_t      backup_engine_idx;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgDeleteBackupEngineRsp, MemMgmt::ClassAlloc<AdmMsgDeleteBackupEngineRsp> > poolT;

public:
    AdmMsgDeleteBackupEngineRsp();
    virtual ~AdmMsgDeleteBackupEngineRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgDeleteBackupEngineRsp *get();
    static void put(AdmMsgDeleteBackupEngineRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgCreateSnapshotEngineReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;
    std::string   snapshot_engine_name;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCreateSnapshotEngineReq, MemMgmt::ClassAlloc<AdmMsgCreateSnapshotEngineReq> > poolT;

public:
    AdmMsgCreateSnapshotEngineReq();
    virtual ~AdmMsgCreateSnapshotEngineReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCreateSnapshotEngineReq *get();
    static void put(AdmMsgCreateSnapshotEngineReq *obj);
    static void registerPool();
};


class AdmMsgCreateSnapshotEngineRsp: public AdmMsgRsp
{
public:
    uint32_t      snapshot_engine_id;
    uint32_t      snapshot_engine_idx;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgCreateSnapshotEngineRsp, MemMgmt::ClassAlloc<AdmMsgCreateSnapshotEngineRsp> > poolT;

public:
    AdmMsgCreateSnapshotEngineRsp();
    virtual ~AdmMsgCreateSnapshotEngineRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgCreateSnapshotEngineRsp *get();
    static void put(AdmMsgCreateSnapshotEngineRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgDeleteSnapshotEngineReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    std::string   snapshot_engine_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;
    uint32_t      snapshot_engine_id;
    uint32_t      snapshot_engine_idx;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgDeleteSnapshotEngineReq, MemMgmt::ClassAlloc<AdmMsgDeleteSnapshotEngineReq> > poolT;

public:
    AdmMsgDeleteSnapshotEngineReq();
    virtual ~AdmMsgDeleteSnapshotEngineReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgDeleteSnapshotEngineReq *get();
    static void put(AdmMsgDeleteSnapshotEngineReq *obj);
    static void registerPool();
};


class AdmMsgDeleteSnapshotEngineRsp: public AdmMsgRsp
{
public:
    uint32_t      snapshot_engine_id;
    uint32_t      snapshot_engine_idx;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgDeleteSnapshotEngineRsp, MemMgmt::ClassAlloc<AdmMsgDeleteSnapshotEngineRsp> > poolT;

public:
    AdmMsgDeleteSnapshotEngineRsp();
    virtual ~AdmMsgDeleteSnapshotEngineRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgDeleteSnapshotEngineRsp *get();
    static void put(AdmMsgDeleteSnapshotEngineRsp *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class AdmMsgRestoreBackupReq: public AdmMsgReq
{
public:
    std::string   cluster_name;
    std::string   space_name;
    std::string   store_name;
    uint64_t      cluster_id;
    uint64_t      space_id;
    uint64_t      store_id;
    uint32_t      sess_thread_id;
    uint32_t      backup_id;
    uint32_t      backup_engine_idx;
    std::string   backup_name;
    std::string   restore_cluster_name;   // cluster where to restore backup, using the engine(backup_engine_idx, backup_name)
    std::string   restore_space_name;     // space where to restore backup, using the engine(backup_engine_idx, backup_name)
    std::string   restore_store_name;     // store where to restore backup, using the engine(backup_engine_idx, backup_name)

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgRestoreBackupReq, MemMgmt::ClassAlloc<AdmMsgRestoreBackupReq> > poolT;

public:
    AdmMsgRestoreBackupReq();
    virtual ~AdmMsgRestoreBackupReq();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgRestoreBackupReq *get();
    static void put(AdmMsgRestoreBackupReq *obj);
    static void registerPool();
};


class AdmMsgRestoreBackupRsp: public AdmMsgRsp
{
public:
    uint32_t      backup_id;

    static int    class_msg_type;
    static MemMgmt::GeneralPool<AdmMsgRestoreBackupRsp, MemMgmt::ClassAlloc<AdmMsgRestoreBackupRsp> > poolT;

public:
    AdmMsgRestoreBackupRsp();
    virtual ~AdmMsgRestoreBackupRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static AdmMsgRestoreBackupRsp *get();
    static void put(AdmMsgRestoreBackupRsp *obj);
    static void registerPool();
};

}

#endif /* SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGADMTYPES_HPP_ */
