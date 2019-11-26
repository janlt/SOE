/**
 * @file    msgnetsolicitors.cpp
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
 * msgnetsolicitors.cpp
 *
 *  Created on: Mar 1, 2017
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
#include <vector>
using namespace std;

#include <json/value.h>
#include <json/reader.h>

#include "soelogger.hpp"
using namespace SoeLogger;

#include "msgnetadkeys.hpp"
#include "msgnetsolicitors.hpp"

namespace MetaNet {

SolicitationItem::SolicitationItem(const std::string &_what,
        const std::string &_desc)
    :what(_what),
     description(_desc)
{}

Solicitation::Solicitation(const std::string &_json)
    :json(_json),
     debug(false)
{}

Solicitation::~Solicitation()
{}

void Solicitation::SetDebug(bool _debug)
{
    debug = _debug;
}

MsgNetMcastSolicitation::MsgNetMcastSolicitation(const std::string &_json)
    :Solicitation(_json),
     requested_ttl(5),
     requested_secure(false)
{}

MsgNetMcastSolicitation::~MsgNetMcastSolicitation()
{}

std::string MsgNetMcastSolicitation::AsJson()
{
    return "";
}

int MsgNetMcastSolicitation::Parse()
{
    return 0;
}

void MsgNetMcastSolicitation::AddItem(const SolicitationItem &_item)
{
    items.push_back(_item);
}

void MsgNetMcastSolicitation::SetRequestedTtl(uint32_t _ttl)
{
    requested_ttl = _ttl;
}

void MsgNetMcastSolicitation::SetRequestedSecure(bool _req_sec)
{
    requested_secure = _req_sec;
}

}


