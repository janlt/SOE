/**
 * @file    msgnetprocessor.hpp
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
 * msgnetprocessor.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETPROCESSOR_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETPROCESSOR_HPP_

namespace MetaNet {

class Processor
{
public:
    Processor(bool _debug = false);
    virtual ~Processor() {}

    // Process CB arguments
    MetaMsgs::AdmMsgBuffer         *proc_buf;
    const IPv4Address              *proc_from_addr;

    // Send CB arguments
    const uint8_t                  *send_msg;
    uint32_t                        send_msg_size;
    const IPv4Address              *send_dst;

    bool                            debug;

    virtual void start() throw(std::runtime_error);

    void setDebug(bool _debug);

protected:
    virtual int processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);

    virtual int processStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::IoEvent *ev = 0);
    virtual int sendStreamEvent(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::IoEvent *ev = 0);

private:
    Processor(const Processor &right);
    Processor & operator=(const Processor &right);
};

inline void Processor::start() throw(std::runtime_error) {}
inline int Processor::processDgramMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt) { return -1; }
inline int Processor::processStreamMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt) { return -1; }
inline int Processor::sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst, const MetaNet::Point *pt) { return -1; }
inline int Processor::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst, const MetaNet::Point *pt) { return -1; }
inline int Processor::processStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::IoEvent *ev) { return -1; }
inline int Processor::sendStreamEvent(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::IoEvent *ev) { return -1; }

}



#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETPROCESSOR_HPP_ */
