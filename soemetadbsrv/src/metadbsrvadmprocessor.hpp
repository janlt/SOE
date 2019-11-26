/**
 * @file    metadbsrvadmprocessor.hpp
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
 * metadbsrvadmprocessor.hpp
 *
 *  Created on: Jun 29, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBSRV_SRC_METADBSRVADMPROCESSOR_HPP_
#define SOE_SOE_SOE_METADBSRV_SRC_METADBSRVADMPROCESSOR_HPP_

using namespace MetaNet;
using namespace SoeLogger;

namespace Metadbsrv {

//
// IPv4Listener msg Processor
//
class AdmSrvProcessor: public Processor
{
public:
    virtual int processDgram(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStream(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgram(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0);
    virtual int sendStream(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0);
    virtual int processTimeout();

    AdmSrvProcessor(bool _debug = false);
    ~AdmSrvProcessor();

private:
    bool                debug;
    SoeLogger::Logger   logger;
};

}

#endif /* SOE_SOE_SOE_METADBSRV_SRC_METADBSRVADMPROCESSOR_HPP_ */
