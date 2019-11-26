/**
 * @file    msgnetipv4mcastlistener.hpp
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
 * msgnetipv4mcastlistener.hpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4MCASTLISTENER_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4MCASTLISTENER_HPP_

typedef boost::shared_mutex Lock;
typedef boost::unique_lock<Lock> WriteLock;
typedef boost::shared_lock<Lock> ReadLock;

namespace MetaNet {

class IPv4MulticastListener: public IPv4Listener
{
public:
    virtual ~IPv4MulticastListener() {}

    virtual void setProcessDgramCB(boost::function<CallbackIPv4ProcessFun> &cb);
    virtual void setProcessStreamCB(boost::function<CallbackIPv4ProcessFun> &cb);
    virtual void setSendDgramCB(boost::function<CallbackIPv4SendFun> &cb);
    virtual void setSendStreamCB(boost::function<CallbackIPv4SendFun> &cb);
    virtual void setProcessTimerCB(boost::function<CallbackIPv4ProcessTimerFun> &cb);

    static IPv4MulticastListener *getInstance(bool _mcast_client = false, bool _no_bind = false);
    void setMcastClientMode(bool _mcast_client);

    virtual void start() throw(std::runtime_error);
    virtual void setAssignedAddress(const IPv4Address &addr) throw(std::runtime_error);
    virtual int AddPoint(MetaNet::Point *_sock);
    virtual int RemovePoint(MetaNet::Point *_sock);

protected:
    IPv4MulticastListener(Socket &sock, bool _mcast_client = false, bool _no_bind = false) throw(std::runtime_error);
    void createSocket(Socket &_sock) throw(std::runtime_error);

    virtual void createEndpoint() throw(std::runtime_error);
    virtual void registerListener();
    virtual int initialise(MetaNet::Point *lis_soc = 0, bool _ignore_exc = false) throw(std::runtime_error);
    virtual int deinitialise(bool _ignore_exc = false) throw(std::runtime_error);
    virtual int processDgramMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst_addr, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst_addr, const MetaNet::Point *pt = 0);
    virtual int processTimer();

    static Lock                     global_lock;
    static IPv4MulticastListener   *instance;

private:
    IPv4MulticastListener(const IPv4MulticastListener &right);
    IPv4MulticastListener & operator=(const IPv4MulticastListener &right);

    bool          mcast_client;

public:
    IPv4Address   dest_addr;
    Socket       &mcast_sock;

    struct msghdr send_msg_hdr;
    struct msghdr rcv_msg_hdr;
};

inline void IPv4MulticastListener::setProcessDgramCB(boost::function<CallbackIPv4ProcessFun> &cb)
{
    proc_dgram_callback = cb;
}

inline void IPv4MulticastListener::setProcessStreamCB(boost::function<CallbackIPv4ProcessFun> &cb)
{
    proc_stream_callback = cb;
}

inline void IPv4MulticastListener::setSendDgramCB(boost::function<CallbackIPv4SendFun> &cb)
{
    send_dgram_callback = cb;
}

inline void IPv4MulticastListener::setSendStreamCB(boost::function<CallbackIPv4SendFun> &cb)
{
    send_stream_callback = cb;
}

inline void IPv4MulticastListener::setProcessTimerCB(boost::function<CallbackIPv4ProcessTimerFun> &cb)
{
    proc_timer_callback = cb;
}

inline void IPv4MulticastListener::setMcastClientMode(bool _mcast_client)
{
    mcast_client = _mcast_client;
}

inline void IPv4MulticastListener::registerListener() {}
inline int IPv4MulticastListener::initialise(MetaNet::Point *lis_soc, bool _ignore_exc)  throw(std::runtime_error) { return 0; }
inline int IPv4MulticastListener::deinitialise(bool _ignore_exc) throw(std::runtime_error) { return 0; }
inline int IPv4MulticastListener::AddPoint(Point *_sock) { return -1; }
inline int IPv4MulticastListener::RemovePoint(Point *_sock) { return -1; }
}


#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4MCASTLISTENER_HPP_ */
