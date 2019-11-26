/**
 * @file    msgnettcplistener.cpp
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
 * msgnettcplistener.cpp
 *
 *  Created on: Jan 11, 2017
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

#include <boost/shared_ptr.hpp>
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

#include "classalloc.hpp"
#include "generalpool.hpp"
#include "dbsrvmsgbuffer.hpp"
#include "msgnetaddress.hpp"
#include "msgnetipv4address.hpp"
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnettcpsocket.hpp"
#include "msgnetudpsocket.hpp"
#include "msgnetlistener.hpp"
#include "msgnetdummylistener.hpp"
#include "msgnetipv4listener.hpp"
#include "msgnetipv4tcplistener.hpp"

namespace MetaNet {

class DefaultIPv4TCPProcessor: public DummyListener
{
public:
    int processStream(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
    {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got tcp req" << SoeLogger::LogDebug() << endl;
        return 0;
    }

    int sendStream(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
    {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Sending tcp rsp" << SoeLogger::LogDebug() << endl;
        return 0;
    }

    int processTimer()
    {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Timer tcp" << SoeLogger::LogDebug() << endl;
        return 0;
    }

    DefaultIPv4TCPProcessor() {}
    ~DefaultIPv4TCPProcessor() {}
};

static DefaultIPv4TCPProcessor dipv4tcpproc;

IPv4TCPListener::IPv4TCPListener(Socket *sock) throw(std::runtime_error)
    :IPv4Listener(sock)
{
    // register default send dgram callback
    boost::function<CallbackIPv4SendFun> s_cb = boost::bind(&MetaNet::DefaultIPv4TCPProcessor::sendStream,
        &dipv4tcpproc, _1, _2, _3, _4, _5);
    setSendStreamCB(s_cb);

    boost::function<CallbackIPv4ProcessFun> p_cb = boost::bind(&MetaNet::DefaultIPv4TCPProcessor::processStream,
        &dipv4tcpproc, _1, _2, _3, _4);
    setProcessStreamCB(p_cb);

    boost::function<CallbackIPv4ProcessTimerFun> t_cb = boost::bind(&MetaNet::DefaultIPv4TCPProcessor::processTimer,
        &dipv4tcpproc);
    setProcessTimerCB(t_cb);
}

IPv4TCPListener::~IPv4TCPListener()
{}

void IPv4TCPListener::createEndpoint() throw(std::runtime_error)
{}

void IPv4TCPListener::setAssignedAddress(const IPv4Address &addr) throw(std::runtime_error)
{
    ipv4_addr = new IPv4Address(addr);

    TcpSocket *tcp_sock = dynamic_cast<TcpSocket *>(sock);
    if ( !tcp_sock ) {
        throw std::runtime_error((std::string(__FUNCTION__) + " Wrong socket type errno: " + to_string(errno)).c_str());
    }

    if ( !tcp_sock->Created() ) {
        if ( tcp_sock->CreateStream() < 0 ) {
            throw std::runtime_error((std::string(__FUNCTION__) +
                " Can't create SOCK_DGRAM socket errno: " + to_string(errno)).c_str());
        }
    }

    createEndpoint();
    registerListener();
}

void IPv4TCPListener::start() throw(std::runtime_error)
{
    if ( !sock ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Null socket errno: " + to_string(errno)).c_str());
    }

    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Begin listening for tcp messages " << getSock()->GetSoc() << SoeLogger::LogDebug() << endl;

    int ret = AddPoint(getSock());
    if ( ret < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Can't add own socket errno: " + to_string(errno)).c_str());
    }

    while (1) {
        // Form up a receive list to select on - this may be useful later
        // if we decide to multiplex listeners into a single thread
        pollfd readfd[1];
        readfd[0].fd = getSock()->GetSoc();
        readfd[0].events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI;

        // Poll the file descriptors with infinite timeout
        int ret = poll(readfd, 1, 400);
        if ( ret < 0 ) {
            if ( errno == EINTR ) {
                continue;
            } else {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " poll failed errno: " << errno << SoeLogger::LogError() << endl;
                break;
            }
        }

        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Received a message type: " << getSock()->GetType() <<
            " ret: " << ret << SoeLogger::LogDebug() << endl;
        if ( getSock()->GetType() == Socket::eType::eTcp ) {
            // Here handle TCP events
        } else if ( getSock()->GetType() == Socket::eType::eUdp ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " socket DGRAM not supported by tcp errno: " << errno << SoeLogger::LogError() << endl;
            break;
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid socket type errno: " << errno << SoeLogger::LogError() << endl;
            break;
        }
    }
}

int IPv4TCPListener::processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( proc_stream_callback ) {
        return proc_stream_callback(*this, buf, from_addr, pt);
    }
    return -1;
}

int IPv4TCPListener::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
{
    if ( send_stream_callback ) {
        return send_stream_callback(*this, msg, msg_size, dst_addr, pt);
    }
    return -1;
}

int IPv4TCPListener::processTimer()
{
    if ( proc_timer_callback ) {
        return proc_timer_callback(*this);
    }
    return -1;
}

}


