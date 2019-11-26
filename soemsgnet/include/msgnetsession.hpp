/**
 * @file    msgnetsession.hpp
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
 * msgnetsession.hpp
 *
 *  Created on: Mar 21, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETSESSION_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETSESSION_HPP_

namespace MetaNet {

class Session
{
public:
    Session(bool _debug = false);
    virtual ~Session() {}

    // Process CB arguments
    MetaMsgs::AdmMsgBuffer         *proc_buf;
    const IPv4Address              *proc_from_addr;

    // Send CB arguments
    const uint8_t                  *send_msg;
    uint32_t                        send_msg_size;
    const IPv4Address              *send_dst;

    bool                            debug;

    virtual void start() throw(std::runtime_error);
    virtual int AddPoint(MetaNet::Point *_sock);
    virtual int RemovePoint(MetaNet::Point *_sock);

    void setDebug(bool _debug);

protected:
    virtual void registerSession() throw(std::runtime_error);
    virtual void deregisterSession() throw(std::runtime_error);
    virtual int initialise(MetaNet::Point *lis_soc = 0, bool _ignore_exc = false) throw(std::runtime_error);
    virtual int deinitialise(bool _ignore_exc = false) throw(std::runtime_error);
    virtual int processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);

private:
    Session(const Session &right);
    Session & operator=(const Session &right);
};

inline void Session::start() throw(std::runtime_error) {}
inline int Session::AddPoint(MetaNet::Point *_sock) { return -1; }
inline int Session::RemovePoint(MetaNet::Point *_sock) { return -1; }
inline void Session::registerSession() throw(std::runtime_error) {}
inline void Session::deregisterSession() throw(std::runtime_error) {}
inline int Session::initialise(MetaNet::Point *lis_soc, bool _ignore_exc) throw(std::runtime_error) { return 0; }
inline int Session::deinitialise(bool _ignore_exc) throw(std::runtime_error) { return 0; }
inline int Session::processDgramMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt) { return -1; }
inline int Session::processStreamMessage(MetaMsgs::MsgBufferBase& buf, const MetaNet::IPv4Address &from_addr, const MetaNet::Point *pt) { return -1; }
inline int Session::sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst, const MetaNet::Point *pt) { return -1; }
inline int Session::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const MetaNet::IPv4Address *dst, const MetaNet::Point *pt) { return -1; }

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETSESSION_HPP_ */
