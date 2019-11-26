/**
 * @file    msgnetipv4listener.hpp
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
 * msgnetipv4listener.hpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4LISTENER_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4LISTENER_HPP_

typedef int (CallbackIPv4ProcessFun)(MetaNet::Listener &, MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt);
typedef int (CallbackIPv4SendFun)(MetaNet::Listener &, const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst_addr, const MetaNet::Point *pt);
typedef int (CallbackIPv4ProcessTimerFun)(MetaNet::Listener &);

typedef boost::mutex PlainLock;
typedef boost::shared_mutex Lock;
typedef boost::unique_lock<Lock> WriteLock;
typedef boost::shared_lock<Lock> ReadLock;
typedef boost::unique_lock<PlainLock> WritePlainLock;

namespace MetaNet {

class IPv4Listener: public Listener
{
public:
    virtual ~IPv4Listener();

    virtual void setProcessDgramCB(boost::function<CallbackIPv4ProcessFun> &cb);
    virtual void setProcessStreamCB(boost::function<CallbackIPv4ProcessFun> &cb);
    virtual void setSendDgramCB(boost::function<CallbackIPv4SendFun> &cb);
    virtual void setSendStreamCB(boost::function<CallbackIPv4SendFun> &cb);
    virtual void setProcessTimerCB(boost::function<CallbackIPv4ProcessTimerFun> &cb);

    static IPv4Listener *getInstance();
    static IPv4Listener *setInstance(Socket *sock);

    const IPv4Address *getIPv4Addr() const;
    void setIPv4OwnAddr();
    IPv4Address &getIPv4OwnAddr();

    void processListenerEvent(int ret);
    void processClientEvent(uint32_t idx, int ret);

    virtual void start() throw(std::runtime_error);
    virtual void setAssignedAddress(const IPv4Address &addr) throw(std::runtime_error);
    virtual int AddPoint(MetaNet::Point *_sock);
    virtual int RemovePoint(MetaNet::Point *_sock);

protected:
    virtual void createEndpoint() throw(std::runtime_error);
    virtual void registerListener();
    virtual int initialise(MetaNet::Point *lis_soc = 0, bool _ignore = false) throw(std::runtime_error);
    virtual int deinitialise(bool _ignore_exc = false) throw(std::runtime_error);
    virtual int processDgramMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst_addr, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst_addr, const MetaNet::Point *pt = 0);
    virtual int processTimer();

    IPv4Listener(Socket *sock, bool _no_bind = false);
    void getHostIdentity();
    void createSocket() throw(std::runtime_error);

private:
    IPv4Listener(const IPv4Listener &right);
    IPv4Listener & operator=(const IPv4Listener &right);

protected:
    IPv4Address                    *ipv4_addr;
    IPv4Address                     own_addr;

public:
    static const uint32_t           max_fds = 1024;
    pollfd                          fds[max_fds];
    uint32_t                        num_fds;

protected:
    static Lock                     global_lock;
    static IPv4Listener            *instance;

    boost::function<CallbackIPv4ProcessFun>       proc_dgram_callback;
    boost::function<CallbackIPv4ProcessFun>       proc_stream_callback;
    boost::function<CallbackIPv4SendFun>          send_dgram_callback;
    boost::function<CallbackIPv4SendFun>          send_stream_callback;
    boost::function<CallbackIPv4ProcessTimerFun>  proc_timer_callback;
};

inline const IPv4Address *IPv4Listener::getIPv4Addr() const
{
    return ipv4_addr;
}

inline void IPv4Listener::setProcessDgramCB(boost::function<CallbackIPv4ProcessFun> &cb)
{
    proc_dgram_callback = cb;
}

inline void IPv4Listener::setProcessStreamCB(boost::function<CallbackIPv4ProcessFun> &cb)
{
    proc_stream_callback = cb;
}

inline void IPv4Listener::setSendDgramCB(boost::function<CallbackIPv4SendFun> &cb)
{
    send_dgram_callback = cb;
}

inline void IPv4Listener::setSendStreamCB(boost::function<CallbackIPv4SendFun> &cb)
{
    send_stream_callback = cb;
}

inline void IPv4Listener::setProcessTimerCB(boost::function<CallbackIPv4ProcessTimerFun> &cb)
{
    proc_timer_callback = cb;
}

}


#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4LISTENER_HPP_ */
