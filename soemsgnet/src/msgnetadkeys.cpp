/**
 * @file    msgnetadkeys.cpp
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
 * msgnetadkeys.cpp
 *
 *  Created on: Mar 1, 2017
 *      Author: Jan Lisowiec
 */

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

namespace MetaNet {

const std::string AdKeys::key_service = "service";
const std::string AdKeys::key_description = "description";
const std::string AdKeys::key_address = "address";
const std::string AdKeys::key_port = "port";
const std::string AdKeys::key_protocol = "protocol";
const std::string AdKeys::key_secure = "secure";
const std::string AdKeys::array_name = "items";

}

