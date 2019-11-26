/**
 * @file    msgneteventsrvsession.hpp
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
 * msgneteventsrvsession.hpp
 *
 *  Created on: Mar 1, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTSRVSESSION_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTSRVSESSION_HPP_

namespace MetaNet {

template <class T>
class EventSrvSession: public EventSession<T>
{
public:
    EventSrvSession(Point &_point, bool _test_mode = false) throw(std::runtime_error);
    virtual ~EventSrvSession();

    virtual int initialise(MetaNet::Point *lis_soc = 0, bool _ignore_exc = false) throw(std::runtime_error);
    virtual int deinitialise(bool _ignore_exc = false) throw(std::runtime_error);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);

    virtual int processInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const T *ev);
    virtual int sendInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, T *&ev);
    virtual int processOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const T *ev);
    virtual int sendOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, T *&ev);
};

template <class T>
inline EventSrvSession<T>::EventSrvSession(Point &_point, bool _test_mode) throw(std::runtime_error)
    :EventSession<T>(_point, _test_mode)
{}

template <class T>
inline EventSrvSession<T>::~EventSrvSession()
{}

template <class T>
inline int EventSrvSession<T>::initialise(MetaNet::Point *lis_point, bool _ignore_exc) throw(std::runtime_error)
{
    if ( EventSession<T>::test_mode == false ) {

        if ( EventSession<T>::event_point.type == Point::eMM && lis_point->type == Point::eMM ) {
            MmPoint *mm_lis_point = dynamic_cast<MmPoint *>(lis_point);
            mm_lis_point->SetConnectMps(dynamic_cast<MmPoint &>(EventSession<T>::event_point));
        }
        int new_sd = lis_point->Accept(EventSession<T>::getPeerAddr());
        if ( new_sd < 0 ) {
            if ( _ignore_exc == false ) {
                throw std::runtime_error((std::string(__PRETTY_FUNCTION__) + " failed errno: " + to_string(errno)).c_str());
            }
            return new_sd;
        }
        EventSession<T>::event_point.SetDesc(new_sd);
        bool force_register = false;
        int try_cnt;
        for ( try_cnt = 0 ; try_cnt < 128 ; try_cnt++ ) {
            if ( EventSession<T>::event_point.Register(_ignore_exc, force_register) ) {
                if ( try_cnt >= 100 ) {
                    (void )EventSession<T>::event_point.Close();
                    if ( _ignore_exc == false ) {
                        throw std::runtime_error((std::string(__PRETTY_FUNCTION__) + " failed setting event_point errno: " + to_string(errno)).c_str());
                    }
                    return new_sd;
                } else if ( try_cnt >= 90 && try_cnt < 100 ) {
                    usleep(1000);
                    force_register = true;
                    continue;
                } else {
                    continue;
                }
            } else {
                break;
            }
        }
        if ( try_cnt >= 128 ) {
            return -1;
        }
    } else {
        EventSession<T>::events_open_counter++;
    }
    return 0;
}

template <class T>
inline int EventSrvSession<T>::deinitialise(bool _ignore_exc) throw(std::runtime_error)
{
    (void )EventSession<T>::event_point.Close();
    bool force_deregister = false;
    int try_cnt;
    for ( try_cnt = 0 ; try_cnt < 128 ; try_cnt++ ) {
        if ( EventSession<T>::event_point.Deregister(_ignore_exc, force_deregister) < 0 ) {
            if ( try_cnt >= 100 ) {
                if ( _ignore_exc == false ) {
                    throw std::runtime_error((std::string(__PRETTY_FUNCTION__) + " failed errno: " + to_string(errno)).c_str());
                }
                return -1;
            } else if ( try_cnt >= 90 && try_cnt < 100 ) {
                usleep(1000);
                force_deregister = true;
                continue;
            } else {
                continue;
            }
        } else {
            break;
        }
    }
    if ( try_cnt >= 128 ) {
        return -1;
    }
    if ( EventSession<T>::test_mode == true ) {
        EventSession<T>::events_close_counter++;
    }
    return 0;
}

template <class T>
inline int EventSrvSession<T>::processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt) { return 0; }

template <class T>
inline int EventSrvSession<T>::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt) { return 0; }

template <class T>
inline int EventSrvSession<T>::processInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const T *ev) { return 0; }

template <class T>
inline int EventSrvSession<T>::sendInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, T *&ev) { return 0; }

template <class T>
inline int EventSrvSession<T>::processOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const T *ev) { return 0; }

template <class T>
inline int EventSrvSession<T>::sendOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, T *&ev) { return 0; }

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETEVENTSRVSESSION_HPP_ */
