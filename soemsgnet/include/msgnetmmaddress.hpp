/**
 * @file    msgnetmmaddress.hpp
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
 * msgnetmmaddress.hpp
 *
 *  Created on: Mar 13, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETMMADDRESS_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETMMADDRESS_HPP_

namespace MetaNet {

#define AF_MM 101

typedef struct _sockaddr_mm
{
    __SOCKADDR_COMMON (mm_);
    char mm_path[108];     /* Path name to MM file.  */
} sockaddr_mm;

class MMAddress: public Address
{
public:
    sockaddr_mm           addr;
    static sockaddr_mm    null_address;

public:
    MMAddress(const sockaddr_mm _addr = null_address)
        :addr(_addr)
    {}
    MMAddress(const MMAddress &r) throw(std::runtime_error);
    MMAddress(const char *_path, uint16_t _family = AF_MM) throw(std::runtime_error);
    virtual ~MMAddress() {}
    const sockaddr *AsSockaddr();
    sockaddr *AsNCSockaddr();
    const char *GetPath() const;
    void SetPath(const char *_path);
    uint16_t GetFamily() const;

    int OpenName();
    int CloseName();

    static const char *node_no_local_addr;
    static MMAddress findNodeNoLocalAddr(bool debug = false);

    virtual bool operator==(const Address &r);
    virtual bool operator!=(const Address &r);

    MMAddress &operator=(const MMAddress &r);
    MMAddress &operator=(const sockaddr_mm &_sockaddr_mm);
    bool operator==(const sockaddr_mm &_sockaddr_mm);
    bool operator!=(const sockaddr_mm &_sockaddr_mm);
};

inline MMAddress &MMAddress::operator=(const MMAddress &r)
{
    ::strcpy(this->addr.mm_path, r.addr.mm_path);
    this->addr.mm_family = r.addr.mm_family;

    return *this;
}

inline const sockaddr *MMAddress::AsSockaddr()
{
    return reinterpret_cast<const sockaddr *>(&addr);
}

inline sockaddr *MMAddress::AsNCSockaddr()
{
    return reinterpret_cast<sockaddr *>(&addr);
}

inline const char *MMAddress::GetPath() const
{
    return addr.mm_path;
}

inline void MMAddress::SetPath(const char *_path)
{
    ::strcpy(addr.mm_path, _path);
}

inline uint16_t MMAddress::GetFamily() const
{
    return addr.mm_family;
}

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETMMADDRESS_HPP_ */
