/**
 * @file    metadbcliadmsession.cpp
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
 * metadbcliadmsession.cpp
 *
 *  Created on: Jun 1, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
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
#include <sys/epoll.h>
#include <netdb.h>

#include <boost/thread.hpp>
#include <boost/function.hpp>

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

#include "msgnetadkeys.hpp"
#include "msgnetadvertisers.hpp"
#include "msgnetsolicitors.hpp"
#include "msgnetaddress.hpp"
#include "msgnetipv4address.hpp"
#include "msgnetunixaddress.hpp"
#include "msgnetmmaddress.hpp"
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnetunixsocket.hpp"
#include "msgnetmmpoint.hpp"
#include "msgnetioevent.hpp"
#include "msgnettcpsocket.hpp"
#include "msgnetudpsocket.hpp"
#include "msgnetunixsocket.hpp"
#include "msgnetlistener.hpp"
#include "msgnetprocessor.hpp"
#include "msgnetipv4listener.hpp"
#include "msgnetipv4mcastlistener.hpp"
#include "msgnetsession.hpp"
#include "msgnetipv4tcpsession.hpp"
#include "msgnetipv4tcpclisession.hpp"
#include "msgneteventsession.hpp"
#include "msgneteventclisession.hpp"
#include "msgnetasynceventclisession.hpp"

using namespace std;
using namespace MetaMsgs;
using namespace MetaNet;

#include "metadbcliiov.hpp"
#include "metadbcliiosession.hpp"
#include "metadbcliasynciosession.hpp"
#include "metadbcliadmsession.hpp"

namespace MetadbcliFacade {

Lock AdmSession::adm_session_lock;
AdmSession *AdmSession::adm_session = 0;

AdmSession::AdmSession(TcpSocket &soc,
        bool _unix_sock_session,
        bool _debug,
        bool _test)
    :IPv4TCPCliSession(soc, _test),
     counter(0),
     started(false),
     run_count(-1),
     unix_sock_session(_unix_sock_session)
{
    own_addr = IPv4Address::findNodeNoLocalAddr();
    setDebug(_debug);

    IPv4Address adm_start_addr;
    adm_start_addr.addr.sin_family = AF_INET;
    adm_start_addr.addr.sin_addr.s_addr = htonl(INADDR_ANY);
    adm_start_addr.addr.sin_port = htons(6111);
    peer_addr = adm_start_addr;

    IPv4Address io_start_addr;
    io_start_addr.addr.sin_family = AF_INET;
    io_start_addr.addr.sin_addr.s_addr = htonl(INADDR_ANY);
    io_start_addr.addr.sin_port = htons(7111);
    io_peer_addr = io_start_addr;

    if ( soc.CreateStream() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't create tcp socket: " << errno << " exiting ..." << SoeLogger::LogError() << endl;
    }
    if ( soc.SetNoDelay() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set nodelay on tcp socket: " << errno << " exiting ..." << SoeLogger::LogError() << endl;
    }
    if ( soc.SetRecvTmout(10000) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set recv tmout on tcp socket: " << errno << " exiting ..." << SoeLogger::LogError() << endl;
    }
    if ( soc.SetSndTmout(10000) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set snd tmout on tcp socket: " << errno << " exiting ..." << SoeLogger::LogError() << endl;
    }
    if ( soc.SetReuseAddr() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set reuse address on tcp socket: " << errno << " exiting ..." << SoeLogger::LogError() << endl;
    }
    if ( soc.SetNoLinger() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set no linger on tcp socket: " << errno << " exiting ..." << SoeLogger::LogError() << endl;
    }
    if ( soc.SetNoSigPipe() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set no sigpipe on tcp socket: " << errno << " exiting ..." << SoeLogger::LogError() << endl;
    }
}

AdmSession::~AdmSession() {}

AdmSession *AdmSession::GetAdmSession()
{
    return adm_session;
}

AdmSession *AdmSession::CreateAdmSession(TcpSocket &soc,
    bool _unix_sock_session,
    bool _debug,
    bool _test)
{
    WriteLock w_lock(AdmSession::adm_session_lock);

    if ( !adm_session ) {
        adm_session = new AdmSession(soc, _unix_sock_session, _debug, _test);
    }
    return adm_session;
}

void AdmSession::DestroyAdmSession()
{
    WriteLock w_lock(AdmSession::adm_session_lock);

    if ( adm_session ) {
        delete adm_session;
        adm_session = 0;
    }
}

void AdmSession::setIOPeerAddr(const IPv4Address &_io_peer_addr)
{
    io_peer_addr = _io_peer_addr;
}

MetaIOSession<TestIov> *AdmSession::CreateLocalUnixSockIOSession(const IPv4Address &_io_peer_addr)
{
    bool _clean_up = false;
    UnixSocket *ss = new UnixSocket;

    if ( ss->Create() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't create unix socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetNoDelay() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set nodelay on unix socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetRecvTmout(10000) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set recv tmout on unix socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetSndTmout(10000) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set snd tmout on unix socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetReuseAddr() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set reuse address on unix socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetNoLinger() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set no linger on unix socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetNoSigPipe() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set no sigpipe on unix socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }

    if ( _clean_up == true ) {
        delete ss;
        return 0;
    }

    MetaIOSession<TestIov> *io_session = new MetaIOSession<TestIov>(*ss, this);
    UnixAddress *uad = new UnixAddress(UnixAddress::findNodeNoLocalAddr());
    if ( uad->OpenName() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed opening name for unix socket peer addr: " << uad->GetPath() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete io_session;
        delete ss;
        delete uad;
        return 0;
    }

    try {
        io_session->setPeerAddr(uad);
    } catch (const runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed setting unix socket peer addr: " << ex.what() << SoeLogger::LogError() << endl;
        delete io_session;
        delete ss;
        delete uad;
        return 0;
    }

    io_session->setSessDebug(debug);
    if ( io_session->initialise(0, true) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed initialize io session errno: " << errno << SoeLogger::LogError() << endl;
        delete io_session;
        delete ss;
        delete uad;
        return 0;
    }

    return io_session;
}

MetaIOSession<TestIov> *AdmSession::CreateLocalMMSockIOSession(const IPv4Address &_io_peer_addr)
{
    bool _clean_up = false;
    MmPoint *ss = new MmPoint;

    if ( ss->Create() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't create mmsock socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetRecvTmout(200) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set recv tmout on mmsock socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetSndTmout(200) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set snd tmout on mmsock socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }

    if ( _clean_up == true ) {
        delete ss;
        return 0;
    }

    MetaIOSession<TestIov> *io_session = new MetaIOSession<TestIov>(*ss, this);
    MMAddress *mmad = new MMAddress(MMAddress::findNodeNoLocalAddr());
    if ( mmad->OpenName() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed opening name for MM socket peer addr: " << mmad->GetPath() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete ss;
        return 0;
    }
    try {
        io_session->setPeerAddr(mmad);
    } catch (const runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed setting MM socket peer addr: " << ex.what() << SoeLogger::LogError() << endl;
        delete io_session;
        delete ss;
        delete mmad;
        return 0;
    }
    io_session->setSessDebug(debug);
    if ( io_session->initialise(0, true) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed initialize io session errno: " << errno << SoeLogger::LogError() << endl;
        delete io_session;
        delete ss;
        delete mmad;
        return 0;
    }

    return io_session;
}

MetaIOSession<TestIov> *AdmSession::CreateRemoteIOSession(const IPv4Address &_io_peer_addr)
{
    bool _clean_up = false;
    TcpSocket *ss = new TcpSocket;

    if ( ss->Create() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't create tcp socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetNoDelay() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set nodelay on tcp socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetRecvTmout(10000) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set recv tmout on tcp socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetSndTmout(10000) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set snd tmout on tcp socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetReuseAddr() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set reuse address on tcp socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetNoLinger() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set no linger on tcp socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetNoSigPipe() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set no sigpipe on tcp socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }

    if ( _clean_up == true ) {
        delete ss;
        return 0;
    }

    MetaIOSession<TestIov> *io_session = new MetaIOSession<TestIov>(*ss, this);
    IPv4Address *ipv4addr = new IPv4Address(io_peer_addr);
    io_session->setPeerAddr(ipv4addr);
    io_session->setSessDebug(debug);
    if ( io_session->initialise(0, true) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed initialize io session errno: " << errno << SoeLogger::LogError() << endl;
        delete io_session;
        delete ss;
        delete ipv4addr;
        return 0;
    }

    return io_session;
}

AsyncIOSession *AdmSession::CreateLocalUnixSockAsyncIOSession(const IPv4Address &_io_peer_addr, bool start_in, bool start_out)
{
    bool _clean_up = false;
    UnixSocket *ss = new UnixSocket;

    if ( ss->Create() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't create unix socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetNoDelay() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set nodelay on unix socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetRecvTmout(10000) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set recv tmout on unix socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetSndTmout(10000) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set snd tmout on unix socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetReuseAddr() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set reuse address on unix socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetNoLinger() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set no linger on unix socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetNoSigPipe() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set no sigpipe on unix socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }

    if ( _clean_up == true ) {
        delete ss;
        return 0;
    }

    AsyncIOSession *io_session = new AsyncIOSession(*ss, this);
    UnixAddress *uad = new UnixAddress(UnixAddress::findNodeNoLocalAddr());
    if ( uad->OpenName() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed opening name for unix socket peer addr: " << uad->GetPath() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete io_session;
        delete ss;
        delete uad;
        return 0;
    }

    try {
        io_session->setPeerAddr(uad);
    } catch (const runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed setting unix socket peer addr: " << ex.what() << SoeLogger::LogError() << endl;
        delete io_session;
        delete ss;
        delete uad;
        return 0;
    }

    io_session->setSessDebug(debug);
    if ( io_session->initialise(0, true) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed initialize io session errno: " << errno << SoeLogger::LogError() << endl;
        delete io_session;
        delete ss;
        delete uad;
        return 0;
    }

    if ( start_in == true ) {
        io_session->in_thread = new boost::thread(&AsyncIOSession::inLoop, io_session);
    }
    if ( start_out == true ) {
        io_session->out_thread = new boost::thread(&AsyncIOSession::outLoop, io_session);
    }

    return io_session;
}

AsyncIOSession *AdmSession::CreateLocalMMSockAsyncIOSession(const IPv4Address &_io_peer_addr, bool start_in, bool start_out)
{
    bool _clean_up = false;
    MmPoint *ss = new MmPoint;

    if ( ss->Create() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't create mmsock socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetRecvTmout(200) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set recv tmout on mmsock socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetSndTmout(200) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set snd tmout on mmsock socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }

    if ( _clean_up == true ) {
        delete ss;
        return 0;
    }

    AsyncIOSession *io_session = new AsyncIOSession(*ss, this);
    MMAddress *mmad = new MMAddress(MMAddress::findNodeNoLocalAddr());
    if ( mmad->OpenName() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed opening name for MM socket peer addr: " << mmad->GetPath() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete ss;
        return 0;
    }
    try {
        io_session->setPeerAddr(mmad);
    } catch (const runtime_error &ex) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed setting MM socket peer addr: " << ex.what() << SoeLogger::LogError() << endl;
        delete io_session;
        delete ss;
        delete mmad;
        return 0;
    }
    io_session->setSessDebug(debug);
    if ( io_session->initialise(0, true) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed initialize io session errno: " << errno << SoeLogger::LogError() << endl;
        delete io_session;
        delete ss;
        delete mmad;
        return 0;
    }

    if ( start_in == true ) {
        io_session->in_thread = new boost::thread(&AsyncIOSession::inLoop, io_session);
    }
    if ( start_out == true ) {
        io_session->out_thread = new boost::thread(&AsyncIOSession::outLoop, io_session);
    }

    return io_session;
}

AsyncIOSession *AdmSession::CreateRemoteAsyncIOSession(const IPv4Address &_io_peer_addr, bool start_in, bool start_out)
{
    bool _clean_up = false;
    TcpSocket *ss = new TcpSocket;

    if ( ss->Create() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't create tcp socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetNoDelay() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set nodelay on tcp socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetRecvTmout(10000) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set recv tmout on tcp socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetSndTmout(10000) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set snd tmout on tcp socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetReuseAddr() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set reuse address on tcp socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetNoLinger() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set no linger on tcp socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }
    if ( ss->SetNoSigPipe() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't set no sigpipe on tcp socket: " << errno << SoeLogger::LogError() << endl;
        _clean_up = true;
    }

    if ( _clean_up == true ) {
        delete ss;
        return 0;
    }

    AsyncIOSession *io_session = new AsyncIOSession(*ss, this);
    IPv4Address *ipv4addr = new IPv4Address(io_peer_addr);
    io_session->setPeerAddr(ipv4addr);
    io_session->setSessDebug(debug);
    if ( io_session->initialise(0, true) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Failed initialize io session errno: " << errno << SoeLogger::LogError() << endl;
        delete io_session;
        delete ss;
        delete ipv4addr;
        return 0;
    }

    if ( start_in == true ) {
        io_session->in_thread = new boost::thread(&AsyncIOSession::inLoop, io_session);
    }
    if ( start_out == true ) {
        io_session->out_thread = new boost::thread(&AsyncIOSession::outLoop, io_session);
    }

    return io_session;
}

void AdmSession::start() throw(runtime_error)
{}

//
// ret < 0 - error parsing
// ret == 0 - need to connect or reconnect (due to server address change or restart)
// ret > 0 - no change
//
int AdmSession::ParseSetAddresses(const string &ad_json)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got json: " << ad_json << SoeLogger::LogDebug() << endl;

    MsgNetMcastAdvertisement ad_msg(ad_json);
    int ret = ad_msg.Parse();
    if ( ret ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "parse failed " << SoeLogger::LogError() << endl;
        return -1;
    }
    ret = 1;

    AdvertisementItem *ad_item = ad_msg.FindItemByWhat("meta_srv_adm");
    if ( ad_item ) {
        uint16_t prot = ad_item->protocol == AdvertisementItem::eProtocol::eInet ? AF_INET : AF_INET;
        IPv4Address new_address(ad_item->address.c_str(), htons(ad_item->port), prot);
        if ( new_address != peer_addr ) {
            if ( debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got new srv json: " << ad_json <<
                    " address: " << new_address.GetNTOASinAddr() <<
                    " port: " << new_address.GetPort() <<
                    " fam: " << new_address.GetFamily() << SoeLogger::LogDebug() << endl;
            }

            this->setPeerAddr(new_address);
            ret = 0;
        }
    }
    ad_item = ad_msg.FindItemByWhat("meta_srv_io");
    if ( ad_item ) {
        uint16_t prot = ad_item->protocol == AdvertisementItem::eProtocol::eInet ? AF_INET : AF_INET;
        IPv4Address io_new_address(ad_item->address.c_str(), htons(ad_item->port), prot);
        if ( io_new_address != io_peer_addr ) {
            if ( debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got new srv json: " << ad_json <<
                    " address: " << io_new_address.GetNTOASinAddr() <<
                    " port: " << io_new_address.GetPort() <<
                    " fam: " << io_new_address.GetFamily() << SoeLogger::LogDebug() << endl;
            }

            this->setIOPeerAddr(io_new_address);
            ret = 0;
        }
    }

    return ret;
}

TcpSocket *AdmSession::GetAdmSessionSock()
{
    int i;
    for ( i = 0 ; i < 5000 ; i++ ) {
        if ( GetAdmSession() && GetAdmSession()->started == true ) {
            break;
        }
        usleep(500);
    }
    if ( i >= 5000 ) {
        return 0;
    }
    return &GetAdmSession()->tcp_sock;
}

int AdmSession::processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid invocation" << SoeLogger::LogError() << endl;
    return -1;
}

int AdmSession::processStreamMessage(MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got msg" << SoeLogger::LogError() << endl;
    return 0;
}

int AdmSession::sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid invocation" << SoeLogger::LogError() << endl;
    return -1;
}

int AdmSession::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Sending msg" << SoeLogger::LogError() << endl;
    return 0;
}

}
