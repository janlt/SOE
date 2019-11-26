/**
 * @file    msgnetaddress.hpp
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
 * msgnetaddress.hpp
 *
 *  Created on: Feb 7, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETADDRESS_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETADDRESS_HPP_

namespace MetaNet {

class Address
{
public:
    Address() {}
    Address(const Address &r) {}
    virtual ~Address() {}

    virtual bool operator==( const Address &r) = 0;
    Address &operator=(const Address &r);
};

inline Address &Address::operator=(const Address &r)
{
    return *this;
}

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETADDRESS_HPP_ */
