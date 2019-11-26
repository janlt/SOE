/**
 * @file    dbsrvmsgiovfactory.cpp
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
 * dbsrvmsgiovfactory.cpp
 *
 *  Created on: Jan 13, 2017
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
#include "dbsrvmsgtypes.hpp"
#include "dbsrvmsgfactory.hpp"
#include "dbsrvmsgiovfactory.hpp"

using namespace std;

namespace MetaMsgs {

class IovMsgBase;

IovMsgBase::GetMsgBaseFunction IovMsgFactory::get_factories[eMsgMax];
IovMsgBase::PutMsgBaseFunction IovMsgFactory::put_factories[eMsgMax];


IovMsgFactory *IovMsgFactory::instance = 0;

IovMsgFactory::IovMsgFactory()
{
    for ( int i = 0 ; i < eMsgMax ; i++ ) {
        get_factories[i] = 0;
        put_factories[i] = 0;
    }
}

IovMsgFactory::~IovMsgFactory() {}

IovMsgFactory *IovMsgFactory::getInstance()
{
    if ( !instance ) {
        instance = new IovMsgFactory;
    }

    return instance;
}

void IovMsgFactory::registerGetFactory(eMsgType typ, IovMsgBase::GetMsgBaseFunction fac) throw(runtime_error)
{
    if ( typ <= eMsgInvalid || typ >= eMsgMax ) {
        throw runtime_error((string(__FUNCTION__) + "Invalid MsgFactory type: " + to_string(typ)).c_str());
    }
    get_factories[typ] = fac;
}

void IovMsgFactory::registerPutFactory(eMsgType typ, IovMsgBase::PutMsgBaseFunction fac) throw(runtime_error)
{
    if ( typ <= eMsgInvalid || typ >= eMsgMax ) {
        throw runtime_error((string(__FUNCTION__) + "Invalid MsgFactory type: " + to_string(typ)).c_str());
    }
    put_factories[typ] = fac;
}


}

