/**
 * @file    msgnetadvertisers.cpp
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
 * msgnetadvertisers.cpp
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

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

#include "soelogger.hpp"
using namespace SoeLogger;

#include "msgnetadkeys.hpp"
#include "msgnetadvertisers.hpp"

namespace MetaNet {

AdvertisementItem::AdvertisementItem(const std::string &_what,
        const std::string &_desc,
        std::string _addr,
        uint16_t _port,
        eProtocol _prot)
    :what(_what),
     description(_desc),
     address(_addr),
     port(_port),
     protocol(_prot)
{}

Advertisement::Advertisement(const std::string &_json)
    :json(_json),
     debug(false)
{}

Advertisement::~Advertisement()
{}

void Advertisement::SetDebug(bool _debug)
{
    debug = _debug;
}
//
// Our own thing
//
MsgNetMcastAdvertisement::MsgNetMcastAdvertisement(const std::string &_json)
    :Advertisement(_json),
     ttl(0),
     secure(false)
{}

MsgNetMcastAdvertisement::~MsgNetMcastAdvertisement()
{}

std::string MsgNetMcastAdvertisement::AsJson()
{
    json.clear();

    std::string secure_bool = secure == true ? "true" : "false";

    json = "{\n\"" + AdKeys::key_secure + "\": " + secure_bool.c_str() + ",\n";
    json += "\"" + AdKeys::array_name + "\": [\n";
    for ( auto it = items.begin() ; it != items.end() ; it++ ) {
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "what: " << (*it).what <<
            " description: " << (*it).description <<
            " address: " << (*it).address <<
            " port: " << (*it).port <<
            " protocol: " << (*it).protocol << SoeLogger::LogDebug() << endl;

        json += "{ \"" + AdKeys::key_service + "\": \"" + (*it).what + "\", " +
            "\"" + AdKeys::key_description + "\": \"" + (*it).description + "\", " +
            "\"" + AdKeys::key_address + "\": \"" + (*it).address + "\", " +
            "\"" + AdKeys::key_port + "\": " + to_string((*it).port) + " , " +
            "\"" + AdKeys::key_protocol + "\": " + to_string((*it).protocol) +
            " }";
        if ( it + 1 != items.end() ) {
            json += ",\n";
        }
    }
    json += "\n]\n}";

    return json;
}

std::string MsgNetMcastAdvertisement::GetJson()
{
    return json;
}

void MsgNetMcastAdvertisement::AddItem(const AdvertisementItem &_item)
{
    items.push_back(_item);
}

int MsgNetMcastAdvertisement::Parse()
{
    Json::Value root;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse(json.c_str(), root);
    if ( !parsingSuccessful ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " parse failed " << SoeLogger::LogDebug() << endl;
        return -1;
    }

    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "json: " << json << SoeLogger::LogDebug() << endl;

    for ( Json::ValueIterator itr = root.begin() ; itr != root.end() ; itr++  ) {
        if ( (*itr).isArray() == true ) {
            if ( itr.key().asString() == AdKeys::array_name ) {
                Json::Value json_val = *itr;

                for ( unsigned int i = 0 ; json_val[i].isNull() == false ; i++ ) {
                    AdvertisementItem item;

                    for ( Json::ValueIterator itr1 = json_val[i].begin() ; itr1 != json_val[i].end() ; itr1++ ) {
                        if ( itr1.key().asString() == AdKeys::key_service ) {
                            if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << "service: " << (*itr1).asString() << SoeLogger::LogDebug() << endl;
                            item.what = (*itr1).asString();
                        } else if ( itr1.key().asString() == AdKeys::key_description ) {
                            if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << "description: " << (*itr1).asString() << SoeLogger::LogDebug() << endl;
                            item.description = (*itr1).asString();
                        } else if ( itr1.key().asString() == AdKeys::key_address ) {
                            if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << "address: " << (*itr1).asString() << SoeLogger::LogDebug() << endl;
                            item.address = (*itr1).asString();
                        } else if ( itr1.key().asString() == AdKeys::key_port ) {
                            if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << "port: " << (*itr1).asString() << SoeLogger::LogDebug() << endl;
                            item.port = static_cast<uint16_t>(::atoi((*itr1).asString().c_str()));
                        } else if ( itr1.key().asString() == AdKeys::key_protocol ) {
                            if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << "protocol: " << (*itr1).asString() << SoeLogger::LogDebug() << endl;
                            item.protocol = static_cast<AdvertisementItem::eProtocol>(::atoi((*itr1).asString().c_str()));
                        } else {
                            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid field parse failed: " << itr1.key().asString() << SoeLogger::LogError() << endl;
                            return -1;
                        }
                    }

                    AddItem(item);
                }
            } else {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid array name parse failed: " << itr.key().asString() << SoeLogger::LogError() << endl;
                return -1;
            }
        } else if ( (*itr).isBool() == true ) {
            if ( itr.key().isString() == true ) {
                if ( itr.key().asString() == AdKeys::key_secure ) {
                    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << "secure: " << (*itr).asString() << SoeLogger::LogDebug() << endl;
                    secure = static_cast<bool>(::atoi((*itr).asString().c_str()));
                } else {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid field detected: " << itr.key().asString() << SoeLogger::LogError() << endl;
                    return -1;
                }
            } else {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid field detected: " << itr.key().asString() << SoeLogger::LogError() << endl;
                return -1;
            }
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid field detected: " << itr.key().asString() << SoeLogger::LogError() << endl;
            return -1;
        }
    }

    return 0;
}

uint32_t MsgNetMcastAdvertisement::GetTtl(uint32_t _ttl)
{
    return ttl;
}

bool MsgNetMcastAdvertisement::GetSecure(bool _req_sec)
{
    return secure;
}

AdvertisementItem *MsgNetMcastAdvertisement::FindItemByWhat(const std::string &_what)
{
    for ( auto it = items.begin() ; it != items.end() ; it++ ) {
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << "what: " << (*it).what <<
            " description: " << (*it).description <<
            " address: " << (*it).address <<
            " port: " << (*it).port <<
            " protocol: " << (*it).protocol << SoeLogger::LogDebug() << endl;

        if ( (*it).what == _what ) {
            return &(*it);
        }
    }

    return 0;
}

}


