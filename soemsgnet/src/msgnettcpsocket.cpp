/**
 * @file    msgnettcpsocket.cpp
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
 * msgnettcpsocket.cpp
 *
 *  Created on: Feb 6, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <stdexcept>
using namespace std;

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

#include "msgnetaddress.hpp"
#include "msgnetipv4address.hpp"
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnettcpsocket.hpp"

namespace MetaNet {

TcpSocket::TcpSocket()
    :Socket(Point::eType::eTcp)
{}

TcpSocket::~TcpSocket()
{}

int TcpSocket::Create(bool _srv)
{
    return CreateStream();
}

int TcpSocket::Close()
{
    return Socket::Close();
}

int TcpSocket::Create(int _domain, int _type, int _protocol)
{
    int soc = ::socket(_domain, _type, _protocol);
    SetDesc(soc);
    SetStateOpened();
    return soc;
}

int TcpSocket::CreateStream()
{
    int soc = ::socket(AF_INET, SOCK_STREAM, 0);
    SetDesc(soc);
    SetStateOpened();
    return soc;
}

int TcpSocket::Shutdown()
{
    SetStateClosed();
    return ::shutdown(GetDesc(), SHUT_RDWR);
}

int TcpSocket::Connect(const Address *_addr)
{
    int ret;
    const IPv4Address *addr = dynamic_cast<const IPv4Address *>(_addr);
    while ( 1 ) {
        ret = ::connect(GetDesc(), const_cast<IPv4Address *>(addr)->AsSockaddr(), static_cast<int>(sizeof(sockaddr)));
        if ( ret < 0 ) {
            if ( errno == EINTR ) {
                continue;
            } else {
                return ret;
            }
        }
        break;
    }
    return ret;
}

int TcpSocket::Accept(Address *_addr)
{
    int ret;
    IPv4Address *addr = dynamic_cast<IPv4Address *>(_addr);
    unsigned int size = addr ? sizeof(sockaddr) : 0;
    while ( 1 ) {
        ret = ::accept(GetDesc(), addr ? addr->AsNCSockaddr() : 0, &size);
        if ( ret < 0 ) {
            if ( errno == EINTR ) {
                continue;
            } else {
                SetStateOpened();
                return ret;
            }
        }
        break;
    }
    SetStateOpened();
    return ret;
}

int TcpSocket::Listen(int backlog)
{
    return ::listen(GetDesc(), backlog);
}

int TcpSocket::SetSendBufferLen(int _buf_len)
{
    return Socket::SetSendBufferLen(_buf_len);
}

int TcpSocket::SetReceiveBufferLen(int _buf_len)
{
    return Socket::SetReceiveBufferLen(_buf_len);
}

int TcpSocket::SetAddMcastMembership(const ip_mreq *_ip_mreq)
{
    return Socket::SetAddMcastMembership(_ip_mreq);
}

int TcpSocket::SetMcastTtl(unsigned int _ttl_val)
{
    return Socket::SetMcastTtl(_ttl_val);
}

int TcpSocket::SetNoDelay()
{
    return Socket::SetNoDelay();
}

int TcpSocket::Cork()
{
    return Socket::Cork();
}

int TcpSocket::Uncork()
{
    return Socket::Uncork();}

int TcpSocket::SetMcastLoop()
{
    return Socket::SetMcastLoop();
}

int TcpSocket::SetReuseAddr()
{
    int val = 1;
    return ::setsockopt(GetDesc(), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&val), static_cast<socklen_t>(sizeof(val)));
}

int TcpSocket::Bind(const sockaddr *_addr)
{
    return Socket::Bind(_addr);
}

int TcpSocket::SetNonBlocking(bool _blocking)
{
    return Socket::SetNonBlocking(_blocking);
}

int TcpSocket::SetNoSigPipe()
{
    return Socket::SetNoSigPipe();
}

int TcpSocket::SetRecvTmout(uint32_t msecs)
{
    return Socket::SetRecvTmout(msecs);
}

int TcpSocket::SetSndTmout(uint32_t msecs)
{
    return Socket::SetSndTmout(msecs);
}

int TcpSocket::SetNoLinger(uint32_t tmout)
{
    return Socket::SetNoLinger(tmout);
}

int TcpSocket::SendIov(MetaMsgs::IovMsgBase &io)
{
    return io.sendSock(GetDesc());
}

int TcpSocket::RecvIov(MetaMsgs::IovMsgBase &io)
{
    return io.recvSock(GetDesc());
}

bool TcpSocket::IsConnected()
{
    return ::fcntl(GetDesc(), F_GETFL) != -1 || errno != EBADF ? true : false;
}

int TcpSocket::GetTcpInfo(struct tcp_info *t_info)
{
    if ( !t_info ) {
        return -1;
    }
    int info_len = sizeof(*t_info);

    return ::getsockopt(GetDesc(), SOL_TCP, TCP_INFO, reinterpret_cast<void *>(t_info), reinterpret_cast<socklen_t *>(&info_len));
}

}
