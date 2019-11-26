/**
 * @file    msgnetfifopoint.cpp
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
 * msgnetfifopoint.cpp
 *
 *  Created on: Mar 16, 2017
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

#include <fcntl.h>
#include <sys/stat.h>

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
#include "msgnetfifoaddress.hpp"
#include "msgnetpoint.hpp"
#include "msgnetfifopoint.hpp"

namespace MetaNet {

FifoPoint::FifoPoint()
    :Point(Point::eType::eFifo)
{}

FifoPoint::~FifoPoint()
{}

int FifoPoint::Create(bool _srv)
{
    return 0;
}

int FifoPoint::Close()
{
    return ::close(GetDesc());
}

int FifoPoint::Create(int _domain, int _type, int _protocol)
{
    return 0;
}

int FifoPoint::CreateFifo(const sockaddr *_addr)
{
    int ret = ::mkfifo(reinterpret_cast<const sockaddr_fifo *>(_addr)->fifo_path, 0666);
    if ( ret < 0 ) {
        return ret;
    }
    return::open(reinterpret_cast<const sockaddr_fifo *>(_addr)->fifo_path, O_RDWR);
}

int FifoPoint::Shutdown()
{
    return Close();
}

int FifoPoint::Connect(const Address *_addr)
{
    return 0;
}

int FifoPoint::Accept(Address *addr)
{
    return 0;
}

int FifoPoint::Bind(const sockaddr *_addr)
{
    SetDesc(CreateFifo(_addr));
    return GetDesc();
}

int FifoPoint::SetNoSigPipe()
{
    return 0;
}

int FifoPoint::SetRecvTmout(uint32_t msecs)
{
    return 0;
}

int FifoPoint::SetSndTmout(uint32_t msecs)
{
    return 0;
}

int FifoPoint::SendIov(MetaMsgs::IovMsgBase &io)
{
    return io.sendSock(GetConstDesc());
}

int FifoPoint::RecvIov(MetaMsgs::IovMsgBase &io)
{
    return io.recvSock(GetDesc());
}

int FifoPoint::Send(const uint8_t *io, uint32_t io_size)
{
    return SendExact(io, io_size, true);
}

int FifoPoint::Recv(uint8_t *io, uint32_t io_size)
{
    return RecvExact(io, io_size, true);
}

int FifoPoint::RecvExact(uint8_t *buf, uint32_t bytes_want, bool _ignore_sig)
{
    return 0;
}

int FifoPoint::SendExact(const uint8_t *data, uint32_t bytes_req, bool _ignore_sig)
{
    return 0;
}

}


