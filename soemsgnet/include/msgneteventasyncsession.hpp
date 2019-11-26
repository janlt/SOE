/**
 * @file    msgneteventasyncsession.hpp
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
 * msgneteventasyncsession.hpp
 *
 *  Created on: Mar 14, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTASYNCSESSION_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTASYNCSESSION_HPP_

namespace MetaNet {

class EventAsyncSession: public EventSession<IoEvent>
{
public:
    EventAsyncSession(Point &_point, bool _test_mode = false) throw(std::runtime_error);
    virtual ~EventAsyncSession();

    int Start();

    virtual int SendIov(const IoEvent *io) const = 0;
    virtual int RecvIov(IoEvent *io) = 0;

    virtual int processInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev) = 0;
    virtual int sendInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev) = 0;
    virtual int processOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev) = 0;
    virtual int sendOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev) = 0;

protected:
    virtual void StartSend() throw(std::runtime_error) = 0;
    virtual void StartRecv() throw(std::runtime_error) = 0;
};

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTASYNCSESSION_HPP_ */
