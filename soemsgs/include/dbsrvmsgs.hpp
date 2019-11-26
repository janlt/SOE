/**
 * @file    dbsrvmsgs.hpp
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
 * dbsrvmsgs.hpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGS_HPP_
#define SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGS_HPP_


namespace MetaMsgs {

typedef enum _eMsgType
{
    eMsgInvalid = 0,            // 0
    eMsgAssignMMChannelReq,
    eMsgAssignMMChannelRsp,
    eMsgCreateClusterReq,
    eMsgCreateClusterRsp,
    eMsgCreateSpaceReq,
    eMsgCreateSpaceRsp,
    eMsgCreateStoreReq,
    eMsgCreateStoreRsp,
    eMsgDeleteClusterReq,
    eMsgDeleteClusterRsp,       // 10
    eMsgDeleteSpaceReq,
    eMsgDeleteSpaceRsp,
    eMsgDeleteStoreReq,
    eMsgDeleteStoreRsp,
    eMsgCreateBackupReq,
    eMsgCreateBackupRsp,
    eMsgDeleteBackupReq,
    eMsgDeleteBackupRsp,
    eMsgRestoreBackupReq,
    eMsgRestoreBackupRsp,       // 20
    eMsgVerifyBackupReq,
    eMsgVerifyBackupRsp,
    eMsgListBackupsReq,
    eMsgListBackupsRsp,
    eMsgCreateSnapshotReq,
    eMsgCreateSnapshotRsp,
    eMsgDeleteSnapshotReq,
    eMsgDeleteSnapshotRsp,
    eMsgListSnapshotsReq,
    eMsgListSnapshotsRsp,       // 30
    eMsgListClustersReq,
    eMsgListClustersRsp,
    eMsgListSpacesReq,
    eMsgListSpacesRsp,
    eMsgListStoresReq,
    eMsgListStoresRsp,
    eMsgListSubsetsReq,
    eMsgListSubsetsRsp,
    eMsgOpenStoreReq,
    eMsgOpenStoreRsp,          // 40
    eMsgCloseStoreReq,
    eMsgCloseStoreRsp,
    eMsgRepairStoreReq,
    eMsgRepairStoreRsp,
    eMsgCreateBackupEngineReq,
    eMsgCreateBackupEngineRsp,
    eMsgCreateSnapshotEngineReq,
    eMsgCreateSnapshotEngineRsp,
    eMsgDeleteBackupEngineReq,
    eMsgDeleteBackupEngineRsp,  // 50
    eMsgDeleteSnapshotEngineReq,
    eMsgDeleteSnapshotEngineRsp,
    eMsgGetReq,
    eMsgGetRsp,
    eMsgPutReq,
    eMsgPutRsp,
    eMsgDeleteReq,
    eMsgDeleteRsp,
    eMsgGetRangeReq,
    eMsgGetRangeRsp,            // 60
    eMsgMergeReq,
    eMsgMergeRsp,
    eMsgGetReqNT,
    eMsgGetRspNT,
    eMsgPutReqNT,
    eMsgPutRspNT,
    eMsgDeleteReqNT,
    eMsgDeleteRspNT,
    eMsgGetRangeReqNT,
    eMsgGetRangeRspNT,          // 70
    eMsgMergeReqNT,
    eMsgMergeRspNT,
    eMsgGetAsyncReq,
    eMsgGetAsyncRsp,
    eMsgPutAsyncReq,
    eMsgPutAsyncRsp,
    eMsgDeleteAsyncReq,
    eMsgDeleteAsyncRsp,
    eMsgGetRangeAsyncReq,
    eMsgGetRangeAsyncRsp,       // 80
    eMsgMergeAsyncReq,
    eMsgMergeAsyncRsp,
    eMsgGetAsyncReqNT,
    eMsgGetAsyncRspNT,
    eMsgPutAsyncReqNT,
    eMsgPutAsyncRspNT,
    eMsgDeleteAsyncReqNT,
    eMsgDeleteAsyncRspNT,
    eMsgGetRangeAsyncReqNT,
    eMsgGetRangeAsyncRspNT,     // 90
    eMsgMergeAsyncReqNT,
    eMsgMergeAsyncRspNT,
    eMsgBeginTransactionReq,
    eMsgBeginTransactionRsp,
    eMsgCommitTransactionReq,
    eMsgCommitTransactionRsp,
    eMsgAbortTransactionReq,
    eMsgAbortTransactionRsp,
    eMsgMetaAd,
    eMsgMetaSolicit,            // 100
    eMsgCreateIteratorReq,
    eMsgCreateIteratorRsp,
    eMsgDeleteIteratorReq,
    eMsgDeleteIteratorRsp,
    eMsgIterateReq,
    eMsgIterateRsp,
    eMsgCreateGroupReq,
    eMsgCreateGroupRsp,
    eMsgDeleteGroupReq,
    eMsgDeleteGroupRsp,         // 110
    eMsgWriteGroupReq,
    eMsgWriteGroupRsp,
    eMsgMax,
    eMsgInvalidRsp = 1000000
} eMsgType;

typedef enum _eMsgStatus
{
    eMsgOk          = 0,
    eMsgDisconnect  = -ECONNRESET,
    eMsgError       = -1000
} MsgStatus;

class MsgTypeInfo
{
#define STRINGIFY(s) #s
#define STRINGIFIED_ELEM(s) s, #s
    static std::vector<std::pair<eMsgType, std::string> > msgtype_info;

public:
    MsgTypeInfo();
    ~MsgTypeInfo();

    class Initializer
    {
    public:
        static bool initialized;
        Initializer();
        ~Initializer() {}
    };
    static Initializer msgtypeinfo_initalized;

    static const char *ToString(eMsgType type);
};


//
// Msgs
//
#define PACKED_ATTRIB(type) __attribute__ ((packed)) type

#ifdef __GNUC__
#define PACKED_ALIGNED_CODE(type) type
#else
#define PACKED_ALIGNED_CODE(type) type
#endif

const uint16_t MsgSomLength = sizeof(uint64_t);
const uint16_t MsgSomethingAnywayLength = sizeof(uint64_t);

typedef struct _MsgSom
{
    uint8_t          marker[MsgSomLength];  // Start-of-msg
} PACKED_ALIGNED_CODE(MsgSom);

typedef struct _MsgHdr
{
    MsgSom           som;             // Start-of-msg
    uint32_t         type;            // eMsgType
    uint32_t         total_length;    // total length
} PACKED_ALIGNED_CODE(MsgHdr);



//
// Admin messages
//

class AdmMsgBase: public AdmMsgMarshallable
{
public:
    MsgHdr           hdr;
    static uint8_t   SOM[MsgSomLength];

    typedef AdmMsgBase *(*GetMsgBaseFunction)();
    typedef void (*PutMsgBaseFunction)(AdmMsgBase *);

    class Initializer
    {
    public:
        static bool initialized;
        Initializer();
        ~Initializer() {}
    };
    static Initializer adm_initalized;

    AdmMsgBase(eMsgType _type = eMsgInvalid);
    virtual ~AdmMsgBase();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

protected:
    int recvExact(int fd, uint32_t bytes_want, AdmMsgBuffer &buf, bool _ignore_sig = true);
    int sendExact(int fd, uint32_t bytes_req, AdmMsgBuffer &buf, bool _ignore_sig = true);

    void AddHdrLength(uint32_t _add)
    {
        const_cast<MsgHdr *>(&hdr)->total_length += _add;
    }
};

class AdmMsg: public AdmMsgBase
{
public:
    std::string      json;

    AdmMsg(eMsgType _type = eMsgInvalid): AdmMsgBase(_type) {}
    virtual ~AdmMsg() {}

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    // DGRAM
    int recvmsg(int fd, sockaddr_in &addr);
    int sendmsg(int fd, const sockaddr_in &addr);
};

class AdmMsgReq: public AdmMsg
{
public:
    static MemMgmt::GeneralPool<AdmMsgReq, MemMgmt::ClassAlloc<AdmMsgReq> > poolT;

    AdmMsgReq(eMsgType _type = eMsgInvalid): AdmMsg(_type) {}
    virtual ~AdmMsgReq() {}

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    // DGRAM
    int recvmsg(int fd, sockaddr_in &addr);
    int sendmsg(int fd, const sockaddr_in &addr);
};

class AdmMsgRsp: public AdmMsg
{
public:
    static MemMgmt::GeneralPool<AdmMsgRsp, MemMgmt::ClassAlloc<AdmMsgRsp> > poolT;
    uint32_t         status;

    AdmMsgRsp(eMsgType _type = eMsgInvalid);
    virtual ~AdmMsgRsp();

    virtual void marshall (AdmMsgBuffer& buf);
    virtual void unmarshall (AdmMsgBuffer& buf);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    // STREAM
    virtual int recv(int fd);
    virtual int recv(int fd, AdmMsgBuffer& buf);
    virtual int send(int fd);

    // DGRAM
    int recvmsg(int fd, sockaddr_in &addr);
    int sendmsg(int fd, const sockaddr_in &addr);
};



//
// Mcast messages
//

class McastMsgBase: public McastMsgMarshallable
{
public:
    MsgHdr           hdr;
    static uint8_t   SOM[MsgSomLength];

    typedef McastMsgBase *(*GetMsgBaseFunction)();
    typedef void (*PutMsgBaseFunction)(McastMsgBase *);

    class Initializer
    {
    public:
        static bool initialized;
        Initializer();
        ~Initializer() {}
    };
    static Initializer mcast_initalized;

    McastMsgBase(eMsgType _type = eMsgInvalid);
    virtual ~McastMsgBase();

    virtual void marshall (McastMsgBuffer& buf);
    virtual void unmarshall (McastMsgBuffer& buf);

    int recv(int fd);
    int send(int fd);

protected:
    int recvExact(int fd, uint32_t bytes_want, McastMsgBuffer &buf);
    int sendExact(int fd, uint32_t bytes_req, McastMsgBuffer &buf);
};

class McastMsg: public McastMsgBase
{
public:
    static MemMgmt::GeneralPool<McastMsg, MemMgmt::ClassAlloc<McastMsg> > poolT;
    std::string      json;

    McastMsg(eMsgType _type = eMsgInvalid): McastMsgBase(_type) {}
    virtual ~McastMsg() {}

    virtual void marshall (McastMsgBuffer& buf);
    virtual void unmarshall (McastMsgBuffer& buf);

    std::string toJson();
    void fromJson(const std::string &_sjon);

    // STREAM
    int recv(int fd);
    int send(int fd);

    // DGRAM
    int recvmsg(int fd, sockaddr_in &addr);
    int sendmsg(int fd, const sockaddr_in &addr);
};

}


#endif /* SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGS_HPP_ */
