/**
 * @file    msgnettcplistener.hpp
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
 * msgnettcplistener.hpp
 *
 *  Created on: Jan 11, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4TCPLISTENER_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4TCPLISTENER_HPP_

namespace MetaNet {

class IPv4TCPListener: public IPv4Listener
{
public:
    IPv4TCPListener(Socket *sock = 0) throw(std::runtime_error);
    virtual ~IPv4TCPListener();

    virtual void setProcessStreamCB(boost::function<CallbackIPv4ProcessFun> &cb);
    virtual void setSendStreamCB(boost::function<CallbackIPv4SendFun> &cb);
    virtual void setProcessDgramCB(boost::function<CallbackIPv4ProcessFun> &cb);
    virtual void setSendDgramCB(boost::function<CallbackIPv4SendFun> &cb);
    virtual void setProcessTimerCB(boost::function<CallbackIPv4ProcessTimerFun> &cb);

    void setPeerAddr(const IPv4Address &_peer_addr);
    void setOwnAddr(const IPv4Address &_own_addr);

    virtual void start() throw(std::runtime_error);
    virtual void setAssignedAddress(const IPv4Address &addr) throw(std::runtime_error);
    virtual int AddPoint(MetaNet::Point *_sock);
    virtual int RemovePoint(MetaNet::Point *_sock);

protected:
    virtual void createEndpoint() throw(std::runtime_error);
    virtual void registerListener();
    virtual int initialise(MetaNet::Point *lis_soc = 0, bool _ignore_exc = false) throw(std::runtime_error);
    virtual int deinitialise(bool _ignore_exc = false) throw(std::runtime_error);
    virtual int processDgramMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst_addr, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst_addr, const MetaNet::Point *pt = 0);
    virtual int processTimer();

private:
    IPv4TCPListener(const IPv4TCPListener &right);
    IPv4TCPListener & operator=(const IPv4TCPListener &right);

    IPv4Address   peer_addr;
    IPv4Address   own_addr;
};

inline void IPv4TCPListener::setProcessStreamCB(boost::function<CallbackIPv4ProcessFun> &cb)
{
    proc_stream_callback = cb;
}

inline void IPv4TCPListener::setSendStreamCB(boost::function<CallbackIPv4SendFun> &cb)
{
    send_stream_callback = cb;
}

inline void IPv4TCPListener::setProcessDgramCB(boost::function<CallbackIPv4ProcessFun> &cb) {}
inline void IPv4TCPListener::setSendDgramCB(boost::function<CallbackIPv4SendFun> &cb) {}
inline void IPv4TCPListener::setProcessTimerCB(boost::function<CallbackIPv4ProcessTimerFun> &cb) {}
inline int IPv4TCPListener::sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst_addr, const MetaNet::Point *pt) { return -1; }
inline int IPv4TCPListener::processDgramMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt) { return -1; }
inline int IPv4TCPListener::AddPoint(MetaNet::Point *_sock) { return -1; }
inline int IPv4TCPListener::RemovePoint(MetaNet::Point *_sock) { return -1; }

inline void IPv4TCPListener::setPeerAddr(const IPv4Address &_peer_addr)
{
    peer_addr = _peer_addr;
}

inline void IPv4TCPListener::setOwnAddr(const IPv4Address &_own_addr)
{
    own_addr = _own_addr;
}

inline void IPv4TCPListener::registerListener() {}
inline int IPv4TCPListener::initialise(MetaNet::Point *lis_soc, bool _ignore_exc)  throw(std::runtime_error) { return 0; }
inline int IPv4TCPListener::deinitialise(bool _ignore_exc) throw(std::runtime_error) { return 0; }

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4TCPLISTENER_HPP_ */
