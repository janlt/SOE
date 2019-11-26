/**
 * @file    msgnetdummylistener.hpp
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
 * msgnetdummylistener.hpp
 *
 *  Created on: Jan 10, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETDUMMYLISTENER_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETDUMMYLISTENER_HPP_

namespace MetaNet {

class DummyListener: public Listener
{
public:
    DummyListener();
    virtual ~DummyListener() {}

    // Process CB arguments
    MetaMsgs::AdmMsgBuffer         *proc_buf;
    const IPv4Address              *proc_from_addr;

    // Send CB arguments
    const uint8_t                  *send_msg;
    uint32_t                        send_msg_size;
    const IPv4Address              *send_dst;

    virtual void start() throw(std::runtime_error);
    virtual void setAssignedAddress(const IPv4Address &addr) throw(std::runtime_error);
    virtual int AddPoint(MetaNet::Point *_sock);
    virtual int RemovePoint(MetaNet::Point *_sock);

protected:
    virtual void createEndpoint() throw(std::runtime_error);
    virtual void registerListener();
    virtual int initialise(MetaNet::Point *lis_soc = 0, bool _ignore_exc = false) throw(std::runtime_error);
    virtual int deinitialise(bool _ignore_exc = false) throw(std::runtime_error);
    virtual int processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);
    virtual int processTimer();

private:
    DummyListener(const DummyListener &right);
    DummyListener & operator=(const DummyListener &right);
};

inline void DummyListener::start() throw(std::runtime_error) {}
inline void DummyListener::setAssignedAddress(const IPv4Address &addr) throw(std::runtime_error) {}
inline int DummyListener::AddPoint(MetaNet::Point *_sock) { return -1; }
inline int DummyListener::RemovePoint(MetaNet::Point *_sock) { return -1; }
inline void DummyListener::createEndpoint() throw(std::runtime_error) {}
inline void DummyListener::registerListener() {}
inline int DummyListener::initialise(MetaNet::Point *lis_soc, bool _ignore_exc) throw(std::runtime_error) { return 0; }
inline int DummyListener::deinitialise(bool _ignore_exc) throw(std::runtime_error) { return 0; }
inline int DummyListener::processDgramMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt) { return -1; }
inline int DummyListener::processStreamMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt) { return -1; }
inline int DummyListener::sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst, const MetaNet::Point *pt) { return -1; }
inline int DummyListener::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst, const MetaNet::Point *pt) { return -1; }
inline int DummyListener::processTimer() { return -1; }

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETDUMMYLISTENER_HPP_ */
