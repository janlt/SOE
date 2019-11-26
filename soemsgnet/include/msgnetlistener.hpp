/**
 * @file    msgnetlistener.hpp
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
 * msgnetlistener.hpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETLISTENER_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETLISTENER_HPP_

namespace MetaNet {

class Listener
{
public:
    Listener(Socket *_sock = 0);
    virtual ~Listener() {}

    Socket *getSock();
    IPv4Address *getSockAddr();
    void setDebug(bool _debug);
    void setNoBind(bool _no_bind);
    void setClientMode(bool _client_mode);
    void setReuseAddr(bool _reuse_addr);


    virtual void start() throw(std::runtime_error) = 0;
    virtual void setAssignedAddress(const IPv4Address &addr) throw(std::runtime_error) = 0;
    virtual int AddPoint(MetaNet::Point *_sock) = 0;
    virtual int RemovePoint(MetaNet::Point *_sock) = 0;

protected:
    virtual void createEndpoint() throw(std::runtime_error) = 0;
    virtual void registerListener() = 0;
    virtual int initialise(MetaNet::Point *lis_soc = 0, bool _ignore_exc = false) throw(std::runtime_error) = 0;
    virtual int deinitialise(bool _ignore_exc = false) throw(std::runtime_error) = 0;
    virtual int processDgramMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt = 0) = 0;
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt = 0) = 0;
    virtual int sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst, const MetaNet::Point *pt = 0) = 0;
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst, const MetaNet::Point *pt = 0) = 0;
    virtual int processTimer() = 0;

    void getHostIdentity();

private:
    Listener(const Listener &right);
    Listener & operator=(const Listener &right);

protected:
    Socket       *sock;
    IPv4Address   sock_addr;
    bool          reuse_addr;
    bool          no_bind;
    bool          client_mode;
    bool          debug;
};

inline Socket *Listener::getSock()
{
    return sock;
}

inline IPv4Address *Listener::getSockAddr()
{
    return &sock_addr;
}
}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETLISTENER_HPP_ */
