/**
 * @file    msgnetfifoaddress.cpp
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
 * msgnetfifoaddress.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
#include <bits/sockaddr.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <poll.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <stdexcept>
using namespace std;

#include <unistd.h>
#include <stdlib.h>

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
#include "msgnetsocket.hpp"
#include "msgnetudpsocket.hpp"

namespace MetaNet {

sockaddr_fifo FifoAddress::null_address = {0};
const char *FifoAddress::node_no_local_addr = "/tmp/dupek_node_no_local_mm_addr.fifo";

FifoAddress::FifoAddress(const FifoAddress &r) throw(std::runtime_error)
{
    ::strcpy(this->addr.fifo_path, r.addr.fifo_path);
    this->addr.fifo_family = r.addr.fifo_family;
}

FifoAddress::FifoAddress(const char *_path, uint16_t _family) throw(std::runtime_error)
{
    ::strcpy(this->addr.fifo_path, _path);
    this->addr.fifo_family = _family;
}

int FifoAddress::OpenName()
{
    FILE *name_fp = ::fopen(this->addr.fifo_path, "w+");
    if ( !name_fp ) {
        if ( errno != ENXIO ) {
            return -1;
        }
    } else {
        ::fclose(name_fp);
    }

    return 0;
}

int FifoAddress::CloseName()
{
    return ::unlink(this->addr.fifo_path);
}

bool FifoAddress::operator==(const Address &r)
{
    const FifoAddress &una = dynamic_cast<const FifoAddress &>(r);
    return !::strcmp(this->addr.fifo_path, una.addr.fifo_path) &&
        this->addr.fifo_family == una.addr.fifo_family;
}

bool FifoAddress::operator!=(const Address &r)
{
    return !(*this == r);
}

FifoAddress &FifoAddress::operator=(const sockaddr_fifo &_sockaddr_fifo)
{
    ::strcpy(this->addr.fifo_path, _sockaddr_fifo.fifo_path);
    this->addr.fifo_family = _sockaddr_fifo.fifo_family;
    return *this;
}

bool FifoAddress::operator==(const sockaddr_fifo &_sockaddr_fifo)
{
    return *this == FifoAddress(_sockaddr_fifo);
}

bool FifoAddress::operator!=(const sockaddr_fifo &_sockaddr_fifo)
{
    return *this != FifoAddress(_sockaddr_fifo);
}

FifoAddress FifoAddress::findNodeNoLocalAddr(bool debug)
{
    return FifoAddress(FifoAddress::node_no_local_addr);
}

}


