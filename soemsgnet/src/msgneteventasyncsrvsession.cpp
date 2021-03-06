/**
 * @file    msgneteventasyncsrvsession.cpp
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
 * msgneteventasyncsrvsession.cpp
 *
 *  Created on: Mar 14, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <stdexcept>
using namespace std;
#include <poll.h>
#include <sys/epoll.h>
#include <netdb.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread/locks.hpp>

#include "thread.hpp"
#include "threadfifoext.hpp"
#include "classalloc.hpp"
#include "generalpool.hpp"
#include "dbsrvmarshallable.hpp"
#include "dbsrvmsgbuffer.hpp"

#include "dbsrvmsgs.hpp"
#include "dbsrvmcasttypes.hpp"
#include "dbsrvmsgadmtypes.hpp"
#include "dbsrvmsgfactory.hpp"
#include "dbsrvmsgadmfactory.hpp"

#include "soelogger.hpp"
using namespace SoeLogger;

#include "dbsrviovmsgsbase.hpp"
#include "dbsrviovmsgs.hpp"
#include "dbsrvmsgiovfactory.hpp"
#include "dbsrvmsgiovtypes.hpp"

#include "thread.hpp"
#include "threadfifoext.hpp"
#include "classalloc.hpp"
#include "generalpool.hpp"
#include "dbsrvmsgbuffer.hpp"
#include "msgnetaddress.hpp"
#include "msgnetipv4address.hpp"
#include "msgnetunixaddress.hpp"
#include "msgnetmmaddress.hpp"
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnetunixsocket.hpp"
#include "msgnetmmpoint.hpp"
#include "msgnettcpsocket.hpp"
#include "msgnetioevent.hpp"
#include "msgnetlistener.hpp"
#include "msgnetsession.hpp"
#include "msgnetprocessor.hpp"
#include "msgneteventsession.hpp"
#include "msgneteventasyncsession.hpp"
#include "msgneteventlistener.hpp"
#include "msgneteventsrvsession.hpp"
#include "msgneteventasyncsession.hpp"
#include "msgnetasynceventsrvsession.hpp"

namespace MetaNet {

EventSrvAsyncSession::EventSrvAsyncSession(Point &_point, bool _test_mode) throw(std::runtime_error)
    :EventSrvSession<IoEvent>(_point, _test_mode),
     in_finito(false),
     out_finito(false)
{}

EventSrvAsyncSession::~EventSrvAsyncSession()
{}

int EventSrvAsyncSession::SendIov(const IoEvent *io) const
{
    return 0;
}

int EventSrvAsyncSession::RecvIov(IoEvent *io)
{
    return 0;
}

void EventSrvAsyncSession::StartSend() throw(std::runtime_error)
{

}

void EventSrvAsyncSession::StartRecv() throw(std::runtime_error)
{

}

int EventSrvAsyncSession::processInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev)
{
    in_fifo.send(ev);
    return 0;
}

int EventSrvAsyncSession::sendInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst, IoEvent *&ev)
{
    ev = in_fifo.receive();
    return 0;
}

int EventSrvAsyncSession::processOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev)
{
    out_fifo.send(ev);
    return 0;
}

int EventSrvAsyncSession::sendOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev)
{
    ev = out_fifo.receive();
    return 0;
}

void EventSrvAsyncSession::inLoop()
{
    MetaMsgs::MsgBufferBase buf;
    IPv4Address addr;
    IoEvent *ev;

    try {
        for ( ;; ) {
            ev = 0;
            if ( in_finito == true ) {
                break;
            }
            int ret = sendInboundStreamEvent(buf, &addr, ev);
            if ( ret ) {
                if ( static_cast<MetaMsgs::MsgStatus>(ret) != MetaMsgs::MsgStatus::eMsgDisconnect ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "dequeue error ret: " << ret << SoeLogger::LogError() << endl;
                } else {
                    break;
                }
            }
        }
    } catch (...) {}
}

void EventSrvAsyncSession::outLoop()
{
    MetaMsgs::MsgBufferBase buf;
    IPv4Address addr;
    IoEvent *ev;

    try {
        for ( ;; ) {
            ev = 0;
            if ( out_finito == true ) {
                break;
            }
            int ret = sendOutboundStreamEvent(buf, &addr, ev);
            if ( ret ) {
                if ( static_cast<MetaMsgs::MsgStatus>(ret) != MetaMsgs::MsgStatus::eMsgDisconnect ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "dequeue error ret: " << ret << SoeLogger::LogError() << endl;
                } else {
                    break;
                }
            }
        }
    } catch (...) {}
}

void EventSrvAsyncSession::inFinito()
{
    in_finito = true;
}

void EventSrvAsyncSession::outFinito()
{
    out_finito = true;
}

}


