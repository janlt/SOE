/**
 * @file    metadbsrveventprocessor.hpp
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
 * metadbsrveventprocessor.hpp
 *
 *  Created on: Jun 29, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBSRV_SRC_METADBSRVEVENTPROCESSOR_HPP_
#define SOE_SOE_SOE_METADBSRV_SRC_METADBSRVEVENTPROCESSOR_HPP_

using namespace MetaNet;

namespace Metadbsrv {

//
// MetaNet::EventListener msg Processor
//
class EventLisSrvProcessor: public Processor
{
public:
    int CreateLocalUnixSockIOSession(const IPv4Address &_io_peer_addr, MetaNet::EventListener &_event_lis, const Point *_lis_pt, int epoll_fd);
    int CreateLocalMMSockIOSession(const IPv4Address &_io_peer_addr, MetaNet::EventListener &_event_lis, const Point *_lis_pt, int epoll_fd);
    int CreateRemoteIOSession(const IPv4Address &_io_peer_addr, MetaNet::EventListener &_event_lis, const Point *_lis_pt, int epoll_fd);

    //
    // Event functions
    //
    int processStreamListenerEvent(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase &_buf, const IPv4Address &from_addr, const MetaNet::IoEvent *ev = 0);
    int sendStreamListenerEvent(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::IoEvent *ev = 0);

    EventLisSrvProcessor(bool _debug = false);
    ~EventLisSrvProcessor();
    void setDebug(bool _debug);

    static MetaNet::EventListener *getEventListener();

    bool                           debug;
    static MetaNet::EventListener *eventlis;
};

}

#endif /* SOE_SOE_SOE_METADBSRV_SRC_METADBSRVEVENTPROCESSOR_HPP_ */
