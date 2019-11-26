/**
 * @file    msgnetipv4tcpsession.cpp
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
 * msgnetipv4tcpsession.cpp
 *
 *  Created on: Jan 11, 2017
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
#include <netdb.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread/locks.hpp>

#include "soelogger.hpp"
using namespace SoeLogger;

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
#include "msgnetlistener.hpp"
#include "msgnetsession.hpp"
#include "msgnetipv4listener.hpp"
#include "msgnetipv4tcplistener.hpp"
#include "msgnetipv4tcpsession.hpp"

namespace MetaNet {

SessionHashEntry  IPv4TCPSession::sessions[session_max];

IPv4TCPSession::Initializer sessions_initalized;

bool      IPv4TCPSession::Initializer::initialized = false;
uint64_t  IPv4TCPSession::open_counter = 0;
uint64_t  IPv4TCPSession::close_counter = 0;

IPv4TCPSession::Initializer::Initializer()
{
    if ( initialized != true ) {
        ::memset(IPv4TCPSession::sessions, '\0', sizeof(IPv4TCPSession::sessions));
    }
    initialized = true;
}

IPv4TCPSession::IPv4TCPSession(TcpSocket &_sock, bool _test_mode) throw(std::runtime_error)
    :tcp_sock(_sock),
    peer_addr(),
    test_mode(_test_mode)
{}

IPv4TCPSession::~IPv4TCPSession()
{}

// Linux is guaranteeing fd uniqueness
void IPv4TCPSession::registerSession() throw(std::runtime_error)
{
    int idx = tcp_sock.GetSoc();
    if ( idx < 0 || idx >= static_cast<int>(session_max) ||
        IPv4TCPSession::sessions[idx].fd || IPv4TCPSession::sessions[idx].sess ) {
        throw std::runtime_error((std::string(__FUNCTION__) + " Session hash corrupt fd: " + to_string(idx)).c_str());
    }
    IPv4TCPSession::sessions[idx].fd = idx;
    IPv4TCPSession::sessions[idx].sess = this;
}

void IPv4TCPSession::deregisterSession() throw(std::runtime_error)
{
    int idx = tcp_sock.GetSoc();
    if ( idx < 0 || idx >= static_cast<int>(session_max) ||
        !IPv4TCPSession::sessions[idx].fd || !IPv4TCPSession::sessions[idx].sess ) {
        throw std::runtime_error((std::string(__FUNCTION__) + " Session hash corrupt fd: " + to_string(idx)).c_str());
    }
    IPv4TCPSession::sessions[idx].fd = 0;
    IPv4TCPSession::sessions[idx].sess = 0;
}

int IPv4TCPSession::processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    return 0;
}

int IPv4TCPSession::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    return 0;
}

}


