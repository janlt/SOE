/**
 * @file    msgnetipv4mcastlistener.cpp
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
 * msgnetipv4mcastlistener.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jan Lisowiec
 */


#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <stdexcept>
using namespace std;
#include <poll.h>
#include <netdb.h>

#include <boost/function.hpp>
#include <boost/thread/locks.hpp>

#include "soelogger.hpp"
using namespace SoeLogger;

#include "thread.hpp"
#include "threadfifoext.hpp"
#include "classalloc.hpp"
#include "generalpool.hpp"
#include "dbsrvmarshallable.hpp"
#include "dbsrvmsgbuffer.hpp"

#include "dbsrvmsgs.hpp"
#include "dbsrvmcasttypes.hpp"
#include "dbsrvmsgadmtypes.hpp"
#include "dbsrvmsgfactory.hpp"
#include "dbsrvmsgadmfactory.hpp"

#include "dbsrviovmsgsbase.hpp"
#include "dbsrviovmsgs.hpp"
#include "dbsrvmsgiovfactory.hpp"
#include "dbsrvmsgiovtypes.hpp"

#include "generalpool.hpp"
#include "dbsrvmsgbuffer.hpp"
#include "msgnetaddress.hpp"
#include "msgnetipv4address.hpp"
#include "msgnetunixaddress.hpp"
#include "msgnetmmaddress.hpp"
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnetunixsocket.hpp"
#include "msgnetmmpoint.hpp"
#include "msgnettcpsocket.hpp"
#include "msgnetudpsocket.hpp"
#include "msgnetlistener.hpp"
#include "msgnetdummylistener.hpp"
#include "msgnetipv4listener.hpp"
#include "msgnetipv4mcastlistener.hpp"

namespace MetaNet {

class DefaultIPv4McastProcessor: public DummyListener
{
public:
    int processDgram(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
    {
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got dgram req" << SoeLogger::LogDebug() << endl;
        return 0;
    }

    int processStream(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
    {
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got tcp req" << SoeLogger::LogDebug() << endl;
        return 0;
    }

    int sendDgram(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
    {
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Sending dgram rsp" << SoeLogger::LogDebug() << endl;
        return 0;
    }

    int sendStream(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
    {
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Sending tcp rsp" << SoeLogger::LogDebug() << endl;

        IPv4MulticastListener &mcast_lis = dynamic_cast<IPv4MulticastListener &>(lis);

        MetaMsgs::AdmMsgBuffer msg_buf;
        MetaMsgs::Uint8Vector vec(const_cast<uint8_t *>(send_msg), send_msg_size);
        msg_buf << vec;
        int res;

        res = ::sendto(mcast_lis.mcast_sock.GetSoc(),
            msg_buf.getBuf(),
            msg_buf.getOffset(),
            0,
            mcast_lis.dest_addr.AsSockaddr(),
            sizeof(mcast_lis.dest_addr));
        if ( res < 0 ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed " << res << SoeLogger::LogError() << endl;
        }

        return res;
    }

    int processTimer()
    {
        return -1;
    }

    DefaultIPv4McastProcessor(bool _debug = false)
        :debug(_debug)
    {}
    ~DefaultIPv4McastProcessor() {}

    bool     debug;
};

static DefaultIPv4McastProcessor dipv4mproc;

IPv4MulticastListener::IPv4MulticastListener(Socket &_sock, bool _mcast_client, bool _no_bind) throw(std::runtime_error)
    :IPv4Listener(&_sock, _no_bind),
    mcast_client(_mcast_client),
    mcast_sock(*(new UdpSocket()))
{
    // initialize msghdr structs
    ::memset(&send_msg_hdr, '\0', sizeof(send_msg_hdr));
    send_msg_hdr.msg_iov = new struct iovec[2];

    ::memset(&rcv_msg_hdr, '\0', sizeof(rcv_msg_hdr));
    rcv_msg_hdr.msg_iov = new struct iovec[1];
    rcv_msg_hdr.msg_iovlen = 1;
    rcv_msg_hdr.msg_iov[0].iov_base = new char[MetaMsgs::maxMsgBufSize + 64];
    rcv_msg_hdr.msg_iov[0].iov_len = MetaMsgs::maxMsgBufSize + 64;

    UdpSocket &udp_mcast_sock = dynamic_cast<UdpSocket &>(mcast_sock);
    int ret = udp_mcast_sock.CreateDgram();
    if ( ret < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Can't create dgram socket errno: " + to_string(errno)).c_str());
    }

    // set up the mcast destination address and bind it to the mcast socket
    dest_addr.addr.sin_family = AF_INET;
    uint16_t port = 2000;
    {
        const char* port_var = "DBSRV_MCAST_PORT";
        char* val;
        if ( (val = ::getenv(port_var)) != 0 ) {
            port = (unsigned short) atoi(val);
        }
    }
    std::string mgroup_addr("224.1.0.52");
    {
        const char* mgroup_var = "DBSRV_MCAST_ADDR";
        char* val;
        if ( (val = ::getenv(mgroup_var)) != 0 ) {
            mgroup_addr = val;
        }
    }
    dest_addr.addr.sin_port = htons(port);
    dest_addr.addr.sin_addr.s_addr = ::inet_addr(mgroup_addr.c_str());

#if BIND_MCAST_ADDR
    ret = mcast_sock.Bind(dest_addr.AsSockaddr(), sizeof(dest_addr));
    if ( ret ) {
        throw std::runtime_error((std::string(__FUNCTION__) + " Can't bind mcast addr").c_str());
    }
#endif

#ifndef _MCAST_TTL_
    unsigned int ttl_val = 0;
    ret = mcast_sock.SetMcastTtl(ttl_val);
#else
    ret = mcast_sock.SetMcastLoop();
#endif

    if ( ret < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) + " ret: " + to_string(ret) +
            " Can't set mcast loop/bind addr errno: " + to_string(errno)).c_str());
    }

    setAssignedAddress(dest_addr);

    // register default send dgram callback
    boost::function<CallbackIPv4SendFun> cd_cb = boost::bind(&DefaultIPv4McastProcessor::sendDgram,
        &dipv4mproc, _1, _2, _3, _4, _5);
    setSendDgramCB(cd_cb);
}

IPv4MulticastListener *IPv4MulticastListener::instance = 0;
Lock                   IPv4MulticastListener::global_lock;

IPv4MulticastListener *IPv4MulticastListener::getInstance(bool _mcast_client, bool _no_bind)
{
    WriteLock w_lock(IPv4MulticastListener::global_lock);

    if ( !IPv4MulticastListener::instance ) {
        IPv4MulticastListener::instance = new IPv4MulticastListener(*(new UdpSocket()), _mcast_client, _no_bind);
    }

    return IPv4MulticastListener::instance;
}

void IPv4MulticastListener::createSocket(Socket &_sock) throw(std::runtime_error)
{
    if ( _sock.Created() == true ) {
        return;
    }

    if ( _sock.GetType() == Socket::eType::eUdp ) {
        UdpSocket &udp_sock = dynamic_cast<UdpSocket &>(_sock);
        if ( udp_sock.CreateDgram() < 0 ) {
            throw std::runtime_error((std::string(__FUNCTION__) +
                " Can't create Dgram socket errno: " + to_string(errno)).c_str());
        }
    } else if ( _sock.GetType() == Socket::eType::eTcp ) {
        TcpSocket &tcp_sock = dynamic_cast<TcpSocket &>(_sock);
        if ( tcp_sock.CreateStream() < 0 ) {
            throw std::runtime_error((std::string(__FUNCTION__) +
                " Can't create Stream socket errno: " + to_string(errno)).c_str());
        }
    } else {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Invalid socket errno: " + to_string(errno)).c_str());
    }
}

void IPv4MulticastListener::createEndpoint() throw(std::runtime_error)
{
    // Make sure that we have created the socket
    // We will be binding to a particular address so must allow for this
    IPv4Listener::setReuseAddr(true);
    IPv4Listener::createEndpoint();

    // Now we need to subscribe to the multicast group
    ip_mreq mgroup;
    mgroup.imr_multiaddr.s_addr = dest_addr.addr.sin_addr.s_addr;
    mgroup.imr_interface.s_addr = htonl(INADDR_ANY);

    int max_count = 100; //10000000;
    for ( int try_c = 0 ; try_c < max_count ; try_c++ ) {
        int ret = getSock()->SetAddMcastMembership(&mgroup);
        if ( ret < 0 ) {
            if ( try_c >= max_count - 1 ) {
                throw std::runtime_error((std::string(__FUNCTION__) + " line: " + to_string(__LINE__) +
                    " Non Multicast address used errno: " + to_string(errno)).c_str());
            } else {
                usleep(10000);
                if ( !try_c || !(try_c%50) ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "SetAddMcastMembership failed errno: " << errno <<
                        " retrying ..." << SoeLogger::LogError() << endl;
                }
            }
        } else {
            break;
        }
    }

    if ( no_bind == false ) {
        if ( mcast_client == true ) {
            int ret = getSock()->SetReuseAddr();
            if ( ret ) {
                throw std::runtime_error((std::string(__FUNCTION__) + " Can't set reuse addr").c_str());
            }
        }

        for ( int try_c = 0 ; try_c < 10 ; try_c++ ) {
            int ret = getSock()->Bind(ipv4_addr->AsSockaddr());
            if ( ret ) {
                if ( try_c >= 9 ) {
                    throw std::runtime_error((std::string(__FUNCTION__) + " Can't bind dest_addr").c_str());
                } else {
                    usleep(10000);
                }
            } else {
                break;
            }
        }
    }
}

void IPv4MulticastListener::setAssignedAddress(const IPv4Address &addr) throw(std::runtime_error)
{
    ipv4_addr = new IPv4Address(addr);

    if ( !sock ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Null socket errno: " + to_string(errno)).c_str());
    }

    createSocket(*getSock());
    createEndpoint();
    registerListener();
}

void IPv4MulticastListener::start() throw(std::runtime_error)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Begining to listen for multicast messages" << SoeLogger::LogDebug() << endl;

    while (1) {
        int handler_ret = 0;

        // Form up a receive list to select on - this may be useful later
        // if we decide to multiplex listeners into a single thread
        pollfd readfd[1];
        readfd[0].fd = getSock()->GetSoc();
        readfd[0].events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI;

        // Poll the file descriptors with 400 mSecs timeout
        int ret = poll(readfd, 1, 400);
        if ( ret < 0 ) {
            if ( errno == EINTR ) {
                continue;
            } else {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "poll failed errno: " << errno << SoeLogger::LogError() << endl;
                break;
            }
        }

        IPv4Address from_addr;
        uint8_t buffer[2048];

        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Received message type: " << getSock()->GetType() <<
            " ret: " << ret << SoeLogger::LogDebug() << endl;

        if ( getSock()->GetType() == Socket::eType::eUdp ) {
            // Now create message buffer and process the message or timeout
            // Timeout should be used by "process" handler to send service advertisements
            if ( ret ) {  // real dgram
                int length;
                {
                    IPv4Address in_addr;
                    socklen_t in_addr_size = sizeof(in_addr);

                    length = ::recvfrom(getSock()->GetSoc(), buffer, sizeof(buffer), 0, in_addr.AsNCSockaddr(), &in_addr_size);
                    if ( length < 0 ) {
                        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "recvfrom failed errno: " << errno << length << SoeLogger::LogError() << endl;
                        break;
                    }
                    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Received " << length << SoeLogger::LogDebug() << endl;

                    // Form up a local address for the message
                    from_addr = in_addr;
                }

                if ( !length ) {
                    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Zero length message received" << SoeLogger::LogError() << endl;
                } else {
                    MetaMsgs::McastMsgBuffer buf(buffer, static_cast<uint32_t>(length));

                    handler_ret = processDgramMessage(buf, from_addr);
                    if ( handler_ret < 0 ) {
                        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "processDgramMessage: " << handler_ret << SoeLogger::LogError() << endl;
                    }
                }
            } else {  // timeout
                MetaMsgs::AdmMsgBuffer buf(buffer, static_cast<uint32_t>(0));
                from_addr = *ipv4_addr;

                handler_ret = processTimer();
                if ( handler_ret < 0 ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "processTimer: " << handler_ret << SoeLogger::LogDebug() << endl;
                }
            }
        } else if ( getSock()->GetType() == Socket::eType::eTcp ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "socket STREAM not supported by mcast errno: " << errno << SoeLogger::LogError() << endl;
            break;
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Invalid socket type errno: " << errno << SoeLogger::LogError() << endl;
            break;
        }
    }
}

int IPv4MulticastListener::processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( proc_dgram_callback ) {
        return proc_dgram_callback(*this, buf, from_addr, pt);
    }
    return 0;
}

int IPv4MulticastListener::processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( proc_stream_callback ) {
        return proc_stream_callback(*this, buf, from_addr, pt);
    }
    return 0;
}

int IPv4MulticastListener::sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
{
    if ( send_dgram_callback ) {
        return send_dgram_callback(*this, msg, msg_size, dst_addr, pt);
    }
    return 0;
}

int IPv4MulticastListener::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
{
    if ( send_stream_callback ) {
        return send_stream_callback(*this, msg, msg_size, dst_addr, pt);
    }
    return 0;
}

int IPv4MulticastListener::processTimer()
{
    if ( proc_timer_callback ) {
        return proc_timer_callback(*this);
    }
    return 0;
}

}

