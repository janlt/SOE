/**
 * @file    metadbsrvasynciosession.hpp
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
 * metadbsrvasynciosession.hpp
 *
 *  Created on: Jun 29, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBSRV_SRC_METADBSRVASYNCIOSESSION_HPP_
#define SOE_SOE_SOE_METADBSRV_SRC_METADBSRVASYNCIOSESSION_HPP_

using namespace MetaNet;
using namespace MetaMsgs;

namespace Metadbsrv {

//#define __DEBUG_ASYNC_IO__

//
// I/O Session
//
struct TestIov
{

};

class StoreProxy1;

class AsyncIOSession: public EventSrvAsyncSession
{
public:
    IPv4Address my_addr;

    static IPv4Address my_static_addr;
    static bool        my_static_addr_assigned;
    static Lock        my_static_addr_lock;

    uint64_t    counter;
    uint64_t    bytes_received;
    uint64_t    bytes_sent;
    bool        sess_debug;
    uint64_t    msg_recv_count;
    uint64_t    msg_sent_count;
    uint64_t    bytes_recv_count;
    uint64_t    bytes_sent_count;
    uint64_t    adm_session_id;
    uint64_t    idle_ticks;

    const static uint64_t                   invalid_adm_session_id = -1;

    typedef enum State_ {
        eCreated    = 0,
        eDestroyed  = 1,
        eDefunct    = 2
    } State;
    State       state;

    StoreProxy1 *store_proxy;

private:
    uint32_t     open_close_ref_count;
    Lock         open_close_ref_count_lock;

public:
    uint32_t     in_defunct_ttl;
    uint32_t     in_idle_ttl;

    static const uint64_t print_divider = 0x40000;

    const static uint32_t                   max_sessions = 100000;
    static std::shared_ptr<AsyncIOSession>  sessions[max_sessions];
    static uint32_t                         session_counter;
    static PlainLock                        global_lock;

    uint32_t       session_id;
    Session       *parent_session;
    MetaNet::EventListener &event_lis;
    boost::thread *in_thread;
    boost::thread *out_thread;

    typedef enum Type_ {
        eSyncAPISession  = 0,
        eAsyncAPISession = 1
    } Type;
    Type           type;

#ifdef __DEBUG_ASYNC_IO__
    static Lock                  io_debug_lock;
#endif

    AsyncIOSession(Point &point, MetaNet::EventListener &_event_lis, Session *_parent, bool _test = false, uint64_t _adm_s_id = AsyncIOSession::invalid_adm_session_id);

    virtual ~AsyncIOSession();

    virtual int processInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev);
    virtual int sendInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev);
    virtual int processOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev);
    virtual int sendOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev);

    virtual void inLoop();
    virtual void outLoop();

    int ParseSetAddresses(const std::string &ad_json);
    int Connect();

    void InitDesc(LocalArgsDescriptor &args) const;

    void setSessDebug(bool _sess_debug);

    void CloseAllStores();
    static int CloseAllSessionStores(uint64_t _adm_s_id);
    static void PurgeAllSessionsAndStores();

    static void SetMyAddres();

    uint32_t IncOpenCloseRefCount();
    uint32_t DecOpenCloseRefCount();
    uint32_t OpenCloseRefCountGet();
    uint32_t OpenCloseRefCountGetNoLock();
    uint32_t OpenCloseRefCountSet(uint32_t new_cnt);

protected:
    RcsdbFacade::ProxyNameBase *SetStoreProxy(const GetReq<TestIov> *gr);
    int sendInboundStreamEventError(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev, uint32_t req_type, RcsdbFacade::ProxyNameBase &error_proxy, RcsdbFacade::Exception::eStatus sts);
    int FindProxy(const GetReq<TestIov> *gr, MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev, uint32_t req_type, StatusBits &s_bits);
};

}

#endif /* SOE_SOE_SOE_METADBSRV_SRC_METADBSRVASYNCIOSESSION_HPP_ */
