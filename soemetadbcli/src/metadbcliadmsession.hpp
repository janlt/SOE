/**
 * @file    metadbcliadmsession.hpp
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
 * metadbcliadmsession.hpp
 *
 *  Created on: Jun 1, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBCLI_SRC_METADBCLIADMSESSION_HPP_
#define SOE_SOE_SOE_METADBCLI_SRC_METADBCLIADMSESSION_HPP_

namespace MetadbcliFacade {

//
// Client Adm Session (Singleton)
// Used to manage name space, i.e. create/delete clusters/spaces/containers/snaphots/backups
//
class AdmSession: public IPv4TCPCliSession
{
public:
    IPv4Address          own_addr;
    uint64_t             counter;
    bool                 started;
    uint64_t             run_count;
    bool                 unix_sock_session;
    IPv4Address          io_peer_addr;

    static Lock          adm_session_lock;
    static AdmSession   *adm_session;

protected:
    AdmSession(TcpSocket &soc, bool _unix_sock_session = true, bool _debug = false, bool _test = false);
    virtual ~AdmSession();

public:
    static AdmSession *GetAdmSession();
    static AdmSession *CreateAdmSession(TcpSocket &soc, bool _unix_sock_session = true, bool _debug = false, bool _test = false);
    static void DestroyAdmSession();

    virtual int processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);

    virtual void start() throw(std::runtime_error);

    int ParseSetAddresses(const std::string &ad_json);
    int Connect();

    MetaIOSession<TestIov> *CreateLocalUnixSockIOSession(const IPv4Address &_io_peer_addr);
    MetaIOSession<TestIov> *CreateLocalMMSockIOSession(const IPv4Address &_io_peer_addr);
    MetaIOSession<TestIov> *CreateRemoteIOSession(const IPv4Address &_io_peer_addr);

    AsyncIOSession *CreateLocalUnixSockAsyncIOSession(const IPv4Address &_io_peer_addr, bool start_in = true, bool start_out = true);
    AsyncIOSession *CreateLocalMMSockAsyncIOSession(const IPv4Address &_io_peer_addr, bool start_in = true, bool start_out = true);
    AsyncIOSession *CreateRemoteAsyncIOSession(const IPv4Address &_io_peer_addr, bool start_in = true, bool start_out = true);

    void setIOPeerAddr(const IPv4Address &_io_peer_addr);

    static TcpSocket *GetAdmSessionSock();
};

}

#endif /* SOE_SOE_SOE_METADBCLI_SRC_METADBCLIADMSESSION_HPP_ */
