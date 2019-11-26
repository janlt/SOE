/**
 * @file    metadbcliasynciosession.cpp
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
 * metadbcliasynciosession.cpp
 *
 *  Created on: Jul 3, 2017
 *      Author: Jan Lisowiec
 */

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include <vector>
#include <map>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

extern "C" {
#include "soeutil.h"
}

#include "soelogger.hpp"
using namespace SoeLogger;

#include "soe_bind_api.hpp"
#include "rcsdbargdefs.hpp"
#include "rcsdbfacadeapi.hpp"
#include "rcsdbfacadebase.hpp"
#include "rcsdbfunctorbase.hpp"

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>

#include <list>
#include <stdexcept>

using namespace std;

#include <poll.h>
#include <sys/epoll.h>
#include <netdb.h>

#include <boost/thread.hpp>
#include <boost/function.hpp>

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

#include "dbsrviovmsgsbase.hpp"
#include "dbsrviovmsgs.hpp"
#include "dbsrviovasyncmsgs.hpp"
#include "dbsrvmsgiovfactory.hpp"
#include "dbsrvmsgiovtypes.hpp"
#include "dbsrvmsgiovasynctypes.hpp"

#include "msgnetadkeys.hpp"
#include "msgnetadvertisers.hpp"
#include "msgnetsolicitors.hpp"
#include "msgnetaddress.hpp"
#include "msgnetipv4address.hpp"
#include "msgnetunixaddress.hpp"
#include "msgnetmmaddress.hpp"
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnetunixsocket.hpp"
#include "msgnetmmpoint.hpp"
#include "msgnetioevent.hpp"
#include "msgnettcpsocket.hpp"
#include "msgnetudpsocket.hpp"
#include "msgnetunixsocket.hpp"
#include "msgnetlistener.hpp"
#include "msgnetprocessor.hpp"
#include "msgnetipv4listener.hpp"
#include "msgnetipv4mcastlistener.hpp"
#include "msgnetsession.hpp"
#include "msgnetipv4tcpsession.hpp"
#include "msgnetipv4tcpclisession.hpp"
#include "msgneteventsession.hpp"
#include "msgneteventclisession.hpp"
#include "msgneteventsession.hpp"
#include "msgneteventclisession.hpp"
#include "msgnetasynceventclisession.hpp"

#include <json/value.h>
#include <json/reader.h>

using namespace std;
using namespace MetaMsgs;
using namespace MetaNet;

#include "metadbcliiov.hpp"
#include "metadbcliiosession.hpp"
#include "metadbcliasynciosession.hpp"
#include "metadbcliadmsession.hpp"

#include "metadbcliargdefs.hpp"
#include "metadbclifacadeapi.hpp"
#include "metadbcliiterator.hpp"
#include "metadbclifacade.hpp"
#include "metadbclimcastprocessor.hpp"
#include "metadbclisessionmgr.hpp"

namespace MetadbcliFacade
{

uint32_t AsyncIOSession::session_counter = 1;

AsyncIOSession::AsyncIOSession(Point &point,
        Session *_parent,
        uint32_t _msg_pause,
        bool _test)
    :EventCliAsyncSession(point, _test),
     counter(0),
     bytes_received(0),
     bytes_sent(0),
     sess_debug(false),
     recv_count(0),
     sent_count(0),
     loop_count(0),
     io_session_thread(0),
     parent_session(_parent),
     msg_pause(_msg_pause),
     in_thread(0),
     out_thread(0),
     session_id(session_counter++),
     state(State::eCreated),
     event_count(0)
{
    my_addr = IPv4Address::findNodeNoLocalAddr();
}

AsyncIOSession::~AsyncIOSession()
{
    try {
        inFinito();
        for ( int ii = 0 ; ii < 32 ; ii++ ) {
            ResetInFifo();
        }

        outFinito();
        for ( int ii = 0 ; ii < 32 ; ii++ ) {
            ResetOutFifo();
        }

        // we need to interrupt and join in/out threads to prevent conditional_variable assert to fire off
        // on the FIFOs
        int jj;
        for ( jj = 0 ; jj < 32 ; jj++ ) {
            if ( state == AsyncIOSession::State::eDefunct ) {
                if ( in_thread ) {
                    //std::cout << "#### in_thread interrupting: " << in_thread->get_id() << std::endl;
                    for ( int ii = 0 ; ii < 128 ; ii++ ) {
                        in_thread->interrupt();
                        if ( in_thread->joinable() == true ) {
                            //std::cout << "#### in_thread joinable " << std::endl;
                            break;
                        }
                        ::usleep(1000);
                    }
                    in_thread->try_join_for(boost::chrono::milliseconds(500));
                    delete in_thread;
                    //std::cout << "#### in_thread deleted " << std::endl;
                }
                if ( out_thread ) {
                    //std::cout << "#### out_thread interrupting: " << out_thread->get_id() << std::endl;
                    for ( int ii = 0 ; ii < 128 ; ii++ ) {
                        out_thread->interrupt();
                        if ( out_thread->joinable() == true ) {
                            //std::cout << "#### out_thread joinable " << std::endl;
                            break;
                        }
                        ::usleep(1000);
                    }
                    out_thread->try_join_for(boost::chrono::milliseconds(500));
                    delete out_thread;
                    //std::cout << "#### out_thread deleted " << std::endl;
                }

                break;
            } else {
                ::usleep(100);
                continue;
            }
        }
        //std::cout << "#### *_thread " << jj << std::endl;

        if ( event_point.GetType() == Socket::eType::eTcp ) {
            //cout << "AsyncIOSession::" << __FUNCTION__ << " sock: " << event_point.GetDesc() << " state: " << state << endl;
            if ( state != AsyncIOSession::State::eDefunct ) {
                deinitialise(true);
                state = AsyncIOSession::State::eDefunct;
            }
        }
    } catch ( ... ) {
        //std::cout << "$$$$ interrupt " << __PRETTY_FUNCTION__ << std::endl;
    }
}

//
// Receiving messages from network
// Receive message from network and enqueue to inbound queue
//
int AsyncIOSession::processInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev)
{
    int ret = EventCliAsyncSession::processInboundStreamEvent(buf, from_addr, ev);
    if ( ret ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed enqueue ret:" << ret << SoeLogger::LogError() << endl;
    }
    if ( sess_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got and enqueued event" << SoeLogger::LogDebug() << endl;
    return ret;
}

//
// Receiving messages from network
// Dequeue messages from inbound queue
//
int AsyncIOSession::sendInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev)
{
    int ret = EventCliAsyncSession::sendInboundStreamEvent(buf, dst_addr, ev);
    if ( ret ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed dequeue ret:" << ret << SoeLogger::LogError() << endl;
    }
    if ( sess_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got and dequeued event" << SoeLogger::LogDebug() << endl;
    return ret;
}

//
// This API is used only for Async i/o's
// Sending messages to network
// Enqueue message to outbound queue
//
int AsyncIOSession::processOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev)
{
    int ret = EventCliAsyncSession::processOutboundStreamEvent(buf, from_addr, ev);
    if ( ret ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed enqueue ret:" << ret << SoeLogger::LogError() << endl;
        return ret;
    }
    if ( sess_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got and enqueued event" << SoeLogger::LogDebug() << endl;
    return ret;
}

//
// This API is used only for Async i/o's
// Sending messages to network
// Dequeue message from outbound queue and send them to network
//
int AsyncIOSession::sendOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev)
{
    IoEvent *events[AsyncIOSession::max_one_shot_events];
    int ret = EventCliAsyncSession::sendOutboundStreamEvent(buf, dst_addr, ev);
    if ( ret ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed dequeue ret:" << ret << SoeLogger::LogError() << endl;
        return ret;
    }
    if ( sess_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got and dequeued event" << SoeLogger::LogDebug() << endl;

    assert(ev->ev.data.ptr);
    StoreProxy *st_pr = reinterpret_cast<StoreProxy *>(ev->ev.data.ptr);

    events[0] = ev;
    uint32_t op_ret = st_pr->SendOutboundAsyncOp(event_point, events, 1);

    return static_cast<int>(op_ret) >= 0 ? 0 : static_cast<int>(op_ret);
}

void AsyncIOSession::inLoop()
{
    EventCliAsyncSession::inLoop();
}

void AsyncIOSession::outLoop()
{
    EventCliAsyncSession::outLoop();
}

inline void AsyncIOSession::start() throw(std::runtime_error)
{
}


}


