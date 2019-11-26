/**
 * @file    msgnetsocket.hpp
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
 * msgnetsocket.hpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETSOCKET_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETSOCKET_HPP_


namespace MetaNet {

class Socket: public Point
{
public:
private:
    int      sock_opt_type;
    bool     async;

public:
    Socket(eType _type = eUndef);
    virtual ~Socket();

    virtual int Create(bool _srv = false) = 0;
    virtual int Close();
    virtual int Connect(const Address *addr) = 0;
    virtual int Accept(Address *addr) = 0;
    virtual int Create(int domain, int type, int protocol) = 0;
    virtual int SetSendBufferLen(int _buf_len);
    virtual int SetReceiveBufferLen(int _buf_len);
    virtual int SetReuseAddr();
    virtual int SetAddMcastMembership(const ip_mreq *_ip_mreq);
    virtual int SetMcastTtl(unsigned int _ttl_val);
    virtual int SetNonBlocking(bool _blocking);
    virtual int Cork();
    virtual int Uncork();
    virtual int SetMcastLoop();
    virtual int SetNoDelay();
    virtual int Bind(const sockaddr *_addr);
    virtual int SetNoSigPipe();
    virtual int SetRecvTmout(uint32_t msecs);
    virtual int SetSndTmout(uint32_t msecs);
    virtual int SetNoLinger(uint32_t tmout = 0);

    virtual int SendIov(MetaMsgs::IovMsgBase &io) = 0;
    virtual int RecvIov(MetaMsgs::IovMsgBase &io) = 0;

    virtual bool IsConnected();

    bool Created();
    int GetName(sockaddr *_addr);
    int SetSockOptType();
    int GetSoc();
    void SetSoc(int fd);
    int RecvExact(uint8_t *buf, uint32_t bytes_want, bool _ignore_sig);
    int SendExact(const uint8_t *data, uint32_t bytes_req, bool _ignore_sig);

private:
    void SetType();
    Socket(const Socket &_soc);
    Socket &operator =(const Socket &_soc);
};

inline int Socket::GetSoc()
{
    return GetDesc();
}

inline void Socket::SetSoc(int fd)
{
    SetDesc(fd);
}

}


#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETSOCKET_HPP_ */
