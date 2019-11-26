/**
 * @file    msgnetipv4listener.cpp
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
 * msgnetipv4listener.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
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
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnettcpsocket.hpp"
#include "msgnetudpsocket.hpp"
#include "msgnetlistener.hpp"
#include "msgnetdummylistener.hpp"
#include "msgnetipv4listener.hpp"

namespace MetaNet {

IPv4Listener::IPv4Listener(Socket *sock, bool _no_bind)
    :Listener(sock),
    ipv4_addr(0),
    num_fds(0)
{
    setIPv4OwnAddr();
    setNoBind(_no_bind);

    for ( uint32_t i = 0 ; i < IPv4Listener::max_fds ; i++ ) {
        fds[i].fd = -1;
        fds[i].revents = 0;
        fds[i].events = 0;
    }
}

IPv4Listener *IPv4Listener::instance = 0;
Lock          IPv4Listener::global_lock;

IPv4Listener *IPv4Listener::getInstance()
{
    WriteLock w_lock(IPv4Listener::global_lock);
    return IPv4Listener::instance;
}

IPv4Listener *IPv4Listener::setInstance(Socket *sock)
{
    WriteLock w_lock(IPv4Listener::global_lock);
    if ( !IPv4Listener::instance ) {
        IPv4Listener::instance = new IPv4Listener(sock);
    }
    return IPv4Listener::instance;
}

IPv4Listener::~IPv4Listener()
{
    if ( ipv4_addr ) {
        delete ipv4_addr;
        ipv4_addr = 0;
    }
    IPv4Listener::instance = 0;
}

void IPv4Listener::createEndpoint() throw(std::runtime_error)
{
    IPv4Address sa;

    sa.addr.sin_family = AF_INET;
    IPv4Address *addr = 0;

    if ( !ipv4_addr ) {
        // We will be unbound as a setAssignedAddress was not done
        sa.addr.sin_port = htons(1234);
        sa.addr.sin_addr.s_addr = sock_addr.addr.sin_addr.s_addr;
    } else {
        sa.addr.sin_port = ipv4_addr->addr.sin_port;
        sa.addr.sin_addr.s_addr = htonl(INADDR_ANY); //::inet_aton("0.0.0.0", &sa.sin_addr);
    }

    addr = reinterpret_cast<IPv4Address *>(&sa);
    if ( !sock ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Wrong socket type errno: " + to_string(errno)).c_str());
    }

    // If reuse is set then allow this to be a reuse of an exsiting
    // port.
    if ( reuse_addr == true && sock->GetType() == Socket::eType::eTcp ) {
        if ( sock->SetReuseAddr() < 0 ) {
            throw std::runtime_error((std::string(__FUNCTION__) +
                " Can't set reuse addr for a socket errno: " + to_string(errno)).c_str());
        }
    }

    if ( no_bind == false &&  sock->GetType() == Socket::eType::eTcp ) {
        for ( int try_c = 0 ; try_c < 10 ; try_c++ ) {
            if ( sock->Bind(addr->AsSockaddr()) < 0 ) {
                if ( try_c >= 9 ) {
                    throw std::runtime_error((std::string(__FUNCTION__) +
                        " Can't bind addr to a socket errno: " + to_string(errno)).c_str());
                } else {
                    usleep(10000);
                }
            } else {
                break;
            }
        }
    }

    if ( !ipv4_addr ) {
        if ( sock->GetName(sa.AsNCSockaddr()) < 0 ) {
            throw std::runtime_error((std::string(__FUNCTION__) +
                " No port assigned to addr errno: " + to_string(errno)).c_str());
        }

        // Now add port to the main address
        sock_addr.addr.sin_port = sa.addr.sin_port;
        ipv4_addr = new IPv4Address(sock_addr);
    }
}

void IPv4Listener::createSocket() throw(std::runtime_error)
{
    if ( !sock ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Null socket errno: " + to_string(errno)).c_str());
    }

    if ( sock->Created() == true ) {
        return;
    }

    if ( sock->GetType() == Socket::eType::eTcp ) {
        TcpSocket *tcp_sock = dynamic_cast<TcpSocket *>(sock);
        if ( !tcp_sock || tcp_sock->CreateStream() < 0 ) {
            throw std::runtime_error((std::string(__FUNCTION__) +
                " Can't create stream socket errno: " + to_string(errno)).c_str());
        }
    } else if ( sock->GetType() == Socket::eType::eUdp ) {
        UdpSocket *udp_sock = dynamic_cast<UdpSocket *>(sock);
        if ( !udp_sock || udp_sock->CreateDgram() < 0 ) {
            throw std::runtime_error((std::string(__FUNCTION__) +
                " Can't create dgram socket errno: " + to_string(errno)).c_str());
        }
    } else {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Invalid socket errno: " + to_string(errno)).c_str());
    }
}

void IPv4Listener::setAssignedAddress(const IPv4Address &addr) throw(std::runtime_error)
{
    ipv4_addr = new IPv4Address(addr);

    if ( !sock ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Null socket errno: " + to_string(errno)).c_str());
    }

    initialise();
}

void IPv4Listener::getHostIdentity ()
{
    // Get the hostname/domainname
    char hostname[64 + 1];
    ::memset(hostname, '\0', sizeof(hostname));
    ::gethostname(hostname, 64 + 1);

    // Now get the entry from the inet database
    struct hostent host_entry;
    ::memset(&host_entry, '\0', sizeof(host_entry));

    struct hostent *rslt;

    rslt = gethostbyname(hostname); // Linux usese thread-specific storage for result

    // Now the printing has been done take the first address only for
    // this listener
    char** q;
    q = rslt->h_addr_list;
    if ( *q ) {
        sock_addr.addr.sin_family = AF_INET;
        sock_addr.addr.sin_port = htons(0);
        sock_addr.addr.sin_addr.s_addr = ((in_addr*) *q)->s_addr;
    }
}

void IPv4Listener::start() throw(std::runtime_error)
{
    if ( !sock ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Null socket errno: " + to_string(errno)).c_str());
    }

    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Begin listening for messages " << sock->GetSoc() << SoeLogger::LogDebug() << endl;

    int ret = AddPoint(getSock());
    if ( ret < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Can't add own socket errno: " + to_string(errno)).c_str());
    }

    // Form up a receive list to select on - this may be useful later
    // if we decide to multiplex listeners into a single thread
    if ( client_mode == false ) {
        fds[0].fd = sock->GetSoc();
        fds[0].events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI;
        num_fds++;
    }

    while (1) {
        // Poll the file descriptors with some timeout
        int ret;
        uint32_t curr_fds = num_fds;
        while (1) {
            ret = ::poll(fds, curr_fds, 1000);
            if ( ret > 0 ) { // got some data
                break;
            } else if ( !ret ) {// timed out
                try {
                    processTimer();
                } catch (const std::bad_alloc &ex) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "bad_alloc errno: " << errno << SoeLogger::LogError() << endl;
                } catch ( ... ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "unknown caught " << SoeLogger::LogError() << endl;
                }
                break;
            } else {
                if ( errno == EINTR ) {
                    continue;
                } else {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "poll failed errno: " << errno << SoeLogger::LogError() << endl;
                    break;
                }
            }
        }

        for ( uint32_t ii = 0 ; ii < max_fds ; ii++ ) {
            if ( ret <= 0 ) {
                break;
            }
            if ( fds[ii].fd == -1 ) {
                continue;
            }
            if ( !fds[ii].revents ) {
                continue;
            }

            try {
                if ( fds[ii].fd == getSock()->GetDesc() ) {
                    processListenerEvent(ret);
                } else {
                    processClientEvent(ii, ret);
                }
            } catch (const std::bad_alloc &ex) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "bad_alloc errno: " << errno << SoeLogger::LogError() << endl;
            } catch ( ... ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "unknown caught " << SoeLogger::LogError() << endl;
            }
            --ret;
        }
    }

    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Quit main loop " << SoeLogger::LogError() << endl;
}

void IPv4Listener::processListenerEvent(int ret)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Received a message type: " << getSock()->GetType() <<
        " ret: " << ret << SoeLogger::LogDebug() << endl;

    if ( getSock()->GetType() == Socket::eType::eUdp ) {
        // true is for big buffer
        uint8_t buffer[MetaMsgs::maxMsgBufSize];
        IPv4Address *from_addr = 0;
        int32_t length;

        {
            // We need to know the address that this message came from also so use recvfrom
            IPv4Address in_addr;
            socklen_t in_addr_size = sizeof(IPv4Address);

            length = ::recvfrom(sock->GetSoc(), buffer, sizeof(buffer), 0, in_addr.AsNCSockaddr(), &in_addr_size);
            if ( length < 0 ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << "recvfrom failed errno: " << errno << length << SoeLogger::LogError() << endl;
                return;
            }
            if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Received " << length << SoeLogger::LogDebug() << endl;
            from_addr = &in_addr;
        }

        if ( !length ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Zero length message received" << SoeLogger::LogError() << endl;
        } else {
            // HexDump TODO
            // Now create a framework message buffer and process the message
            MetaMsgs::AdmMsgBuffer buf(buffer, static_cast<uint32_t>(length));
            processDgramMessage(buf, *from_addr, getSock());
        }
    } else if ( getSock()->GetType() == Socket::eType::eTcp ) {
        // This must be connect
        IPv4Address from_addr;
        MetaMsgs::AdmMsgBuffer buf;

        processStreamMessage(buf, from_addr, getSock());
    } else {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "bad socket type errno: " << errno << SoeLogger::LogError() << endl;
    }
}

void IPv4Listener::processClientEvent(uint32_t idx, int ret)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Received a message soc: " << fds[idx].fd <<
        " ret: " << ret << SoeLogger::LogDebug() << endl;

    Point *pt = Point::points[fds[idx].fd].point;
    if ( !pt ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "No point exists for fd: " << fds[idx].fd << SoeLogger::LogError() << endl;
        fds[idx].fd = -1;
        fds[idx].events = 0;
        if ( idx + 1 >= num_fds ) {
            num_fds--;
        }
        return;
    }

    int handler_ret = 0;
    if ( pt->type == Point::eType::eUdp ) {
        UdpSocket *udp_sock = dynamic_cast<UdpSocket *>(pt);
        if ( !udp_sock ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "No UdpSocket for fd: " << fds[idx].fd << SoeLogger::LogError() << endl;
        }
        // true is for big buffer
        uint8_t buffer[MetaMsgs::maxMsgBufSize];
        IPv4Address *from_addr = 0;
        int32_t length;

        {
            // We need to know the address that this message came from also so use recvfrom
            IPv4Address in_addr;
            socklen_t in_addr_size = sizeof(IPv4Address);

            length = ::recvfrom(udp_sock->GetSoc(), buffer, sizeof(buffer), 0, in_addr.AsNCSockaddr(), &in_addr_size);
            if ( length < 0 ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "recvfrom failed errno: " << errno << length << SoeLogger::LogError() << endl;
                return;
            }
            if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Received " << length << SoeLogger::LogDebug() << endl;
            from_addr = &in_addr;
        }

        if ( !length ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Zero length message received" << SoeLogger::LogError() << endl;
        } else {
            // HexDump TODO
            // Now create a framework message buffer and process the message
            MetaMsgs::AdmMsgBuffer buf(buffer, static_cast<uint32_t>(length));

            handler_ret = processDgramMessage(buf, *from_addr, udp_sock);
            if ( handler_ret < 0 ) {
                if ( static_cast<MetaMsgs::MsgStatus>(handler_ret) != MetaMsgs::MsgStatus::eMsgDisconnect ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "processDgramMessage: " << handler_ret << SoeLogger::LogError() << endl;
                }
            }
        }
    } else if ( pt->type == Point::eType::eTcp ) {
        TcpSocket *tcp_sock = dynamic_cast<TcpSocket *>(pt);
        if ( !tcp_sock ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "No TcpSocket for fd: " << fds[idx].fd << SoeLogger::LogError() << endl;
        }

        // This must be msg
        IPv4Address from_addr;
        MetaMsgs::AdmMsgBuffer buf;

        handler_ret = processStreamMessage(buf, from_addr, tcp_sock);
        if ( handler_ret < 0 ) {
            if ( static_cast<MetaMsgs::MsgStatus>(handler_ret) != MetaMsgs::MsgStatus::eMsgDisconnect ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "processStreamMessage: " << handler_ret << SoeLogger::LogError() << endl;
            }
        }
    } else {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "bad socket type errno: " << errno << SoeLogger::LogError() << endl;
    }
}

int IPv4Listener::AddPoint(MetaNet::Point *_sock)
{
    for ( uint32_t i = 0 ; i < IPv4Listener::max_fds ; i++ ) {
        if ( fds[i].fd == -1 ) {
            fds[i].fd = _sock->GetDesc();
            fds[i].events = POLLIN;
            if ( i >= num_fds ) {
                num_fds++;
            }
            return static_cast<int>(i);
        }
    }

    return -1;
}

int IPv4Listener::RemovePoint(MetaNet::Point *_sock)
{
    for ( uint32_t i = 0 ; i < num_fds ; i++ ) {
        if ( _sock->GetDesc() == fds[i].fd ) {
            fds[i].fd = -1;
            fds[i].events = 0;
            if ( i + 1 >= num_fds ) {
                num_fds--;
            }
            return static_cast<int>(i);
        }
    }

    return -1;
}

int IPv4Listener::processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    UdpSocket *udp_sock = dynamic_cast<UdpSocket *>(sock);
    if ( !udp_sock ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Wrong socket type errno: " + to_string(errno)).c_str());
    }

    if ( proc_dgram_callback ) {
        return proc_dgram_callback(*this, buf, from_addr, pt);
    }
    return -1;
}

int IPv4Listener::processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    TcpSocket *tcp_sock = dynamic_cast<TcpSocket *>(sock);
    if ( !tcp_sock ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Wrong socket type errno: " + to_string(errno)).c_str());
    }

    if ( proc_stream_callback ) {
        return proc_stream_callback(*this, buf, from_addr, pt);
    }
    return -1;
}

int IPv4Listener::sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
{
    UdpSocket *udp_sock = dynamic_cast<UdpSocket *>(sock);
    if ( !udp_sock ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Wrong socket type errno: " + to_string(errno)).c_str());
    }

    if ( send_dgram_callback ) {
        return send_dgram_callback(*this, msg, msg_size, dst_addr, pt);
    }
    return -1;
}

int IPv4Listener::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
{
    TcpSocket *tcp_sock = dynamic_cast<TcpSocket *>(sock);
    if ( !tcp_sock ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Wrong socket type errno: " + to_string(errno)).c_str());
    }

    if ( send_stream_callback ) {
        return send_stream_callback(*this, msg, msg_size, dst_addr, pt);
    }
    return -1;
}

int IPv4Listener::processTimer()
{
    if ( proc_timer_callback ) {
        return proc_timer_callback(*this);
    }
    return -1;
}

void IPv4Listener::registerListener ()
{}

int IPv4Listener::initialise(MetaNet::Point *lis_soc, bool _ignore_exc) throw(std::runtime_error)
{
    createSocket();
    createEndpoint();
    registerListener();
    return 0;
}

int IPv4Listener::deinitialise(bool _ignore_exc) throw(std::runtime_error)
{
    return 0;
}

void IPv4Listener::setIPv4OwnAddr()
{
    own_addr = IPv4Address::findNodeNoLocalAddr();
}

IPv4Address &IPv4Listener::getIPv4OwnAddr()
{
    return own_addr;
}
}

