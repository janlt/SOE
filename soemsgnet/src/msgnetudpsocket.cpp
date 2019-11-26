/**
 * @file    msgnetudpsocket.cpp
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
 * msgnetudpsocket.cpp
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
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnetudpsocket.hpp"

namespace MetaNet {

UdpSocket::UdpSocket()
    :Socket(Point::eType::eUdp)
{}

UdpSocket::~UdpSocket()
{}

int UdpSocket::Create(bool _srv)
{
    return CreateDgram();
}

int UdpSocket::Close()
{
    return Socket::Close();
}

int UdpSocket::Create(int _domain, int _type, int _protocol)
{
    int soc = ::socket(_domain, _type, _protocol);
    SetDesc(soc);
    SetStateOpened();
    return soc;
}

int UdpSocket::CreateDgram()
{
    int soc = ::socket(AF_INET, SOCK_DGRAM, 0);
    SetDesc(soc);
    SetStateOpened();
    return soc;
}

int UdpSocket::SetSendBufferLen(int _buf_len)
{
    return Socket::SetSendBufferLen(_buf_len);
}

int UdpSocket::SetReceiveBufferLen(int _buf_len)
{
    return Socket::SetReceiveBufferLen(_buf_len);
}

int UdpSocket::SetAddMcastMembership(const ip_mreq *_ip_mreq)
{
    return Socket::SetAddMcastMembership(_ip_mreq);
}

int UdpSocket::SetMcastTtl(unsigned int _ttl_val)
{
    return Socket::SetMcastTtl(_ttl_val);
}

int UdpSocket::SetNoDelay()
{
    return Socket::SetNoDelay();
}

int UdpSocket::Cork()
{
    return Socket::Cork();
}

int UdpSocket::Uncork()
{
    return Socket::Uncork();}

int UdpSocket::SetMcastLoop()
{
    return Socket::SetMcastLoop();
}

int UdpSocket::SetReuseAddr()
{
    int val = 1;
    return ::setsockopt(GetDesc(), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&val), static_cast<socklen_t>(sizeof(val)));
}

int UdpSocket::Bind(const sockaddr *_addr)
{
    return Socket::Bind(_addr);
}

int UdpSocket::SetNonBlocking(bool _blocking)
{
    return Socket::SetNonBlocking(_blocking);
}

int UdpSocket::SetNoSigPipe()
{
    return Socket::SetNoSigPipe();
}

int UdpSocket::SetRecvTmout(uint32_t msecs)
{
    return Socket::SetRecvTmout(msecs);
}

int UdpSocket::SetSndTmout(uint32_t msecs)
{
    return Socket::SetSndTmout(msecs);
}

int UdpSocket::SetNoLinger(uint32_t tmout)
{
    return Socket::SetNoLinger(tmout);
}

}


