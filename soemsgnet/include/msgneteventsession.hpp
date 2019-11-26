/**
 * @file    msgneteventsession.hpp
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
 * msgneteventsession.hpp
 *
 *  Created on: Mar 1, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTSESSION_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTSESSION_HPP_

namespace MetaNet {

template <class T> class EventSession;

template <class T>
struct EventSessionHashEntry
{
    int                   fd;
    EventSession<T>      *sess;
};

const uint32_t event_session_max = 1 << 12;

template <class T>
class EventSession: public Session
{
public:
    EventSession(Point &_point, bool _test_mode = false) throw(std::runtime_error);
    virtual ~EventSession();

    class Initializer
    {
    public:
        static bool initialized;
        Initializer();
        ~Initializer() {}
    };
    static Initializer event_sessions_initalized;

    // Note: no overhead of std or boost containers
    static EventSessionHashEntry<T>  sessions[event_session_max];

    virtual void registerSession() throw(std::runtime_error);
    virtual void deregisterSession() throw(std::runtime_error);
    virtual int initialise(MetaNet::Point *point = 0, bool _ignore_exc = false) throw(std::runtime_error);
    virtual int deinitialise(bool _ignore_exc = false) throw(std::runtime_error);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);

    void setPeerAddr(Address *_peer_addr);
    Address *getPeerAddr();

    void ResetInFifo();
    void ResetOutFifo();

public:
    Point             &event_point;
    Address           *peer_addr;
    bool               test_mode;

    Thread_FIFO_t<T>   in_fifo;
    Thread_FIFO_t<T>   out_fifo;

public:
    static uint64_t   events_open_counter;
    static uint64_t   events_close_counter;
};

//
// statics
//
template <class T>
EventSessionHashEntry<T>  EventSession<T>::sessions[event_session_max];

template <class T> bool      EventSession<T>::Initializer::initialized = false;
template <class T> uint64_t  EventSession<T>::events_open_counter = 0;
template <class T> uint64_t  EventSession<T>::events_close_counter = 0;

template <class T>
inline EventSession<T>::Initializer::Initializer()
{
    if ( initialized != true ) {
        ::memset(EventSession<T>::sessions, '\0', sizeof(EventSession<T>::sessions));
    }
    initialized = true;
}

//
// Methods
//
template <class T>
inline EventSession<T>::EventSession(Point &_point, bool _test_mode) throw(std::runtime_error)
    :event_point(_point),
    peer_addr(0),
    test_mode(_test_mode)
{}

template <class T>
inline EventSession<T>::~EventSession()
{}

// Linux is guaranteeing fd uniqueness
template <class T>
inline void EventSession<T>::registerSession() throw(std::runtime_error)
{
    int idx = event_point.GetDesc();
    if ( idx < 0 || idx >= static_cast<int>(event_session_max) ||
            EventSession<T>::sessions[idx].fd || EventSession<T>::sessions[idx].sess ) {
        throw std::runtime_error((std::string(__PRETTY_FUNCTION__) + " Session hash corrupt fd: " + to_string(idx)).c_str());
    }
    EventSession<T>::sessions[idx].fd = idx;
    EventSession<T>::sessions[idx].sess = this;
}

template <class T>
inline void EventSession<T>::deregisterSession() throw(std::runtime_error)
{
    int idx = event_point.GetDesc();
    if ( idx < 0 || idx >= static_cast<int>(event_session_max) ||
        !EventSession<T>::sessions[idx].fd || !EventSession<T>::sessions[idx].sess ) {
        throw std::runtime_error((std::string(__PRETTY_FUNCTION__) + " Session hash corrupt fd: " + to_string(idx)).c_str());
    }
    EventSession<T>::sessions[idx].fd = 0;
    EventSession<T>::sessions[idx].sess = 0;
}

template <class T>
inline void EventSession<T>::ResetInFifo()
{
    in_fifo.reset();
}

template <class T>
inline void EventSession<T>::ResetOutFifo()
{
    out_fifo.reset();
}

//
// Virtual methods
//
template <class T>
inline int EventSession<T>::processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    return 0;
}

template <class T>
inline int EventSession<T>::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    return 0;
}

//
// Accessors
//
template <class T>
inline void EventSession<T>::setPeerAddr(Address *_peer_addr)
{
    peer_addr = _peer_addr;
}

template <class T>
inline Address *EventSession<T>::getPeerAddr()
{
    return peer_addr;
}

template <class T>
inline int EventSession<T>::initialise(MetaNet::Point *point, bool _ignore) throw(std::runtime_error) { return 0; }

template <class T>
inline int EventSession<T>::deinitialise(bool _ignore) throw(std::runtime_error) { return 0; }

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTSESSION_HPP_ */
