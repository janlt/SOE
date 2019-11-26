/**
 * @file    msgneteventlistener.cpp
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
 * msgneteventlistener.cpp
 *
 *  Created on: Feb 10, 2017
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
#include <sys/epoll.h>
#include <netdb.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread/locks.hpp>

#include "thread.hpp"
#include "threadfifoext.hpp"
#include "classalloc.hpp"
#include "generalpool.hpp"
#include "dbsrvmarshallable.hpp"
#include "dbsrvmsgbuffer.hpp"

#include "soelogger.hpp"
using namespace SoeLogger;

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
#include "msgnetioevent.hpp"
#include "msgnetprocessor.hpp"
#include "msgnetlistener.hpp"
#include "msgnetdummylistener.hpp"
#include "msgnetlistener.hpp"
#include "msgnetsession.hpp"
#include "msgneteventsession.hpp"
#include "msgneteventasyncsession.hpp"
#include "msgneteventlistener.hpp"


#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)

namespace MetaNet {

class DefaultEventProcessor: public DummyListener
{
public:
    int processDgram(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
    {
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << " Got dgram req" << SoeLogger::LogDebug() << endl;
        EventListener &event_lis = dynamic_cast<EventListener &>(lis);
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) <<  event_lis.getIPv4OwnAddr().addr.sin_addr.s_addr << SoeLogger::LogDebug() << endl;
        return 0;
    }

    int processStream(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
    {
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << " Got tcp req" << SoeLogger::LogDebug() << endl;
        EventListener &event_lis = dynamic_cast<EventListener &>(lis);
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) <<  event_lis.getIPv4OwnAddr().addr.sin_addr.s_addr << SoeLogger::LogDebug() << endl;
        return 0;
    }

    int sendDgram(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
    {
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << " Sending dgram rsp" << SoeLogger::LogDebug() << endl;
        EventListener &event_lis = dynamic_cast<EventListener &>(lis);
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) <<  event_lis.getIPv4OwnAddr().addr.sin_addr.s_addr << SoeLogger::LogDebug() << endl;
        return 0;
    }

    int sendStream(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
    {
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << " Sending tcp rsp" << SoeLogger::LogDebug() << endl;
        EventListener &event_lis = dynamic_cast<EventListener &>(lis);
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) <<  event_lis.getIPv4OwnAddr().addr.sin_addr.s_addr << SoeLogger::LogDebug() << endl;
        return 0;
    }

    DefaultEventProcessor(bool _debug = false)
        :debug(_debug)
    {}
    ~DefaultEventProcessor() {}

    bool     debug;
};

static DefaultEventProcessor eventproc;

EventListener::EventListener(Socket &_sock, bool _no_bind) throw(std::runtime_error)
    :Listener(&_sock),
     unix_sock(0),
     num_events(0),
     async_initialized(false)
{
    setNoBind(_no_bind);

    for ( int i = 0 ; i < max_events ; i++ ) {
        epoll_fds[i] = -1;
    }

    setIPv4OwnAddr();
    setUnixOwnAddr();
    setMmOwnAddr();

    unix_sock = new UnixSocket();
    mm_point = new MmPoint(MMAddress(), getMmOwnAddr());

    // register default send stream/dgram callbacks
    boost::function<CallbackEventSendFun> send_dgram_cb = boost::bind(&DefaultEventProcessor::sendDgram, &eventproc, _1, _2, _3, _4, _5);
    boost::function<CallbackEventSendFun> send_stream_cb = boost::bind(&DefaultEventProcessor::sendStream, &eventproc, _1, _2, _3, _4, _5);
    boost::function<CallbackEventProcessFun> process_dgram_cb = boost::bind(&DefaultEventProcessor::processDgram, &eventproc, _1, _2, _3, _4);
    boost::function<CallbackEventProcessFun> process_stream_cb = boost::bind(&DefaultEventProcessor::processStream, &eventproc, _1, _2, _3, _4);

    setSendDgramCB(send_dgram_cb);
    setSendStreamCB(send_stream_cb);
    setProcessDgramCB(process_dgram_cb);
    setProcessStreamCB(process_stream_cb);
}

EventListener::~EventListener()
{
    if ( unix_sock ) {
        delete unix_sock;
    }
}

void EventListener::createEndpoint() throw(std::runtime_error)
{

    if ( !sock ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Null socket errno: " + to_string(errno)).c_str());
    }

    TcpSocket *tcp_sock = dynamic_cast<TcpSocket *>(sock);
    if ( !tcp_sock ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Not TCP socket errno: " + to_string(errno)).c_str());
    }

    int ret = 0;
    if ( reuse_addr == true && sock->GetType() == Socket::eType::eTcp ) {
        if ( sock->SetReuseAddr() < 0 ) {
            throw std::runtime_error((std::string(__FUNCTION__) +
                " Can't set reuse addr for a socket errno: " + to_string(errno)).c_str());
        }
    }

    if ( no_bind == false &&  sock->GetType() == Socket::eType::eTcp ) {
        for ( int try_c = 0 ; try_c < 10 ; try_c++ ) {
            ret = tcp_sock->Bind(const_cast<IPv4Address &>(sock_addr).AsSockaddr());
            if ( ret < 0 ) {
                if ( try_c >= 9 ) {
                    throw std::runtime_error((std::string(__FUNCTION__) +
                        " Can't bind TCP socket to addr: " + sock_addr.GetNTOASinAddr() + " errno: " + to_string(errno)).c_str());
                } else {
                    usleep(10000);
                }
            } else {
                break;
            }
        }
    }

    if ( no_bind == false ) {
        ret = unix_sock->Bind(const_cast<UnixAddress &>(unix_own_addr).AsSockaddr());
        if ( ret < 0 ) {
            throw std::runtime_error((std::string(__FUNCTION__) +
                " Can't bind Unix socket to addr: " + unix_own_addr.GetPath() + " errno: " + to_string(errno)).c_str());
        }

        ret = mm_point->Bind(UnixAddress(mm_own_addr.GetPath()).AsSockaddr());
        if ( ret < 0 ) {
            throw std::runtime_error((std::string(__FUNCTION__) +
                " Can't bind MM socket to addr: " + unix_own_addr.GetPath() + " errno: " + to_string(errno)).c_str());
        }
    }
}

void EventListener::setAssignedAddress(const IPv4Address &addr) throw(std::runtime_error)
{
    if ( !sock ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Null socket errno: " + to_string(errno)).c_str());
    }
    sock_addr = addr;

    initialise();
}

void EventListener::createSocket() throw(std::runtime_error)
{
    if ( !sock || !unix_sock || !mm_point ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Null socket(s) errno: " + to_string(errno)).c_str());
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


    if ( unix_sock->Create() < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) + " failed create Unix socket errno: " + to_string(errno)).c_str());
    }
    (void )unix_own_addr.CloseName();

    if ( mm_point->Create(true) < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) + " failed create MM socket errno: " + to_string(errno)).c_str());
    }
    (void )mm_own_addr.CloseName();
}

void EventListener::registerListener()
{
    if ( unix_sock->Register(true) < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Can't register Unix own socket errno: " + to_string(errno)).c_str());
    }

    if ( mm_point->Register(true) < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Can't register MM own socket errno: " + to_string(errno)).c_str());
    }
}

int EventListener::initialise(MetaNet::Point *lis_soc, bool _ignore_exc)  throw(std::runtime_error)
{
    createSocket();
    createEndpoint();
    registerListener();
    return 0;
}

int EventListener::deinitialise(bool _ignore_exc) throw(std::runtime_error)
{
    return 0;
}

void EventListener::start() throw(std::runtime_error)
{
    if ( !sock || !unix_sock || !mm_point ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Null socket errno: " + to_string(errno)).c_str());
    }

    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Begin listening for events " << getSock()->GetSoc() << SoeLogger::LogDebug() << endl;


    int ret = AddPoint(getSock());
    if ( ret < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Can't add TCP own socket errno: " + to_string(errno)).c_str());
    }

    ret = AddPoint(unix_sock);
    if ( ret < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Can't add Unix own socket errno: " + to_string(errno)).c_str());
    }

    ret = AddPoint(mm_point);
    if ( ret < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Can't add MM own socket errno: " + to_string(errno)).c_str());
    }

    struct epoll_event   loc_events[max_events];
    ::memset(loc_events, '\0', sizeof(loc_events));

    epoll_fds[0] = ::epoll_create1(0);
    if ( epoll_fds[0] < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) + " epoll_create failed errno: " + to_string(errno)).c_str());
    }

    while (1) {
        int nfds = epoll_wait(epoll_fds[0], loc_events, EventListener::max_events, -1);
        if (nfds == -1) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " epoll_wait error errno: " << errno << SoeLogger::LogError() << endl;
            exit(EXIT_FAILURE);
        }

        for ( int n = 0 ; n < nfds ; ++n ) {
            if ( !loc_events[n].data.ptr ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " loc_events[n].data.ptr not set" << SoeLogger::LogError() << endl;
                exit(EXIT_FAILURE);;
            }

            // We have to be able distinguish new connection request
            // on INET(TCP), UNIX, or MM listener
            Point *pt = reinterpret_cast<Point *>(loc_events[n].data.ptr);
            IoEvent ev(loc_events[n]);
            if ( pt == sock || pt == unix_sock || pt == mm_point ) {
                processListenerEvent(n, &ev);
            } else {
                dispatchClientEvent(n, &ev);
            }
        }
    }
}

void EventListener::processListenerEvent(int idx, IoEvent *ev) throw(std::runtime_error)
{
    if ( !ev->ev.data.ptr ) {
        throw std::runtime_error((std::string(__FUNCTION__) + " Null or invalid pt").c_str());
    }
    Point *pt = reinterpret_cast<Point *>(ev->ev.data.ptr);

    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << " Received a message type: " <<
        pt->GetType() << SoeLogger::LogDebug() << endl;

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
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " recvfrom failed errno: " << errno << length << SoeLogger::LogError() << endl;
                return;
            }
            if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << " Received " << length << SoeLogger::LogDebug() << endl;
            from_addr = &in_addr;
        }

        if ( !length ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Zero length message received" << SoeLogger::LogError() << endl;
        } else {
            // HexDump TODO
            // Now create a framework message buffer and process the message
            MetaMsgs::AdmMsgBuffer buf(buffer, static_cast<uint32_t>(length));
            processDgramMessage(buf, *from_addr, pt);
        }
    } else if ( getSock()->GetType() == Socket::eType::eTcp ) {
        // This must be connect on TCP socket
        IPv4Address from_addr;
        MetaMsgs::AdmMsgBuffer buf;

        processStreamMessage(buf, from_addr, pt);
    } else if ( getSock()->GetType() == Socket::eType::eUnix ) {
        // This must be connect on UNIX socket
        IPv4Address from_addr;
        MetaMsgs::AdmMsgBuffer buf;

        processStreamMessage(buf, from_addr, pt);
    } else if ( getSock()->GetType() == Socket::eType::eMM ) {
        // This must be connect on MM socket
        IPv4Address from_addr;
        MetaMsgs::AdmMsgBuffer buf;

        processStreamMessage(buf, from_addr, pt);
    } else {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " bad socket type errno: " << errno << SoeLogger::LogError() << endl;
    }
}

void EventListener::dispatchClientEvent(int idx, IoEvent *ev) throw(std::runtime_error)
{
    if ( !ev->ev.data.ptr ) {
        throw std::runtime_error((std::string(__FUNCTION__) + " Null pt").c_str());
    }
    Point *pt = reinterpret_cast<Point *>(ev->ev.data.ptr);

    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << " Received a message soc: " << pt->GetDesc() <<
        " idx: " << idx << SoeLogger::LogDebug() << endl;

    Point *key_pt = Point::points[pt->GetDesc()].point;
    if ( !key_pt || key_pt != pt ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " No point exists for fd: " << pt->GetDesc() << SoeLogger::LogError() << endl;
    }

    int handler_ret = 0;
    if ( pt->type == Point::eType::eUdp ) {
        UdpSocket *udp_sock = dynamic_cast<UdpSocket *>(pt);
        if ( !udp_sock ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " No UdpSocket for fd: " << pt->GetDesc() << SoeLogger::LogError() << endl;
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
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " recvfrom failed errno: " << errno << length << SoeLogger::LogError() << endl;
                return;
            }
            if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << " Received " << length << SoeLogger::LogDebug() << endl;
            from_addr = &in_addr;
        }

        if ( !length ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Zero length message received" << SoeLogger::LogError() << endl;
        } else {
            // HexDump TODO
            // Now create a framework message buffer and process the message
            MetaMsgs::AdmMsgBuffer buf(buffer, static_cast<uint32_t>(length));

            handler_ret = processDgramMessage(buf, *from_addr, udp_sock);
            if ( handler_ret < 0 ) {
                if ( static_cast<MetaMsgs::MsgStatus>(handler_ret) != MetaMsgs::MsgStatus::eMsgDisconnect ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " processDgramMessage: " << handler_ret << SoeLogger::LogError() << endl;
                }
            }
        }
    } else if ( pt->type == Point::eType::eTcp ) {
        TcpSocket *tcp_sock = dynamic_cast<TcpSocket *>(pt);
        if ( !tcp_sock ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " No TcpSocket for fd: " << pt->GetDesc() << SoeLogger::LogError() << endl;
        }

        // This must be msg
        IPv4Address from_addr;
        MetaMsgs::AdmMsgBuffer buf;

        handler_ret = processStreamMessage(buf, from_addr, tcp_sock);
        if ( handler_ret < 0 ) {
            if ( static_cast<MetaMsgs::MsgStatus>(handler_ret) != MetaMsgs::MsgStatus::eMsgDisconnect ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " processStreamMessage: " << handler_ret << SoeLogger::LogError() << endl;
            }
        }
    } else if ( pt->type == Socket::eType::eUnix ) {
        UnixSocket *unix_sock = dynamic_cast<UnixSocket *>(pt);
        if ( !unix_sock ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " No UnixSocket for fd: " << pt->GetDesc() << SoeLogger::LogError() << endl;
        }

        // This must be msg
        IPv4Address from_addr;
        MetaMsgs::AdmMsgBuffer buf;

        handler_ret = processStreamMessage(buf, from_addr, unix_sock);
        if ( handler_ret < 0 ) {
            if ( static_cast<MetaMsgs::MsgStatus>(handler_ret) != MetaMsgs::MsgStatus::eMsgDisconnect ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " processStreamMessage: " << handler_ret << SoeLogger::LogError() << endl;
            }
        }
    } else if ( pt->type == Socket::eType::eMM ) {
        MmPoint *mm_sock = dynamic_cast<MmPoint *>(pt);
        if ( !mm_sock ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " No UnixSocket for fd: " << pt->GetDesc() << SoeLogger::LogError() << endl;
        }

        // This must be msg
        IPv4Address from_addr;
        MetaMsgs::AdmMsgBuffer buf;

        handler_ret = processStreamMessage(buf, from_addr, mm_sock);
        if ( handler_ret < 0 ) {
            if ( static_cast<MetaMsgs::MsgStatus>(handler_ret) != MetaMsgs::MsgStatus::eMsgDisconnect ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " processStreamMessage: " << handler_ret << SoeLogger::LogError() << endl;
            }
        }
    } else {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " bad socket type errno: " << errno << SoeLogger::LogError() << endl;
    }
}


//
//
//
void EventListener::initAsync(int &e_fd) throw(std::runtime_error)
{
    WriteLock w_lock(init_proc_lock);

    if ( async_initialized == true ) {
        return;
    }

    e_fd = ::epoll_create1(0);
    if ( e_fd < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) + " epoll_create failed errno: " + to_string(errno)).c_str());
    }

    if ( !sock || !unix_sock || !mm_point ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Null socket errno: " + to_string(errno)).c_str());
    }

    int ret = AddAsyncPoint(getSock(), e_fd);
    if ( ret < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Can't add TCP own socket errno: " + to_string(errno)).c_str());
    }

    ret = AddAsyncPoint(unix_sock, e_fd);
    if ( ret < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Can't add Unix own socket errno: " + to_string(errno)).c_str());
    }

    ret = AddAsyncPoint(mm_point, e_fd);
    if ( ret < 0 ) {
        throw std::runtime_error((std::string(__FUNCTION__) +
            " Can't add MM own socket errno: " + to_string(errno)).c_str());
    }

    async_initialized = true;
    return;
}

//
//
//
void EventListener::startAsync() throw(std::runtime_error)
{
    SoeLogger::Logger logger;

    uint64_t msg_counter = 1;
    uint64_t msg_counter_quant = 0x1000;

    pid_t tid = gettid();
    int fd_idx = 0;

    if ( debug == true ) {
        logger.Clear(CLEAR_DEFAULT_ARGS)  << " Begin listening for events fd: " << getSock()->GetSoc() <<
        " tid: " << tid << " epoll_id slot: " << fd_idx << SoeLogger::LogDebug() << endl;
    }

    struct epoll_event   loc_events[max_events];
    ::memset(loc_events, '\0', sizeof(loc_events));

    initAsync(epoll_fds[fd_idx]);
    int epoll_fd = epoll_fds[fd_idx];

    while (1) {
        int nfds = epoll_wait(epoll_fd, loc_events, EventListener::max_events, -1);
        if ( nfds == -1 ) {
            if ( errno != EINTR ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " epoll_wait error errno: " << errno << SoeLogger::LogError() << endl;
                exit(EXIT_FAILURE);
            }
            continue;
        }

        for ( int n = 0 ; n < nfds ; ++n ) {
            if ( !loc_events[n].data.ptr ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " loc_events[n].data.ptr not set" << SoeLogger::LogError() << endl;
                exit(EXIT_FAILURE);;
            }

            if ( debug == true ) {
                if ( !(msg_counter%msg_counter_quant) ) {
                    logger.Clear(CLEAR_DEFAULT_ARGS) << __FUNCTION__ << " th: " << boost::this_thread::get_id() <<
                        " msg_counter: " << msg_counter << SoeLogger::LogDebug() << endl;
                }
                msg_counter++;
            }

            // We have to be able distinguish new connection request
            // on INET(TCP), UNIX, or MM listener
            Point *pt = reinterpret_cast<Point *>(loc_events[n].data.ptr);

            if ( debug == true ) {
                logger.Clear(CLEAR_DEFAULT_ARGS)  <<
                " tid: " << tid <<
                " nfds: " << nfds <<
                " pt->fd " << pt->GetDesc() <<
                " s/u/mm " << sock->GetDesc() << "/" <<
                unix_sock->GetDesc() << "/" <<
                mm_point->GetDesc() << SoeLogger::LogDebug() << endl;
            }

            if ( pt == sock || pt == unix_sock || pt == mm_point ) {
                IoEvent *ev = new IoEvent(loc_events[n], epoll_fd);
                processListenerAsyncEvent(n, ev);
                RearmAsyncPoint(pt, epoll_fd);
                delete ev;
            } else {
                IoEvent *ev = new IoEvent(loc_events[n], epoll_fd);
                dispatchClientAsyncEvent(n, ev);
            }
        }
    }
}

void EventListener::dispatchClientAsyncEvent(int idx, IoEvent *ev) throw(std::runtime_error)
{
    if ( !ev->ev.data.ptr ) {
        throw std::runtime_error((std::string(__FUNCTION__) + " Null pt").c_str());
    }
    Point *pt = reinterpret_cast<Point *>(ev->ev.data.ptr);

    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << " Received a message soc: " << pt->GetDesc() <<
        " idx: " << idx << SoeLogger::LogDebug() << endl;

    Point *key_pt = Point::points[pt->GetDesc()].point;
    if ( !key_pt || key_pt != pt ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " No point exists for fd: " << pt->GetDesc() << " ... ignoring" << SoeLogger::LogInfo() << endl;
    }

    int handler_ret = 0;
    if ( pt->type == Point::eType::eTcp ) {
        TcpSocket *tcp_sock = dynamic_cast<TcpSocket *>(pt);
        if ( !tcp_sock ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " No TcpSocket for fd: " << pt->GetDesc() << SoeLogger::LogError() << endl;
        }

        // This must be msg
        IPv4Address from_addr;
        MetaMsgs::AdmMsgBuffer buf;

        handler_ret = processStreamAsyncEvent(buf, from_addr, ev);
        if ( handler_ret < 0 ) {
            if ( static_cast<MetaMsgs::MsgStatus>(handler_ret) != MetaMsgs::MsgStatus::eMsgDisconnect ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " processStreamMessage: " << handler_ret << SoeLogger::LogError() << endl;
            }
        }
    } else if ( pt->type == Socket::eType::eUnix ) {
        UnixSocket *unix_sock = dynamic_cast<UnixSocket *>(pt);
        if ( !unix_sock ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " No UnixSocket for fd: " << pt->GetDesc() << SoeLogger::LogError() << endl;
        }

        // This must be msg
        IPv4Address from_addr;
        MetaMsgs::AdmMsgBuffer buf;

        handler_ret = processStreamAsyncEvent(buf, from_addr, ev);
        if ( handler_ret < 0 ) {
            if ( static_cast<MetaMsgs::MsgStatus>(handler_ret) != MetaMsgs::MsgStatus::eMsgDisconnect ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " processStreamMessage: " << handler_ret << SoeLogger::LogError() << endl;
            }
        }
    } else if ( pt->type == Socket::eType::eMM ) {
        MmPoint *mm_sock = dynamic_cast<MmPoint *>(pt);
        if ( !mm_sock ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " No UnixSocket for fd: " << pt->GetDesc() << SoeLogger::LogError() << endl;
        }

        // This must be msg
        IPv4Address from_addr;
        MetaMsgs::AdmMsgBuffer buf;

        handler_ret = processStreamAsyncEvent(buf, from_addr, ev);
        if ( handler_ret < 0 ) {
            if ( static_cast<MetaMsgs::MsgStatus>(handler_ret) != MetaMsgs::MsgStatus::eMsgDisconnect ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " processStreamMessage: " << handler_ret << SoeLogger::LogError() << endl;
            }
        }
    } else {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " bad socket type errno: " << errno << SoeLogger::LogError() << endl;
    }
}

void EventListener::processListenerAsyncEvent(int idx, IoEvent *ev) throw(std::runtime_error)
{
    if ( !ev->ev.data.ptr ) {
        throw std::runtime_error((std::string(__FUNCTION__) + " Null or invalid pt").c_str());
    }
    Point *pt = reinterpret_cast<Point *>(ev->ev.data.ptr);

    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << " Received a message type: " <<
        pt->GetType() << SoeLogger::LogDebug() << endl;

    if ( pt->type == Socket::eType::eTcp ) {
        // This must be connect on TCP socket
        IPv4Address from_addr;
        MetaMsgs::AdmMsgBuffer buf;

        processStreamAsyncEvent(buf, from_addr, ev);
    } else if ( pt->type == Socket::eType::eUnix ) {
        // This must be connect on UNIX socket
        IPv4Address from_addr;
        MetaMsgs::AdmMsgBuffer buf;

        processStreamAsyncEvent(buf, from_addr, ev);
    } else if ( pt->type == Socket::eType::eMM ) {
        // This must be connect on MM socket
        IPv4Address from_addr;
        MetaMsgs::AdmMsgBuffer buf;

        processStreamAsyncEvent(buf, from_addr, ev);
    } else {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " bad socket type errno: " << errno << SoeLogger::LogError() << endl;
    }
}

int EventListener::processStreamAsyncEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev)
{
    if ( proc_event_stream_callback ) {
        return proc_event_stream_callback(*this, buf, from_addr, ev);
    }
    return -1;
}

int EventListener::sendStreamAsyncEvent(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const IoEvent *ev)
{
    if ( send_event_stream_callback ) {
        return send_event_stream_callback(*this, msg, msg_size, dst_addr, ev);
    }
    return -1;
}

//
//
//
int EventListener::processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( proc_dgram_callback ) {
        return proc_dgram_callback(*this, buf, from_addr, pt);
    }
    return -1;
}

int EventListener::processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( proc_stream_callback ) {
        return proc_stream_callback(*this, buf, from_addr, pt);
    }
    return -1;
}

int EventListener::sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
{
    if ( send_dgram_callback ) {
        return send_dgram_callback(*this, msg, msg_size, dst_addr, pt);
    }
    return -1;
}

int EventListener::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
{
    if ( send_stream_callback ) {
        return send_stream_callback(*this, msg, msg_size, dst_addr, pt);
    }
    return -1;
}

int EventListener::processTimer()
{
    return -1;
}
//
//
//
int EventListener::AddPoint(MetaNet::Point *_point)
{
    WriteLock w_lock(add_remove_lock);

    if ( num_events++ > EventListener::max_events ) {
        return -1;
    }
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    ev.data.ptr = _point;
    if ( ::epoll_ctl(epoll_fds[0], EPOLL_CTL_ADD, _point->GetDesc(), &ev) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " epoll_ctl failed errno: " << errno << SoeLogger::LogError() << endl;
        return -1;
    }

    return 0;
}

int EventListener::RemovePoint(MetaNet::Point *_point)
{
    WriteLock w_lock(add_remove_lock);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    ev.data.ptr = _point;
    if ( ::epoll_ctl(epoll_fds[0], EPOLL_CTL_DEL, _point->GetDesc(), &ev) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " epoll_ctl fd: " << _point->GetDesc() <<
            " failed errno: " << errno << " ignoring ...." << SoeLogger::LogInfo() << endl;
        return -1;
    }

    num_events--;
    return 0;
}

int EventListener::AddAsyncPoint(MetaNet::Point *_point, int e_fd)
{
    WriteLock w_lock(add_remove_lock);

    if ( num_events++ > EventListener::max_events ) {
        return -1;
    }
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    ev.data.ptr = _point;
    if ( ::epoll_ctl(e_fd, EPOLL_CTL_ADD, _point->GetDesc(), &ev) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " epoll_ctl fd: " << _point->GetDesc() << " failed errno: " << errno << SoeLogger::LogError() << endl;
        return -1;
    }
    //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << " epoll_ctl added fd: " << _point->GetDesc() << SoeLogger::LogDebug() << endl;

    return 0;
}

int EventListener::RemoveAsyncPoint(MetaNet::Point *_point, int e_fd)
{
    WriteLock w_lock(add_remove_lock);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    ev.data.ptr = _point;
    if ( ::epoll_ctl(e_fd, EPOLL_CTL_DEL, _point->GetDesc(), &ev) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " epoll_ctl fd: " << _point->GetDesc() <<
             " failed errno: " << errno << " ignoring ..." << SoeLogger::LogInfo() << endl;
        return -1;
    }
    //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << " epoll_ctl removed fd: " << _point->GetDesc() << SoeLogger::LogDebug() << endl;

    num_events--;
    return 0;
}

int EventListener::RearmAsyncPoint(MetaNet::Point *_point, int e_fd)
{
    WriteLock w_lock(add_remove_lock);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    ev.data.ptr = _point;
    if ( ::epoll_ctl(e_fd, EPOLL_CTL_MOD, _point->GetDesc(), &ev) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " epoll_ctl fd: " << _point->GetDesc() << " failed errno: " << errno << SoeLogger::LogError() << endl;
        return -1;
    }
    //SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS)  << " epoll_ctl rearmed fd: " << _point->GetDesc() << SoeLogger::LogDebug() << endl;

    return 0;
}

}


