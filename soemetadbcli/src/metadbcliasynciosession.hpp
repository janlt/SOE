/**
 * @file    metadbcliasynciosession.hpp
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
 * metadbcliasynciosession.hpp
 *
 *  Created on: Jul 3, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBCLI_SRC_METADBCLIASYNCIOSESSION_HPP_
#define SOE_SOE_SOE_METADBCLI_SRC_METADBCLIASYNCIOSESSION_HPP_

using namespace std;
using namespace MetaMsgs;
using namespace MetaNet;

namespace MetadbcliFacade {

class AsyncIOSession: public EventCliAsyncSession
{
public:
    IPv4Address           my_addr;
    uint64_t              counter;
    uint64_t              bytes_received;
    uint64_t              bytes_sent;
    bool                  sess_debug;
    uint64_t              recv_count;
    uint64_t              sent_count;
    uint64_t              loop_count;
    boost::thread        *io_session_thread;
    Session              *parent_session;
    uint32_t              msg_pause;
    boost::thread        *in_thread;
    boost::thread        *out_thread;
    static uint32_t       session_counter;
    uint32_t              session_id;

    typedef enum Stet_ {
        eCreated    = 0,
        eDefunct    = 1
    } State;
    State                 state;
    const static uint32_t max_async_events = 100000;  // max fire off in one go
    const static uint32_t max_one_shot_events = 64;   // initial fire off that many in one go
    uint32_t              event_count;

    AsyncIOSession(Point &point,
            Session *_parent,
            uint32_t _msg_pause = 0,
            bool _test = false);

    virtual ~AsyncIOSession();

    virtual int processInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev);
    virtual int sendInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev);
    virtual int processOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev);
    virtual int sendOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev);

    virtual void inLoop();
    virtual void outLoop();

    void setSessDebug(bool _sess_debug)
    {
        sess_debug = _sess_debug;
    }
    int ParseSetAddresses(const std::string &ad_json);
    int Connect();
    int testAllIOStream(const IPv4Address &from_addr, const MetaNet::Point *pt, uint64_t loop_count = -1);
    virtual void start() throw(std::runtime_error);
};

}

#endif /* SOE_SOE_SOE_METADBCLI_SRC_METADBCLIASYNCIOSESSION_HPP_ */
