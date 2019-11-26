/**
 * @file    msgnetipv4address.hpp
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
 * msgnetipv4address.hpp
 *
 *  Created on: Feb 7, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4ADDRESS_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4ADDRESS_HPP_

namespace MetaNet {

class IPv4Address: public Address
{
public:
    sockaddr_in           addr;
    static sockaddr_in    null_address;

public:
    IPv4Address(const sockaddr_in _addr = null_address)
        :addr(_addr)
    {}
    IPv4Address(const IPv4Address &r)
    {
        this->addr.sin_addr.s_addr = r.addr.sin_addr.s_addr;
        this->addr.sin_port = r.addr.sin_port;
        this->addr.sin_family = r.addr.sin_family;
    }
    IPv4Address(const char *_ipv4_addr, uint16_t _port, uint16_t _family)
    {
        ::inet_aton(_ipv4_addr, &this->addr.sin_addr);
        this->addr.sin_port = _port;
        this->addr.sin_family = _family;
    }
    virtual ~IPv4Address() {}
    const sockaddr *AsSockaddr();
    sockaddr *AsNCSockaddr();
    uint16_t GetPort() const;
    uint16_t GetNTOHSPort() const;
    uint16_t GetFamily() const;
    in_addr_t GetSinAddr() const;
    char *GetNTOASinAddr() const;

    static IPv4Address findNodeNoLocalAddr(bool debug = false);

    virtual bool operator==(const Address &r);
    virtual bool operator!=(const Address &r);

    IPv4Address &operator=(const IPv4Address &r);
    IPv4Address &operator=(const sockaddr_in _sockaddr_in);
    bool operator==(const sockaddr_in _sockaddr_in);
    bool operator!=(const sockaddr_in _sockaddr_in);
};

inline IPv4Address &IPv4Address::operator=(const IPv4Address &r)
{
    this->addr.sin_addr.s_addr = r.addr.sin_addr.s_addr;
    this->addr.sin_port = r.addr.sin_port;
    this->addr.sin_family = r.addr.sin_family;

    return *this;
}

inline const sockaddr *IPv4Address::AsSockaddr()
{
    return reinterpret_cast<const sockaddr *>(&addr);
}

inline sockaddr *IPv4Address::AsNCSockaddr()
{
    return reinterpret_cast<sockaddr *>(&addr);
}

inline uint16_t IPv4Address::GetPort() const
{
    return addr.sin_port;
}

inline uint16_t IPv4Address::GetNTOHSPort() const
{
    return ntohs(addr.sin_port);
}

inline uint16_t IPv4Address::GetFamily() const
{
    return addr.sin_family;
}

inline in_addr_t IPv4Address::GetSinAddr() const
{
    return addr.sin_addr.s_addr;
}

inline char *IPv4Address::GetNTOASinAddr() const
{
    return ::inet_ntoa(addr.sin_addr);
}

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4ADDRESS_HPP_ */
