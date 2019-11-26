/**
 * @file    msgnetfifopoint.hpp
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
 * msgnetfifopoint.hpp
 *
 *  Created on: Mar 16, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETFIFOPOINT_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETFIFOPOINT_HPP_

namespace MetaNet {

class FifoPoint: public Point
{
public:
    FifoPoint();
    virtual ~FifoPoint();

    virtual int Create(bool _srv = false);
    virtual int Close();
    virtual int Shutdown();
    virtual int Connect(const Address *addr);
    virtual int Accept(Address *addr);
    virtual int Create(int domain, int type, int protocol);
    virtual int SetSendBufferLen(int _buf_len) { return 0; }
    virtual int SetReceiveBufferLen(int _buf_len) { return 0; }
    virtual int SetReuseAddr() { return 0; }
    virtual int SetAddMcastMembership(const ip_mreq *_ip_mreq) { return 0; }
    virtual int SetMcastTtl(unsigned int _ttl_val) { return 0; }
    virtual int SetNonBlocking(bool _blocking) { return 0; }
    virtual int SetNoSigPipe();
    virtual int SetRecvTmout(uint32_t msecs);
    virtual int SetSndTmout(uint32_t msecs);
    virtual int Cork() { return 0; }
    virtual int Uncork() { return 0; }
    virtual int SetMcastLoop() { return 0; }
    virtual int SetNoDelay() { return 0; }
    virtual int Bind(const sockaddr *_addr);
    virtual int SetNoLinger(uint32_t tmout = 0) { return 0; }

    virtual int SendIov(MetaMsgs::IovMsgBase &io);
    virtual int RecvIov(MetaMsgs::IovMsgBase &io);

    virtual int Send(const uint8_t *io, uint32_t io_size);
    virtual int Recv(uint8_t *io, uint32_t io_size);

    int CreateFifo(const sockaddr *_addr);
    int Listen(int backlog);

    int RecvExact(uint8_t *buf, uint32_t bytes_want, bool _ignore_sig);
    int SendExact(const uint8_t *data, uint32_t bytes_req, bool _ignore_sig);
};

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETFIFOPOINT_HPP_ */
