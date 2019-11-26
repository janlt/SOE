/**
 * @file    metadbsrvmcastprocessor.hpp
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
 * metadbsrvmcastprocessor.hpp
 *
 *  Created on: Jun 29, 2017
 *      Author: sysprog
 */

#ifndef SOE_SOE_SOE_METADBSRV_SRC_METADBSRVMCASTPROCESSOR_HPP_
#define SOE_SOE_SOE_METADBSRV_SRC_METADBSRVMCASTPROCESSOR_HPP_

using namespace MetaNet;

namespace Metadbsrv {

//
// IPv4McastListener msg Processor
//
class McastlistSrvProcessor: public Processor
{
public:
    virtual int processDgram(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStream(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgram(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0);
    virtual int sendStream(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0);
    int processEvent(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);

    McastlistSrvProcessor(bool _debug = false);
    ~McastlistSrvProcessor();

    int SendAd(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt);

private:
    IPv4Address   srv_addr;   // for sending advertisements
    bool          debug;
};

}

#endif /* SOE_SOE_SOE_METADBSRV_SRC_METADBSRVMCASTPROCESSOR_HPP_ */
