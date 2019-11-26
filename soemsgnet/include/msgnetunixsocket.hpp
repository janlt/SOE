/**
 * @file    msgnetunixsocket.hpp
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
 * msgnetunixsocket.hpp
 *
 *  Created on: Mar 10, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETUNIXSOCKET_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETUNIXSOCKET_HPP_

namespace MetaNet {

class UnixSocket: public Socket
{
public:
    UnixSocket();
    virtual ~UnixSocket();

    virtual int Create(bool _srv = false);
    virtual int Close();
    virtual int Shutdown();
    virtual int Connect(const Address *addr);
    virtual int Accept(Address *addr);
    virtual int Create(int domain, int type, int protocol);
    virtual int SetSendBufferLen(int _buf_len);
    virtual int SetReceiveBufferLen(int _buf_len);
    virtual int SetReuseAddr();
    virtual int SetAddMcastMembership(const ip_mreq *_ip_mreq);
    virtual int SetMcastTtl(unsigned int _ttl_val);
    virtual int SetNonBlocking(bool _blocking);
    virtual int SetNoSigPipe();
    virtual int SetRecvTmout(uint32_t msecs);
    virtual int SetSndTmout(uint32_t msecs);
    virtual int Cork();
    virtual int Uncork();
    virtual int SetMcastLoop();
    virtual int SetNoDelay();
    virtual int Bind(const sockaddr *_addr);
    virtual int SetNoLinger(uint32_t tmout = 0);

    virtual int SendIov(MetaMsgs::IovMsgBase &io);
    virtual int RecvIov(MetaMsgs::IovMsgBase &io);

    virtual int Send(const uint8_t *io, uint32_t io_size);
    virtual int Recv(uint8_t *io, uint32_t io_size);

    virtual bool IsConnected();

    int CreateUnix();
    int Listen(int backlog);
};

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETUNIXSOCKET_HPP_ */
