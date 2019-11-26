/**
 * @file    dbsrvmcasttypes.cpp
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
 * dbsrvmcasttypes.cpp
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
#include "dbsrvmcasttypes.hpp"
#include "dbsrvmcastfactory.hpp"

using namespace std;
using namespace MemMgmt;

namespace MetaMsgs {

static McastMsgPoolInitializer initalizer;
bool McastMsgPoolInitializer::initialized = false;

McastMsgPoolInitializer::McastMsgPoolInitializer()
{
    if ( initialized != true ) {
        McastMsgMetaAd::registerPool();
        McastMsgMetaSolicit::registerPool();
    }
    initialized = true;
}


// -----------------------------------------------------------------------------------------

int McastMsgMetaAd::class_msg_type = eMsgMetaAd;
MemMgmt::GeneralPool<McastMsgMetaAd, MemMgmt::ClassAlloc<McastMsgMetaAd> > McastMsgMetaAd::poolT(10, "McastMsgMetaAd");
McastMsgMetaAd::McastMsgMetaAd()
    :McastMsg(static_cast<eMsgType>(McastMsgMetaAd::class_msg_type))
{}

McastMsgMetaAd::~McastMsgMetaAd() {}

void McastMsgMetaAd::marshall(McastMsgBuffer& buf)
{
    McastMsg::marshall(buf);
}

void McastMsgMetaAd::unmarshall(McastMsgBuffer& buf)
{
    McastMsg::unmarshall(buf);
}

string McastMsgMetaAd::toJson()
{
    return json;
}

void McastMsgMetaAd::fromJson(const string &_sjon)
{

}

McastMsgMetaAd *McastMsgMetaAd::get()
{
    return McastMsgMetaAd::poolT.get();
}

void McastMsgMetaAd::put(McastMsgMetaAd *obj)
{
    McastMsgMetaAd::poolT.put(obj);
}

void McastMsgMetaAd::registerPool()
{
    try {
        McastMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(McastMsgMetaAd::class_msg_type),
            reinterpret_cast<McastMsgBase::GetMsgBaseFunction>(&McastMsgMetaAd::get));
        McastMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(McastMsgMetaAd::class_msg_type),
            reinterpret_cast<McastMsgBase::PutMsgBaseFunction>(&McastMsgMetaAd::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr<< "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

// -----------------------------------------------------------------------------------------

int McastMsgMetaSolicit::class_msg_type = eMsgMetaSolicit;
MemMgmt::GeneralPool<McastMsgMetaSolicit, MemMgmt::ClassAlloc<McastMsgMetaSolicit> > McastMsgMetaSolicit::poolT(10, "McastMsgMetaSolicit");
McastMsgMetaSolicit::McastMsgMetaSolicit()
    :McastMsg(static_cast<eMsgType>(McastMsgMetaSolicit::class_msg_type))
{}

McastMsgMetaSolicit::~McastMsgMetaSolicit() {}

void McastMsgMetaSolicit::marshall(McastMsgBuffer& buf)
{
    McastMsg::marshall(buf);
}

void McastMsgMetaSolicit::unmarshall(McastMsgBuffer& buf)
{
    McastMsg::unmarshall(buf);
}

string McastMsgMetaSolicit::toJson()
{
    return json;
}

void McastMsgMetaSolicit::fromJson(const string &_sjon)
{

}

McastMsgMetaSolicit *McastMsgMetaSolicit::get()
{
    return McastMsgMetaSolicit::poolT.get();
}

void McastMsgMetaSolicit::put(McastMsgMetaSolicit *obj)
{
    McastMsgMetaSolicit::poolT.put(obj);
}

void McastMsgMetaSolicit::registerPool()
{
    try {
        McastMsgFactory::getInstance()->registerGetFactory(static_cast<eMsgType>(McastMsgMetaSolicit::class_msg_type),
            reinterpret_cast<McastMsgBase::GetMsgBaseFunction>(&McastMsgMetaAd::get));
        McastMsgFactory::getInstance()->registerPutFactory(static_cast<eMsgType>(McastMsgMetaSolicit::class_msg_type),
            reinterpret_cast<McastMsgBase::PutMsgBaseFunction>(&McastMsgMetaAd::put));
    } catch (const runtime_error &ex) {
#ifdef __GNUC__
        cerr << "failed registering factory" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed registering factory" << SoeLogger::LogError() << endl;
#endif
    }
}

}


