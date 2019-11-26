/**
 * @file    msgnetunixaddress.hpp
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
 * msgnetunixaddress.hpp
 *
 *  Created on: Mar 10, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETUNIXADDRESS_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETUNIXADDRESS_HPP_

namespace MetaNet {

class UnixAddress: public Address
{
public:
    sockaddr_un           addr;
    static sockaddr_un    null_address;

public:
    UnixAddress(const sockaddr_un _addr = null_address)
        :addr(_addr)
    {}
    UnixAddress(const UnixAddress &r) throw(std::runtime_error);
    UnixAddress(const char *_path, uint16_t _family = AF_UNIX) throw(std::runtime_error);
    virtual ~UnixAddress() {}
    const sockaddr *AsSockaddr();
    sockaddr *AsNCSockaddr();
    const char *GetPath() const;
    void SetPath(const char *_path);
    uint16_t GetFamily() const;

    int OpenName();
    int CloseName();

    static const char *node_no_local_addr;
    static UnixAddress findNodeNoLocalAddr(bool debug = false);

    virtual bool operator==(const Address &r);
    virtual bool operator!=(const Address &r);

    UnixAddress &operator=(const UnixAddress &r);
    UnixAddress &operator=(const sockaddr_un _sockaddr_un);
    bool operator==(const sockaddr_un _sockaddr_un);
    bool operator!=(const sockaddr_un _sockaddr_un);
};

inline UnixAddress &UnixAddress::operator=(const UnixAddress &r)
{
    ::strcpy(this->addr.sun_path, r.addr.sun_path);
    this->addr.sun_family = r.addr.sun_family;

    return *this;
}

inline const sockaddr *UnixAddress::AsSockaddr()
{
    return reinterpret_cast<const sockaddr *>(&addr);
}

inline sockaddr *UnixAddress::AsNCSockaddr()
{
    return reinterpret_cast<sockaddr *>(&addr);
}

inline const char *UnixAddress::GetPath() const
{
    return addr.sun_path;
}

inline void UnixAddress::SetPath(const char *_path)
{
    ::strcpy(addr.sun_path, _path);
}

inline uint16_t UnixAddress::GetFamily() const
{
    return addr.sun_family;
}

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETUNIXADDRESS_HPP_ */
