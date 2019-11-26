/**
 * @file    msgnetunixsocket.cpp
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
 * msgnetunixsocket.cpp
 *
 *  Created on: Mar 10, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

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
#include "msgnetunixaddress.hpp"
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnetunixsocket.hpp"

namespace MetaNet {

UnixSocket::UnixSocket()
    :Socket(Point::eType::eUnix)
{}

UnixSocket::~UnixSocket()
{}

int UnixSocket::Create(bool _srv)
{
    return CreateUnix();
}

int UnixSocket::Close()
{
    return Socket::Close();
}

int UnixSocket::Create(int _domain, int _type, int _protocol)
{
    int soc = ::socket(_domain, _type, _protocol);
    SetDesc(soc);
    SetStateOpened();
    return soc;
}

int UnixSocket::CreateUnix()
{
    int soc = ::socket(AF_UNIX, SOCK_STREAM, 0);
    SetDesc(soc);
    SetStateOpened();
    return soc;
}

int UnixSocket::Shutdown()
{
    SetStateClosed();
    return ::shutdown(GetDesc(), SHUT_RDWR);
}

int UnixSocket::Connect(const Address *_addr)
{
    int ret;
    const UnixAddress *u_addr = dynamic_cast<const UnixAddress *>(_addr);
    if ( !u_addr ) {
        return -1;
    }
    int r_len = static_cast<int>(::strlen(u_addr->addr.sun_path) + sizeof(u_addr->addr.sun_family));
    while ( 1 ) {
        ret = ::connect(GetDesc(), reinterpret_cast<const sockaddr*>(&u_addr->addr), r_len);
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

int UnixSocket::Accept(Address *_addr)
{
    int ret;
    UnixAddress *addr = dynamic_cast<UnixAddress *>(_addr);
    unsigned int size = addr ? sizeof(addr->addr) : 0;
    while ( 1 ) {
        ret = ::accept(GetDesc(), addr ? reinterpret_cast<sockaddr*>(&addr->addr) : 0, &size);
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

int UnixSocket::Listen(int backlog)
{
    return ::listen(GetDesc(), backlog);
}

int UnixSocket::SetSendBufferLen(int _buf_len)
{
    return Socket::SetSendBufferLen(_buf_len);
}

int UnixSocket::SetReceiveBufferLen(int _buf_len)
{
    return Socket::SetReceiveBufferLen(_buf_len);
}

int UnixSocket::SetAddMcastMembership(const ip_mreq *_ip_mreq)
{
    return -1;
}

int UnixSocket::SetMcastTtl(unsigned int _ttl_val)
{
    return -1;
}

int UnixSocket::SetNoDelay()
{
    return Socket::SetNoDelay();
}

int UnixSocket::Cork()
{
    return Socket::Cork();
}

int UnixSocket::Uncork()
{
    return Socket::Uncork();}

int UnixSocket::SetMcastLoop()
{
    return Socket::SetMcastLoop();
}

int UnixSocket::SetReuseAddr()
{
    int val = 1;
    return ::setsockopt(GetDesc(), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&val), static_cast<socklen_t>(sizeof(val)));
}

int UnixSocket::Bind(const sockaddr *_addr)
{
    return ::bind(GetDesc(), _addr, sizeof(struct sockaddr_un));
}

int UnixSocket::SetNonBlocking(bool _blocking)
{
    return Socket::SetNonBlocking(_blocking);
}

int UnixSocket::SetNoSigPipe()
{
    return Socket::SetNoSigPipe();
}

int UnixSocket::SetRecvTmout(uint32_t msecs)
{
    return Socket::SetRecvTmout(msecs);
}

int UnixSocket::SetSndTmout(uint32_t msecs)
{
    return Socket::SetSndTmout(msecs);
}

int UnixSocket::SetNoLinger(uint32_t tmout)
{
    return Socket::SetNoLinger(tmout);
}

int UnixSocket::SendIov(MetaMsgs::IovMsgBase &io)
{
    return io.sendSock(GetDesc());
}

int UnixSocket::RecvIov(MetaMsgs::IovMsgBase &io)
{
    return io.recvSock(GetDesc());
}

int UnixSocket::Send(const uint8_t *io, uint32_t io_size)
{
    return Socket::SendExact(io, io_size, true);
}

int UnixSocket::Recv(uint8_t *io, uint32_t io_size)
{
    return Socket::RecvExact(io, io_size, true);
}

bool UnixSocket::IsConnected()
{
    return false;
}

}


