/**
 * @file    metadbsrvjson.hpp
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
 * metadbsrvjson.hpp
 *
 *  Created on: Aug 2, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBSRV_SRC_METADBSRVJSON_HPP_
#define SOE_SOE_SOE_METADBSRV_SRC_METADBSRVJSON_HPP_

using namespace std;

namespace Metadbsrv {

class AdmMsgJsonParser
{
    string   json;
    bool     debug;

public:
    AdmMsgJsonParser(string &_json);
    ~AdmMsgJsonParser();

    int GetBoolTagValue(string tag_name, bool &val);
    int Parse();
};

}

#endif /* SOE_SOE_SOE_METADBSRV_SRC_METADBSRVJSON_HPP_ */
