/**
 * @file    msgnetipv4tcpsession.hpp
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
 * msgnetipv4tcpsession.hpp
 *
 *  Created on: Jan 11, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4TCPSESSION_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4TCPSESSION_HPP_

namespace MetaNet {

class IPv4TCPSession;

struct SessionHashEntry
{
    int                fd;
    IPv4TCPSession    *sess;
};

const uint32_t session_max = 1 << 12;

class IPv4TCPSession: public Session
{
public:
    IPv4TCPSession(TcpSocket &_sock, bool _test_mode = false) throw(std::runtime_error);
    virtual ~IPv4TCPSession();

    class Initializer
    {
    public:
        static bool initialized;
        Initializer();
        ~Initializer() {}
    };
    static Initializer sessions_initalized;

    // Note: no overhead of std or boost containers
    static SessionHashEntry  sessions[session_max];

    virtual void registerSession() throw(std::runtime_error);
    virtual void deregisterSession() throw(std::runtime_error);
    virtual int initialise(MetaNet::Point *lis_soc = 0, bool _ignore_exc = false) throw(std::runtime_error);
    virtual int deinitialise(bool _ignore_exc = false) throw(std::runtime_error);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);

    void setPeerAddr(const IPv4Address &_peer_addr);

public:
    TcpSocket        &tcp_sock;
    IPv4Address       peer_addr;
    bool              test_mode;

public:
    static uint64_t   open_counter;
    static uint64_t   close_counter;
};

inline void IPv4TCPSession::setPeerAddr(const IPv4Address &_peer_addr)
{
    peer_addr= _peer_addr;
}

inline int IPv4TCPSession::initialise(MetaNet::Point *lis_soc, bool _ignore) throw(std::runtime_error) { return 0; }
inline int IPv4TCPSession::deinitialise(bool _ignore) throw(std::runtime_error) { return 0; }

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIPV4TCPSESSION_HPP_ */
