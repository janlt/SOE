/**
 * @file    msgnetmmaddress.cpp
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
 * msgnetmmaddress.cpp
 *
 *  Created on: Mar 13, 2017
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
#include "msgnetmmaddress.hpp"
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnetudpsocket.hpp"

namespace MetaNet {

sockaddr_mm MMAddress::null_address = {0};
const char *MMAddress::node_no_local_addr = "/tmp/dupek_node_no_local_mm_addr.socket";

MMAddress::MMAddress(const MMAddress &r) throw(std::runtime_error)
{
    ::strcpy(this->addr.mm_path, r.addr.mm_path);
    this->addr.mm_family = r.addr.mm_family;
}

MMAddress::MMAddress(const char *_path, uint16_t _family) throw(std::runtime_error)
{
    ::strcpy(this->addr.mm_path, _path);
    this->addr.mm_family = _family;
}

int MMAddress::OpenName()
{
    FILE *name_fp = ::fopen(this->addr.mm_path, "w+");
    if ( !name_fp ) {
        if ( errno != ENXIO ) {
            return -1;
        }
    } else {
        ::fclose(name_fp);
    }

    return 0;
}

int MMAddress::CloseName()
{
    return ::unlink(this->addr.mm_path);
}

bool MMAddress::operator==(const Address &r)
{
    const MMAddress &una = dynamic_cast<const MMAddress &>(r);
    return !::strcmp(this->addr.mm_path, una.addr.mm_path) &&
        this->addr.mm_family == una.addr.mm_family;
}

bool MMAddress::operator!=(const Address &r)
{
    return !(*this == r);
}

MMAddress &MMAddress::operator=(const sockaddr_mm &_sockaddr_mm)
{
    ::strcpy(this->addr.mm_path, _sockaddr_mm.mm_path);
    this->addr.mm_family = _sockaddr_mm.mm_family;
    return *this;
}

bool MMAddress::operator==(const sockaddr_mm &_sockaddr_mm)
{
    return *this == MMAddress(_sockaddr_mm);
}

bool MMAddress::operator!=(const sockaddr_mm &_sockaddr_mm)
{
    return *this != MMAddress(_sockaddr_mm);
}

MMAddress MMAddress::findNodeNoLocalAddr(bool debug)
{
    return MMAddress(MMAddress::node_no_local_addr);
}

}


