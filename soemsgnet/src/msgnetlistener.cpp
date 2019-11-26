/**
 * @file    msgnetlistener.cpp
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
 *  soe_soe
 *
 * @section Source
 *
 *    
 *    
 *
 */

/*
 * msgnetlistener.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
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

#include "generalpool.hpp"
#include "dbsrvmsgbuffer.hpp"
#include "msgnetaddress.hpp"
#include "msgnetipv4address.hpp"
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnetlistener.hpp"

namespace MetaNet {

Listener::Listener(Socket *_sock)
    :sock(_sock),
    sock_addr(),
    reuse_addr(true),
    no_bind(false),
    client_mode(false),
    debug(false)
{}

void Listener::setDebug(bool _debug)
{
    debug = _debug;
}

void Listener::setNoBind(bool _no_bind)
{
    no_bind = _no_bind;
}

void Listener::setClientMode(bool _client)
{
    client_mode = _client;
}

void Listener::setReuseAddr(bool _reuse_addr)
{
    reuse_addr = _reuse_addr;
}

}


