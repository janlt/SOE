/**
 * @file    metadbclisessionmgr.cpp
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
 * metadbclisessionmgr.cpp
 *
 *  Created on: Jun 1, 2017
 *      Author: Jan Lisowiec
 */

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

#include <iostream>
#include <sstream>
#include <iomanip>
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

#include "soelogger.hpp"
using namespace SoeLogger;

#include "dbsrvmsgs.hpp"
#include "dbsrvmcasttypes.hpp"
#include "dbsrvmsgadmtypes.hpp"
#include "dbsrvmsgfactory.hpp"
#include "dbsrvmsgadmfactory.hpp"

#include "dbsrviovmsgsbase.hpp"
#include "dbsrviovmsgs.hpp"
#include "dbsrvmsgiovfactory.hpp"
#include "dbsrvmsgiovtypes.hpp"

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
#include "msgnetasynceventclisession.hpp"

using namespace std;
using namespace MetaMsgs;
using namespace MetaNet;

#include "metadbcliiov.hpp"
#include "metadbcliiosession.hpp"
#include "metadbcliasynciosession.hpp"
#include "metadbcliadmsession.hpp"
#include "metadbclimcastprocessor.hpp"
#include "metadbclisessionmgr.hpp"

void __attribute__ ((constructor)) MetadbcliFacade::SessionMgrStaticInitializerEnter(void);
void __attribute__ ((destructor)) MetadbcliFacade::SessionMgrStaticInitializerExit(void);

namespace MetadbcliFacade {

SessionMgr *SessionMgr::sess_mgr = 0;

SessionMgr::Initializer *session_mgr_static_initializer_ptr = 0;

bool SessionMgr::Initializer::initialized = false;
Lock SessionMgr::global_lock;
SessionMgr::Initializer sess_mgr_initalizer;

void SessionMgrStaticInitializerEnter(void)
{
    session_mgr_static_initializer_ptr = &sess_mgr_initalizer;
}

void SessionMgrStaticInitializerExit(void)
{
    session_mgr_static_initializer_ptr = 0;
}

SessionMgr::Initializer::Initializer()
{
    WriteLock w_lock(SessionMgr::global_lock);

    if ( Initializer::initialized == false ) {
        GetInstance();
    }
    Initializer::initialized = true;
}

SessionMgr::Initializer::~Initializer()
{
    WriteLock w_lock(SessionMgr::global_lock);

    if ( Initializer::initialized == true ) {
        PutInstance();
    }
    Initializer::initialized = false;
}

SessionMgr::SessionMgr()
    :mcast_proc(0)
{}

bool SessionMgr::started = false;
void SessionMgr::StartMgr()
{
    if ( SessionMgr::started == false ) {
        boost::thread sess_thread(&SessionMgr::start, SessionMgr::GetInstance());
        SessionMgr::started = true;
    }
}

SessionMgr *SessionMgr::GetInstance()
{
    if ( !SessionMgr::sess_mgr ) {
        SessionMgr::sess_mgr = new SessionMgr();
    }
    return SessionMgr::sess_mgr;
}

void SessionMgr::PutInstance()
{
    if ( SessionMgr::sess_mgr ) {
        delete SessionMgr::sess_mgr;
        SessionMgr::sess_mgr = 0;
    }
}

SessionMgr::~SessionMgr() {}

void SessionMgr::start()
{
    bool debug_mode = false;
    bool msg_debug_mode = false;
    bool allow_remote_srv = false;
    bool mm_sock_session = false;
    uint32_t num_io_sessions = 1;

    for ( ;; ) {
        MetaNet::IPv4MulticastListener *ipv4mcastlis = 0;
        try {
            ipv4mcastlis = MetaNet::IPv4MulticastListener::getInstance(true);
            ipv4mcastlis->setMcastClientMode(true);
            ipv4mcastlis->setDebug(false);
            McastlistCliProcessor mcli_processor(num_io_sessions, mm_sock_session, debug_mode, msg_debug_mode, allow_remote_srv);
            mcast_proc = &mcli_processor;

            boost::function<CallbackIPv4ProcessFun> m_p1 = boost::bind(&McastlistCliProcessor::processStream, &mcli_processor, _1, _2, _3, _4);
            ipv4mcastlis->setProcessStreamCB(m_p1);
            boost::function<CallbackIPv4ProcessFun> m_p2 = boost::bind(&McastlistCliProcessor::processDgram, &mcli_processor, _1, _2, _3, _4);
            ipv4mcastlis->setProcessDgramCB(m_p2);
            boost::function<CallbackIPv4SendFun> m_s1 = boost::bind(&McastlistCliProcessor::sendStream, &mcli_processor, _1, _2, _3, _4, _5);
            ipv4mcastlis->setSendStreamCB(m_s1);
            boost::function<CallbackIPv4SendFun> m_s2 = boost::bind(&McastlistCliProcessor::sendDgram, &mcli_processor, _1, _2, _3, _4, _5);
            ipv4mcastlis->setSendDgramCB(m_s2);
            boost::function<CallbackIPv4ProcessTimerFun> m_timer = boost::bind(&McastlistCliProcessor::processTimer, &mcli_processor);
            ipv4mcastlis->setProcessTimerCB(m_timer);

            // send solicitations
            mcli_processor.sendSolicitations(*ipv4mcastlis);

            boost::thread mcastlis_thread(&MetaNet::IPv4MulticastListener::start, ipv4mcastlis);

            mcastlis_thread.join();
            break;
        } catch (const std::runtime_error &ex) {
            delete ipv4mcastlis;
            ipv4mcastlis = 0;
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "runtime_error caught: " << ex.what() << ", will try again" << SoeLogger::LogError() << endl;
            usleep(10000);
        }
    }

    return;
}

}


