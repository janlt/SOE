/**
 * @file    msgnetipv4tcpclisession.hpp
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
 * msgnetipv4tcpclisession.hpp
 *
 *  Created on: Jan 11, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4TCPCLISESSION_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4TCPCLISESSION_HPP_

namespace MetaNet {

class IPv4TCPCliSession: public IPv4TCPSession
{
public:
    IPv4TCPCliSession(TcpSocket &_sock, bool _test_mode = false) throw(std::runtime_error);
    virtual ~IPv4TCPCliSession();

    virtual int initialise(MetaNet::Point *lis_soc = 0, bool _ignore_exc = false) throw(std::runtime_error);
    virtual int deinitialise(bool _ignore_exc = false) throw(std::runtime_error);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);
};

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4TCPCLISESSION_HPP_ */
