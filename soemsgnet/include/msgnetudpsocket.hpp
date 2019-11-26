/**
 * @file    msgnetudpsocket.hpp
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
 * msgnetudpsocket.hpp
 *
 *  Created on: Feb 6, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETUDPSOCKET_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETUDPSOCKET_HPP_

namespace MetaNet {

class UdpSocket: public Socket
{
public:
    UdpSocket();
    virtual ~UdpSocket();

    virtual int Create(bool _srv = false);
    virtual int Close();
    virtual int Shutdown() { return -1; }
    virtual int Connect(const Address *addr) { return -1; }
    virtual int Accept(Address *addr) { return -1; }
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

    virtual int SendIov(MetaMsgs::IovMsgBase &io) { return -1; }
    virtual int RecvIov(MetaMsgs::IovMsgBase &io) { return -1; }

    virtual int Send(const uint8_t *io, uint32_t io_size) { return -1; }
    virtual int Recv(uint8_t *io, uint32_t io_size) { return -1; }

    virtual bool IsConnected() { return false; }

    int CreateDgram();
};

}

#endif /* SOE_SOE__SOE_MSGNET_INCLUDE_MSGNETUDPSOCKET_HPP_ */
