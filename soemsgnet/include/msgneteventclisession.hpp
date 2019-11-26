/**
 * @file    msgneteventclisession.hpp
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
 * msgneteventclisession.hpp
 *
 *  Created on: Mar 1, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTCLISESSION_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTCLISESSION_HPP_

namespace MetaNet {

template <class T>
class EventCliSession: public EventSession<T>
{
public:
    EventCliSession(Point &_point, bool _test_mode = false) throw(std::runtime_error);
    virtual ~EventCliSession();

    virtual int initialise(MetaNet::Point *lis_soc = 0, bool _ignore_exc = false) throw(std::runtime_error);
    virtual int deinitialise(bool _ignore_exc = false) throw(std::runtime_error);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);

    virtual int processInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const T *ev);
    virtual int sendInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, T *&ev);
    virtual int processOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const T *ev);
    virtual int sendOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, T *&ev);
    virtual int outboundStreamPeekEventCount(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, uint32_t &count);
    virtual int inboundStreamPeekEventCount(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, uint32_t &count);
};

template <class T>
inline EventCliSession<T>::EventCliSession(Point &_point, bool _test_mode) throw(std::runtime_error)
    :EventSession<T>(_point, _test_mode)
{}

template <class T>
inline EventCliSession<T>::~EventCliSession()
{}

template <class T>
inline int EventCliSession<T>::initialise(MetaNet::Point *lis_point, bool _ignore_exc) throw(std::runtime_error)
{
    if ( EventSession<T>::test_mode == false ) {
        if ( EventSession<T>::event_point.Register(_ignore_exc) ) {
            if ( _ignore_exc == false ) {
                throw std::runtime_error((std::string(__PRETTY_FUNCTION__) + " failed errno: " + to_string(errno)).c_str());
            }
            return -1;
        }
        for ( ;; ) {
            if ( EventSession<T>::event_point.Connect(EventSession<T>::getPeerAddr()) < 0 ) {
                if ( errno == EINPROGRESS || errno == EINTR ) {
                    continue;
                }
                (void )EventSession<T>::event_point.Close();
                (void )EventSession<T>::event_point.Deregister(_ignore_exc);
                if ( _ignore_exc == false ) {
                    throw std::runtime_error((std::string(__PRETTY_FUNCTION__) + " failed errno: " + to_string(errno)).c_str());
                }
                return -1;
            }
            break;
        }
    } else {
        EventSession<T>::events_open_counter++;
    }
    return 0;
}

template <class T>
inline int EventCliSession<T>::deinitialise(bool _ignore_exc) throw(std::runtime_error)
{
    (void )EventSession<T>::event_point.Close();
    if ( EventSession<T>::event_point.Deregister(_ignore_exc) < 0 ) {
        if ( _ignore_exc == false ) {
            throw std::runtime_error((std::string(__PRETTY_FUNCTION__) + " failed errno: " + to_string(errno)).c_str());
        }
        return -1;
    }
    if ( EventSession<T>::test_mode == true ) {
        EventSession<T>::events_close_counter++;
    }
    return 0;
}

template <class T>
inline int EventCliSession<T>::processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt) { return 0; }

template <class T>
inline int EventCliSession<T>::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt) { return 0; }

template <class T>
inline int EventCliSession<T>::processInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const T *ev) { return 0; }

template <class T>
inline int EventCliSession<T>::sendInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, T *&ev) { return 0; }

template <class T>
inline int EventCliSession<T>::processOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const T *ev) { return 0; }

template <class T>
inline int EventCliSession<T>::sendOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, T *&ev) { return 0; }

template <class T>
inline int EventCliSession<T>::outboundStreamPeekEventCount(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, uint32_t &count) { return 0; }

template <class T>
inline int EventCliSession<T>::inboundStreamPeekEventCount(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, uint32_t &count) { return 0; }

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTCLISESSION_HPP_ */
