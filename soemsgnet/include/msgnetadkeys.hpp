/**
 * @file    msgnetadkeys.hpp
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
 * msgnetadkeys.hpp
 *
 *  Created on: Mar 1, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETADKEYS_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETADKEYS_HPP_

namespace MetaNet {

struct AdKeys
{
    const static std::string              key_service;
    const static std::string              key_description;
    const static std::string              key_address;
    const static std::string              key_port;
    const static std::string              key_protocol;
    const static std::string              key_secure;

    const static std::string              array_name;
};

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETADKEYS_HPP_ */
