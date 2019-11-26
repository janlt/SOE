/**
 * @file    metadbclimcastprocessor.cpp
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
 * metadbclimcastprocessor.cpp
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

namespace MetadbcliFacade {

//
// processAdvertisement and processTimer run sequentially
//
int McastlistCliProcessor::processAdvertisement(MetaMsgs::McastMsgMetaAd *ad,
     const IPv4Address &from_addr,
     const MetaNet::Point *pt)
{
    int ret = 0;

    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got ad msg: " << " from_addr/own_addr: " <<
        from_addr.GetNTOASinAddr() << "/" << own_addr.GetNTOASinAddr() << " json: " << ad->json << SoeLogger::LogDebug() << endl;

    // If the ad was from our own address (identified by function call!!!!!)
    // In real application it should come from a config file
    if ( from_addr.GetSinAddr() != own_addr.GetSinAddr() ) {
        if ( allow_remote_srv == false ) {
            return 0;
        }
    }

    // Create session
    TcpSocket *ss = 0;
    if ( !AdmSession::GetAdmSession() ) {
        ss = new TcpSocket;
        try {
            AdmSession::CreateAdmSession(*ss, mm_sock_session, debug);
        } catch (const runtime_error &ex) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Fatal runtime_error caught: " << ex.what() << "  ... " << SoeLogger::LogError() << endl;
            delete ss;
            AdmSession::DestroyAdmSession();
            return -1;
        }
    }

    IPv4Address adm_start_addr;
    adm_start_addr.addr.sin_family = AF_INET;
    adm_start_addr.addr.sin_addr.s_addr = htonl(INADDR_ANY);
    adm_start_addr.addr.sin_port = htons(6111);

    if ( AdmSession::GetAdmSession()->peer_addr == IPv4Address::null_address ||
        AdmSession::GetAdmSession()->peer_addr == adm_start_addr ) {
        ret = AdmSession::GetAdmSession()->ParseSetAddresses(ad->json);
        if ( ret < 0 ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid mcast ad: " << ret << SoeLogger::LogError() << endl;
            if ( ss ) {
                delete ss;
            }
            AdmSession::DestroyAdmSession();
            return -1;
        }

        // that means session has been deinitialized
        srv_addr = AdmSession::GetAdmSession()->peer_addr;

        if ( AdmSession::GetAdmSession()->started == true ) {
            (void )AdmSession::GetAdmSession()->deinitialise(true);
        }
        ret = AdmSession::GetAdmSession()->initialise(0, true); // For client we pass NULL as listening socket since we are connection our own
        if ( ret < 0 ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't initialize session errno: " << errno << "  ..." << SoeLogger::LogError() << endl;
            (void )AdmSession::GetAdmSession()->deinitialise(true);
            if ( ss ) {
                delete ss;
            }
            AdmSession::DestroyAdmSession();
            return -1;
        }
    }

    if ( AdmSession::GetAdmSession()->started != true ) {
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " starting adm session ... " << SoeLogger::LogDebug() << endl;
        AdmSession::GetAdmSession()->started = true;
    }

    return ret;
}

// This callback runs session recovery logic as well
int McastlistCliProcessor::processDgram(MetaNet::Listener &lis,
    MetaMsgs::MsgBufferBase& _buf,
    const IPv4Address &from_addr,
    const MetaNet::Point *pt)
{
    if ( !_buf.getSize() ) { // timeout - ignore
        return 0;
    }

    int ret = 0;

    MetaMsgs::McastMsgMetaAd *ad = MetaMsgs::McastMsgMetaAd::poolT.get();
    ad->unmarshall(dynamic_cast<MetaMsgs::McastMsgBuffer &>(_buf));

    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got dgram req type: " << ad->hdr.type << " ad_type: " <<
        static_cast<uint32_t>(MetaMsgs::McastMsgMetaAd::class_msg_type) << SoeLogger::LogDebug() << endl;

    // we are interested only in advertisements
    if ( ad->hdr.type != static_cast<uint32_t>(MetaMsgs::McastMsgMetaAd::class_msg_type) ) {
        MetaMsgs::McastMsgMetaAd::poolT.put(ad);
        return 0;
    }
    try {
        ret = processAdvertisement(ad, from_addr, pt);
    } catch (const runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed initializing client session: " << ex.what() << SoeLogger::LogError() << endl;
        MetaMsgs::McastMsgMetaAd::poolT.put(ad);
        return -1;
    }

    MetaMsgs::McastMsgMetaAd::poolT.put(ad);
    return ret;
}

int McastlistCliProcessor::processStream(MetaNet::Listener &lis,
    MetaMsgs::MsgBufferBase& buf,
    const IPv4Address &from_addr,
    const MetaNet::Point *pt)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got tsp req" << SoeLogger::LogDebug() << endl;
    return 0;
}

int McastlistCliProcessor::sendDgram(MetaNet::Listener &lis,
    const uint8_t *msg,
    uint32_t msg_size,
    const IPv4Address *dst_addr,
    const MetaNet::Point *pt)
{
    int ret = 0;

    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Sending solicit req" << SoeLogger::LogDebug() << endl;

    MetaNet::IPv4MulticastListener &mcast_lis = dynamic_cast<MetaNet::IPv4MulticastListener &>(lis);

    for ( int i = 0 ; i < 4 ; i++ ) {
        MetaMsgs::McastMsgMetaSolicit sol;
        ret = sol.sendmsg(mcast_lis.mcast_sock.GetSoc(), dst_addr->addr);
    }

    return ret;
}

int McastlistCliProcessor::sendStream(MetaNet::Listener &lis,
    const uint8_t *msg,
    uint32_t msg_size,
    const IPv4Address *dst_addr,
    const MetaNet::Point *pt)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Sending tcp req" << SoeLogger::LogDebug() << endl;
    return 0;
}

int McastlistCliProcessor::processTimer()
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got timeout" << SoeLogger::LogDebug() << endl;

    if ( AdmSession::GetAdmSession() && AdmSession::GetAdmSession()->started == true ) {
        return 0;
    }

    // Create session
    TcpSocket *ss = 0;
    if ( !AdmSession::GetAdmSession() ) {
        ss = new TcpSocket;
        try {
            AdmSession::CreateAdmSession(*ss, mm_sock_session, debug);
        } catch (const runtime_error &ex) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Fatal runtime_error caught: " << ex.what() << "  ... " << SoeLogger::LogError() << endl;
            AdmSession::DestroyAdmSession();
            return -1;
        }
    }

    IPv4Address adm_start_addr;
    adm_start_addr.addr.sin_family = AF_INET;
    adm_start_addr.addr.sin_addr.s_addr = htonl(INADDR_ANY);
    adm_start_addr.addr.sin_port = htons(6111);

    if ( AdmSession::GetAdmSession()->peer_addr == adm_start_addr ) {
        // that means session has been deinitialized
        srv_addr = AdmSession::GetAdmSession()->peer_addr;

        int ret = AdmSession::GetAdmSession()->initialise(0, true); // For client we pass NULL as listening socket since we are connection our own
        if ( ret < 0 ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't initialize session errno: " << errno << "  ..." << SoeLogger::LogDebug() << endl;
            AdmSession::DestroyAdmSession();
            return -1;
        }
    }

    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " starting adm session ... " << SoeLogger::LogDebug() << endl;
    AdmSession::GetAdmSession()->started = true;
    return 0;
}

McastlistCliProcessor::McastlistCliProcessor(uint32_t _num_io_sessions_per_adm_session,
        bool _mm_sock_session,
        bool _debug,
        bool _msg_debug,
        bool _allow_remote_srv)
    :counter(0),
     debug(_debug),
     msg_debug(_msg_debug),
     allow_remote_srv(_allow_remote_srv),
     mm_sock_session(_mm_sock_session),
     num_io_sessions_per_adm_session(_num_io_sessions_per_adm_session),
     timer_tics(0)
{
    own_addr = IPv4Address::findNodeNoLocalAddr();
}

McastlistCliProcessor::~McastlistCliProcessor() {}

void McastlistCliProcessor::sendSolicitations(MetaNet::Listener &lis)
{
    MetaNet::IPv4MulticastListener &mcast_lis = dynamic_cast<MetaNet::IPv4MulticastListener &>(lis);
    sendDgram(lis, 0, 0, mcast_lis.getIPv4Addr(), 0);
}

}


