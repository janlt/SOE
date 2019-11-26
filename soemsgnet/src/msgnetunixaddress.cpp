/**
 * @file    msgnetunixaddress.cpp
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
 * msgnetunixaddress.cpp
 *
 *  Created on: Mar 10, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
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
#include "msgnetunixaddress.hpp"
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnetudpsocket.hpp"

namespace MetaNet {

sockaddr_un UnixAddress::null_address = {0};
const char *UnixAddress::node_no_local_addr = "/tmp/dupek_node_no_local_unix_addr.socket";

UnixAddress::UnixAddress(const UnixAddress &r) throw(std::runtime_error)
{
    ::strcpy(this->addr.sun_path, r.addr.sun_path);
    this->addr.sun_family = r.addr.sun_family;
}

UnixAddress::UnixAddress(const char *_path, uint16_t _family) throw(std::runtime_error)
{
    ::strcpy(this->addr.sun_path, _path);
    this->addr.sun_family = _family;
}

int UnixAddress::OpenName()
{
    FILE *name_fp = ::fopen(this->addr.sun_path, "w+");
    if ( !name_fp ) {
        if ( errno != ENXIO ) {
            return -1;
        }
    } else {
        ::fclose(name_fp);
    }

    return 0;
}

int UnixAddress::CloseName()
{
    return ::unlink(this->addr.sun_path);
}

bool UnixAddress::operator==(const Address &r)
{
    const UnixAddress &una = dynamic_cast<const UnixAddress &>(r);
    return !::strcmp(this->addr.sun_path, una.addr.sun_path) &&
        this->addr.sun_family == una.addr.sun_family;
}

bool UnixAddress::operator!=(const Address &r)
{
    return !(*this == r);
}

UnixAddress &UnixAddress::operator=(const sockaddr_un _sockaddr_un)
{
    ::strcpy(this->addr.sun_path, _sockaddr_un.sun_path);
    this->addr.sun_family = _sockaddr_un.sun_family;
    return *this;
}

bool UnixAddress::operator==(const sockaddr_un _sockaddr_un)
{
    return *this == UnixAddress(_sockaddr_un);
}

bool UnixAddress::operator!=(const sockaddr_un _sockaddr_un)
{
    return *this != UnixAddress(_sockaddr_un);
}

UnixAddress UnixAddress::findNodeNoLocalAddr(bool debug)
{
    return UnixAddress(UnixAddress::node_no_local_addr);
}

}


