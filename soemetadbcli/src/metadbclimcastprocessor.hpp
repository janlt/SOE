/**
 * @file    metadbclimcastprocessor.hpp
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
 * metadbclimcastprocessor.hpp
 *
 *  Created on: Jun 1, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBCLI_SRC_METADBCLIMCASTPROCESSOR_HPP_
#define SOE_SOE_SOE_METADBCLI_SRC_METADBCLIMCASTPROCESSOR_HPP_

namespace MetadbcliFacade {

//
// Mcast msg Processor (Part of Auto Discovery Protocol SOE-ADP)
//
struct McastlistCliProcessor: public Processor
{
    McastlistCliProcessor(uint32_t _num_io_sessions_per_adm_session = 1,
            bool _mm_sock_session = false,
            bool _debug = false,
            bool _msg_debug = false,
            bool _allow_remote_srv = false);
    ~McastlistCliProcessor();

    IPv4Address            srv_addr;   // metasrv address for communication
    IPv4Address            own_addr;
    uint64_t               counter;
    bool                   debug;
    bool                   msg_debug;
    bool                   allow_remote_srv;
    bool                   mm_sock_session;
    uint32_t               num_io_sessions_per_adm_session;
    uint32_t               timer_tics;


    int processAdvertisement(MetaMsgs::McastMsgMetaAd *ad, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);

    // This callback runs session recovery logic as well
    virtual int processDgram(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStream(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgram(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0);
    virtual int sendStream(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0);
    virtual int processTimer();

public:
    void sendSolicitations(MetaNet::Listener &lis);
};

}

#endif /* SOE_SOE_SOE_METADBCLI_SRC_METADBCLIMCASTPROCESSOR_HPP_ */
