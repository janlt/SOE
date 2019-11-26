/**
 * @file    metadbsrvjson.cpp
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
 * metadbsrvjson.cpp
 *
 *  Created on: Aug 2, 2017
 *      Author: Jan Lisowiec
 */

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

#include "metadbsrvjson.hpp"

using namespace std;

namespace Metadbsrv {

AdmMsgJsonParser::AdmMsgJsonParser(string &_json)
    :json(_json),
     debug(false)
{}

AdmMsgJsonParser::~AdmMsgJsonParser()
{}

int AdmMsgJsonParser::Parse()
{
    if ( !json.size() ) {
        return 0;
    }
    Json::Value root;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse(json.c_str(), root);
    if ( !parsingSuccessful ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "parse failed " << json << SoeLogger::LogError() << endl;
        return -1;
    }

    return 0;
}

int AdmMsgJsonParser::GetBoolTagValue(string tag_name, bool &val)
{
    SoeLogger::Logger logger;

    if ( !json.size() ) {
        return -1;
    }
    Json::Value root;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse(json.c_str(), root);
    if ( !parsingSuccessful ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "parse failed " << SoeLogger::LogError() << endl;
        return -1;
    }

    if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "json: " << json << SoeLogger::LogDebug() << endl;

    for ( Json::ValueIterator itr = root.begin() ; itr != root.end() ; itr++  ) {
        if ( (*itr).isArray() == true ) {
            return -1;
        } else if ( (*itr).isBool() == true ) {
            if ( itr.key().isString() == true ) {
                if ( itr.key().asString() == tag_name ) {
                    if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "tag_name: " << (*itr).asString() << SoeLogger::LogDebug() << endl;
                    val = static_cast<bool>(::atoi((*itr).asString().c_str()));
                    return 0;
                } else {
                    logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid field detected: " << itr.key().asString() << SoeLogger::LogError() << endl;
                    return -1;
                }
            } else {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid field detected: " << itr.key().asString() << SoeLogger::LogError() << endl;
                return -1;
            }
        } else {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid field detected: " << itr.key().asString() << SoeLogger::LogError() << endl;
            return -1;
        }
    }

    return -1;
}

}


