/**
 * @file    metadbsrvadmprocessor.hpp
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
 * metadbsrvadmprocessor.cpp
 *
 *  Created on: Jun 29, 2017
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

#include <vector>
#include <fstream>
#include <string>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <chrono>
#include <boost/random.hpp>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <stdexcept>
using namespace std;
#include <poll.h>
#include <sys/epoll.h>
#include <netdb.h>

#include "soelogger.hpp"
using namespace SoeLogger;

#include "rcsdbargdefs.hpp"
#include "rcsdbidgener.hpp"

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
#include "msgnettcpsocket.hpp"
#include "msgnetudpsocket.hpp"
#include "msgnetioevent.hpp"
#include "msgnetprocessor.hpp"
#include "msgnetlistener.hpp"
#include "msgneteventlistener.hpp"
#include "msgnetipv4listener.hpp"
#include "msgnetipv4mcastlistener.hpp"
#include "msgnetsession.hpp"
#include "msgnetipv4tcpsession.hpp"
#include "msgnetipv4tcpsrvsession.hpp"
#include "msgneteventsession.hpp"
#include "msgneteventsrvsession.hpp"
#include "msgnetasynceventsrvsession.hpp"

#include "metadbsrvadmsession.hpp"
#include "metadbsrvadmprocessor.hpp"

using namespace std;
using namespace MetaNet;

namespace Metadbsrv {

//
// IPv4Listener msg Processor
//
int AdmSrvProcessor::processDgram(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got dgram req" << SoeLogger::LogDebug() << endl;
    return 0;
}

int AdmSrvProcessor::processStream(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    int ret = 0;
    if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Got tcp req" << SoeLogger::LogDebug() << endl;

    IPv4Listener &ipv4_lis = dynamic_cast<IPv4Listener &>(lis);
    const TcpSocket *lis_soc = dynamic_cast<const TcpSocket *>(pt);
    if ( !lis_soc ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Wrong listening socket type" << SoeLogger::LogDebug() << endl;
        return -1;
    }

    if ( ipv4_lis.getSock() == lis_soc ) { // connect request
        if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Got tcp connection req" << SoeLogger::LogDebug() << endl;

        try {
            bool clean_up = false;
            TcpSocket *ss = new TcpSocket();
            AdmSession *sss = new AdmSession(*ss);
            sss->SetSessDebug(debug);
            if ( sss->initialise(const_cast<TcpSocket *>(lis_soc), true) < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't initialize session errno: " << errno << SoeLogger::LogError() << endl;
                delete ss;
                delete sss;
                return -1;
            }
            if ( ss->SetNoDelay() < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't SetNoDelay() on sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
                clean_up = true;
            }
            if ( ss->SetNoLinger() < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't set no linger address on tcp sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
                clean_up = true;
            }
            if ( ss->SetReuseAddr() < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't set reuse address on tcp sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
                clean_up = true;
            }
            if ( ss->SetRecvTmout(10000) ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't set recv tmout on tcp sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
                clean_up = true;
            }
            if ( ss->SetSndTmout(10000) ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't set snd tmout on tcp sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
                clean_up = true;
            }
            if ( clean_up == true ) {
                delete ss;
                delete sss;
                return -1;
            }

            WriteLock w_lock(AdmSession::global_lock);

            if ( static_cast<uint32_t>(sss->tcp_sock.GetDesc()) >= AdmSession::max_sessions ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Max number of adm connected sockets exceeded, refusing connection: " << sss->tcp_sock.GetDesc() << SoeLogger::LogError() << endl;
                ss->Shutdown();
                delete ss;
                delete sss;
                return -1;
            }

            AdmSession::sessions[sss->tcp_sock.GetDesc()].reset(sss);
            if ( debug == true ) {
                AdmSession::sessions[sss->tcp_sock.GetDesc()]->SetSessDebug(true);
            }

            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Create new session sock: " << sss->tcp_sock.GetDesc() << SoeLogger::LogDebug() << endl;

            int ret = ipv4_lis.AddPoint(&AdmSession::sessions[sss->tcp_sock.GetDesc()]->tcp_sock);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't add cli session" << SoeLogger::LogError() << endl;
                (void )AdmSession::sessions[sss->tcp_sock.GetDesc()]->deinitialise(true);
                AdmSession::sessions[sss->tcp_sock.GetDesc()].reset();
                delete ss;
                return -1;
            }
        } catch (const runtime_error &ex) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "Failed connecting cli session: " << ex.what() << SoeLogger::LogDebug() << endl;
        }
    } else { // msg
        if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Got msg, session sock: " << const_cast<Point*>(pt)->GetDesc() << SoeLogger::LogDebug() << endl;

        if ( AdmSession::sessions[const_cast<Point*>(pt)->GetDesc()] ) {
            ret = AdmSession::sessions[const_cast<Point*>(pt)->GetDesc()]->processStreamMessage(_buf, from_addr, pt);
        } else {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "session not found " << const_cast<Point*>(pt)->GetDesc() << SoeLogger::LogDebug() << endl;
        }

        // Check if it's connection reset so we can remove the session from the listener
        if ( ret == -ECONNRESET ) {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Disconnecting client: " << const_cast<Point*>(pt)->GetDesc() << SoeLogger::LogDebug() << endl;

            WriteLock w_lock(AdmSession::global_lock);

            //cout << "AdmSrvProcessor::" << __FUNCTION__ << " RESET fd=" << const_cast<Point*>(pt)->GetDesc() << endl;

            ipv4_lis.RemovePoint(&AdmSession::sessions[const_cast<Point*>(pt)->GetDesc()]->tcp_sock);
            AdmSession::sessions[const_cast<Point*>(pt)->GetDesc()]->tcp_sock.Shutdown();
            AdmSession::sessions[const_cast<Point*>(pt)->GetDesc()]->deinitialise(true);
            AdmSession::sessions[const_cast<Point*>(pt)->GetDesc()]->CloseAllStores();
            ::close(dynamic_cast<TcpSocket &>(AdmSession::sessions[const_cast<Point*>(pt)->GetDesc()]->tcp_sock).GetConstDesc());
            AdmSession::sessions[const_cast<Point*>(pt)->GetDesc()].reset();
        }
    }

    return ret;
}

int AdmSrvProcessor::processTimeout()
{
    if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Got timeout" << SoeLogger::LogDebug() << endl;
    AdmSession::PurgeStaleStores();
    AdmSession::PruneArchive();
    return 0;
}

int AdmSrvProcessor::sendDgram(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
{
    if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Sending dgram rsp" << SoeLogger::LogDebug() << endl;
    return 0;
}

int AdmSrvProcessor::sendStream(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
{
    //IPv4Listener &ipv4lis = dynamic_cast<IPv4Listener &>(lis);
    if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Sending tcp rsp" << SoeLogger::LogDebug() << endl;
    return 0;
}

AdmSrvProcessor::AdmSrvProcessor(bool _debug)
    :debug(_debug)
{}

AdmSrvProcessor::~AdmSrvProcessor() {}

}


