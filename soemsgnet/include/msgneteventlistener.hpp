/**
 * @file    msgneteventlistener.hpp
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
 * msgneteventlistener.hpp
 *
 *  Created on: Feb 10, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTLISTENER_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTLISTENER_HPP_

typedef int (CallbackEventProcessFun)(MetaNet::Listener &, MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt);
typedef int (CallbackEventSendFun)(MetaNet::Listener &, const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst_addr, const MetaNet::Point *pt);

typedef int (CallbackEventProcessSessionFun)(MetaNet::Listener &, MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::IoEvent *ev);
typedef int (CallbackEventSendSessionFun)(MetaNet::Listener &, const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst_addr, const MetaNet::IoEvent *ev);

typedef boost::mutex PlainLock;
typedef boost::shared_mutex Lock;
typedef boost::unique_lock<Lock> WriteLock;
typedef boost::shared_lock<Lock> ReadLock;

namespace MetaNet {

class EventListener: public Listener
{
public:
    EventListener(Socket &_sock, bool _no_bind = false) throw(std::runtime_error);
    virtual ~EventListener();

    virtual void setProcessDgramCB(boost::function<CallbackEventProcessFun> &cb);
    virtual void setProcessStreamCB(boost::function<CallbackEventProcessFun> &cb);
    virtual void setSendDgramCB(boost::function<CallbackEventSendFun> &cb);
    virtual void setSendStreamCB(boost::function<CallbackEventSendFun> &cb);

    virtual void setEventProcessSessionCB(boost::function<CallbackEventProcessSessionFun> &cb);
    virtual void setEventSendSessionCB(boost::function<CallbackEventSendSessionFun> &cb);

    void setIPv4OwnAddr();
    IPv4Address &getIPv4OwnAddr();

    void setUnixOwnAddr();
    UnixAddress &getUnixOwnAddr();

    void setMmOwnAddr();
    MMAddress &getMmOwnAddr();

    UnixSocket *getUnixSock();

    MmPoint *getMmSock();

    virtual void start() throw(std::runtime_error);
    virtual void startAsync() throw(std::runtime_error);
    virtual void setAssignedAddress(const IPv4Address &addr) throw(std::runtime_error);
    virtual int AddPoint(MetaNet::Point *_point);
    virtual int RemovePoint(MetaNet::Point *_point);

    int AddAsyncPoint(MetaNet::Point *_point, int e_fd);
    int RemoveAsyncPoint(MetaNet::Point *_point, int e_fd);
    int RearmAsyncPoint(MetaNet::Point *_point, int e_fd);


protected:
    virtual void createEndpoint() throw(std::runtime_error);
    virtual void registerListener();
    virtual int initialise(MetaNet::Point *lis_point = 0, bool _ignore_exc = false) throw(std::runtime_error);
    virtual int deinitialise(bool _ignore_exc = false) throw(std::runtime_error);
    virtual int processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0);
    virtual int processTimer();

    int processStreamAsyncEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev = 0);
    int sendStreamAsyncEvent(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const IoEvent *ev = 0);
    void initAsync(int &e_fd) throw(std::runtime_error);

    void createSocket() throw(std::runtime_error);

    IPv4Address          own_addr;
    UnixAddress          unix_own_addr;
    MMAddress            mm_own_addr;

    UnixSocket          *unix_sock;
    MmPoint             *mm_point;

    const static int     max_events = 100000;
    uint32_t             num_events;
    Lock                 add_remove_lock;
    int                  epoll_fds[max_events];
    Lock                 init_proc_lock;
    bool                 async_initialized;


    void dispatchClientEvent(int idx, IoEvent *ev) throw(std::runtime_error);
    void processListenerEvent(int idx, IoEvent *ev) throw(std::runtime_error);

    boost::function<CallbackEventProcessFun> proc_dgram_callback;
    boost::function<CallbackEventProcessFun> proc_stream_callback;
    boost::function<CallbackEventSendFun>    send_dgram_callback;
    boost::function<CallbackEventSendFun>    send_stream_callback;

    void dispatchClientAsyncEvent(int idx, IoEvent *ev) throw(std::runtime_error);
    void processListenerAsyncEvent(int idx, IoEvent *ev) throw(std::runtime_error);

    boost::function<CallbackEventProcessSessionFun> proc_event_stream_callback;
    boost::function<CallbackEventSendSessionFun>    send_event_stream_callback;

private:
    EventListener(const EventListener &right);
    EventListener & operator=(const EventListener &right);

public:
};

inline void EventListener::setProcessDgramCB(boost::function<CallbackEventProcessFun> &cb)
{
    proc_dgram_callback = cb;
}

inline void EventListener::setProcessStreamCB(boost::function<CallbackEventProcessFun> &cb)
{
    proc_stream_callback = cb;
}

inline void EventListener::setSendDgramCB(boost::function<CallbackEventSendFun> &cb)
{
    send_dgram_callback = cb;
}

inline void EventListener::setSendStreamCB(boost::function<CallbackEventSendFun> &cb)
{
    send_stream_callback = cb;
}

inline void EventListener::setEventProcessSessionCB(boost::function<CallbackEventProcessSessionFun> &cb)
{
    proc_event_stream_callback = cb;
}

inline void EventListener::setEventSendSessionCB(boost::function<CallbackEventSendSessionFun> &cb)
{
    send_event_stream_callback = cb;
}

inline void EventListener::setIPv4OwnAddr()
{
    own_addr = IPv4Address::findNodeNoLocalAddr();
}

inline IPv4Address &EventListener::getIPv4OwnAddr()
{
    return own_addr;
}

inline void EventListener::setUnixOwnAddr()
{
    unix_own_addr = UnixAddress::findNodeNoLocalAddr();
}

inline UnixAddress &EventListener::getUnixOwnAddr()
{
    return unix_own_addr;
}

inline UnixSocket *EventListener::getUnixSock()
{
    return unix_sock;
}

inline void EventListener::setMmOwnAddr()
{
    mm_own_addr = MMAddress::findNodeNoLocalAddr();
}

inline MMAddress &EventListener::getMmOwnAddr()
{
    return mm_own_addr;
}

inline MmPoint *EventListener::getMmSock()
{
    return mm_point;
}

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTLISTENER_HPP_ */
