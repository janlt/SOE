/**
 * @file    dbsrvmsgadmtypes.cpp
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
 * dbsrvmsgadmtypes.cpp
 *
 *  Created on: Feb 7, 2017
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
#include "dbsrvmsgtypes.hpp"
#include "dbsrvmsgfactory.hpp"
#include "dbsrvmsgadmtypes.hpp"
#include "dbsrvmsgadmfactory.hpp"

using namespace std;

using namespace MemMgmt;

namespace MetaMsgs {

static AdmMsgPoolInitializer initalizer;
bool AdmMsgPoolInitializer::initialized = false;

AdmMsgPoolInitializer::AdmMsgPoolInitializer()
{
    if ( initialized != true ) {
        AdmMsgAssignMMChannelReq::registerPool();
        AdmMsgAssignMMChannelRsp::registerPool();
        AdmMsgCreateStoreReq::registerPool();
        AdmMsgCreateStoreRsp::registerPool();
        AdmMsgRepairStoreReq::registerPool();
        AdmMsgRepairStoreRsp::registerPool();
        AdmMsgCreateSpaceReq::registerPool();
        AdmMsgCreateSpaceRsp::registerPool();
        AdmMsgCreateClusterReq::registerPool();
        AdmMsgCreateClusterRsp::registerPool();
        AdmMsgDeleteStoreReq::registerPool();
        AdmMsgDeleteStoreRsp::registerPool();
        AdmMsgDeleteSpaceReq::registerPool();
        AdmMsgDeleteSpaceRsp::registerPool();
        AdmMsgDeleteClusterReq::registerPool();
        AdmMsgDeleteClusterRsp::registerPool();
        AdmMsgCreateBackupReq::registerPool();
        AdmMsgCreateBackupRsp::registerPool();
        AdmMsgRestoreBackupReq::registerPool();
        AdmMsgRestoreBackupRsp::registerPool();
        AdmMsgDeleteBackupReq::registerPool();
        AdmMsgDeleteBackupRsp::registerPool();
        AdmMsgVerifyBackupReq::registerPool();
        AdmMsgVerifyBackupRsp::registerPool();
        AdmMsgCreateBackupEngineReq::registerPool();
        AdmMsgCreateBackupEngineRsp::registerPool();
        AdmMsgDeleteBackupEngineReq::registerPool();
        AdmMsgDeleteBackupEngineRsp::registerPool();
        AdmMsgListBackupsReq::registerPool();
        AdmMsgListBackupsRsp::registerPool();
        AdmMsgCreateSnapshotReq::registerPool();
        AdmMsgCreateSnapshotRsp::registerPool();
        AdmMsgDeleteSnapshotReq::registerPool();
        AdmMsgDeleteSnapshotRsp::registerPool();
        AdmMsgCreateSnapshotEngineReq::registerPool();
        AdmMsgCreateSnapshotEngineRsp::registerPool();
        AdmMsgDeleteSnapshotEngineReq::registerPool();
        AdmMsgDeleteSnapshotEngineRsp::registerPool();
        AdmMsgListSnapshotsReq::registerPool();
        AdmMsgListSnapshotsRsp::registerPool();
        AdmMsgOpenStoreReq::registerPool();
        AdmMsgOpenStoreRsp::registerPool();
        AdmMsgCloseStoreReq::registerPool();
        AdmMsgCloseStoreRsp::registerPool();
        AdmMsgListClustersReq::registerPool();
        AdmMsgListClustersRsp::registerPool();
        AdmMsgListSpacesReq::registerPool();
        AdmMsgListSpacesRsp::registerPool();
        AdmMsgListStoresReq::registerPool();
        AdmMsgListStoresRsp::registerPool();
        AdmMsgListSubsetsReq::registerPool();
        AdmMsgListSubsetsRsp::registerPool();
    }
    initialized = true;
}


// -----------------------------------------------------------------------------------------

int AdmMsgAssignMMChannelReq::class_msg_type = eMsgAssignMMChannelReq;
MemMgmt::GeneralPool<AdmMsgAssignMMChannelReq, MemMgmt::ClassAlloc<AdmMsgAssignMMChannelReq> > AdmMsgAssignMMChannelReq::poolT(10, "AdmMsgAssignMMChannelReq");
AdmMsgAssignMMChannelReq::AdmMsgAssignMMChannelReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgAssignMMChannelReq::class_msg_type)),
    pid(0)
{}

AdmMsgAssignMMChannelReq::~AdmMsgAssignMMChannelReq() {}

void AdmMsgAssignMMChannelReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << pid;
    AddHdrLength(static_cast<uint32_t>(sizeof(pid)));
}

void AdmMsgAssignMMChannelReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> pid;
}

// STREAM
int AdmMsgAssignMMChannelReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgAssignMMChannelReq::unmarshall(buf);

    return ret;
}

int AdmMsgAssignMMChannelReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgAssignMMChannelReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgAssignMMChannelReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgAssignMMChannelReq::toJson()
{
    return json;
}

void AdmMsgAssignMMChannelReq::fromJson(const string &_sjon)
{

}

AdmMsgAssignMMChannelReq *AdmMsgAssignMMChannelReq::get()
{
    return AdmMsgAssignMMChannelReq::poolT.get();
}

void AdmMsgAssignMMChannelReq::put(AdmMsgAssignMMChannelReq *obj)
{
    AdmMsgAssignMMChannelReq::poolT.put(obj);
}

void AdmMsgAssignMMChannelReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgAssignMMChannelReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgAssignMMChannelReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgAssignMMChannelReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgAssignMMChannelReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgAssignMMChannelRsp::class_msg_type = eMsgAssignMMChannelRsp;
MemMgmt::GeneralPool<AdmMsgAssignMMChannelRsp, MemMgmt::ClassAlloc<AdmMsgAssignMMChannelRsp> > AdmMsgAssignMMChannelRsp::poolT(10, "AdmMsgAssignMMChannelRsp");
AdmMsgAssignMMChannelRsp::AdmMsgAssignMMChannelRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgAssignMMChannelRsp::class_msg_type)),
    path()
{}

AdmMsgAssignMMChannelRsp::~AdmMsgAssignMMChannelRsp() {}

string AdmMsgAssignMMChannelRsp::toJson()
{
    return json;
}

void AdmMsgAssignMMChannelRsp::fromJson(const string &_sjon)
{

}

void AdmMsgAssignMMChannelRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << path;
    AddHdrLength(static_cast<uint32_t>(path.length() + sizeof(uint16_t)));
    buf << pid;
    AddHdrLength(static_cast<uint32_t>(sizeof(pid)));
}

void AdmMsgAssignMMChannelRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> path;
    buf >> pid;
}

// STREAM
int AdmMsgAssignMMChannelRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgAssignMMChannelRsp::unmarshall(buf);

    return ret;
}

int AdmMsgAssignMMChannelRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgAssignMMChannelRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgAssignMMChannelRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

AdmMsgAssignMMChannelRsp *AdmMsgAssignMMChannelRsp::get()
{
    return AdmMsgAssignMMChannelRsp::poolT.get();
}

void AdmMsgAssignMMChannelRsp::put(AdmMsgAssignMMChannelRsp *obj)
{
    AdmMsgAssignMMChannelRsp::poolT.put(obj);
}

void AdmMsgAssignMMChannelRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgAssignMMChannelRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgAssignMMChannelRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgAssignMMChannelRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgAssignMMChannelRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgCreateStoreReq::class_msg_type = eMsgCreateStoreReq;
MemMgmt::GeneralPool<AdmMsgCreateStoreReq, MemMgmt::ClassAlloc<AdmMsgCreateStoreReq> > AdmMsgCreateStoreReq::poolT(10, "AdmMsgCreateStoreReq");
AdmMsgCreateStoreReq::AdmMsgCreateStoreReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgCreateStoreReq::class_msg_type)),
     cluster_id(-1),
     space_id(-1),
     cr_thread_id(-1),
     only_get_by_name(0)
{}

AdmMsgCreateStoreReq::~AdmMsgCreateStoreReq() {}

void AdmMsgCreateStoreReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << cr_thread_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cr_thread_id)));
    buf << only_get_by_name;
    AddHdrLength(static_cast<uint32_t>(sizeof(only_get_by_name)));
    buf << flags.fields;
    AddHdrLength(static_cast<uint32_t>(sizeof(flags.fields)));
}

void AdmMsgCreateStoreReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> cr_thread_id;
    buf >> only_get_by_name;
    buf >> flags.fields;
}

// STREAM
int AdmMsgCreateStoreReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCreateStoreReq::unmarshall(buf);

    return ret;
}

int AdmMsgCreateStoreReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCreateStoreReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgCreateStoreReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCreateStoreReq::toJson()
{
    return json;
}

void AdmMsgCreateStoreReq::fromJson(const string &_sjon)
{

}

AdmMsgCreateStoreReq *AdmMsgCreateStoreReq::get()
{
    return AdmMsgCreateStoreReq::poolT.get();
}

void AdmMsgCreateStoreReq::put(AdmMsgCreateStoreReq *obj)
{
    AdmMsgCreateStoreReq::poolT.put(obj);
}

void AdmMsgCreateStoreReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCreateStoreReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCreateStoreReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCreateStoreReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCreateStoreReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgCreateStoreRsp::class_msg_type = eMsgCreateStoreRsp;
MemMgmt::GeneralPool<AdmMsgCreateStoreRsp, MemMgmt::ClassAlloc<AdmMsgCreateStoreRsp> > AdmMsgCreateStoreRsp::poolT(10, "AdmMsgCreateStoreRsp");
AdmMsgCreateStoreRsp::AdmMsgCreateStoreRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgCreateStoreRsp::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0)
{}

AdmMsgCreateStoreRsp::~AdmMsgCreateStoreRsp() {}

void AdmMsgCreateStoreRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
}

void AdmMsgCreateStoreRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
}

// STREAM
int AdmMsgCreateStoreRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCreateStoreRsp::unmarshall(buf);

    return ret;
}

int AdmMsgCreateStoreRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCreateStoreRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgCreateStoreRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCreateStoreRsp::toJson()
{
    return json;
}

void AdmMsgCreateStoreRsp::fromJson(const string &_sjon)
{

}

AdmMsgCreateStoreRsp *AdmMsgCreateStoreRsp::get()
{
    return AdmMsgCreateStoreRsp::poolT.get();
}

void AdmMsgCreateStoreRsp::put(AdmMsgCreateStoreRsp *obj)
{
    AdmMsgCreateStoreRsp::poolT.put(obj);
}

void AdmMsgCreateStoreRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCreateStoreRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCreateStoreRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCreateStoreRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCreateStoreRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgRepairStoreReq::class_msg_type = eMsgRepairStoreReq;
MemMgmt::GeneralPool<AdmMsgRepairStoreReq, MemMgmt::ClassAlloc<AdmMsgRepairStoreReq> > AdmMsgRepairStoreReq::poolT(10, "AdmMsgRepairStoreReq");
AdmMsgRepairStoreReq::AdmMsgRepairStoreReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgRepairStoreReq::class_msg_type)),
     cluster_id(-1),
     space_id(-1),
     store_id(-1),
     cr_thread_id(-1),
     only_get_by_name(0)
{}

AdmMsgRepairStoreReq::~AdmMsgRepairStoreReq() {}

void AdmMsgRepairStoreReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
    buf << cr_thread_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cr_thread_id)));
    buf << only_get_by_name;
    AddHdrLength(static_cast<uint32_t>(sizeof(only_get_by_name)));
}

void AdmMsgRepairStoreReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
    buf >> cr_thread_id;
    buf >> only_get_by_name;
}

// STREAM
int AdmMsgRepairStoreReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgRepairStoreReq::unmarshall(buf);

    return ret;
}

int AdmMsgRepairStoreReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgRepairStoreReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgRepairStoreReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgRepairStoreReq::toJson()
{
    return json;
}

void AdmMsgRepairStoreReq::fromJson(const string &_sjon)
{

}

AdmMsgRepairStoreReq *AdmMsgRepairStoreReq::get()
{
    return AdmMsgRepairStoreReq::poolT.get();
}

void AdmMsgRepairStoreReq::put(AdmMsgRepairStoreReq *obj)
{
    AdmMsgRepairStoreReq::poolT.put(obj);
}

void AdmMsgRepairStoreReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgRepairStoreReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgRepairStoreReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgRepairStoreReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgRepairStoreReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgRepairStoreRsp::class_msg_type = eMsgRepairStoreRsp;
MemMgmt::GeneralPool<AdmMsgRepairStoreRsp, MemMgmt::ClassAlloc<AdmMsgRepairStoreRsp> > AdmMsgRepairStoreRsp::poolT(10, "AdmMsgRepairStoreRsp");
AdmMsgRepairStoreRsp::AdmMsgRepairStoreRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgRepairStoreRsp::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0)
{}

AdmMsgRepairStoreRsp::~AdmMsgRepairStoreRsp() {}

void AdmMsgRepairStoreRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
}

void AdmMsgRepairStoreRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
}

// STREAM
int AdmMsgRepairStoreRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgRepairStoreRsp::unmarshall(buf);

    return ret;
}

int AdmMsgRepairStoreRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgRepairStoreRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgRepairStoreRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgRepairStoreRsp::toJson()
{
    return json;
}

void AdmMsgRepairStoreRsp::fromJson(const string &_sjon)
{

}

AdmMsgRepairStoreRsp *AdmMsgRepairStoreRsp::get()
{
    return AdmMsgRepairStoreRsp::poolT.get();
}

void AdmMsgRepairStoreRsp::put(AdmMsgRepairStoreRsp *obj)
{
    AdmMsgRepairStoreRsp::poolT.put(obj);
}

void AdmMsgRepairStoreRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgRepairStoreRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgRepairStoreRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgRepairStoreRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgRepairStoreRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgCreateSpaceReq::class_msg_type = eMsgCreateSpaceReq;
MemMgmt::GeneralPool<AdmMsgCreateSpaceReq, MemMgmt::ClassAlloc<AdmMsgCreateSpaceReq> > AdmMsgCreateSpaceReq::poolT(10, "AdmMsgCreateSpaceReq");
AdmMsgCreateSpaceReq::AdmMsgCreateSpaceReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgCreateSpaceReq::class_msg_type)),
     cluster_id(-1),
     only_get_by_name(0)
{}

AdmMsgCreateSpaceReq::~AdmMsgCreateSpaceReq() {}

void AdmMsgCreateSpaceReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << only_get_by_name;
    AddHdrLength(static_cast<uint32_t>(sizeof(only_get_by_name)));
}

void AdmMsgCreateSpaceReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> cluster_id;
    buf >> only_get_by_name;
}

// STREAM
int AdmMsgCreateSpaceReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCreateSpaceReq::unmarshall(buf);

    return ret;
}

int AdmMsgCreateSpaceReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCreateSpaceReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgCreateSpaceReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}


string AdmMsgCreateSpaceReq::toJson()
{
    return json;
}

void AdmMsgCreateSpaceReq::fromJson(const string &_sjon)
{

}

AdmMsgCreateSpaceReq *AdmMsgCreateSpaceReq::get()
{
    return AdmMsgCreateSpaceReq::poolT.get();
}

void AdmMsgCreateSpaceReq::put(AdmMsgCreateSpaceReq *obj)
{
    AdmMsgCreateSpaceReq::poolT.put(obj);
}

void AdmMsgCreateSpaceReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCreateSpaceReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCreateSpaceReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCreateSpaceReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCreateSpaceReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgCreateSpaceRsp::class_msg_type = eMsgCreateSpaceRsp;
MemMgmt::GeneralPool<AdmMsgCreateSpaceRsp, MemMgmt::ClassAlloc<AdmMsgCreateSpaceRsp> > AdmMsgCreateSpaceRsp::poolT(10, "AdmMsgCreateSpaceRsp");
AdmMsgCreateSpaceRsp::AdmMsgCreateSpaceRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgCreateSpaceRsp::class_msg_type)),
    cluster_id(0),
    space_id(0)
{}

AdmMsgCreateSpaceRsp::~AdmMsgCreateSpaceRsp() {}

void AdmMsgCreateSpaceRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
}

void AdmMsgCreateSpaceRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> cluster_id;
    buf >> space_id;
}

// STREAM
int AdmMsgCreateSpaceRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCreateSpaceRsp::unmarshall(buf);

    return ret;
}

int AdmMsgCreateSpaceRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCreateSpaceRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgCreateSpaceRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCreateSpaceRsp::toJson()
{
    return json;
}

void AdmMsgCreateSpaceRsp::fromJson(const string &_sjon)
{

}

AdmMsgCreateSpaceRsp *AdmMsgCreateSpaceRsp::get()
{
    return AdmMsgCreateSpaceRsp::poolT.get();
}

void AdmMsgCreateSpaceRsp::put(AdmMsgCreateSpaceRsp *obj)
{
    AdmMsgCreateSpaceRsp::poolT.put(obj);
}

void AdmMsgCreateSpaceRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCreateSpaceRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCreateSpaceRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCreateSpaceRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCreateSpaceRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgCreateClusterReq::class_msg_type = eMsgCreateClusterReq;
MemMgmt::GeneralPool<AdmMsgCreateClusterReq, MemMgmt::ClassAlloc<AdmMsgCreateClusterReq> > AdmMsgCreateClusterReq::poolT(10, "AdmMsgCreateClusterReq");
AdmMsgCreateClusterReq::AdmMsgCreateClusterReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgCreateClusterReq::class_msg_type)),
     only_get_by_name(0)
{}

AdmMsgCreateClusterReq::~AdmMsgCreateClusterReq() {}

void AdmMsgCreateClusterReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << only_get_by_name;
    AddHdrLength(static_cast<uint32_t>(sizeof(only_get_by_name)));
}

void AdmMsgCreateClusterReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> only_get_by_name;
}

// STREAM
int AdmMsgCreateClusterReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCreateClusterReq::unmarshall(buf);

    return ret;
}

int AdmMsgCreateClusterReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCreateClusterReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgCreateClusterReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCreateClusterReq::toJson()
{
    return json;
}

void AdmMsgCreateClusterReq::fromJson(const string &_sjon)
{

}

AdmMsgCreateClusterReq *AdmMsgCreateClusterReq::get()
{
    return AdmMsgCreateClusterReq::poolT.get();
}

void AdmMsgCreateClusterReq::put(AdmMsgCreateClusterReq *obj)
{
    AdmMsgCreateClusterReq::poolT.put(obj);
}

void AdmMsgCreateClusterReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCreateClusterReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCreateClusterReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCreateClusterReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCreateClusterReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgCreateClusterRsp::class_msg_type = eMsgCreateClusterRsp;
MemMgmt::GeneralPool<AdmMsgCreateClusterRsp, MemMgmt::ClassAlloc<AdmMsgCreateClusterRsp> > AdmMsgCreateClusterRsp::poolT(10, "AdmMsgCreateClusterRsp");
AdmMsgCreateClusterRsp::AdmMsgCreateClusterRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgCreateClusterRsp::class_msg_type)),
    cluster_id(0)
{}

AdmMsgCreateClusterRsp::~AdmMsgCreateClusterRsp() {}

void AdmMsgCreateClusterRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
}

void AdmMsgCreateClusterRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> cluster_id;
}

// STREAM
int AdmMsgCreateClusterRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCreateClusterRsp::unmarshall(buf);

    return ret;
}

int AdmMsgCreateClusterRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCreateClusterRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgCreateClusterRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCreateClusterRsp::toJson()
{
    return json;
}

void AdmMsgCreateClusterRsp::fromJson(const string &_sjon)
{

}

AdmMsgCreateClusterRsp *AdmMsgCreateClusterRsp::get()
{
    return AdmMsgCreateClusterRsp::poolT.get();
}

void AdmMsgCreateClusterRsp::put(AdmMsgCreateClusterRsp *obj)
{
    AdmMsgCreateClusterRsp::poolT.put(obj);
}

void AdmMsgCreateClusterRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCreateClusterRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCreateClusterRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCreateClusterRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCreateClusterRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgDeleteStoreReq::class_msg_type = eMsgDeleteStoreReq;
MemMgmt::GeneralPool<AdmMsgDeleteStoreReq, MemMgmt::ClassAlloc<AdmMsgDeleteStoreReq> > AdmMsgDeleteStoreReq::poolT(10, "AdmMsgDeleteStoreReq");
AdmMsgDeleteStoreReq::AdmMsgDeleteStoreReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgDeleteStoreReq::class_msg_type)),
     cluster_id(-1),
     space_id(-1),
     store_id(-1),
     cr_thread_id(-1)
{}

AdmMsgDeleteStoreReq::~AdmMsgDeleteStoreReq() {}

void AdmMsgDeleteStoreReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << cr_thread_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cr_thread_id)));
}

void AdmMsgDeleteStoreReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
    buf >> cr_thread_id;
}

// STREAM
int AdmMsgDeleteStoreReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgDeleteStoreReq::unmarshall(buf);

    return ret;
}

int AdmMsgDeleteStoreReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgDeleteStoreReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgDeleteStoreReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgDeleteStoreReq::toJson()
{
    return json;
}

void AdmMsgDeleteStoreReq::fromJson(const string &_sjon)
{

}

AdmMsgDeleteStoreReq *AdmMsgDeleteStoreReq::get()
{
    return AdmMsgDeleteStoreReq::poolT.get();
}

void AdmMsgDeleteStoreReq::put(AdmMsgDeleteStoreReq *obj)
{
    AdmMsgDeleteStoreReq::poolT.put(obj);
}

void AdmMsgDeleteStoreReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgDeleteStoreReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgDeleteStoreReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgDeleteStoreReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgDeleteStoreReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgDeleteStoreRsp::class_msg_type = eMsgDeleteStoreRsp;
MemMgmt::GeneralPool<AdmMsgDeleteStoreRsp, MemMgmt::ClassAlloc<AdmMsgDeleteStoreRsp> > AdmMsgDeleteStoreRsp::poolT(10, "AdmMsgDeleteStoreRsp");
AdmMsgDeleteStoreRsp::AdmMsgDeleteStoreRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgDeleteStoreRsp::class_msg_type)),
    cluster_id(-1),
    space_id(-1),
    store_id(-1)
{}

AdmMsgDeleteStoreRsp::~AdmMsgDeleteStoreRsp() {}

void AdmMsgDeleteStoreRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
}

void AdmMsgDeleteStoreRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
}

// STREAM
int AdmMsgDeleteStoreRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgDeleteStoreRsp::unmarshall(buf);

    return ret;
}

int AdmMsgDeleteStoreRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgDeleteStoreRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgDeleteStoreRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgDeleteStoreRsp::toJson()
{
    return json;
}

void AdmMsgDeleteStoreRsp::fromJson(const string &_sjon)
{

}

AdmMsgDeleteStoreRsp *AdmMsgDeleteStoreRsp::get()
{
    return AdmMsgDeleteStoreRsp::poolT.get();
}

void AdmMsgDeleteStoreRsp::put(AdmMsgDeleteStoreRsp *obj)
{
    AdmMsgDeleteStoreRsp::poolT.put(obj);
}

void AdmMsgDeleteStoreRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgDeleteStoreRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgDeleteStoreRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgDeleteStoreRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgDeleteStoreRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgDeleteSpaceReq::class_msg_type = eMsgDeleteSpaceReq;
MemMgmt::GeneralPool<AdmMsgDeleteSpaceReq, MemMgmt::ClassAlloc<AdmMsgDeleteSpaceReq> > AdmMsgDeleteSpaceReq::poolT(10, "AdmMsgDeleteSpaceReq");
AdmMsgDeleteSpaceReq::AdmMsgDeleteSpaceReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgDeleteSpaceReq::class_msg_type)),
     cluster_id(-1),
     space_id(-1)
{}

AdmMsgDeleteSpaceReq::~AdmMsgDeleteSpaceReq() {}

void AdmMsgDeleteSpaceReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
}

void AdmMsgDeleteSpaceReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> cluster_id;
    buf >> space_id;
}

// STREAM
int AdmMsgDeleteSpaceReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgDeleteSpaceReq::unmarshall(buf);

    return ret;
}

int AdmMsgDeleteSpaceReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgDeleteSpaceReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgDeleteSpaceReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgDeleteSpaceReq::toJson()
{
    return json;
}

void AdmMsgDeleteSpaceReq::fromJson(const string &_sjon)
{

}

AdmMsgDeleteSpaceReq *AdmMsgDeleteSpaceReq::get()
{
    return AdmMsgDeleteSpaceReq::poolT.get();
}

void AdmMsgDeleteSpaceReq::put(AdmMsgDeleteSpaceReq *obj)
{
    AdmMsgDeleteSpaceReq::poolT.put(obj);
}

void AdmMsgDeleteSpaceReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgDeleteSpaceReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgDeleteSpaceReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgDeleteSpaceReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgDeleteSpaceReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgDeleteSpaceRsp::class_msg_type = eMsgDeleteSpaceRsp;
MemMgmt::GeneralPool<AdmMsgDeleteSpaceRsp, MemMgmt::ClassAlloc<AdmMsgDeleteSpaceRsp> > AdmMsgDeleteSpaceRsp::poolT(10, "AdmMsgDeleteSpaceRsp");

AdmMsgDeleteSpaceRsp::AdmMsgDeleteSpaceRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgDeleteSpaceRsp::class_msg_type)),
    cluster_id(-1),
    space_id(-1)
{}

AdmMsgDeleteSpaceRsp::~AdmMsgDeleteSpaceRsp() {}

void AdmMsgDeleteSpaceRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
}

void AdmMsgDeleteSpaceRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> cluster_id;
    buf >> space_id;
}

// STREAM
int AdmMsgDeleteSpaceRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgDeleteSpaceRsp::unmarshall(buf);

    return ret;
}

int AdmMsgDeleteSpaceRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgDeleteSpaceRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgDeleteSpaceRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgDeleteSpaceRsp::toJson()
{
    return json;
}

void AdmMsgDeleteSpaceRsp::fromJson(const string &_sjon)
{

}

AdmMsgDeleteSpaceRsp *AdmMsgDeleteSpaceRsp::get()
{
    return AdmMsgDeleteSpaceRsp::poolT.get();
}

void AdmMsgDeleteSpaceRsp::put(AdmMsgDeleteSpaceRsp *obj)
{
    AdmMsgDeleteSpaceRsp::poolT.put(obj);
}

void AdmMsgDeleteSpaceRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgDeleteSpaceRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgDeleteSpaceRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgDeleteSpaceRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgDeleteSpaceRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgDeleteClusterReq::class_msg_type = eMsgDeleteClusterReq;
MemMgmt::GeneralPool<AdmMsgDeleteClusterReq, MemMgmt::ClassAlloc<AdmMsgDeleteClusterReq> > AdmMsgDeleteClusterReq::poolT(10, "AdmMsgDeleteClusterReq");
AdmMsgDeleteClusterReq::AdmMsgDeleteClusterReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgDeleteClusterReq::class_msg_type)),
     cluster_id(-1)
{}

AdmMsgDeleteClusterReq::~AdmMsgDeleteClusterReq() {}

void AdmMsgDeleteClusterReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
}

void AdmMsgDeleteClusterReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> cluster_id;
}

// STREAM
int AdmMsgDeleteClusterReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgDeleteClusterReq::unmarshall(buf);

    return ret;
}

int AdmMsgDeleteClusterReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgDeleteClusterReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgDeleteClusterReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgDeleteClusterReq::toJson()
{
    return json;
}

void AdmMsgDeleteClusterReq::fromJson(const string &_sjon)
{

}

AdmMsgDeleteClusterReq *AdmMsgDeleteClusterReq::get()
{
    return AdmMsgDeleteClusterReq::poolT.get();
}

void AdmMsgDeleteClusterReq::put(AdmMsgDeleteClusterReq *obj)
{
    AdmMsgDeleteClusterReq::poolT.put(obj);
}

void AdmMsgDeleteClusterReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgDeleteClusterReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgDeleteClusterReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgDeleteClusterReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgDeleteClusterReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgDeleteClusterRsp::class_msg_type = eMsgDeleteClusterRsp;
MemMgmt::GeneralPool<AdmMsgDeleteClusterRsp, MemMgmt::ClassAlloc<AdmMsgDeleteClusterRsp> > AdmMsgDeleteClusterRsp::poolT(10, "AdmMsgDeleteClusterRsp");
AdmMsgDeleteClusterRsp::AdmMsgDeleteClusterRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgDeleteClusterRsp::class_msg_type)),
    cluster_id(-1)
{}

AdmMsgDeleteClusterRsp::~AdmMsgDeleteClusterRsp() {}

void AdmMsgDeleteClusterRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
}

void AdmMsgDeleteClusterRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> cluster_id;
}

// STREAM
int AdmMsgDeleteClusterRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgDeleteClusterRsp::unmarshall(buf);

    return ret;
}

int AdmMsgDeleteClusterRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgDeleteClusterRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgDeleteClusterRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgDeleteClusterRsp::toJson()
{
    return json;
}

void AdmMsgDeleteClusterRsp::fromJson(const string &_sjon)
{

}

AdmMsgDeleteClusterRsp *AdmMsgDeleteClusterRsp::get()
{
    return AdmMsgDeleteClusterRsp::poolT.get();
}

void AdmMsgDeleteClusterRsp::put(AdmMsgDeleteClusterRsp *obj)
{
    AdmMsgDeleteClusterRsp::poolT.put(obj);
}

void AdmMsgDeleteClusterRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgDeleteClusterRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgDeleteClusterRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgDeleteClusterRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgDeleteClusterRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgCreateBackupReq::class_msg_type = eMsgCreateBackupReq;
MemMgmt::GeneralPool<AdmMsgCreateBackupReq, MemMgmt::ClassAlloc<AdmMsgCreateBackupReq> > AdmMsgCreateBackupReq::poolT(10, "AdmMsgCreateBackupReq");
AdmMsgCreateBackupReq::AdmMsgCreateBackupReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgCreateBackupReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0),
    sess_thread_id(0),
    backup_engine_idx(0),
    backup_engine_id(0)
{}

AdmMsgCreateBackupReq::~AdmMsgCreateBackupReq() {}

void AdmMsgCreateBackupReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
    buf << sess_thread_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(sess_thread_id)));
    buf << backup_engine_name;
    AddHdrLength(static_cast<uint32_t>(backup_engine_name.length() + sizeof(uint16_t)));
    buf << backup_engine_idx;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_engine_idx)));
    buf << backup_engine_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_engine_id)));
}

void AdmMsgCreateBackupReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
    buf >> sess_thread_id;
    buf >> backup_engine_name;
    buf >> backup_engine_idx;
    buf >> backup_engine_id;
}

// STREAM
int AdmMsgCreateBackupReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCreateBackupReq::unmarshall(buf);

    return ret;
}

int AdmMsgCreateBackupReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCreateBackupReq::send(int fd)
{
    AdmMsgBuffer buf;
    AdmMsgCreateBackupReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCreateBackupReq::toJson()
{
    return json;
}

void AdmMsgCreateBackupReq::fromJson(const string &_sjon)
{

}

AdmMsgCreateBackupReq *AdmMsgCreateBackupReq::get()
{
    return AdmMsgCreateBackupReq::poolT.get();
}

void AdmMsgCreateBackupReq::put(AdmMsgCreateBackupReq *obj)
{
    AdmMsgCreateBackupReq::poolT.put(obj);
}

void AdmMsgCreateBackupReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCreateBackupReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCreateBackupReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCreateBackupReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCreateBackupReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgCreateBackupRsp::class_msg_type = eMsgCreateBackupRsp;
MemMgmt::GeneralPool<AdmMsgCreateBackupRsp, MemMgmt::ClassAlloc<AdmMsgCreateBackupRsp> > AdmMsgCreateBackupRsp::poolT(10, "AdmMsgCreateBackupRsp");
AdmMsgCreateBackupRsp::AdmMsgCreateBackupRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgCreateBackupRsp::class_msg_type)),
    backup_id(0)
{}

AdmMsgCreateBackupRsp::~AdmMsgCreateBackupRsp() {}

void AdmMsgCreateBackupRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << backup_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_id)));
}

void AdmMsgCreateBackupRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> backup_id;
}

// STREAM
int AdmMsgCreateBackupRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCreateBackupRsp::unmarshall(buf);

    return ret;
}

int AdmMsgCreateBackupRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCreateBackupRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgCreateBackupRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCreateBackupRsp::toJson()
{
    return json;
}

void AdmMsgCreateBackupRsp::fromJson(const string &_sjon)
{

}

AdmMsgCreateBackupRsp *AdmMsgCreateBackupRsp::get()
{
    return AdmMsgCreateBackupRsp::poolT.get();
}

void AdmMsgCreateBackupRsp::put(AdmMsgCreateBackupRsp *obj)
{
    AdmMsgCreateBackupRsp::poolT.put(obj);
}

void AdmMsgCreateBackupRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCreateBackupRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCreateBackupRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCreateBackupRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCreateBackupRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgDeleteBackupReq::class_msg_type = eMsgDeleteBackupReq;
MemMgmt::GeneralPool<AdmMsgDeleteBackupReq, MemMgmt::ClassAlloc<AdmMsgDeleteBackupReq> > AdmMsgDeleteBackupReq::poolT(10, "AdmMsgDeleteBackupReq");
AdmMsgDeleteBackupReq::AdmMsgDeleteBackupReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgDeleteBackupReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0),
    sess_thread_id(0),
    backup_id(0)
{}

AdmMsgDeleteBackupReq::~AdmMsgDeleteBackupReq() {}

void AdmMsgDeleteBackupReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << backup_name;
    AddHdrLength(static_cast<uint32_t>(backup_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
    buf << sess_thread_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(sess_thread_id)));
    buf << backup_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_id)));
}

void AdmMsgDeleteBackupReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> backup_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
    buf >> sess_thread_id;
    buf >> backup_id;
}

// STREAM
int AdmMsgDeleteBackupReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgDeleteBackupReq::unmarshall(buf);

    return ret;
}

int AdmMsgDeleteBackupReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgDeleteBackupReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgDeleteBackupReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgDeleteBackupReq::toJson()
{
    return json;
}

void AdmMsgDeleteBackupReq::fromJson(const string &_sjon)
{

}

AdmMsgDeleteBackupReq *AdmMsgDeleteBackupReq::get()
{
    return AdmMsgDeleteBackupReq::poolT.get();
}

void AdmMsgDeleteBackupReq::put(AdmMsgDeleteBackupReq *obj)
{
    AdmMsgDeleteBackupReq::poolT.put(obj);
}

void AdmMsgDeleteBackupReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgDeleteBackupReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgDeleteBackupReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgDeleteBackupReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgDeleteBackupReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgDeleteBackupRsp::class_msg_type = eMsgDeleteBackupRsp;
MemMgmt::GeneralPool<AdmMsgDeleteBackupRsp, MemMgmt::ClassAlloc<AdmMsgDeleteBackupRsp> > AdmMsgDeleteBackupRsp::poolT(10, "AdmMsgDeleteBackupRsp");
AdmMsgDeleteBackupRsp::AdmMsgDeleteBackupRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgDeleteBackupRsp::class_msg_type)),
    backup_id(0)
{}

AdmMsgDeleteBackupRsp::~AdmMsgDeleteBackupRsp() {}

void AdmMsgDeleteBackupRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << backup_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_id)));
}

void AdmMsgDeleteBackupRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> backup_id;
}

// STREAM
int AdmMsgDeleteBackupRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgDeleteBackupRsp::unmarshall(buf);

    return ret;
}

int AdmMsgDeleteBackupRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgDeleteBackupRsp::send(int fd)
{
    AdmMsgBuffer buf;
    AdmMsgDeleteBackupRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgDeleteBackupRsp::toJson()
{
    return json;
}

void AdmMsgDeleteBackupRsp::fromJson(const string &_sjon)
{

}

AdmMsgDeleteBackupRsp *AdmMsgDeleteBackupRsp::get()
{
    return AdmMsgDeleteBackupRsp::poolT.get();
}

void AdmMsgDeleteBackupRsp::put(AdmMsgDeleteBackupRsp *obj)
{
    AdmMsgDeleteBackupRsp::poolT.put(obj);
}

void AdmMsgDeleteBackupRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgDeleteBackupRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgDeleteBackupRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgDeleteBackupRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgDeleteBackupRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgVerifyBackupReq::class_msg_type = eMsgVerifyBackupReq;
MemMgmt::GeneralPool<AdmMsgVerifyBackupReq, MemMgmt::ClassAlloc<AdmMsgVerifyBackupReq> > AdmMsgVerifyBackupReq::poolT(10, "AdmMsgVerifyBackupReq");
AdmMsgVerifyBackupReq::AdmMsgVerifyBackupReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgVerifyBackupReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0),
    sess_thread_id(0),
    backup_id(0)
{}

AdmMsgVerifyBackupReq::~AdmMsgVerifyBackupReq() {}

void AdmMsgVerifyBackupReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << backup_name;
    AddHdrLength(static_cast<uint32_t>(backup_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
    buf << sess_thread_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(sess_thread_id)));
    buf << backup_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_id)));
}

void AdmMsgVerifyBackupReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> backup_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
    buf >> sess_thread_id;
    buf >> backup_id;
}

// STREAM
int AdmMsgVerifyBackupReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgVerifyBackupReq::unmarshall(buf);

    return ret;
}

int AdmMsgVerifyBackupReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgVerifyBackupReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgVerifyBackupReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgVerifyBackupReq::toJson()
{
    return json;
}

void AdmMsgVerifyBackupReq::fromJson(const string &_sjon)
{

}

AdmMsgVerifyBackupReq *AdmMsgVerifyBackupReq::get()
{
    return AdmMsgVerifyBackupReq::poolT.get();
}

void AdmMsgVerifyBackupReq::put(AdmMsgVerifyBackupReq *obj)
{
    AdmMsgVerifyBackupReq::poolT.put(obj);
}

void AdmMsgVerifyBackupReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgVerifyBackupReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgVerifyBackupReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgVerifyBackupReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgVerifyBackupReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgVerifyBackupRsp::class_msg_type = eMsgVerifyBackupRsp;
MemMgmt::GeneralPool<AdmMsgVerifyBackupRsp, MemMgmt::ClassAlloc<AdmMsgVerifyBackupRsp> > AdmMsgVerifyBackupRsp::poolT(10, "AdmMsgVerifyBackupRsp");
AdmMsgVerifyBackupRsp::AdmMsgVerifyBackupRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgVerifyBackupRsp::class_msg_type)),
    backup_id(0)
{}

AdmMsgVerifyBackupRsp::~AdmMsgVerifyBackupRsp() {}

void AdmMsgVerifyBackupRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << backup_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_id)));
}

void AdmMsgVerifyBackupRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> backup_id;
}

// STREAM
int AdmMsgVerifyBackupRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgVerifyBackupRsp::unmarshall(buf);

    return ret;
}

int AdmMsgVerifyBackupRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgVerifyBackupRsp::send(int fd)
{
    AdmMsgBuffer buf;
    AdmMsgVerifyBackupRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgVerifyBackupRsp::toJson()
{
    return json;
}

void AdmMsgVerifyBackupRsp::fromJson(const string &_sjon)
{

}

AdmMsgVerifyBackupRsp *AdmMsgVerifyBackupRsp::get()
{
    return AdmMsgVerifyBackupRsp::poolT.get();
}

void AdmMsgVerifyBackupRsp::put(AdmMsgVerifyBackupRsp *obj)
{
    AdmMsgVerifyBackupRsp::poolT.put(obj);
}

void AdmMsgVerifyBackupRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgVerifyBackupRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgVerifyBackupRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgVerifyBackupRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgVerifyBackupRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgListBackupsReq::class_msg_type = eMsgListBackupsReq;
MemMgmt::GeneralPool<AdmMsgListBackupsReq, MemMgmt::ClassAlloc<AdmMsgListBackupsReq> > AdmMsgListBackupsReq::poolT(10, "AdmMsgListBackupsReq");
AdmMsgListBackupsReq::AdmMsgListBackupsReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgListBackupsReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0)
{}

AdmMsgListBackupsReq::~AdmMsgListBackupsReq() {}

void AdmMsgListBackupsReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
}

void AdmMsgListBackupsReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
}

// STREAM
int AdmMsgListBackupsReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgListBackupsReq::unmarshall(buf);

    return ret;
}

int AdmMsgListBackupsReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgListBackupsReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgListBackupsReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgListBackupsReq::toJson()
{
    return json;
}

void AdmMsgListBackupsReq::fromJson(const string &_sjon)
{

}

AdmMsgListBackupsReq *AdmMsgListBackupsReq::get()
{
    return AdmMsgListBackupsReq::poolT.get();
}

void AdmMsgListBackupsReq::put(AdmMsgListBackupsReq *obj)
{
    AdmMsgListBackupsReq::poolT.put(obj);
}

void AdmMsgListBackupsReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgListBackupsReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgListBackupsReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgListBackupsReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgListBackupsReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgListBackupsRsp::class_msg_type = eMsgListBackupsRsp;
MemMgmt::GeneralPool<AdmMsgListBackupsRsp, MemMgmt::ClassAlloc<AdmMsgListBackupsRsp> > AdmMsgListBackupsRsp::poolT(10, "AdmMsgListBackupsRsp");
AdmMsgListBackupsRsp::AdmMsgListBackupsRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgListBackupsRsp::class_msg_type))
{}

AdmMsgListBackupsRsp::~AdmMsgListBackupsRsp() {}

void AdmMsgListBackupsRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << num_backups;
    AddHdrLength(static_cast<uint32_t>(sizeof(num_backups)));
    uint32_t i = 0;
    for ( auto it = backups.begin() ; it != backups.end() && i < num_backups ; it++, i++ ) {
        buf << it->first;
        AddHdrLength(static_cast<uint32_t>(sizeof(it->first)));
        buf << it->second;
        AddHdrLength(static_cast<uint32_t>(it->second.length() + sizeof(uint16_t)));
    }
}

void AdmMsgListBackupsRsp::unmarshall (AdmMsgBuffer& buf)
{
    string b_name;
    uint64_t b_id;
    AdmMsgRsp::unmarshall(buf);
    buf >> num_backups;
    for ( uint32_t i = 0 ; i < num_backups ; i++ ) {
        buf >> b_id;
        buf >> b_name;
        backups.push_back(pair<uint64_t, string>(b_id, b_name));
    }
}

// STREAM
int AdmMsgListBackupsRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgListBackupsRsp::unmarshall(buf);

    return ret;
}

int AdmMsgListBackupsRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgListBackupsRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf(true);
    AdmMsgListBackupsRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgListBackupsRsp::toJson()
{
    return json;
}

void AdmMsgListBackupsRsp::fromJson(const string &_sjon)
{

}

AdmMsgListBackupsRsp *AdmMsgListBackupsRsp::get()
{
    return AdmMsgListBackupsRsp::poolT.get();
}

void AdmMsgListBackupsRsp::put(AdmMsgListBackupsRsp *obj)
{
    AdmMsgListBackupsRsp::poolT.put(obj);
}

void AdmMsgListBackupsRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgListBackupsRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgListBackupsRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgListBackupsRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgListBackupsRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgCreateSnapshotReq::class_msg_type = eMsgCreateSnapshotReq;
MemMgmt::GeneralPool<AdmMsgCreateSnapshotReq, MemMgmt::ClassAlloc<AdmMsgCreateSnapshotReq> > AdmMsgCreateSnapshotReq::poolT(10, "AdmMsgCreateSnapshotReq");
AdmMsgCreateSnapshotReq::AdmMsgCreateSnapshotReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgCreateSnapshotReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0),
    sess_thread_id(0),
    snapshot_engine_idx(0),
    snapshot_engine_id(0)
{}

AdmMsgCreateSnapshotReq::~AdmMsgCreateSnapshotReq() {}

void AdmMsgCreateSnapshotReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
    buf << sess_thread_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(sess_thread_id)));
    buf << snapshot_engine_name;
    AddHdrLength(static_cast<uint32_t>(snapshot_engine_name.length() + sizeof(uint16_t)));
    buf << snapshot_engine_idx;
    AddHdrLength(static_cast<uint32_t>(sizeof(snapshot_engine_idx)));
    buf << snapshot_engine_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(snapshot_engine_id)));
}

void AdmMsgCreateSnapshotReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
    buf >> sess_thread_id;
    buf >> snapshot_engine_name;
    buf >> snapshot_engine_idx;
    buf >> snapshot_engine_id;
}

// STREAM
int AdmMsgCreateSnapshotReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCreateSnapshotReq::unmarshall(buf);

    return ret;
}

int AdmMsgCreateSnapshotReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCreateSnapshotReq::send(int fd)
{
    AdmMsgBuffer buf;
    AdmMsgCreateSnapshotReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCreateSnapshotReq::toJson()
{
    return json;
}

void AdmMsgCreateSnapshotReq::fromJson(const string &_sjon)
{

}

AdmMsgCreateSnapshotReq *AdmMsgCreateSnapshotReq::get()
{
    return AdmMsgCreateSnapshotReq::poolT.get();
}

void AdmMsgCreateSnapshotReq::put(AdmMsgCreateSnapshotReq *obj)
{
    AdmMsgCreateSnapshotReq::poolT.put(obj);
}

void AdmMsgCreateSnapshotReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCreateSnapshotReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCreateSnapshotReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCreateSnapshotReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCreateSnapshotReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgCreateSnapshotRsp::class_msg_type = eMsgCreateSnapshotRsp;
MemMgmt::GeneralPool<AdmMsgCreateSnapshotRsp, MemMgmt::ClassAlloc<AdmMsgCreateSnapshotRsp> > AdmMsgCreateSnapshotRsp::poolT(10, "AdmMsgCreateSnapshotRsp");
AdmMsgCreateSnapshotRsp::AdmMsgCreateSnapshotRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgCreateSnapshotRsp::class_msg_type)),
    snapshot_id(0)
{}

AdmMsgCreateSnapshotRsp::~AdmMsgCreateSnapshotRsp() {}

void AdmMsgCreateSnapshotRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << snapshot_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(snapshot_id)));
}

void AdmMsgCreateSnapshotRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> snapshot_id;
}

// STREAM
int AdmMsgCreateSnapshotRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCreateSnapshotRsp::unmarshall(buf);

    return ret;
}

int AdmMsgCreateSnapshotRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCreateSnapshotRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgCreateSnapshotRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCreateSnapshotRsp::toJson()
{
    return json;
}

void AdmMsgCreateSnapshotRsp::fromJson(const string &_sjon)
{

}

AdmMsgCreateSnapshotRsp *AdmMsgCreateSnapshotRsp::get()
{
    return AdmMsgCreateSnapshotRsp::poolT.get();
}

void AdmMsgCreateSnapshotRsp::put(AdmMsgCreateSnapshotRsp *obj)
{
    AdmMsgCreateSnapshotRsp::poolT.put(obj);
}

void AdmMsgCreateSnapshotRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCreateSnapshotRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCreateSnapshotRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCreateSnapshotRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCreateSnapshotRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgDeleteSnapshotReq::class_msg_type = eMsgDeleteSnapshotReq;
MemMgmt::GeneralPool<AdmMsgDeleteSnapshotReq, MemMgmt::ClassAlloc<AdmMsgDeleteSnapshotReq> > AdmMsgDeleteSnapshotReq::poolT(10, "AdmMsgDeleteSnapshotReq");
AdmMsgDeleteSnapshotReq::AdmMsgDeleteSnapshotReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgDeleteSnapshotReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0),
    sess_thread_id(0),
    snapshot_id(0)
{}

AdmMsgDeleteSnapshotReq::~AdmMsgDeleteSnapshotReq() {}

void AdmMsgDeleteSnapshotReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << snapshot_name;
    AddHdrLength(static_cast<uint32_t>(snapshot_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
    buf << sess_thread_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(sess_thread_id)));
    buf << snapshot_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(snapshot_id)));
}

void AdmMsgDeleteSnapshotReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> snapshot_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
    buf >> sess_thread_id;
    buf >> snapshot_id;
}

// STREAM
int AdmMsgDeleteSnapshotReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgDeleteSnapshotReq::unmarshall(buf);

    return ret;
}

int AdmMsgDeleteSnapshotReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgDeleteSnapshotReq::send(int fd)
{
    AdmMsgBuffer buf;
    AdmMsgDeleteSnapshotReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgDeleteSnapshotReq::toJson()
{
    return json;
}

void AdmMsgDeleteSnapshotReq::fromJson(const string &_sjon)
{

}

AdmMsgDeleteSnapshotReq *AdmMsgDeleteSnapshotReq::get()
{
    return AdmMsgDeleteSnapshotReq::poolT.get();
}

void AdmMsgDeleteSnapshotReq::put(AdmMsgDeleteSnapshotReq *obj)
{
    AdmMsgDeleteSnapshotReq::poolT.put(obj);
}

void AdmMsgDeleteSnapshotReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgDeleteSnapshotReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgDeleteSnapshotReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgDeleteSnapshotReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgDeleteSnapshotReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgDeleteSnapshotRsp::class_msg_type = eMsgDeleteSnapshotRsp;
MemMgmt::GeneralPool<AdmMsgDeleteSnapshotRsp, MemMgmt::ClassAlloc<AdmMsgDeleteSnapshotRsp> > AdmMsgDeleteSnapshotRsp::poolT(10, "AdmMsgDeleteSnapshotRsp");
AdmMsgDeleteSnapshotRsp::AdmMsgDeleteSnapshotRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgDeleteSnapshotRsp::class_msg_type)),
    snapshot_id(0)
{}

AdmMsgDeleteSnapshotRsp::~AdmMsgDeleteSnapshotRsp() {}

void AdmMsgDeleteSnapshotRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << snapshot_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(snapshot_id)));
}

void AdmMsgDeleteSnapshotRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> snapshot_id;
}

// STREAM
int AdmMsgDeleteSnapshotRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgDeleteSnapshotRsp::unmarshall(buf);

    return ret;
}

int AdmMsgDeleteSnapshotRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgDeleteSnapshotRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgDeleteSnapshotRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgDeleteSnapshotRsp::toJson()
{
    return json;
}

void AdmMsgDeleteSnapshotRsp::fromJson(const string &_sjon)
{

}

AdmMsgDeleteSnapshotRsp *AdmMsgDeleteSnapshotRsp::get()
{
    return AdmMsgDeleteSnapshotRsp::poolT.get();
}

void AdmMsgDeleteSnapshotRsp::put(AdmMsgDeleteSnapshotRsp *obj)
{
    AdmMsgDeleteSnapshotRsp::poolT.put(obj);
}

void AdmMsgDeleteSnapshotRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgDeleteSnapshotRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgDeleteSnapshotRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgDeleteSnapshotRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgDeleteSnapshotRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgListSnapshotsReq::class_msg_type = eMsgListSnapshotsReq;
MemMgmt::GeneralPool<AdmMsgListSnapshotsReq, MemMgmt::ClassAlloc<AdmMsgListSnapshotsReq> > AdmMsgListSnapshotsReq::poolT(10, "AdmMsgListSnapshotpsReq");
AdmMsgListSnapshotsReq::AdmMsgListSnapshotsReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgListSnapshotsReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0)
{}

AdmMsgListSnapshotsReq::~AdmMsgListSnapshotsReq() {}

void AdmMsgListSnapshotsReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
}

void AdmMsgListSnapshotsReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
}

// STREAM
int AdmMsgListSnapshotsReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgListSnapshotsReq::unmarshall(buf);

    return ret;
}

int AdmMsgListSnapshotsReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgListSnapshotsReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgListSnapshotsReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgListSnapshotsReq::toJson()
{
    return json;
}

void AdmMsgListSnapshotsReq::fromJson(const string &_sjon)
{

}

AdmMsgListSnapshotsReq *AdmMsgListSnapshotsReq::get()
{
    return AdmMsgListSnapshotsReq::poolT.get();
}

void AdmMsgListSnapshotsReq::put(AdmMsgListSnapshotsReq *obj)
{
    AdmMsgListSnapshotsReq::poolT.put(obj);
}

void AdmMsgListSnapshotsReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgListSnapshotsReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgListSnapshotsReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgListSnapshotsReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgListSnapshotsReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgListSnapshotsRsp::class_msg_type = eMsgListSnapshotsRsp;
MemMgmt::GeneralPool<AdmMsgListSnapshotsRsp, MemMgmt::ClassAlloc<AdmMsgListSnapshotsRsp> > AdmMsgListSnapshotsRsp::poolT(10, "AdmMsgListSnapshotsRsp");
AdmMsgListSnapshotsRsp::AdmMsgListSnapshotsRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgListSnapshotsRsp::class_msg_type))
{}

AdmMsgListSnapshotsRsp::~AdmMsgListSnapshotsRsp() {}

void AdmMsgListSnapshotsRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << num_snapshots;
    AddHdrLength(static_cast<uint32_t>(sizeof(num_snapshots)));
    uint32_t i = 0;
    for ( auto it = snapshots.begin() ; it != snapshots.end() && i < num_snapshots ; it++, i++ ) {
        buf << it->first;
        AddHdrLength(static_cast<uint32_t>(sizeof(it->first)));
        buf << it->second;
        AddHdrLength(static_cast<uint32_t>(it->second.length() + sizeof(uint16_t)));
    }
}

void AdmMsgListSnapshotsRsp::unmarshall (AdmMsgBuffer& buf)
{
    string s_name;
    uint64_t s_id;
    AdmMsgRsp::unmarshall(buf);
    buf >> num_snapshots;
    for ( uint32_t i = 0 ; i < num_snapshots ; i++ ) {
        buf >> s_id;
        buf >> s_name;
        snapshots.push_back(pair<uint64_t, string>(s_id, s_name));
    }
}

// STREAM
int AdmMsgListSnapshotsRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgListSnapshotsRsp::unmarshall(buf);

    return ret;
}

int AdmMsgListSnapshotsRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgListSnapshotsRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf(true);
    AdmMsgListSnapshotsRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgListSnapshotsRsp::toJson()
{
    return json;
}

void AdmMsgListSnapshotsRsp::fromJson(const string &_sjon)
{

}

AdmMsgListSnapshotsRsp *AdmMsgListSnapshotsRsp::get()
{
    return AdmMsgListSnapshotsRsp::poolT.get();
}

void AdmMsgListSnapshotsRsp::put(AdmMsgListSnapshotsRsp *obj)
{
    AdmMsgListSnapshotsRsp::poolT.put(obj);
}

void AdmMsgListSnapshotsRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgListSnapshotsRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgListSnapshotsRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgListSnapshotsRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgListSnapshotsRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgOpenStoreReq::class_msg_type = eMsgOpenStoreReq;
MemMgmt::GeneralPool<AdmMsgOpenStoreReq, MemMgmt::ClassAlloc<AdmMsgOpenStoreReq> > AdmMsgOpenStoreReq::poolT(10, "AdmMsgOpenStoreReq");
AdmMsgOpenStoreReq::AdmMsgOpenStoreReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgOpenStoreReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0)
{}

AdmMsgOpenStoreReq::~AdmMsgOpenStoreReq() {}

void AdmMsgOpenStoreReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
}

void AdmMsgOpenStoreReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
}

// STREAM
int AdmMsgOpenStoreReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgOpenStoreReq::unmarshall(buf);

    return ret;
}

int AdmMsgOpenStoreReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgOpenStoreReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgOpenStoreReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgOpenStoreReq::toJson()
{
    return json;
}

void AdmMsgOpenStoreReq::fromJson(const string &_sjon)
{

}

AdmMsgOpenStoreReq *AdmMsgOpenStoreReq::get()
{
    return AdmMsgOpenStoreReq::poolT.get();
}

void AdmMsgOpenStoreReq::put(AdmMsgOpenStoreReq *obj)
{
    AdmMsgOpenStoreReq::poolT.put(obj);
}

void AdmMsgOpenStoreReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgOpenStoreReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgOpenStoreReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgOpenStoreReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgOpenStoreReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgOpenStoreRsp::class_msg_type = eMsgOpenStoreRsp;
MemMgmt::GeneralPool<AdmMsgOpenStoreRsp, MemMgmt::ClassAlloc<AdmMsgOpenStoreRsp> > AdmMsgOpenStoreRsp::poolT(10, "AdmMsgOpenStoreRsp");
AdmMsgOpenStoreRsp::AdmMsgOpenStoreRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgOpenStoreRsp::class_msg_type))
{}

AdmMsgOpenStoreRsp::~AdmMsgOpenStoreRsp() {}

void AdmMsgOpenStoreRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
}

void AdmMsgOpenStoreRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> store_id;
}

// STREAM
int AdmMsgOpenStoreRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgOpenStoreRsp::unmarshall(buf);

    return ret;
}

int AdmMsgOpenStoreRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgOpenStoreRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgOpenStoreRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgOpenStoreRsp::toJson()
{
    return json;
}

void AdmMsgOpenStoreRsp::fromJson(const string &_sjon)
{

}

AdmMsgOpenStoreRsp *AdmMsgOpenStoreRsp::get()
{
    return AdmMsgOpenStoreRsp::poolT.get();
}

void AdmMsgOpenStoreRsp::put(AdmMsgOpenStoreRsp *obj)
{
    AdmMsgOpenStoreRsp::poolT.put(obj);
}

void AdmMsgOpenStoreRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgOpenStoreRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgOpenStoreRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgOpenStoreRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgOpenStoreRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgCloseStoreReq::class_msg_type = eMsgCloseStoreReq;
MemMgmt::GeneralPool<AdmMsgCloseStoreReq, MemMgmt::ClassAlloc<AdmMsgCloseStoreReq> > AdmMsgCloseStoreReq::poolT(10, "AdmMsgCloseStoreReq");
AdmMsgCloseStoreReq::AdmMsgCloseStoreReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgCloseStoreReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0)
{}

AdmMsgCloseStoreReq::~AdmMsgCloseStoreReq() {}

void AdmMsgCloseStoreReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
}

void AdmMsgCloseStoreReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
}

// STREAM
int AdmMsgCloseStoreReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCloseStoreReq::unmarshall(buf);

    return ret;
}

int AdmMsgCloseStoreReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCloseStoreReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgCloseStoreReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCloseStoreReq::toJson()
{
    return json;
}

void AdmMsgCloseStoreReq::fromJson(const string &_sjon)
{

}

AdmMsgCloseStoreReq *AdmMsgCloseStoreReq::get()
{
    return AdmMsgCloseStoreReq::poolT.get();
}

void AdmMsgCloseStoreReq::put(AdmMsgCloseStoreReq *obj)
{
    AdmMsgCloseStoreReq::poolT.put(obj);
}

void AdmMsgCloseStoreReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCloseStoreReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCloseStoreReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCloseStoreReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCloseStoreReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgCloseStoreRsp::class_msg_type = eMsgCloseStoreRsp;
MemMgmt::GeneralPool<AdmMsgCloseStoreRsp, MemMgmt::ClassAlloc<AdmMsgCloseStoreRsp> > AdmMsgCloseStoreRsp::poolT(10, "AdmMsgCloseStoreRsp");
AdmMsgCloseStoreRsp::AdmMsgCloseStoreRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgCloseStoreRsp::class_msg_type))
{}

AdmMsgCloseStoreRsp::~AdmMsgCloseStoreRsp() {}

void AdmMsgCloseStoreRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
}

void AdmMsgCloseStoreRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> store_id;
}

// STREAM
int AdmMsgCloseStoreRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCloseStoreRsp::unmarshall(buf);

    return ret;
}

int AdmMsgCloseStoreRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCloseStoreRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgCloseStoreRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCloseStoreRsp::toJson()
{
    return json;
}

void AdmMsgCloseStoreRsp::fromJson(const string &_sjon)
{

}

AdmMsgCloseStoreRsp *AdmMsgCloseStoreRsp::get()
{
    return AdmMsgCloseStoreRsp::poolT.get();
}

void AdmMsgCloseStoreRsp::put(AdmMsgCloseStoreRsp *obj)
{
    AdmMsgCloseStoreRsp::poolT.put(obj);
}

void AdmMsgCloseStoreRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCloseStoreRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCloseStoreRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCloseStoreRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCloseStoreRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgListClustersReq::class_msg_type = eMsgListClustersReq;
MemMgmt::GeneralPool<AdmMsgListClustersReq, MemMgmt::ClassAlloc<AdmMsgListClustersReq> > AdmMsgListClustersReq::poolT(10, "AdmMsgListClustersReq");
AdmMsgListClustersReq::AdmMsgListClustersReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgListClustersReq::class_msg_type)),
    backup_req(0)
{}

AdmMsgListClustersReq::~AdmMsgListClustersReq() {}

void AdmMsgListClustersReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << backup_path;
    AddHdrLength(static_cast<uint32_t>(backup_path.length() + sizeof(uint16_t)));
    buf << backup_req;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_req)));
}

void AdmMsgListClustersReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> backup_path;
    buf >> backup_req;
}

// STREAM
int AdmMsgListClustersReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgListClustersReq::unmarshall(buf);

    return ret;
}

int AdmMsgListClustersReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgListClustersReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgListClustersReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgListClustersReq::toJson()
{
    return json;
}

void AdmMsgListClustersReq::fromJson(const string &_sjon)
{

}

AdmMsgListClustersReq *AdmMsgListClustersReq::get()
{
    return AdmMsgListClustersReq::poolT.get();
}

void AdmMsgListClustersReq::put(AdmMsgListClustersReq *obj)
{
    AdmMsgListClustersReq::poolT.put(obj);
}

void AdmMsgListClustersReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgListClustersReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgListClustersReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgListClustersReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgListClustersReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgListClustersRsp::class_msg_type = eMsgListClustersRsp;
MemMgmt::GeneralPool<AdmMsgListClustersRsp, MemMgmt::ClassAlloc<AdmMsgListClustersRsp> > AdmMsgListClustersRsp::poolT(10, "AdmMsgListClustersRsp");
AdmMsgListClustersRsp::AdmMsgListClustersRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgListClustersRsp::class_msg_type)),
    num_clusters(0)
{}

AdmMsgListClustersRsp::~AdmMsgListClustersRsp() {}

void AdmMsgListClustersRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << num_clusters;
    AddHdrLength(static_cast<uint32_t>(sizeof(num_clusters)));
    uint32_t i = 0;
    for ( auto it = clusters.begin() ; it != clusters.end() && i < num_clusters ; it++, i++ ) {
        buf << it->first;
        AddHdrLength(static_cast<uint32_t>(sizeof(it->first)));
        buf << it->second;
        AddHdrLength(static_cast<uint32_t>(it->second.length() + sizeof(uint16_t)));
    }
}

void AdmMsgListClustersRsp::unmarshall (AdmMsgBuffer& buf)
{
    string b_name;
    uint64_t b_id;
    AdmMsgRsp::unmarshall(buf);
    buf >> num_clusters;
    for ( uint32_t i = 0 ; i < num_clusters ; i++ ) {
        buf >> b_id;
        buf >> b_name;
        clusters.push_back(pair<uint64_t, string>(b_id, b_name));
    }
}

// STREAM
int AdmMsgListClustersRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgListClustersRsp::unmarshall(buf);

    return ret;
}

int AdmMsgListClustersRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgListClustersRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf(true);
    AdmMsgListClustersRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgListClustersRsp::toJson()
{
    return json;
}

void AdmMsgListClustersRsp::fromJson(const string &_sjon)
{

}

AdmMsgListClustersRsp *AdmMsgListClustersRsp::get()
{
    return AdmMsgListClustersRsp::poolT.get();
}

void AdmMsgListClustersRsp::put(AdmMsgListClustersRsp *obj)
{
    AdmMsgListClustersRsp::poolT.put(obj);
}

void AdmMsgListClustersRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgListClustersRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgListClustersRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgListClustersRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgListClustersRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgListSpacesReq::class_msg_type = eMsgListSpacesReq;
MemMgmt::GeneralPool<AdmMsgListSpacesReq, MemMgmt::ClassAlloc<AdmMsgListSpacesReq> > AdmMsgListSpacesReq::poolT(10, "AdmMsgListSpacesReq");
AdmMsgListSpacesReq::AdmMsgListSpacesReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgListSpacesReq::class_msg_type)),
    cluster_id(0),
    backup_req(0)
{}

AdmMsgListSpacesReq::~AdmMsgListSpacesReq() {}

void AdmMsgListSpacesReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << backup_path;
    AddHdrLength(static_cast<uint32_t>(backup_path.length() + sizeof(uint16_t)));
    buf << backup_req;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_req)));
}

void AdmMsgListSpacesReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> cluster_id;
    buf >> backup_path;
    buf >> backup_req;
}

// STREAM
int AdmMsgListSpacesReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgListSpacesReq::unmarshall(buf);

    return ret;
}

int AdmMsgListSpacesReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgListSpacesReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgListSpacesReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgListSpacesReq::toJson()
{
    return json;
}

void AdmMsgListSpacesReq::fromJson(const string &_sjon)
{

}

AdmMsgListSpacesReq *AdmMsgListSpacesReq::get()
{
    return AdmMsgListSpacesReq::poolT.get();
}

void AdmMsgListSpacesReq::put(AdmMsgListSpacesReq *obj)
{
    AdmMsgListSpacesReq::poolT.put(obj);
}

void AdmMsgListSpacesReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgListSpacesReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgListSpacesReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgListSpacesReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgListSpacesReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgListSpacesRsp::class_msg_type = eMsgListSpacesRsp;
MemMgmt::GeneralPool<AdmMsgListSpacesRsp, MemMgmt::ClassAlloc<AdmMsgListSpacesRsp> > AdmMsgListSpacesRsp::poolT(10, "AdmMsgListSpacesRsp");
AdmMsgListSpacesRsp::AdmMsgListSpacesRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgListSpacesRsp::class_msg_type)),
    num_spaces(0)
{}

AdmMsgListSpacesRsp::~AdmMsgListSpacesRsp() {}

void AdmMsgListSpacesRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << num_spaces;
    AddHdrLength(static_cast<uint32_t>(sizeof(num_spaces)));
    uint32_t i = 0;
    for ( auto it = spaces.begin() ; it != spaces.end() && i < num_spaces ; it++, i++ ) {
        buf << it->first;
        AddHdrLength(static_cast<uint32_t>(sizeof(it->first)));
        buf << it->second;
        AddHdrLength(static_cast<uint32_t>(it->second.length() + sizeof(uint16_t)));
    }
}

void AdmMsgListSpacesRsp::unmarshall (AdmMsgBuffer& buf)
{
    string b_name;
    uint64_t b_id;
    AdmMsgRsp::unmarshall(buf);
    buf >> num_spaces;
    for ( uint32_t i = 0 ; i < num_spaces ; i++ ) {
        buf >> b_id;
        buf >> b_name;
        spaces.push_back(pair<uint64_t, string>(b_id, b_name));
    }
}

// STREAM
int AdmMsgListSpacesRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgListSpacesRsp::unmarshall(buf);

    return ret;
}

int AdmMsgListSpacesRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgListSpacesRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf(true);
    AdmMsgListSpacesRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgListSpacesRsp::toJson()
{
    return json;
}

void AdmMsgListSpacesRsp::fromJson(const string &_sjon)
{

}

AdmMsgListSpacesRsp *AdmMsgListSpacesRsp::get()
{
    return AdmMsgListSpacesRsp::poolT.get();
}

void AdmMsgListSpacesRsp::put(AdmMsgListSpacesRsp *obj)
{
    AdmMsgListSpacesRsp::poolT.put(obj);
}

void AdmMsgListSpacesRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgListSpacesRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgListSpacesRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgListSpacesRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgListSpacesRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgListStoresReq::class_msg_type = eMsgListStoresReq;
MemMgmt::GeneralPool<AdmMsgListStoresReq, MemMgmt::ClassAlloc<AdmMsgListStoresReq> > AdmMsgListStoresReq::poolT(10, "AdmMsgListStoresReq");
AdmMsgListStoresReq::AdmMsgListStoresReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgListStoresReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    backup_req(0)
{}

AdmMsgListStoresReq::~AdmMsgListStoresReq() {}

void AdmMsgListStoresReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << backup_path;
    AddHdrLength(static_cast<uint32_t>(backup_path.length() + sizeof(uint16_t)));
    buf << backup_req;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_req)));
}

void AdmMsgListStoresReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> backup_path;
    buf >> backup_req;
}

// STREAM
int AdmMsgListStoresReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgListStoresReq::unmarshall(buf);

    return ret;
}

int AdmMsgListStoresReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgListStoresReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgListStoresReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgListStoresReq::toJson()
{
    return json;
}

void AdmMsgListStoresReq::fromJson(const string &_sjon)
{

}

AdmMsgListStoresReq *AdmMsgListStoresReq::get()
{
    return AdmMsgListStoresReq::poolT.get();
}

void AdmMsgListStoresReq::put(AdmMsgListStoresReq *obj)
{
    AdmMsgListStoresReq::poolT.put(obj);
}

void AdmMsgListStoresReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgListStoresReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgListStoresReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgListStoresReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgListStoresReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgListStoresRsp::class_msg_type = eMsgListStoresRsp;
MemMgmt::GeneralPool<AdmMsgListStoresRsp, MemMgmt::ClassAlloc<AdmMsgListStoresRsp> > AdmMsgListStoresRsp::poolT(10, "AdmMsgListStoresRsp");
AdmMsgListStoresRsp::AdmMsgListStoresRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgListStoresRsp::class_msg_type)),
    num_stores(0)
{}

AdmMsgListStoresRsp::~AdmMsgListStoresRsp() {}

void AdmMsgListStoresRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << num_stores;
    AddHdrLength(static_cast<uint32_t>(sizeof(num_stores)));
    uint32_t i = 0;
    for ( auto it = stores.begin() ; it != stores.end() && i < num_stores ; it++, i++ ) {
        buf << it->first;
        AddHdrLength(static_cast<uint32_t>(sizeof(it->first)));
        buf << it->second;
        AddHdrLength(static_cast<uint32_t>(it->second.length() + sizeof(uint16_t)));
    }
}

void AdmMsgListStoresRsp::unmarshall (AdmMsgBuffer& buf)
{
    string b_name;
    uint64_t b_id;
    AdmMsgRsp::unmarshall(buf);
    buf >> num_stores;
    for ( uint32_t i = 0 ; i < num_stores ; i++ ) {
        buf >> b_id;
        buf >> b_name;
        stores.push_back(pair<uint64_t, string>(b_id, b_name));
    }
}

// STREAM
int AdmMsgListStoresRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgListStoresRsp::unmarshall(buf);

    return ret;
}

int AdmMsgListStoresRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgListStoresRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf(true);
    AdmMsgListStoresRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgListStoresRsp::toJson()
{
    return json;
}

void AdmMsgListStoresRsp::fromJson(const string &_sjon)
{

}

AdmMsgListStoresRsp *AdmMsgListStoresRsp::get()
{
    return AdmMsgListStoresRsp::poolT.get();
}

void AdmMsgListStoresRsp::put(AdmMsgListStoresRsp *obj)
{
    AdmMsgListStoresRsp::poolT.put(obj);
}

void AdmMsgListStoresRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgListStoresRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgListStoresRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgListStoresRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgListStoresRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgListSubsetsReq::class_msg_type = eMsgListSubsetsReq;
MemMgmt::GeneralPool<AdmMsgListSubsetsReq, MemMgmt::ClassAlloc<AdmMsgListSubsetsReq> > AdmMsgListSubsetsReq::poolT(10, "AdmMsgListSubsetsReq");
AdmMsgListSubsetsReq::AdmMsgListSubsetsReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgListSubsetsReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0)
{}

AdmMsgListSubsetsReq::~AdmMsgListSubsetsReq() {}

void AdmMsgListSubsetsReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
}

void AdmMsgListSubsetsReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
}

// STREAM
int AdmMsgListSubsetsReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgListSubsetsReq::unmarshall(buf);

    return ret;
}

int AdmMsgListSubsetsReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgListSubsetsReq::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgListSubsetsReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgListSubsetsReq::toJson()
{
    return json;
}

void AdmMsgListSubsetsReq::fromJson(const string &_sjon)
{

}

AdmMsgListSubsetsReq *AdmMsgListSubsetsReq::get()
{
    return AdmMsgListSubsetsReq::poolT.get();
}

void AdmMsgListSubsetsReq::put(AdmMsgListSubsetsReq *obj)
{
    AdmMsgListSubsetsReq::poolT.put(obj);
}

void AdmMsgListSubsetsReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgListSubsetsReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgListSubsetsReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgListSubsetsReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgListSubsetsReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgListSubsetsRsp::class_msg_type = eMsgListSubsetsRsp;
MemMgmt::GeneralPool<AdmMsgListSubsetsRsp, MemMgmt::ClassAlloc<AdmMsgListSubsetsRsp> > AdmMsgListSubsetsRsp::poolT(10, "AdmMsgListSubsetsRsp");
AdmMsgListSubsetsRsp::AdmMsgListSubsetsRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgListSubsetsRsp::class_msg_type)),
    num_subsets(0)
{}

AdmMsgListSubsetsRsp::~AdmMsgListSubsetsRsp() {}

void AdmMsgListSubsetsRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << num_subsets;
    AddHdrLength(static_cast<uint32_t>(sizeof(num_subsets)));
    uint32_t i = 0;
    for ( auto it = subsets.begin() ; it != subsets.end() && i < num_subsets ; it++, i++ ) {
        buf << it->first;
        AddHdrLength(static_cast<uint32_t>(sizeof(it->first)));
        buf << it->second;
        AddHdrLength(static_cast<uint32_t>(it->second.length() + sizeof(uint16_t)));
    }
}

void AdmMsgListSubsetsRsp::unmarshall (AdmMsgBuffer& buf)
{
    string b_name;
    uint64_t b_id;
    AdmMsgRsp::unmarshall(buf);
    buf >> num_subsets;
    for ( uint32_t i = 0 ; i < num_subsets ; i++ ) {
        buf >> b_id;
        buf >> b_name;
        subsets.push_back(pair<uint64_t, string>(b_id, b_name));
    }
}

// STREAM
int AdmMsgListSubsetsRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgListSubsetsRsp::unmarshall(buf);

    return ret;
}

int AdmMsgListSubsetsRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgListSubsetsRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf(true);
    AdmMsgListSubsetsRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgListSubsetsRsp::toJson()
{
    return json;
}

void AdmMsgListSubsetsRsp::fromJson(const string &_sjon)
{

}

AdmMsgListSubsetsRsp *AdmMsgListSubsetsRsp::get()
{
    return AdmMsgListSubsetsRsp::poolT.get();
}

void AdmMsgListSubsetsRsp::put(AdmMsgListSubsetsRsp *obj)
{
    AdmMsgListSubsetsRsp::poolT.put(obj);
}

void AdmMsgListSubsetsRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgListSubsetsRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgListSubsetsRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgListSubsetsRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgListSubsetsRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgCreateBackupEngineReq::class_msg_type = eMsgCreateBackupEngineReq;
MemMgmt::GeneralPool<AdmMsgCreateBackupEngineReq, MemMgmt::ClassAlloc<AdmMsgCreateBackupEngineReq> > AdmMsgCreateBackupEngineReq::poolT(10, "AdmMsgCreateBackupEngineReq");
AdmMsgCreateBackupEngineReq::AdmMsgCreateBackupEngineReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgCreateBackupEngineReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0)
{}

AdmMsgCreateBackupEngineReq::~AdmMsgCreateBackupEngineReq() {}

void AdmMsgCreateBackupEngineReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
    buf << backup_engine_name;
    AddHdrLength(static_cast<uint32_t>(backup_engine_name.length() + sizeof(uint16_t)));
    buf << backup_cluster_name;
    AddHdrLength(static_cast<uint32_t>(backup_cluster_name.length() + sizeof(uint16_t)));
    buf << backup_space_name;
    AddHdrLength(static_cast<uint32_t>(backup_space_name.length() + sizeof(uint16_t)));
    buf << backup_store_name;
    AddHdrLength(static_cast<uint32_t>(backup_store_name.length() + sizeof(uint16_t)));
}

void AdmMsgCreateBackupEngineReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
    buf >> backup_engine_name;
    buf >> backup_cluster_name;
    buf >> backup_space_name;
    buf >> backup_store_name;
}

// STREAM
int AdmMsgCreateBackupEngineReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCreateBackupEngineReq::unmarshall(buf);

    return ret;
}

int AdmMsgCreateBackupEngineReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCreateBackupEngineReq::send(int fd)
{
    AdmMsgBuffer buf;
    AdmMsgCreateBackupEngineReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCreateBackupEngineReq::toJson()
{
    return json;
}

void AdmMsgCreateBackupEngineReq::fromJson(const string &_sjon)
{

}

AdmMsgCreateBackupEngineReq *AdmMsgCreateBackupEngineReq::get()
{
    return AdmMsgCreateBackupEngineReq::poolT.get();
}

void AdmMsgCreateBackupEngineReq::put(AdmMsgCreateBackupEngineReq *obj)
{
    AdmMsgCreateBackupEngineReq::poolT.put(obj);
}

void AdmMsgCreateBackupEngineReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCreateBackupEngineReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCreateBackupEngineReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCreateBackupEngineReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCreateBackupEngineReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgCreateBackupEngineRsp::class_msg_type = eMsgCreateBackupEngineRsp;
MemMgmt::GeneralPool<AdmMsgCreateBackupEngineRsp, MemMgmt::ClassAlloc<AdmMsgCreateBackupEngineRsp> > AdmMsgCreateBackupEngineRsp::poolT(10, "AdmMsgCreateBackupEngineRsp");
AdmMsgCreateBackupEngineRsp::AdmMsgCreateBackupEngineRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgCreateBackupEngineRsp::class_msg_type)),
    backup_engine_id(0),
    backup_engine_idx(0)
{}

AdmMsgCreateBackupEngineRsp::~AdmMsgCreateBackupEngineRsp() {}

void AdmMsgCreateBackupEngineRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << backup_engine_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_engine_id)));
    buf << backup_engine_idx;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_engine_idx)));
}

void AdmMsgCreateBackupEngineRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> backup_engine_id;
    buf >> backup_engine_idx;
}

// STREAM
int AdmMsgCreateBackupEngineRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCreateBackupEngineRsp::unmarshall(buf);

    return ret;
}

int AdmMsgCreateBackupEngineRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCreateBackupEngineRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgCreateBackupEngineRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCreateBackupEngineRsp::toJson()
{
    return json;
}

void AdmMsgCreateBackupEngineRsp::fromJson(const string &_sjon)
{

}

AdmMsgCreateBackupEngineRsp *AdmMsgCreateBackupEngineRsp::get()
{
    return AdmMsgCreateBackupEngineRsp::poolT.get();
}

void AdmMsgCreateBackupEngineRsp::put(AdmMsgCreateBackupEngineRsp *obj)
{
    AdmMsgCreateBackupEngineRsp::poolT.put(obj);
}

void AdmMsgCreateBackupEngineRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCreateBackupEngineRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCreateBackupEngineRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCreateBackupEngineRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCreateBackupEngineRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgDeleteBackupEngineReq::class_msg_type = eMsgDeleteBackupEngineReq;
MemMgmt::GeneralPool<AdmMsgDeleteBackupEngineReq, MemMgmt::ClassAlloc<AdmMsgDeleteBackupEngineReq> > AdmMsgDeleteBackupEngineReq::poolT(10, "AdmMsgDeleteBackupEngineReq");
AdmMsgDeleteBackupEngineReq::AdmMsgDeleteBackupEngineReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgDeleteBackupEngineReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0),
    backup_engine_id(0),
    backup_engine_idx(0)
{}

AdmMsgDeleteBackupEngineReq::~AdmMsgDeleteBackupEngineReq() {}

void AdmMsgDeleteBackupEngineReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << backup_engine_name;
    AddHdrLength(static_cast<uint32_t>(backup_engine_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
    buf << backup_engine_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_engine_id)));
    buf << backup_engine_idx;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_engine_idx)));
}

void AdmMsgDeleteBackupEngineReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> backup_engine_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
    buf >> backup_engine_id;
    buf >> backup_engine_idx;
}

// STREAM
int AdmMsgDeleteBackupEngineReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgDeleteBackupEngineReq::unmarshall(buf);

    return ret;
}

int AdmMsgDeleteBackupEngineReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgDeleteBackupEngineReq::send(int fd)
{
    AdmMsgBuffer buf;
    AdmMsgDeleteBackupEngineReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgDeleteBackupEngineReq::toJson()
{
    return json;
}

void AdmMsgDeleteBackupEngineReq::fromJson(const string &_sjon)
{

}

AdmMsgDeleteBackupEngineReq *AdmMsgDeleteBackupEngineReq::get()
{
    return AdmMsgDeleteBackupEngineReq::poolT.get();
}

void AdmMsgDeleteBackupEngineReq::put(AdmMsgDeleteBackupEngineReq *obj)
{
    AdmMsgDeleteBackupEngineReq::poolT.put(obj);
}

void AdmMsgDeleteBackupEngineReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgDeleteBackupEngineReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgDeleteBackupEngineReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgDeleteBackupEngineReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgDeleteBackupEngineReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgDeleteBackupEngineRsp::class_msg_type = eMsgDeleteBackupEngineRsp;
MemMgmt::GeneralPool<AdmMsgDeleteBackupEngineRsp, MemMgmt::ClassAlloc<AdmMsgDeleteBackupEngineRsp> > AdmMsgDeleteBackupEngineRsp::poolT(10, "AdmMsgDeleteBackupEngineRsp");
AdmMsgDeleteBackupEngineRsp::AdmMsgDeleteBackupEngineRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgDeleteBackupEngineRsp::class_msg_type)),
    backup_engine_id(0),
    backup_engine_idx(0)

{}

AdmMsgDeleteBackupEngineRsp::~AdmMsgDeleteBackupEngineRsp() {}

void AdmMsgDeleteBackupEngineRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << backup_engine_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_engine_id)));
    buf << backup_engine_idx;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_engine_idx)));
}

void AdmMsgDeleteBackupEngineRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> backup_engine_id;
    buf >> backup_engine_idx;
}

// STREAM
int AdmMsgDeleteBackupEngineRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgDeleteBackupEngineRsp::unmarshall(buf);

    return ret;
}

int AdmMsgDeleteBackupEngineRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgDeleteBackupEngineRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgDeleteBackupEngineRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgDeleteBackupEngineRsp::toJson()
{
    return json;
}

void AdmMsgDeleteBackupEngineRsp::fromJson(const string &_sjon)
{

}

AdmMsgDeleteBackupEngineRsp *AdmMsgDeleteBackupEngineRsp::get()
{
    return AdmMsgDeleteBackupEngineRsp::poolT.get();
}

void AdmMsgDeleteBackupEngineRsp::put(AdmMsgDeleteBackupEngineRsp *obj)
{
    AdmMsgDeleteBackupEngineRsp::poolT.put(obj);
}

void AdmMsgDeleteBackupEngineRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgDeleteBackupEngineRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgDeleteBackupEngineRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgDeleteBackupEngineRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgDeleteBackupEngineRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgCreateSnapshotEngineReq::class_msg_type = eMsgCreateSnapshotEngineReq;
MemMgmt::GeneralPool<AdmMsgCreateSnapshotEngineReq, MemMgmt::ClassAlloc<AdmMsgCreateSnapshotEngineReq> > AdmMsgCreateSnapshotEngineReq::poolT(10, "AdmMsgCreateSnapshotEngineReq");
AdmMsgCreateSnapshotEngineReq::AdmMsgCreateSnapshotEngineReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgCreateSnapshotEngineReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0)
{}

AdmMsgCreateSnapshotEngineReq::~AdmMsgCreateSnapshotEngineReq() {}

void AdmMsgCreateSnapshotEngineReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
    buf << snapshot_engine_name;
    AddHdrLength(static_cast<uint32_t>(snapshot_engine_name.length() + sizeof(uint16_t)));
}

void AdmMsgCreateSnapshotEngineReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
    buf >> snapshot_engine_name;
}

// STREAM
int AdmMsgCreateSnapshotEngineReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCreateSnapshotEngineReq::unmarshall(buf);

    return ret;
}

int AdmMsgCreateSnapshotEngineReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCreateSnapshotEngineReq::send(int fd)
{
    AdmMsgBuffer buf;
    AdmMsgCreateSnapshotEngineReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCreateSnapshotEngineReq::toJson()
{
    return json;
}

void AdmMsgCreateSnapshotEngineReq::fromJson(const string &_sjon)
{

}

AdmMsgCreateSnapshotEngineReq *AdmMsgCreateSnapshotEngineReq::get()
{
    return AdmMsgCreateSnapshotEngineReq::poolT.get();
}

void AdmMsgCreateSnapshotEngineReq::put(AdmMsgCreateSnapshotEngineReq *obj)
{
    AdmMsgCreateSnapshotEngineReq::poolT.put(obj);
}

void AdmMsgCreateSnapshotEngineReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCreateSnapshotEngineReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCreateSnapshotEngineReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCreateSnapshotEngineReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCreateSnapshotEngineReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgCreateSnapshotEngineRsp::class_msg_type = eMsgCreateSnapshotEngineRsp;
MemMgmt::GeneralPool<AdmMsgCreateSnapshotEngineRsp, MemMgmt::ClassAlloc<AdmMsgCreateSnapshotEngineRsp> > AdmMsgCreateSnapshotEngineRsp::poolT(10, "AdmMsgCreateSnapshotEngineRsp");
AdmMsgCreateSnapshotEngineRsp::AdmMsgCreateSnapshotEngineRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgCreateSnapshotEngineRsp::class_msg_type)),
    snapshot_engine_id(0),
    snapshot_engine_idx(0)
{}

AdmMsgCreateSnapshotEngineRsp::~AdmMsgCreateSnapshotEngineRsp() {}

void AdmMsgCreateSnapshotEngineRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << snapshot_engine_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(snapshot_engine_id)));
    buf << snapshot_engine_idx;
    AddHdrLength(static_cast<uint32_t>(sizeof(snapshot_engine_idx)));
}

void AdmMsgCreateSnapshotEngineRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> snapshot_engine_id;
    buf >> snapshot_engine_idx;
}

// STREAM
int AdmMsgCreateSnapshotEngineRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgCreateSnapshotEngineRsp::unmarshall(buf);

    return ret;
}

int AdmMsgCreateSnapshotEngineRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgCreateSnapshotEngineRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgCreateSnapshotEngineRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgCreateSnapshotEngineRsp::toJson()
{
    return json;
}

void AdmMsgCreateSnapshotEngineRsp::fromJson(const string &_sjon)
{

}

AdmMsgCreateSnapshotEngineRsp *AdmMsgCreateSnapshotEngineRsp::get()
{
    return AdmMsgCreateSnapshotEngineRsp::poolT.get();
}

void AdmMsgCreateSnapshotEngineRsp::put(AdmMsgCreateSnapshotEngineRsp *obj)
{
    AdmMsgCreateSnapshotEngineRsp::poolT.put(obj);
}

void AdmMsgCreateSnapshotEngineRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgCreateSnapshotEngineRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgCreateSnapshotEngineRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgCreateSnapshotEngineRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgCreateSnapshotEngineRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgDeleteSnapshotEngineReq::class_msg_type = eMsgDeleteSnapshotEngineReq;
MemMgmt::GeneralPool<AdmMsgDeleteSnapshotEngineReq, MemMgmt::ClassAlloc<AdmMsgDeleteSnapshotEngineReq> > AdmMsgDeleteSnapshotEngineReq::poolT(10, "AdmMsgDeleteSnapshotEngineReq");
AdmMsgDeleteSnapshotEngineReq::AdmMsgDeleteSnapshotEngineReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgDeleteSnapshotEngineReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0),
    snapshot_engine_id(0),
    snapshot_engine_idx(0)
{}

AdmMsgDeleteSnapshotEngineReq::~AdmMsgDeleteSnapshotEngineReq() {}

void AdmMsgDeleteSnapshotEngineReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << snapshot_engine_name;
    AddHdrLength(static_cast<uint32_t>(snapshot_engine_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
    buf << snapshot_engine_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(snapshot_engine_id)));
    buf << snapshot_engine_idx;
    AddHdrLength(static_cast<uint32_t>(sizeof(snapshot_engine_idx)));
}

void AdmMsgDeleteSnapshotEngineReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> snapshot_engine_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
    buf >> snapshot_engine_id;
    buf >> snapshot_engine_idx;
}

// STREAM
int AdmMsgDeleteSnapshotEngineReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgDeleteSnapshotEngineReq::unmarshall(buf);

    return ret;
}

int AdmMsgDeleteSnapshotEngineReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgDeleteSnapshotEngineReq::send(int fd)
{
    AdmMsgBuffer buf;
    AdmMsgDeleteSnapshotEngineReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgDeleteSnapshotEngineReq::toJson()
{
    return json;
}

void AdmMsgDeleteSnapshotEngineReq::fromJson(const string &_sjon)
{

}

AdmMsgDeleteSnapshotEngineReq *AdmMsgDeleteSnapshotEngineReq::get()
{
    return AdmMsgDeleteSnapshotEngineReq::poolT.get();
}

void AdmMsgDeleteSnapshotEngineReq::put(AdmMsgDeleteSnapshotEngineReq *obj)
{
    AdmMsgDeleteSnapshotEngineReq::poolT.put(obj);
}

void AdmMsgDeleteSnapshotEngineReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgDeleteSnapshotEngineReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgDeleteSnapshotEngineReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgDeleteSnapshotEngineReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgDeleteSnapshotEngineReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgDeleteSnapshotEngineRsp::class_msg_type = eMsgDeleteSnapshotEngineRsp;
MemMgmt::GeneralPool<AdmMsgDeleteSnapshotEngineRsp, MemMgmt::ClassAlloc<AdmMsgDeleteSnapshotEngineRsp> > AdmMsgDeleteSnapshotEngineRsp::poolT(10, "AdmMsgDeleteSnapshotEngineRsp");
AdmMsgDeleteSnapshotEngineRsp::AdmMsgDeleteSnapshotEngineRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgDeleteSnapshotEngineRsp::class_msg_type)),
    snapshot_engine_id(0),
    snapshot_engine_idx(0)
{}

AdmMsgDeleteSnapshotEngineRsp::~AdmMsgDeleteSnapshotEngineRsp() {}

void AdmMsgDeleteSnapshotEngineRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << snapshot_engine_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(snapshot_engine_id)));
    buf << snapshot_engine_idx;
    AddHdrLength(static_cast<uint32_t>(sizeof(snapshot_engine_idx)));
}

void AdmMsgDeleteSnapshotEngineRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> snapshot_engine_id;
    buf >> snapshot_engine_idx;
}

// STREAM
int AdmMsgDeleteSnapshotEngineRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgDeleteSnapshotEngineRsp::unmarshall(buf);

    return ret;
}

int AdmMsgDeleteSnapshotEngineRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgDeleteSnapshotEngineRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgDeleteSnapshotEngineRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgDeleteSnapshotEngineRsp::toJson()
{
    return json;
}

void AdmMsgDeleteSnapshotEngineRsp::fromJson(const string &_sjon)
{

}

AdmMsgDeleteSnapshotEngineRsp *AdmMsgDeleteSnapshotEngineRsp::get()
{
    return AdmMsgDeleteSnapshotEngineRsp::poolT.get();
}

void AdmMsgDeleteSnapshotEngineRsp::put(AdmMsgDeleteSnapshotEngineRsp *obj)
{
    AdmMsgDeleteSnapshotEngineRsp::poolT.put(obj);
}

void AdmMsgDeleteSnapshotEngineRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgDeleteSnapshotEngineRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgDeleteSnapshotEngineRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgDeleteSnapshotEngineRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgDeleteSnapshotEngineRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int AdmMsgRestoreBackupReq::class_msg_type = eMsgRestoreBackupReq;
MemMgmt::GeneralPool<AdmMsgRestoreBackupReq, MemMgmt::ClassAlloc<AdmMsgRestoreBackupReq> > AdmMsgRestoreBackupReq::poolT(10, "AdmMsgRestoreBackupReq");
AdmMsgRestoreBackupReq::AdmMsgRestoreBackupReq()
    :AdmMsgReq(static_cast<eMsgType>(AdmMsgRestoreBackupReq::class_msg_type)),
    cluster_id(0),
    space_id(0),
    store_id(0),
    sess_thread_id(0),
    backup_id(0),
    backup_engine_idx(0)
{}

AdmMsgRestoreBackupReq::~AdmMsgRestoreBackupReq() {}

void AdmMsgRestoreBackupReq::marshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::marshall(buf);
    buf << cluster_name;
    AddHdrLength(static_cast<uint32_t>(cluster_name.length() + sizeof(uint16_t)));
    buf << space_name;
    AddHdrLength(static_cast<uint32_t>(space_name.length() + sizeof(uint16_t)));
    buf << store_name;
    AddHdrLength(static_cast<uint32_t>(store_name.length() + sizeof(uint16_t)));
    buf << cluster_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(cluster_id)));
    buf << space_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(space_id)));
    buf << store_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(store_id)));
    buf << sess_thread_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(sess_thread_id)));
    buf << backup_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_id)));
    buf << backup_engine_idx;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_engine_idx)));
    buf << backup_name;
    AddHdrLength(static_cast<uint32_t>(backup_name.length() + sizeof(uint16_t)));
    buf << restore_cluster_name;
    AddHdrLength(static_cast<uint32_t>(restore_cluster_name.length() + sizeof(uint16_t)));
    buf << restore_space_name;
    AddHdrLength(static_cast<uint32_t>(restore_space_name.length() + sizeof(uint16_t)));
    buf << restore_store_name;
    AddHdrLength(static_cast<uint32_t>(restore_store_name.length() + sizeof(uint16_t)));
}

void AdmMsgRestoreBackupReq::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgReq::unmarshall(buf);
    buf >> cluster_name;
    buf >> space_name;
    buf >> store_name;
    buf >> cluster_id;
    buf >> space_id;
    buf >> store_id;
    buf >> sess_thread_id;
    buf >> backup_id;
    buf >> backup_engine_idx;
    buf >> backup_name;
    buf >> restore_cluster_name;
    buf >> restore_space_name;
    buf >> restore_store_name;
}

// STREAM
int AdmMsgRestoreBackupReq::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgRestoreBackupReq::unmarshall(buf);

    return ret;
}

int AdmMsgRestoreBackupReq::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgRestoreBackupReq::send(int fd)
{
    AdmMsgBuffer buf;
    AdmMsgRestoreBackupReq::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgRestoreBackupReq::toJson()
{
    return json;
}

void AdmMsgRestoreBackupReq::fromJson(const string &_sjon)
{

}

AdmMsgRestoreBackupReq *AdmMsgRestoreBackupReq::get()
{
    return AdmMsgRestoreBackupReq::poolT.get();
}

void AdmMsgRestoreBackupReq::put(AdmMsgRestoreBackupReq *obj)
{
    AdmMsgRestoreBackupReq::poolT.put(obj);
}

void AdmMsgRestoreBackupReq::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgRestoreBackupReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgRestoreBackupReq::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgRestoreBackupReq::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgRestoreBackupReq::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//========//

int AdmMsgRestoreBackupRsp::class_msg_type = eMsgRestoreBackupRsp;
MemMgmt::GeneralPool<AdmMsgRestoreBackupRsp, MemMgmt::ClassAlloc<AdmMsgRestoreBackupRsp> > AdmMsgRestoreBackupRsp::poolT(10, "AdmMsgRestoreBackupRsp");
AdmMsgRestoreBackupRsp::AdmMsgRestoreBackupRsp()
    :AdmMsgRsp(static_cast<eMsgType>(AdmMsgRestoreBackupRsp::class_msg_type)),
    backup_id(0)
{}

AdmMsgRestoreBackupRsp::~AdmMsgRestoreBackupRsp() {}

void AdmMsgRestoreBackupRsp::marshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::marshall(buf);
    buf << backup_id;
    AddHdrLength(static_cast<uint32_t>(sizeof(backup_id)));
}

void AdmMsgRestoreBackupRsp::unmarshall (AdmMsgBuffer& buf)
{
    AdmMsgRsp::unmarshall(buf);
    buf >> backup_id;
}

// STREAM
int AdmMsgRestoreBackupRsp::recv(int fd, AdmMsgBuffer& buf)
{
    int ret = recvExact(fd, hdr.total_length - static_cast<uint32_t>(sizeof(hdr)), buf);
    if ( ret < 0 ) {
        return ret;
    }
    buf.reset();
    AdmMsgRestoreBackupRsp::unmarshall(buf);

    return ret;
}

int AdmMsgRestoreBackupRsp::recv(int fd)
{
    AdmMsgBuffer buf;
    int ret = AdmMsgBase::recv(fd, buf);
    if ( ret < 0 ) {
        return ret;
    }
    return recv(fd, buf);
}

int AdmMsgRestoreBackupRsp::send(int fd)
{
    const_cast<MsgHdr *>(&hdr)->total_length = sizeof(hdr);
    AdmMsgBuffer buf;
    AdmMsgRestoreBackupRsp::marshall(buf);
    buf.reset();
    AdmMsgBase::marshall(buf);
    buf.reset();
    return sendExact(fd, hdr.total_length, buf);
}

string AdmMsgRestoreBackupRsp::toJson()
{
    return json;
}

void AdmMsgRestoreBackupRsp::fromJson(const string &_sjon)
{

}

AdmMsgRestoreBackupRsp *AdmMsgRestoreBackupRsp::get()
{
    return AdmMsgRestoreBackupRsp::poolT.get();
}

void AdmMsgRestoreBackupRsp::put(AdmMsgRestoreBackupRsp *obj)
{
    AdmMsgRestoreBackupRsp::poolT.put(obj);
}

void AdmMsgRestoreBackupRsp::registerPool()
{
    try {
        AdmMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(AdmMsgRestoreBackupRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::GetMsgBaseFunction>(&AdmMsgRestoreBackupRsp::get));
        AdmMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(AdmMsgRestoreBackupRsp::class_msg_type),
            reinterpret_cast<AdmMsgBase::PutMsgBaseFunction>(&AdmMsgRestoreBackupRsp::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

}




