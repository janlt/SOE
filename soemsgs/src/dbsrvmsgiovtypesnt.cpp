/**
 * @file    dbsrviovtypesnt.cpp
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
 * dbsrviovtypesnt.cpp
 *
 *  Created on: Feb 27, 2017
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
#include "dbsrviovmsgsbase.hpp"
#include "dbsrviovmsgs.hpp"
#include "dbsrvmsgiovtypesnt.hpp"
#include "dbsrvmsgfactory.hpp"
#include "dbsrvmsgiovfactory.hpp"

using namespace std;
using namespace MemMgmt;

namespace MetaMsgs {

static IovMsgNTPoolInitializer nt_initalizer;

bool IovMsgNTPoolInitializer::initialized = false;

IovMsgNTPoolInitializer::IovMsgNTPoolInitializer()
{
    if ( initialized != true ) {
        GetReqNT::registerPool();
        GetRspNT::registerPool();
        PutReqNT::registerPool();
        PutRspNT::registerPool();
        DeleteReqNT::registerPool();
        DeleteRspNT::registerPool();
        GetRangeReqNT::registerPool();
        GetRangeRspNT::registerPool();
        MergeReqNT::registerPool();
        MergeRspNT::registerPool();
    }
    initialized = true;
}

// -----------------------------------------------------------------------------------------

int GetReqNT::class_msg_type = eMsgGetReqNT;
MemMgmt::GeneralPool<GetReqNT, MemMgmt::ClassAlloc<GetReqNT> > GetReqNT::poolT(10, "GetReqNT");
GetReqNT::GetReqNT()
    :IovMsgReqNT(static_cast<eMsgType>(GetReqNT::class_msg_type))
{}

GetReqNT::~GetReqNT() {}

GetReqNT *GetReqNT::get()
{
    return GetReqNT::poolT.get();
}

void GetReqNT::put(GetReqNT *obj)
{
    GetReqNT::poolT.put(obj);
}

void GetReqNT::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(GetReqNT::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&GetReqNT::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(GetReqNT::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&GetReqNT::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//=====//

int GetRspNT::class_msg_type = eMsgGetRspNT;
MemMgmt::GeneralPool<GetRspNT, MemMgmt::ClassAlloc<GetRspNT> > GetRspNT::poolT(10, "GetRspNT");
GetRspNT::GetRspNT()
    :IovMsgRspNT(static_cast<eMsgType>(GetRspNT::class_msg_type))
{}

GetRspNT::~GetRspNT() {}

GetRspNT *GetRspNT::get()
{
    return GetRspNT::poolT.get();
}

void GetRspNT::put(GetRspNT *obj)
{
    GetRspNT::poolT.put(obj);
}

void GetRspNT::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(GetRspNT::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&GetRspNT::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(GetRspNT::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&GetRspNT::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int PutReqNT::class_msg_type = eMsgPutReqNT;
MemMgmt::GeneralPool<PutReqNT, MemMgmt::ClassAlloc<PutReqNT> > PutReqNT::poolT(10, "PutReqNT");
PutReqNT::PutReqNT()
    :IovMsgReqNT(static_cast<eMsgType>(PutReqNT::class_msg_type))
{}

PutReqNT::~PutReqNT() {}

PutReqNT *PutReqNT::get()
{
    return PutReqNT::poolT.get();
}

void PutReqNT::put(PutReqNT *obj)
{
    PutReqNT::poolT.put(obj);
}

void PutReqNT::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(PutReqNT::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&PutReqNT::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(PutReqNT::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&PutReqNT::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//=====//

int PutRspNT::class_msg_type = eMsgPutRspNT;
MemMgmt::GeneralPool<PutRspNT, MemMgmt::ClassAlloc<PutRspNT> > PutRspNT::poolT(10, "PutRspNT");
PutRspNT::PutRspNT()
    :IovMsgRspNT(static_cast<eMsgType>(PutRspNT::class_msg_type))
{}

PutRspNT::~PutRspNT() {}

PutRspNT *PutRspNT::get()
{
    return PutRspNT::poolT.get();
}

void PutRspNT::put(PutRspNT *obj)
{
    PutRspNT::poolT.put(obj);
}

void PutRspNT::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(PutRspNT::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&PutRspNT::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(PutRspNT::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&PutRspNT::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int DeleteReqNT::class_msg_type = eMsgDeleteReqNT;
MemMgmt::GeneralPool<DeleteReqNT, MemMgmt::ClassAlloc<DeleteReqNT> > DeleteReqNT::poolT(10, "DeleteReqNT");
DeleteReqNT::DeleteReqNT()
    :IovMsgReqNT(static_cast<eMsgType>(DeleteReqNT::class_msg_type))
{}

DeleteReqNT::~DeleteReqNT() {}

DeleteReqNT *DeleteReqNT::get()
{
    return DeleteReqNT::poolT.get();
}

void DeleteReqNT::put(DeleteReqNT *obj)
{
    DeleteReqNT::poolT.put(obj);
}

void DeleteReqNT::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(DeleteReqNT::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&DeleteReqNT::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(DeleteReqNT::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&DeleteReqNT::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//=====//

int DeleteRspNT::class_msg_type = eMsgDeleteRspNT;
MemMgmt::GeneralPool<DeleteRspNT, MemMgmt::ClassAlloc<DeleteRspNT> > DeleteRspNT::poolT(10, "DeleteRspNT");
DeleteRspNT::DeleteRspNT()
    :IovMsgRspNT(static_cast<eMsgType>(DeleteRspNT::class_msg_type))
{}

DeleteRspNT::~DeleteRspNT() {}

DeleteRspNT *DeleteRspNT::get()
{
    return DeleteRspNT::poolT.get();
}

void DeleteRspNT::put(DeleteRspNT *obj)
{
    DeleteRspNT::poolT.put(obj);
}

void DeleteRspNT::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(DeleteRspNT::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&DeleteRspNT::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(DeleteRspNT::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&DeleteRspNT::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int GetRangeReqNT::class_msg_type = eMsgGetRangeReqNT;
MemMgmt::GeneralPool<GetRangeReqNT, MemMgmt::ClassAlloc<GetRangeReqNT> > GetRangeReqNT::poolT(10, "GetRangeReqNT");
GetRangeReqNT::GetRangeReqNT()
    :IovMsgReqNT(static_cast<eMsgType>(GetRangeReqNT::class_msg_type))
{}

GetRangeReqNT::~GetRangeReqNT() {}

GetRangeReqNT *GetRangeReqNT::get()
{
    return GetRangeReqNT::poolT.get();
}

void GetRangeReqNT::put(GetRangeReqNT *obj)
{
    GetRangeReqNT::poolT.put(obj);
}

void GetRangeReqNT::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(GetRangeReqNT::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&GetRangeReqNT::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(GetRangeReqNT::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&GetRangeReqNT::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//=====//

int GetRangeRspNT::class_msg_type = eMsgGetRangeRspNT;
MemMgmt::GeneralPool<GetRangeRspNT, MemMgmt::ClassAlloc<GetRangeRspNT> > GetRangeRspNT::poolT(10, "GetRangeRspNT");
GetRangeRspNT::GetRangeRspNT()
    :IovMsgRspNT(static_cast<eMsgType>(GetRangeRspNT::class_msg_type))
{}

GetRangeRspNT::~GetRangeRspNT() {}

GetRangeRspNT *GetRangeRspNT::get()
{
    return GetRangeRspNT::poolT.get();
}

void GetRangeRspNT::put(GetRangeRspNT *obj)
{
    GetRangeRspNT::poolT.put(obj);
}

void GetRangeRspNT::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(GetRangeRspNT::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&GetRangeRspNT::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(GetRangeRspNT::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&GetRangeRspNT::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int MergeReqNT::class_msg_type = eMsgMergeReqNT;
MemMgmt::GeneralPool<MergeReqNT, MemMgmt::ClassAlloc<MergeReqNT> > MergeReqNT::poolT(10, "MergeReqNT");
MergeReqNT::MergeReqNT()
    :IovMsgReqNT(static_cast<eMsgType>(MergeReqNT::class_msg_type))
{}

MergeReqNT::~MergeReqNT() {}

MergeReqNT *MergeReqNT::get()
{
    return MergeReqNT::poolT.get();
}

void MergeReqNT::put(MergeReqNT *obj)
{
    MergeReqNT::poolT.put(obj);
}

void MergeReqNT::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(MergeReqNT::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&MergeReqNT::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(MergeReqNT::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&MergeReqNT::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

//=====//

int MergeRspNT::class_msg_type = eMsgMergeRspNT;
MemMgmt::GeneralPool<MergeRspNT, MemMgmt::ClassAlloc<MergeRspNT> > MergeRspNT::poolT(10, "MergeRspNT");
MergeRspNT::MergeRspNT()
    :IovMsgRspNT(static_cast<eMsgType>(MergeRspNT::class_msg_type))
{}

MergeRspNT::~MergeRspNT() {}

MergeRspNT *MergeRspNT::get()
{
    return MergeRspNT::poolT.get();
}

void MergeRspNT::put(MergeRspNT *obj)
{
    MergeRspNT::poolT.put(obj);
}

void MergeRspNT::registerPool()
{
    try {
        IovMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(MergeRspNT::class_msg_type),
            reinterpret_cast<IovMsgBase::GetMsgBaseFunction>(&MergeRspNT::get));
        IovMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(MergeRspNT::class_msg_type),
            reinterpret_cast<IovMsgBase::PutMsgBaseFunction>(&MergeRspNT::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------
}

