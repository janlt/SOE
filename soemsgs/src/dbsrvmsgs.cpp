/**
 * @file    dbsrvmsgs.cpp
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
 * dbsrvmsgs.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jan Lisowiec
 */

#include <sys/uio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

#include "soelogger.hpp"
using namespace SoeLogger;

#include "classalloc.hpp"
#include "generalpool.hpp"
#include "dbsrvmsgbuffer.hpp"
#include "dbsrvmarshallable.hpp"
#include "dbsrvmsgs.hpp"

#include <netinet/in.h>

using namespace std;
using namespace MemMgmt;

namespace MetaMsgs {

vector<pair<eMsgType, string> >  MsgTypeInfo::msgtype_info;
MsgTypeInfo::Initializer msgtypeinfo_initalized;
bool MsgTypeInfo::Initializer::initialized = false;

MsgTypeInfo::Initializer::Initializer()
{
    if ( initialized != true ) {
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgInvalid)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgAssignMMChannelReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgAssignMMChannelRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateClusterReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateClusterRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateSpaceReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateSpaceRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateStoreReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateStoreRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteClusterReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteClusterRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteSpaceReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteSpaceRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteStoreReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteStoreRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateBackupReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateBackupRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteBackupReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteBackupRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgVerifyBackupReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgVerifyBackupRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgRestoreBackupReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgRestoreBackupRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgListBackupsReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgListBackupsRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateSnapshotReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateSnapshotRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteSnapshotReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteSnapshotRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgListSnapshotsReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgListSnapshotsRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgListClustersReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgListClustersRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgListSpacesReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgListSpacesRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgListStoresReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgListStoresRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgListSubsetsReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgListSubsetsRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgOpenStoreReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgOpenStoreRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCloseStoreReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCloseStoreRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgRepairStoreReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgRepairStoreRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateBackupEngineReq))),
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateBackupEngineRsp))),
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateSnapshotEngineReq))),
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateSnapshotEngineRsp))),
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteBackupEngineReq))),
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteBackupEngineRsp))),
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteSnapshotEngineReq))),
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteSnapshotEngineRsp))),
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgPutReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgPutRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetRangeReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetRangeRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgMergeReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgMergeRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetReqNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetRspNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgPutReqNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgPutRspNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteReqNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteRspNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetRangeReqNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetRangeRspNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgMergeReqNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgMergeRspNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetAsyncReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetAsyncRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgPutAsyncReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgPutAsyncRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteAsyncReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteAsyncRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetRangeAsyncReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetRangeAsyncRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgMergeAsyncReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgMergeAsyncRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetAsyncReqNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetAsyncRspNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgPutAsyncReqNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgPutAsyncRspNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteAsyncReqNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteAsyncRspNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetRangeAsyncReqNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgGetRangeAsyncRspNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgMergeAsyncReqNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgMergeAsyncRspNT)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgBeginTransactionReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgBeginTransactionRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCommitTransactionReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCommitTransactionRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgAbortTransactionReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgAbortTransactionRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgMetaAd)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgMetaSolicit)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateIteratorReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateIteratorRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteIteratorReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteIteratorRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgIterateReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgIterateRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateGroupReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgCreateGroupRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteGroupReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgDeleteGroupRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgWriteGroupReq)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgWriteGroupRsp)));
        msgtype_info.push_back(pair<eMsgType, string>(STRINGIFIED_ELEM(eMsgMax)));
    }
    initialized = true;
}

const char *MsgTypeInfo::ToString(eMsgType type)
{
    unsigned long idx = static_cast<unsigned long>(type);
    if ( idx >= msgtype_info.size() ) {
        return 0;
    }
    return msgtype_info[idx].second.c_str();
}


//
// Msgs
//
MemMgmt::GeneralPool<AdmMsgReq, MemMgmt::ClassAlloc<AdmMsgReq> > AdmMsgReq::poolT(10, "AdmMsgReq");
MemMgmt::GeneralPool<AdmMsgRsp, MemMgmt::ClassAlloc<AdmMsgRsp> > AdmMsgRsp::poolT(10, "AdmMsgRsp");

uint8_t  AdmMsgBase::SOM[MsgSomLength];
AdmMsgBase::Initializer adm_initalized;
bool AdmMsgBase::Initializer::initialized = false;

AdmMsgBase::Initializer::Initializer()
{
    if ( initialized != true ) {
        ::memcpy(reinterpret_cast<void *>(AdmMsgBase::SOM), "DUPEKTAM", static_cast<size_t>(sizeof(AdmMsgBase::SOM)));
    }
    initialized = true;
}



AdmMsgBase::AdmMsgBase(eMsgType _type)
{
    ::memset(reinterpret_cast<void *>(&hdr), '\0', static_cast<size_t>(sizeof(hdr)));
    ::memcpy(reinterpret_cast<void *>(hdr.som.marker), reinterpret_cast<const void *>(AdmMsgBase::SOM), static_cast<size_t>(sizeof(AdmMsgBase::SOM)));
    hdr.type = static_cast<uint32_t>(_type);
    hdr.total_length = static_cast<uint32_t>(sizeof(hdr));
}

AdmMsgBase::~AdmMsgBase()
{}

void AdmMsgBase::marshall(AdmMsgBuffer& buf)
{
    Uint8Vector som(const_cast<uint8_t *>(hdr.som.marker), sizeof(hdr.som.marker));
    buf << som;
    buf << hdr.type;
    buf << hdr.total_length;
}

void AdmMsgBase::unmarshall(AdmMsgBuffer& buf)
{
    Uint8Vector som(hdr.som.marker, sizeof(hdr.som.marker));
    buf >> som;
    buf >> hdr.type;
    buf >> hdr.total_length;
}

AdmMsgRsp::AdmMsgRsp(eMsgType _type)
    :AdmMsg(_type)
    ,status(0)
{}

AdmMsgRsp::~AdmMsgRsp() {}

void AdmMsg::marshall(AdmMsgBuffer& buf)
{
    AdmMsgBase::marshall(buf);
    buf << json;
    AddHdrLength(static_cast<uint32_t>(json.length() + sizeof(uint16_t)));
}

void AdmMsg::unmarshall(AdmMsgBuffer& buf)
{
    AdmMsgBase::unmarshall(buf);
    buf >> json;
}

void AdmMsgReq::marshall(AdmMsgBuffer& buf)
{
    AdmMsg::marshall(buf);
}

void AdmMsgReq::unmarshall(AdmMsgBuffer& buf)
{
    AdmMsg::unmarshall(buf);
}

void AdmMsgRsp::marshall(AdmMsgBuffer& buf)
{
    AdmMsg::marshall(buf);
    buf << status;
    AddHdrLength(static_cast<uint32_t>(sizeof(status)));
}

void AdmMsgRsp::unmarshall(AdmMsgBuffer& buf)
{
    AdmMsg::unmarshall(buf);
    buf >> status;
}





int AdmMsgBase::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = recvExact(fd, sizeof(hdr), buf);
    if ( ret < 0 ) {
        return ret;
    }
    AdmMsgBase::unmarshall(buf);
    return ret;
}

int AdmMsgBase::send(int fd)
{
    AdmMsgBuffer buf;
    AdmMsgBase::marshall(buf);
    return sendExact(fd, sizeof(MsgHdr), buf);
}

int AdmMsgBase::recv(int fd, AdmMsgBuffer &buf)
{
    int ret = recvExact(fd, static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    AdmMsgBase::unmarshall(buf);
#if !defined(__clang__)
    if ( ::memcmp(hdr.som.marker, AdmMsgBase::SOM, sizeof(AdmMsgBase::SOM)) ) {
#else
    if ( *reinterpret_cast<uint64_t *>(hdr.som.marker) != *reinterpret_cast<uint64_t *>(AdmMsgBase::SOM) ) {
#endif
        return -1;
    }
    return ret;
}

int AdmMsgBase::recvExact(int fd, uint32_t bytes_want, AdmMsgBuffer &buf, bool _ignore_sig)
{
    uint32_t bytes_read = 0;
    int ret;
    uint8_t *dbuf = buf.getBuf() + buf.getOffset();

    while ( bytes_read < bytes_want ) {
        ret = ::recv(fd, dbuf + bytes_read, bytes_want - bytes_read, _ignore_sig == false ? 0 : MSG_NOSIGNAL);
        if ( ret < 0 ) {
            if ( errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK ) {
                usleep(50000);
                ret = 0;
                continue;
            } else {
                if ( errno == ECONNRESET || errno == EPIPE ) {
                    return -ECONNRESET;
                } else if ( _ignore_sig == true && errno != EAGAIN && errno != EWOULDBLOCK ) {
                    return -ECONNRESET;
                } else {
#if !defined(__clang__)
                    cerr << " read failed errno: " << errno << endl;
#else
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " read failed errno: " << errno << SoeLogger::LogError() << endl;
#endif
                    return ret;
                }
            }
        }
        if ( !ret ) {
#if !defined(__clang__)
            cerr << " read failed errno: " << errno << endl;
#else
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " client disconnected: " << errno << SoeLogger::LogInfo() << endl;
#endif

            if ( errno == EBADF ) {
                return -ECONNRESET;
            } else if ( errno ) {
                return -ECONNRESET;
            } else {
                return -ECONNRESET;
            }
        }

        bytes_read += static_cast<uint32_t>(ret);
    }

    return static_cast<int>(bytes_read);
}

int AdmMsgBase::sendExact(int fd, uint32_t bytes_req, AdmMsgBuffer &buf, bool _ignore_sig)
{
    uint32_t bytes_written = 0;
    int ret;
    uint8_t *dbuf = buf.getBuf() + buf.getOffset();

    while ( bytes_written < bytes_req ) {
        ret = ::send(fd, dbuf + bytes_written, bytes_req - bytes_written, _ignore_sig == false ? 0 : MSG_NOSIGNAL);
        if ( ret < 0 ) {
            if ( errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK ) {
                usleep(50000);
                ret = 0;
                continue;
            } else {
                if ( errno == ECONNRESET || errno == EPIPE ) {
                    return -ECONNRESET;
                } else if ( _ignore_sig == true && errno != EAGAIN && errno != EWOULDBLOCK ) {
                    return -ECONNRESET;
                } else {
#if !defined(__clang__)
                    cerr << " read failed errno: " << errno << endl;
#else
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " send failed errno: " << errno << SoeLogger::LogError() << endl;
#endif
                    return ret;
                }
            }
        }
        if ( !ret ) {
#if !defined(__clang__)
            cerr << " read failed errno: " << errno << endl;
#else
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " send ret(0) errno: " << errno << SoeLogger::LogError() << endl;
#endif

            if ( errno == EBADF ) {
                return -ECONNRESET;
            } else if ( errno ) {
                return -1;
            } else {
                ;
            }
        }

        bytes_written += static_cast<uint32_t>(ret);
    }

    return static_cast<int>(bytes_written);
}



int AdmMsg::recv(int fd, AdmMsgBuffer &buf)
{
    int ret = recvExact(fd, hdr.total_length - buf.offset, buf);
    if ( ret > 0 ) {
        AdmMsg::unmarshall(buf);
    }
    return ret;
}

int AdmMsg::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsg::send(int fd)
{
    int ret = AdmMsgBase::send(fd);
    if ( ret < 0 ) {
        return ret;
    }
    AdmMsgBuffer buf;
    AdmMsg::marshall(buf);
    return sendExact(fd, hdr.total_length - buf.offset, buf);
}

int AdmMsg::recvmsg(int fd, sockaddr_in &addr)
{
    AdmMsgBuffer buf;
    uint8_t *bp = buf.getBuf();
    uint32_t bs = buf.getSize();
    socklen_t addr_size = sizeof(addr);
    ssize_t len = ::recvfrom(fd, static_cast<void *>(bp), static_cast<size_t>(bs), 0, reinterpret_cast<sockaddr*>(&addr), &addr_size);
    if ( len < 0 ) {
#if !defined(__clang__)
        cerr << " recvfrom failed errno: " << errno << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " recvfrom failed errno: " << errno << SoeLogger::LogError() << endl;
#endif
    }
    AdmMsg::unmarshall(buf);
    return static_cast<ssize_t>(len);
}

int AdmMsg::sendmsg(int fd, const sockaddr_in &addr)
{
    AdmMsgBuffer buf;
    AdmMsg::marshall(buf);
    const uint8_t *bp = buf.getCBuf();
    uint32_t bs = buf.getOffset();
    ssize_t len = ::sendto(fd, static_cast<const void *>(bp), static_cast<size_t>(bs), 0, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if ( len < 0 ) {
#if !defined(__clang__)
        cerr << " sendto failed errno: " << errno << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " sendto failed errno: " << errno << SoeLogger::LogError() << endl;
#endif
    }
    return static_cast<int>(len);
}



int AdmMsgReq::recv(int fd, AdmMsgBuffer &buf)
{
    int ret = recvExact(fd, hdr.total_length - buf.offset, buf);
    if ( ret > 0 ) {
        AdmMsgReq::unmarshall(buf);
    }
    return ret;
}

int AdmMsgReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgReq::send(int fd)
{
    int ret = AdmMsg::send(fd);
    if ( ret < 0 ) {
        return ret;
    }
    AdmMsgBuffer buf;
    AdmMsgReq::marshall(buf);
    return sendExact(fd, hdr.total_length - buf.offset, buf);
}

int AdmMsgReq::recvmsg(int fd, sockaddr_in &addr)
{
    AdmMsgBuffer buf;
    uint8_t *bp = buf.getBuf();
    uint32_t bs = buf.getSize();
    socklen_t addr_size = sizeof(addr);
    ssize_t len = ::recvfrom(fd, static_cast<void *>(bp), static_cast<size_t>(bs), 0, reinterpret_cast<sockaddr*>(&addr), &addr_size);
    if ( len < 0 ) {
#if !defined(__clang__)
        cerr << " recvfrom failed errno: " << errno << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " recvfrom failed errno: " << errno << SoeLogger::LogError() << endl;
#endif
    }
    AdmMsgReq::unmarshall(buf);
    return static_cast<ssize_t>(len);
}

int AdmMsgReq::sendmsg(int fd, const sockaddr_in &addr)
{
    AdmMsgBuffer buf;
    AdmMsgReq::marshall(buf);
    const uint8_t *bp = buf.getCBuf();
    uint32_t bs = buf.getOffset();
    ssize_t len = ::sendto(fd, static_cast<const void *>(bp), static_cast<size_t>(bs), 0, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if ( len < 0 ) {
#if !defined(__clang__)
        cerr << " sendto failed errno: " << errno << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " sendto failed errno: " << errno << SoeLogger::LogError() << endl;
#endif
    }
    return static_cast<int>(len);
}

int AdmMsgRsp::send(int fd)
{
    int ret = AdmMsg::send(fd);
    if ( ret < 0 ) {
        return ret;
    }
    AdmMsgBuffer buf;
    AdmMsgRsp::marshall(buf);
    return sendExact(fd, hdr.total_length - buf.offset, buf);
}

int AdmMsgRsp::recv(int fd, AdmMsgBuffer &buf)
{
    int ret = recvExact(fd, hdr.total_length - buf.offset, buf);
    if ( ret > 0 ) {
        AdmMsgRsp::unmarshall(buf);
    }
    return ret;
}

int AdmMsgRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgRsp::sendmsg(int fd, const sockaddr_in &addr)
{
    AdmMsgBuffer buf;
    AdmMsgRsp::marshall(buf);
    const uint8_t *bp = buf.getCBuf();
    uint32_t bs = buf.getOffset();
    ssize_t len = ::sendto(fd, static_cast<const void *>(bp), static_cast<size_t>(bs), 0, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if ( len < 0 ) {
#if !defined(__clang__)
        cerr << " sendto failed errno: " << errno << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " sendto failed errno: " << errno << SoeLogger::LogError() << endl;
#endif
    }
    return static_cast<int>(len);
}

int AdmMsgRsp::recvmsg(int fd, sockaddr_in &addr)
{
    AdmMsgBuffer buf;
    uint8_t *bp = buf.getBuf();
    uint32_t bs = buf.getSize();
    socklen_t addr_size = sizeof(addr);
    ssize_t len = ::recvfrom(fd, static_cast<void *>(bp), static_cast<size_t>(bs), 0, reinterpret_cast<sockaddr*>(&addr), &addr_size);
    if ( len < 0 ) {
#if !defined(__clang__)
        cerr << " recvfrom failed errno: " << errno << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " recvfrom failed errno: " << errno << SoeLogger::LogError() << endl;
#endif
    }
    AdmMsgRsp::unmarshall(buf);
    return static_cast<ssize_t>(len);
}



string AdmMsg::toJson()
{
    return "";
}

void AdmMsg::fromJson(const string &_sjon)
{
}

string AdmMsgReq::toJson()
{
    return "";
}

void AdmMsgReq::fromJson(const string &_sjon)
{
}

string AdmMsgRsp::toJson()
{
    return "";
}

void AdmMsgRsp::fromJson(const string &_sjon)
{
}




//
// Mcast
//
MemMgmt::GeneralPool<McastMsg, MemMgmt::ClassAlloc<McastMsg> > McastMsg::poolT(10, "McastMsg");

uint8_t  McastMsgBase::SOM[MsgSomLength];
McastMsgBase::Initializer mcast_initalized;
bool McastMsgBase::Initializer::initialized = false;

McastMsgBase::Initializer::Initializer()
{
    if ( initialized != true ) {
        ::memcpy(reinterpret_cast<void *>(McastMsgBase::SOM), "DUPMCAST", static_cast<size_t>(sizeof(McastMsgBase::SOM)));
    }
    initialized = true;
}



int McastMsgBase::recv(int fd)
{
    McastMsgBuffer buf;
    int ret = recvExact(fd, sizeof(MsgHdr), buf);
    if ( ret > 0 ) {
        McastMsgBase::unmarshall(buf);
    }
#if !defined(__clang__)
    if ( ::memcmp(hdr.som.marker, McastMsgBase::SOM, sizeof(McastMsgBase::SOM)) ) {
#else
    if ( *reinterpret_cast<uint64_t *>(hdr.som.marker) != *reinterpret_cast<uint64_t *>(McastMsgBase::SOM) ) {
#endif
        return -1;
    }
    return ret;
}

int McastMsgBase::send(int fd)
{
    McastMsgBuffer buf;
    McastMsgBase::marshall(buf);
    return sendExact(fd, sizeof(MsgHdr), buf);
}

int McastMsgBase::recvExact(int fd, uint32_t bytes_want, McastMsgBuffer &buf)
{
    uint32_t bytes_read = 0;
    int ret;
    uint8_t *dbuf = buf.getBuf();

    while ( bytes_read < bytes_want ) {
        ret = ::read(fd, dbuf + bytes_read, bytes_want - bytes_read);
        if ( ret < 0 ) {
            if ( errno == EINTR || errno == EAGAIN ) {
                continue;
            } else {
                if ( errno == ECONNRESET ) {
                    return -ECONNRESET;
                } else {
#if !defined(__clang__)
                    cerr << " read failed errno: " << errno << endl;
#else
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " read failed errno: " << errno << SoeLogger::LogError() << endl;
#endif
                    return ret;
                }
            }
        }
        if ( !ret ) {
            if ( errno == EBADF ) {
                return -ECONNRESET;
            } else if ( errno ) {
                return -1;
            } else {
                ;
            }
        }

        bytes_read += static_cast<uint32_t>(ret);
    }

    return static_cast<int>(bytes_read);
}

int McastMsgBase::sendExact(int fd, uint32_t bytes_req, McastMsgBuffer &buf)
{
    uint32_t bytes_written = 0;
    int ret;
    uint8_t *dbuf = buf.getBuf();

    while ( bytes_written < bytes_req ) {
        ret = ::write(fd, dbuf + bytes_written, bytes_req - bytes_written);
        if ( ret < 0 ) {
            if ( errno == EINTR || errno == EAGAIN ) {
                continue;
            } else {
                if ( errno == ECONNRESET ) {
                    return -ECONNRESET;
                } else {
#if !defined(__clang__)
                    cerr << " write failed errno: " << errno << endl;
#else
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " write failed errno: " << errno << SoeLogger::LogError() << endl;
#endif
                    return ret;
                }
            }
        }
        if ( !ret ) {
            if ( errno == EBADF ) {
                return -ECONNRESET;
            } else if ( errno ) {
                return -1;
            } else {
                ;
            }
        }

        bytes_written += static_cast<uint32_t>(ret);
    }

    return static_cast<int>(bytes_written);
}

McastMsgBase::McastMsgBase(eMsgType _type)
{
    ::memset(reinterpret_cast<void *>(&hdr), '\0', static_cast<size_t>(sizeof(hdr)));
    ::memcpy(reinterpret_cast<void *>(hdr.som.marker), reinterpret_cast<const void *>(McastMsgBase::SOM), static_cast<size_t>(sizeof(McastMsgBase::SOM)));
    hdr.type = static_cast<uint32_t>(_type);
}

McastMsgBase::~McastMsgBase()
{

}

void McastMsgBase::marshall(McastMsgBuffer& buf)
{
    Uint8Vector som(const_cast<uint8_t *>(hdr.som.marker), sizeof(hdr.som.marker));
    buf << som;
    buf << hdr.type;
    buf << hdr.total_length;
}

void McastMsgBase::unmarshall(McastMsgBuffer& buf)
{
    Uint8Vector som(hdr.som.marker, sizeof(hdr.som.marker));
    buf >> som;
    buf >> hdr.type;
    buf >> hdr.total_length;
}

void McastMsg::marshall(McastMsgBuffer& buf)
{
    McastMsgBase::marshall(buf);
    buf << json;
}

void McastMsg::unmarshall(McastMsgBuffer& buf)
{
    McastMsgBase::unmarshall(buf);
    buf >> json;
}

int McastMsg::send(int fd)
{
    int ret = McastMsgBase::send(fd);
    if ( ret < 0 ) {
        return ret;
    }
    McastMsgBuffer buf;
    McastMsg::marshall(buf);
    return sendExact(fd, hdr.total_length - buf.offset, buf);
}

int McastMsg::recv(int fd)
{
    int ret = McastMsgBase::recv(fd);
    if ( ret < 0 ) {
        return ret;
    }
    McastMsgBuffer buf;
    ret = recvExact(fd, hdr.total_length - buf.offset, buf);
    if ( ret ) {
        McastMsg::unmarshall(buf);
    }
    return ret;
}

int McastMsg::sendmsg(int fd, const sockaddr_in &addr)
{
    McastMsgBuffer buf;
    McastMsg::marshall(buf);
    const uint8_t *bp = buf.getCBuf();
    uint32_t bs = buf.getOffset();
    ssize_t len = ::sendto(fd, static_cast<const void *>(bp), static_cast<size_t>(bs), 0, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if ( len < 0 ) {
#if !defined(__clang__)
        cerr << " sendto failed errno: " << errno << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " sendto failed errno: " << errno << SoeLogger::LogError() << endl;
#endif
    }
    return static_cast<int>(len);
}

int McastMsg::recvmsg(int fd, sockaddr_in &addr)
{
    McastMsgBuffer buf;
    uint8_t *bp = buf.getBuf();
    uint32_t bs = buf.getSize();
    socklen_t addr_size = sizeof(addr);
    ssize_t len = ::recvfrom(fd, static_cast<void *>(bp), static_cast<size_t>(bs), 0, reinterpret_cast<sockaddr*>(&addr), &addr_size);
    if ( len < 0 ) {
#if !defined(__clang__)
        cerr << " recvfrom failed errno: " << errno << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " recvfrom failed errno: " << errno << SoeLogger::LogError() << endl;
#endif
    }
    McastMsg::unmarshall(buf);
    return static_cast<ssize_t>(len);
}

string McastMsg::toJson()
{
    return "";
}

void McastMsg::fromJson(const string &_sjon)
{
}

}



