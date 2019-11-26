/**
 * @file    metadbsrvadmsession.hpp
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
 * metadbsrvadmsession.hpp
 *
 *  Created on: Jun 29, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBSRV_SRC_METADBSRVADMSESSION_HPP_
#define SOE_SOE_SOE_METADBSRV_SRC_METADBSRVADMSESSION_HPP_

using namespace MetaNet;
using namespace MetaMsgs;

namespace Metadbsrv {

typedef boost::shared_mutex Lock;
typedef boost::unique_lock<Lock> WriteLock;
typedef boost::shared_lock<Lock> ReadLock;

//
// Adm Session
//
class AdmSession: public IPv4TCPSrvSession
{
    friend class AdmSrvProcessor;

public:
    IPv4Address my_addr;
    uint64_t    counter;
    uint64_t    bytes_received;
    uint64_t    bytes_sent;
    bool        sess_debug;
    uint64_t    session_id;
    Lock        adm_op_lock;

    static uint64_t                      session_id_counter;

    const static uint32_t                max_sessions = 100000;
    static std::shared_ptr<AdmSession>   sessions[max_sessions];
    static Lock                          global_lock;

    AdmSession(TcpSocket &soc, bool _test = false);

    virtual ~AdmSession();
    virtual int processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);

    void InitDesc(LocalArgsDescriptor &args) const;
    void InitAdmDesc(LocalAdmArgsDescriptor &args) const;

    int ParseSetAddresses(const std::string &ad_json);
    int Connect();

    void CloseAllStores();
    static void PurgeStaleStores();
    static void PruneArchive();

    void SetSessDebug(bool _sess_debug);
};

}

#endif /* SOE_SOE_SOE_METADBSRV_SRC_METADBSRVADMSESSION_HPP_ */
