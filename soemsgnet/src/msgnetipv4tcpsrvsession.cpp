/**
 * @file    msgnetipv4tcpsrvsession.cpp
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
 * msgnetipv4tcpsrvsession.cpp
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
#include "msgnetipv4tcpsrvsession.hpp"

namespace MetaNet {

IPv4TCPSrvSession::IPv4TCPSrvSession(TcpSocket &_sock, bool _test_mode) throw(std::runtime_error)
    :IPv4TCPSession(_sock, _test_mode)
{}

IPv4TCPSrvSession::~IPv4TCPSrvSession()
{}

int IPv4TCPSrvSession::initialise(MetaNet::Point *lis_soc, bool _ignore_exc) throw(std::runtime_error)
{
    TcpSocket *tcp_lis_soc = dynamic_cast<TcpSocket *>(lis_soc);
    if ( !tcp_lis_soc ) {
        if ( _ignore_exc == false ) {
            throw std::runtime_error((std::string(__FUNCTION__) + " No socket to accept connection: " + to_string(errno)).c_str());
        }
        return -1;
    }
    if ( test_mode == false ) {
        int new_sd = tcp_lis_soc->Accept(&peer_addr);
        if ( new_sd < 0 ) {
            if ( _ignore_exc == false ) {
                throw std::runtime_error((std::string(__FUNCTION__) + " failed errno: " + to_string(errno)).c_str());
            }
            return new_sd;
        }
        tcp_sock.SetDesc(new_sd);
        if ( tcp_sock.Register(_ignore_exc) < 0 ) {
            (void )tcp_sock.Close();
            if ( _ignore_exc == false ) {
                throw std::runtime_error((std::string(__FUNCTION__) + " failed errno: " + to_string(errno)).c_str());
            }
            return new_sd;
        }
    } else {
        IPv4TCPSession::open_counter++;
    }
    return 0;
}

int IPv4TCPSrvSession::deinitialise(bool _ignore_exc) throw(std::runtime_error)
{
    (void )tcp_sock.Close();
    if ( tcp_sock.Deregister(_ignore_exc) < 0 ) {
        if ( _ignore_exc == false ) {
            throw std::runtime_error((std::string(__FUNCTION__) + " failed errno: " + to_string(errno)).c_str());
        }
        return -1;
    }
    if ( test_mode == true ) {
        IPv4TCPSession::close_counter++;
    }
    return 0;
}

int IPv4TCPSrvSession::processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    return 0;
}

int IPv4TCPSrvSession::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    return 0;
}

}
