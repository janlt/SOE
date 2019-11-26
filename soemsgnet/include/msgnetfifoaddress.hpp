/**
 * @file    msgnetfifoaddress.hpp
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
 * msgnetfifoaddress.hpp
 *
 *  Created on: Mar 16, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETFIFOADDRESS_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETFIFOADDRESS_HPP_

namespace MetaNet {

#define AF_FIFO 102

typedef struct _sockaddr_fifo
{
    __SOCKADDR_COMMON (fifo_);
    char fifo_path[108];     /* Path name to FIFO file.  */
} sockaddr_fifo;

class FifoAddress: public Address
{
public:
    sockaddr_fifo           addr;
    static sockaddr_fifo    null_address;

public:
    FifoAddress(const sockaddr_fifo _addr = null_address)
        :addr(_addr)
    {}
    FifoAddress(const FifoAddress &r) throw(std::runtime_error);
    FifoAddress(const char *_path, uint16_t _family = AF_FIFO) throw(std::runtime_error);
    virtual ~FifoAddress() {}
    const sockaddr *AsSockaddr();
    sockaddr *AsNCSockaddr();
    const char *GetPath() const;
    void SetPath(const char *_path);
    uint16_t GetFamily() const;

    int OpenName();
    int CloseName();

    static const char *node_no_local_addr;
    static FifoAddress findNodeNoLocalAddr(bool debug = false);

    virtual bool operator==(const Address &r);
    virtual bool operator!=(const Address &r);

    FifoAddress &operator=(const FifoAddress &r);
    FifoAddress &operator=(const sockaddr_fifo &_sockaddr_fifo);
    bool operator==(const sockaddr_fifo &_sockaddr_fifo);
    bool operator!=(const sockaddr_fifo &_sockaddr_fifo);
};

inline FifoAddress &FifoAddress::operator=(const FifoAddress &r)
{
    ::strcpy(this->addr.fifo_path, r.addr.fifo_path);
    this->addr.fifo_family = r.addr.fifo_family;

    return *this;
}

inline const sockaddr *FifoAddress::AsSockaddr()
{
    return reinterpret_cast<const sockaddr *>(&addr);
}

inline sockaddr *FifoAddress::AsNCSockaddr()
{
    return reinterpret_cast<sockaddr *>(&addr);
}

inline const char *FifoAddress::GetPath() const
{
    return addr.fifo_path;
}

inline void FifoAddress::SetPath(const char *_path)
{
    ::strcpy(addr.fifo_path, _path);
}

inline uint16_t FifoAddress::GetFamily() const
{
    return addr.fifo_family;
}

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETFIFOADDRESS_HPP_ */
