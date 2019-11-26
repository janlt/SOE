/**
 * @file    metadbsrveventprocessor.cpp
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
 * metadbsrveventprocessor.cpp
 *
 *  Created on: Jun 29, 2017
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
#include <dirent.h>
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

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <chrono>
#include <boost/random.hpp>

extern "C" {
#include "soeutil.h"
}

#include "soelogger.hpp"
using namespace SoeLogger;

#include "soe_bind_api.hpp"
#include "rcsdbargdefs.hpp"
#include "rcsdbidgener.hpp"
#include "rcsdbfacadeapi.hpp"
#include "rcsdbproxyfacadebase.hpp"

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
#include "msgnettcpsocket.hpp"
#include "msgnetudpsocket.hpp"
#include "msgnetioevent.hpp"
#include "msgnetprocessor.hpp"
#include "msgnetlistener.hpp"
#include "msgneteventlistener.hpp"
#include "msgnetipv4listener.hpp"
#include "msgnetipv4mcastlistener.hpp"
#include "msgnetsession.hpp"
#include "msgnetipv4tcpsession.hpp"
#include "msgnetipv4tcpsrvsession.hpp"
#include "msgneteventsession.hpp"
#include "msgneteventsrvsession.hpp"
#include "msgnetasynceventsrvsession.hpp"

#include <rocksdb/cache.h>
#include <rocksdb/compaction_filter.h>
#include <rocksdb/slice.h>
#include <rocksdb/table.h>
#include <rocksdb/utilities/options_util.h>
#include <rocksdb/db.h>
#include <rocksdb/utilities/backupable_db.h>
#include <rocksdb/merge_operator.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/utilities/checkpoint.h>

using namespace rocksdb;

#include "rcsdbargdefs.hpp"
#include "rcsdbfacadeapi.hpp"
#include "rcsdbfacadebase.hpp"
#include "rcsdbconfigbase.hpp"
#include "rcsdbconfig.hpp"
#include "rcsdbfilters.hpp"

using namespace MetaNet;

#include "metadbsrvrcsdbname.hpp"
#include "metadbsrvrcsdbstoreiterator.hpp"
#include "metadbsrvrangecontext.hpp"
#include "metadbsrvrcsdbstore.hpp"
#include "metadbsrvrcsdbspace.hpp"
#include "metadbsrvrcsdbcluster.hpp"
#include "metadbsrvrcsdbstorebackup.hpp"
#include "metadbsrvrcsdbsnapshot.hpp"

#include "metadbsrveventprocessor.hpp"
#include "metadbsrvasynciosession.hpp"
#include "metadbsrveventprocessor.hpp"
#include "rcsdbargdefs.hpp"
#include "metadbsrvaccessors.hpp"
#include "metadbsrvaccessors1.hpp"
#include "metadbsrvdefaultstore.hpp"
#include "metadbsrvfacade1.hpp"

using namespace std;

namespace Metadbsrv {

PROXY_TYPES

//
// MetaNet::EventListener msg Processor
//
int EventLisSrvProcessor::CreateLocalUnixSockIOSession(const IPv4Address &_io_peer_addr, MetaNet::EventListener &_event_lis, const Point *_lis_pt, int epoll_fd)
{
    SoeLogger::Logger logger;

    UnixSocket *ss = new UnixSocket();
    AsyncIOSession *sss = new AsyncIOSession(*ss, _event_lis, 0);
    int ret = sss->initialise(const_cast<Point *>(_lis_pt), true);
    logger.Clear(CLEAR_DEFAULT_ARGS) << "Initialized sss: " << sss->event_point.GetDesc() << " ret: " <<
        ret << " errno: " << errno << SoeLogger::LogDebug() << endl;

    if ( ss->SetNoDelay() < 0 ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't SetNoDelay() on sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }
    if ( ss->SetNonBlocking(true) < 0 ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't SetNonBlocking(true) on Unix sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }
    if ( ss->SetRecvTmout(10000) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't set recv tmout on Unix sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }
    if ( ss->SetSndTmout(10000) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't set snd tmout on Unix sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }
    if ( ss->SetNoLinger() < 0 ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't set reuse address on Unix sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }

    WritePlainLock w_lock(AsyncIOSession::global_lock);

    // boundary check
    if ( static_cast<unsigned int>(sss->event_point.GetDesc()) >= AsyncIOSession::max_sessions ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Max number of connected sockets exceeded, refusing connection: " << ss->GetDesc() << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }

    // the slot corresponding to fd must be free
    if ( AsyncIOSession::sessions[sss->event_point.GetDesc()].get() &&
            AsyncIOSession::sessions[sss->event_point.GetDesc()]->state != AsyncIOSession::State::eDefunct ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't add cli session to AsyncIOSession::sessions" << SoeLogger::LogError() << endl;

        if ( AsyncIOSession::sessions[sss->event_point.GetDesc()]->store_proxy ) {
            StoreProxyType *st_pr = dynamic_cast<StoreProxyType *>(AsyncIOSession::sessions[sss->event_point.GetDesc()]->store_proxy);
            if ( st_pr ) {
                if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) <<
                    " Closing store name: " << st_pr->GetName() << " id: " << st_pr->GetId() << SoeLogger::LogDebug() << endl;

                LocalArgsDescriptor args;
                AsyncIOSession::sessions[sss->event_point.GetDesc()]->InitDesc(args);
                try {
                    if ( st_pr->adm_session_id == AsyncIOSession::sessions[sss->event_point.GetDesc()]->adm_session_id ) {
                        st_pr->Close(args);
                        //cout << __FUNCTION__ << "(1.1) ios[" << sss->event_point.GetDesc() << "]->adi=" <<
                             //AsyncIOSession::sessions[sss->event_point.GetDesc()]->adm_session_id << " st_pr->adi=" << st_pr->adm_session_id << endl;
                    }
                } catch (const RcsdbFacade::Exception &ex) {
                    if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Close failed what: " << ex.what() << SoeLogger::LogDebug() << endl;
                }
            }
        }

        sss->deinitialise(true);
        sss->state = AsyncIOSession::State::eDefunct;
        return -1;
    }

    AsyncIOSession::sessions[sss->event_point.GetDesc()].reset(sss);

    ret = _event_lis.AddAsyncPoint(&sss->event_point, epoll_fd);
    if ( ret < 0 ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't add cli async point" << SoeLogger::LogError() << endl;
        sss->deinitialise(true);
        sss->state = AsyncIOSession::State::eDefunct;
        return -1;
    }

    sss->setSessDebug(debug);

    sss->in_thread = new boost::thread(&AsyncIOSession::inLoop, sss);
    sss->out_thread = new boost::thread(&AsyncIOSession::outLoop, sss);

    return 0;
}

int EventLisSrvProcessor::CreateLocalMMSockIOSession(const IPv4Address &_io_peer_addr, MetaNet::EventListener &_event_lis, const Point *_lis_pt, int epoll_fd)
{
    SoeLogger::Logger logger;

    MmPoint *ss = new MmPoint(16);
    if ( ss->Create(true) < 0 ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't Create MM sock errno: " << errno << SoeLogger::LogError() << endl;
        return -1;
    }
    AsyncIOSession *sss = new AsyncIOSession(*ss, _event_lis, 0);
    int ret = sss->initialise(const_cast<Point *>(_lis_pt), true);
    logger.Clear(CLEAR_DEFAULT_ARGS) << "Initialized sss: " << sss->event_point.GetDesc() << " ret: " <<
        ret << " errno: " << errno << SoeLogger::LogDebug() << endl;

    if ( ss->SetNoDelay() < 0 ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't SetNoDelay() on MM sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }
    if ( ss->SetNonBlocking(true) < 0 ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't SetNonBlocking(true) on MM sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }
    if ( ss->SetRecvTmout(10000) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't set recv tmout on MM sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }
    if ( ss->SetSndTmout(10000) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't set snd tmout on MM sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }

    WritePlainLock w_lock(AsyncIOSession::global_lock);

    // boundary check
    if ( static_cast<unsigned int>(sss->event_point.GetDesc()) >= AsyncIOSession::max_sessions ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Max number of connected sockets exceeded, refusing connection: " << ss->GetDesc() << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }

    // the slot corresponding to fd must be free
    if ( AsyncIOSession::sessions[sss->event_point.GetDesc()].get() &&
            AsyncIOSession::sessions[sss->event_point.GetDesc()]->state != AsyncIOSession::State::eDefunct ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't add cli session to AsyncIOSession::sessions" << SoeLogger::LogError() << endl;

        if ( AsyncIOSession::sessions[sss->event_point.GetDesc()]->store_proxy ) {
            StoreProxyType *st_pr = dynamic_cast<StoreProxyType *>(AsyncIOSession::sessions[sss->event_point.GetDesc()]->store_proxy);
            if ( st_pr ) {
                if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) <<
                    " Closing store name: " << st_pr->GetName() << " id: " << st_pr->GetId() << SoeLogger::LogDebug() << endl;

                LocalArgsDescriptor args;
                AsyncIOSession::sessions[sss->event_point.GetDesc()]->InitDesc(args);
                try {
                    if ( st_pr->adm_session_id == AsyncIOSession::sessions[sss->event_point.GetDesc()]->adm_session_id ) {
                        st_pr->Close(args);
                        //cout << __FUNCTION__ << "(1.2) ios[" << sss->event_point.GetDesc() << "]->adi=" <<
                             //AsyncIOSession::sessions[sss->event_point.GetDesc()]->adm_session_id << " st_pr->adi=" << st_pr->adm_session_id << endl;
                    }
                } catch (const RcsdbFacade::Exception &ex) {
                    if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Close failed what: " << ex.what() << SoeLogger::LogDebug() << endl;
                }
            }
        }

        sss->deinitialise(true);
        sss->state = AsyncIOSession::State::eDefunct;
        return -1;
    }

    AsyncIOSession::sessions[sss->event_point.GetDesc()].reset(sss);

    ret = _event_lis.AddAsyncPoint(&sss->event_point, epoll_fd);
    if ( ret < 0 ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't add cli session" << SoeLogger::LogError() << endl;
        sss->deinitialise(true);
        sss->state = AsyncIOSession::State::eDefunct;
        return -1;
    }

    sss->setSessDebug(debug);

    sss->in_thread = new boost::thread(&AsyncIOSession::inLoop, sss);
    sss->out_thread = new boost::thread(&AsyncIOSession::outLoop, sss);

    return 0;
}

int EventLisSrvProcessor::CreateRemoteIOSession(const IPv4Address &_io_peer_addr, MetaNet::EventListener &_event_lis, const Point *_lis_pt, int epoll_fd)
{
    SoeLogger::Logger logger;

    TcpSocket *ss = new TcpSocket();
    AsyncIOSession *sss = new AsyncIOSession(*ss, _event_lis, 0);
    int ret = sss->initialise(const_cast<Point *>(_lis_pt), true);
    logger.Clear(CLEAR_DEFAULT_ARGS) << "Initialized sss: " << sss->event_point.GetDesc() << " ret: " <<
        ret << " errno: " << errno << SoeLogger::LogDebug() << endl;

    if ( ss->SetNoDelay() < 0 ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't SetNoDelay() on TCP sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }
    if ( ss->SetNonBlocking(true) < 0 ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't SetNonBlocking(true) on TCP sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }
    if ( ss->SetRecvTmout(10000) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't set recv tmout on TCP sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }
    if ( ss->SetSndTmout(10000) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't set snd tmout on TCP sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        delete ss;
        return -1;
    }
    if ( ss->SetReuseAddr() < 0 ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't set reuse address on TCP sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }
    if ( ss->SetNoLinger() < 0 ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't set reuse address on TCP sock: " << ss->GetDesc() << " errno: " << errno << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }

    WritePlainLock w_lock(AsyncIOSession::global_lock);

    // boundary check
    if ( static_cast<unsigned int>(sss->event_point.GetDesc()) >= AsyncIOSession::max_sessions ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Max number of connected sockets exceeded, refusing connection: " << ss->GetDesc() << SoeLogger::LogError() << endl;
        delete sss;
        ss->Shutdown();
        delete ss;
        return -1;
    }

    // the slot corresponding to fd must be free
    if ( AsyncIOSession::sessions[sss->event_point.GetDesc()].get() &&
            AsyncIOSession::sessions[sss->event_point.GetDesc()]->state != AsyncIOSession::State::eDefunct ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't add cli session to AsyncIOSession::sessions" << SoeLogger::LogError() << endl;

        //cout << "EventLisSrvProcessor::" << __FUNCTION__ << " sock: " << sss->event_point.GetDesc() <<
            //" type: " << AsyncIOSession::sessions[sss->event_point.GetDesc()]->type <<
            //" state: " << AsyncIOSession::sessions[sss->event_point.GetDesc()]->state << endl;

        // If proxy exists for this i/o session we need to issues a close
        if ( AsyncIOSession::sessions[sss->event_point.GetDesc()]->store_proxy ) {
            StoreProxyType *st_pr = dynamic_cast<StoreProxyType *>(AsyncIOSession::sessions[sss->event_point.GetDesc()]->store_proxy);
            if ( st_pr ) {
                if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) <<
                    " Closing store name: " << st_pr->GetName() << " id: " << st_pr->GetId() << SoeLogger::LogDebug() << endl;

                LocalArgsDescriptor args;
                AsyncIOSession::sessions[sss->event_point.GetDesc()]->InitDesc(args);
                try {
                    if ( st_pr->adm_session_id == AsyncIOSession::sessions[sss->event_point.GetDesc()]->adm_session_id ) {
                        st_pr->Close(args);
                    }
                } catch (const RcsdbFacade::Exception &ex) {
                    if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Close failed what: " << ex.what() << SoeLogger::LogDebug() << endl;
                }
            }

            sss->deinitialise(true);
            sss->state = AsyncIOSession::State::eDefunct;
            return -1;
        }
    }

    AsyncIOSession::sessions[sss->event_point.GetDesc()].reset(sss);

    ret = _event_lis.AddAsyncPoint(&sss->event_point, epoll_fd);
    if ( ret < 0 ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't add cli session" << SoeLogger::LogError() << endl;
        sss->deinitialise(true);
        sss->state = AsyncIOSession::State::eDefunct;
        return -1;
    }

    sss->setSessDebug(debug);

    sss->in_thread = new boost::thread(&AsyncIOSession::inLoop, sss);
    sss->out_thread = new boost::thread(&AsyncIOSession::outLoop, sss);

    return 0;
}

//
// Event functions
//
int EventLisSrvProcessor::processStreamListenerEvent(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase &_buf, const IPv4Address &from_addr, const MetaNet::IoEvent *ev)
{
    SoeLogger::Logger logger;

    int ret = 0;
    if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got event" << SoeLogger::LogDebug() << endl;

    MetaNet::EventListener &event_lis = dynamic_cast<MetaNet::EventListener &>(lis);

    if ( !ev ) {
        return -1;
    }
    Point *pt = reinterpret_cast<Point *>(ev->ev.data.ptr);

    if ( event_lis.getSock() == pt || event_lis.getUnixSock() == pt || event_lis.getMmSock() == pt ) { // connect request
        if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got connect event type: " << pt->type << " fd: " << pt->GetConstDesc() << SoeLogger::LogDebug() << endl;

         try {
            int ret = 0;
            if ( event_lis.getUnixSock() == pt ) {
                //ret = CreateLocalUnixSockIOSession(from_addr, event_lis, pt, ev->epoll_fd);
                ret = CreateRemoteIOSession(from_addr, event_lis, pt, ev->epoll_fd);
            } else if ( event_lis.getSock() == pt ){
                ret = CreateRemoteIOSession(from_addr, event_lis, pt, ev->epoll_fd);
            } else if ( event_lis.getMmSock() == pt ){
                ret = CreateLocalMMSockIOSession(from_addr, event_lis, pt, ev->epoll_fd);
            } else {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid Point requesting connection connecting " << SoeLogger::LogError() << endl;
                return -1;
            }

            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Failed connecting cli io session from: " << from_addr.GetNTOASinAddr() <<
                    " to: " << event_lis.getIPv4OwnAddr().GetNTOASinAddr() << SoeLogger::LogError() << endl;
                return ret;
            }
        } catch (const runtime_error &ex) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "Failed connecting cli session: " << ex.what() << SoeLogger::LogError() << endl;
        }
    } else { // msg request, either disconnect or real msg
        if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got msg ev fd: " << pt->GetDesc() << SoeLogger::LogDebug() << endl;

        if ( ev->ev.events & EPOLLRDHUP ) { // connection reset event
            logger.Clear(CLEAR_DEFAULT_ARGS) << " Disconnecting io session: " << pt->GetDesc() << SoeLogger::LogDebug() << endl;

            WritePlainLock w_lock(AsyncIOSession::global_lock);

            // go through this logic only for sync API session
            if ( AsyncIOSession::sessions[pt->GetDesc()].get() ) {
                // remove point from epoll vector so we don't get any more events
                event_lis.RemovePoint(pt);

                AsyncIOSession *io_s = AsyncIOSession::sessions[pt->GetDesc()].get();
#if 0
                if ( dynamic_cast<StoreProxyType *>(io_s->store_proxy) ) {
                    cout << "EventLisSrvProcessor::" << __FUNCTION__ << " HUP fd=" << pt->GetDesc() <<
                        " type/state=" << io_s->type << "/" << io_s->state <<
                        " pr=" << dynamic_cast<StoreProxyType *>(io_s->store_proxy)->GetName() <<
                        "/" << dynamic_cast<StoreProxyType *>(io_s->store_proxy)->GetId() <<
                        " adm_sess_id=" << io_s->adm_session_id <<
                        "/" << dynamic_cast<StoreProxyType *>(io_s->store_proxy)->adm_session_id << endl;
                } else {
                    cout << "EventLisSrvProcessor::" << __FUNCTION__ << " HUP fd=" << pt->GetDesc() <<
                        " type/state=" << io_s->type << "/" << io_s->state <<
                        " adm_sess_id=" << io_s->adm_session_id << endl;
                }
#endif

                if ( io_s->type == AsyncIOSession::Type::eSyncAPISession ) {
                    // If we have a proxy it means connected then we need to close a store
                    // we post an event via inbound queue
                    StoreProxyType *st_pr = dynamic_cast<StoreProxyType *>(io_s->store_proxy);
                    if ( st_pr ) {
                        if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) <<
                            " Closing store name: " << st_pr->GetName() << " id: " << st_pr->GetId() << SoeLogger::LogDebug() << endl;
#if 0
                        cout << "EventLisSrvProcessor::" << __FUNCTION__ << " io_s->state=" << io_s->state <<
                            " st_pr->a_s_id=" << st_pr->adm_session_id << " io_s->a_s_id=" << io_s->adm_session_id << endl;
#endif

                        if ( io_s->state == AsyncIOSession::State::eCreated ) {
                            if ( st_pr->adm_session_id == io_s->adm_session_id ) {
                                // queue up a close event because there might be i/os in the inbound queue
                                // so we can't close the store here
                                ret = io_s->processInboundStreamEvent(_buf, from_addr, ev);
                            }
                        } else {
                            logger.Clear(CLEAR_DEFAULT_ARGS) << " Disconnecting Sync io session: " << pt->GetDesc() <<
                                " CheckFd(): " << pt->CheckFd() << " errno: " << errno << SoeLogger::LogDebug() << endl;
                            io_s->deinitialise(true);
                            io_s->state = AsyncIOSession::State::eDefunct;
                        }
                    } else {
                        io_s->state = AsyncIOSession::State::eDefunct;
                    }
                } else {
                    logger.Clear(CLEAR_DEFAULT_ARGS) << " Disconnecting Async io session: " << pt->GetDesc() <<
                        " CheckFd(): " << pt->CheckFd() << " errno: " << errno << SoeLogger::LogDebug() << endl;
                    io_s->deinitialise(true);
                    io_s->state = AsyncIOSession::State::eDefunct;
                }
            } else {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Session not found " << pt->GetDesc() << SoeLogger::LogError() << endl;
                pt->Close();
            }
        } else { // new msg event to be processed
            // Queue up an event for processing
            if ( AsyncIOSession::sessions[pt->GetDesc()].get() ) {
                if ( AsyncIOSession::sessions[pt->GetDesc()]->state == AsyncIOSession::State::eCreated ) {
                    ret = AsyncIOSession::sessions[pt->GetDesc()]->processInboundStreamEvent(_buf, from_addr, ev);
                }
            } else {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Session not found " << pt->GetDesc() << SoeLogger::LogError() << endl;
                pt->Close();
            }
        }
    }

    return ret;
}

int EventLisSrvProcessor::sendStreamListenerEvent(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::IoEvent *ev)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got event" << SoeLogger::LogDebug() << endl;
    return 0;
}

EventLisSrvProcessor::EventLisSrvProcessor(bool _debug)
    :debug(_debug)
{}

EventLisSrvProcessor::~EventLisSrvProcessor() {}

MetaNet::EventListener *EventLisSrvProcessor::eventlis = 0;
MetaNet::EventListener *EventLisSrvProcessor::getEventListener()
{
    return EventLisSrvProcessor::eventlis;
}

void EventLisSrvProcessor::setDebug(bool _debug)
{
    debug = _debug;
}

}


