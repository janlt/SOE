/**
 * @file    metadbsrvasynciosession.hpp
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
 * metadbsrvasynciosession.cpp
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
#include "dbsrviovasyncmsgs.hpp"
#include "dbsrvmsgiovfactory.hpp"
#include "dbsrvmsgiovtypes.hpp"
#include "dbsrvmsgiovasynctypes.hpp"

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

#include "metadbsrvrcsdbname.hpp"
#include "metadbsrvjson.hpp"
#include "metadbsrvrcsdbstoreiterator.hpp"
#include "metadbsrvrangecontext.hpp"
#include "metadbsrvrcsdbstore.hpp"
#include "metadbsrvrcsdbspace.hpp"
#include "metadbsrvrcsdbcluster.hpp"
#include "metadbsrvrcsdbstorebackup.hpp"
#include "metadbsrvrcsdbsnapshot.hpp"
#include "metadbsrvaccessors.hpp"
#include "metadbsrvaccessors1.hpp"
#include "metadbsrvfacadeapi.hpp"
#include "metadbsrvasynciosession.hpp"
#include "metadbsrvdefaultstore.hpp"
#include "metadbsrvfacade1.hpp"
#include "metadbsrvadmsession.hpp"

using namespace std;
using namespace MetaNet;
using namespace MetaMsgs;

namespace Metadbsrv {

#ifdef __DEBUG_ASYNC_IO__
Lock AsyncIOSession::io_debug_lock;
#endif

PROXY_TYPES

shared_ptr<AsyncIOSession> AsyncIOSession::sessions[max_sessions];
uint32_t AsyncIOSession::session_counter = 1;
PlainLock AsyncIOSession::global_lock;
IPv4Address AsyncIOSession::my_static_addr;
bool AsyncIOSession::my_static_addr_assigned = false;
Lock AsyncIOSession::my_static_addr_lock;;

AsyncIOSession::AsyncIOSession(Point &point, MetaNet::EventListener &_event_lis, Session *_parent, bool _test, uint64_t _adm_s_id)
    :EventSrvAsyncSession(point, _test),
     counter(0),
     bytes_received(0),
     bytes_sent(0),
     sess_debug(false),
     msg_recv_count(0),
     msg_sent_count(0),
     bytes_recv_count(0),
     bytes_sent_count(0),
     adm_session_id(_adm_s_id),
     idle_ticks(0),
     state(State::eCreated),
     store_proxy(0),
     open_close_ref_count(0),
     in_defunct_ttl(0),
     in_idle_ttl(0),
     session_id(AsyncIOSession::session_counter++),
     parent_session(_parent),
     event_lis(_event_lis),
     in_thread(0),
     out_thread(0),
     type(Type::eAsyncAPISession)
{
    AsyncIOSession::SetMyAddres();
    my_addr = my_static_addr;
}

void AsyncIOSession::setSessDebug(bool _sess_debug)
{
    sess_debug = _sess_debug;
}

AsyncIOSession::~AsyncIOSession()
{
    try {
        inFinito();
        for ( int ii = 0 ; ii < 32 ; ii++ ) {
            ResetInFifo();
        }
        outFinito();
        for ( int ii = 0 ; ii < 32 ; ii++ ) {
            ResetOutFifo();
        }

        // we need to interrupt and join in/out threads to prevent conditional_variable assert to fire off
        // on the FIFOs
        for ( int jj = 0 ; jj < 32 ; jj++ ) {
            if ( state == AsyncIOSession::State::eDefunct ) {
                if ( in_thread ) {
                    for ( int ii = 0 ; ii < 128 ; ii++ ) {
                        in_thread->interrupt();
                        if ( in_thread->joinable() == true ) {
                            break;
                        }
                        ::usleep(1000);
                    }
                    in_thread->try_join_for(boost::chrono::milliseconds(500));
                    delete in_thread;
                }
                if ( out_thread ) {
                    for ( int ii = 0 ; ii < 128 ; ii++ ) {
                        out_thread->interrupt();
                        if ( out_thread->joinable() == true ) {
                            break;
                        }
                        ::usleep(1000);
                    }
                    out_thread->try_join_for(boost::chrono::milliseconds(500));
                    delete out_thread;
                }

                if ( store_proxy && type == Type::eSyncAPISession ) {
                    StoreProxy1 *hang_ptr = new StoreProxy1("", "", "", 0, 0, 0, 0);
                    store_proxy = hang_ptr;
                }

                break;
            } else {
                ::usleep(100);
                continue;
            }
        }

        if ( event_point.GetType() == Socket::eType::eTcp ) {
            //cout << "AsyncIOSession::" << __FUNCTION__ << " sock: " << event_point.GetDesc() << " state: " << state << endl;
            if ( state != AsyncIOSession::State::eDefunct ) {
                deinitialise(true);
                state = AsyncIOSession::State::eDefunct;
            } else {
                dynamic_cast<TcpSocket &>(event_point).Shutdown();
                ::close(dynamic_cast<TcpSocket &>(event_point).GetConstDesc());
            }
        }
    } catch ( ... ) {
        ;
    }
}

uint32_t AsyncIOSession::IncOpenCloseRefCount()
{
    WriteLock w_lock(AsyncIOSession::open_close_ref_count_lock);
    uint32_t prev_cnt = open_close_ref_count;
    open_close_ref_count++;
    return prev_cnt;
}

uint32_t AsyncIOSession::DecOpenCloseRefCount()
{
    WriteLock w_lock(AsyncIOSession::open_close_ref_count_lock);
    uint32_t prev_cnt = open_close_ref_count;
    open_close_ref_count--;
    return prev_cnt;
}

uint32_t AsyncIOSession::OpenCloseRefCountGet()
{
    WriteLock w_lock(AsyncIOSession::open_close_ref_count_lock);
    return open_close_ref_count;
}

uint32_t AsyncIOSession::OpenCloseRefCountGetNoLock()
{
    return open_close_ref_count;
}

uint32_t AsyncIOSession::OpenCloseRefCountSet(uint32_t new_cnt)
{
    WriteLock w_lock(AsyncIOSession::open_close_ref_count_lock);
    uint32_t prev_cnt = open_close_ref_count;
    open_close_ref_count = new_cnt;
    return prev_cnt;
}

void AsyncIOSession::InitDesc(LocalArgsDescriptor &args) const
{
    ::memset(&args, '\0', sizeof(args));
    args.group_idx = -1;
    args.transaction_idx = -1;
    args.iterator_idx = -1;
}

void AsyncIOSession::SetMyAddres()
{
    WriteLock w_lock(AsyncIOSession::my_static_addr_lock);

    if ( my_static_addr_assigned == false ) {
        AsyncIOSession::my_static_addr = IPv4Address::findNodeNoLocalAddr();
        my_static_addr_assigned = true;
    }
}

RcsdbFacade::ProxyNameBase *AsyncIOSession::SetStoreProxy(const GetReq<TestIov> *gr)
{
    // thread_id handling as part of the messaging
    pid_t cr_pid_t = static_cast<pid_t>(ClusterProxyType::no_thread_id);

    StatusBits s_bits;
    s_bits.status_bits = gr->iovhdr.descr.status;
    if ( !s_bits.bits.async_io_session ) {
        if ( gr->meta_desc.desc.end_key_len == 2*sizeof(uint32_t) ) {
            uint32_t thread_id_marker[2];
            ::memcpy(thread_id_marker, gr->meta_desc.desc.end_key, sizeof(thread_id_marker));
            if ( thread_id_marker[1] == 0xa5a5a5a5 ) {
                cr_pid_t = thread_id_marker[0];
            }
        }
    } else {
        if ( gr->meta_desc.desc.buf_len == 2*sizeof(uint32_t) ) {
            uint32_t thread_id_marker[2];
            ::memcpy(thread_id_marker, gr->meta_desc.desc.buf, sizeof(thread_id_marker));
            if ( thread_id_marker[1] == 0xa5a5a5a5 ) {
                cr_pid_t = thread_id_marker[0];
            }
        }
    }
    StoreProxyType *sp = ClusterProxyType::StaticFindStoreProxy(ProxyMapKeyType(gr->meta_desc.desc.cluster_id, static_cast<pid_t>(ClusterProxyType::no_thread_id), gr->meta_desc.desc.cluster_name),
        ProxyMapKeyType(gr->meta_desc.desc.space_id, static_cast<pid_t>(ClusterProxyType::no_thread_id), gr->meta_desc.desc.space_name),
        ProxyMapKeyType(gr->meta_desc.desc.store_id, cr_pid_t, gr->meta_desc.desc.store_name));

    if ( sp ) {
        //cout << "RcsdbFacade::" << __FUNCTION__ << "/" << __LINE__ << " add proxy ok name: " << gr->meta_desc.desc.store_name <<
            //" id: " << gr->meta_desc.desc.store_id << " th_id: " << cr_pid_t << endl;

        LocalArgsDescriptor args;
        InitDesc(args);
        string cl_name(gr->meta_desc.desc.cluster_name);
        string sp_name(gr->meta_desc.desc.space_name);
        string st_name(gr->meta_desc.desc.store_name);
        args.cluster_name = &cl_name;
        args.cluster_id = gr->meta_desc.desc.cluster_id;
        args.space_name = &sp_name;
        args.space_id = gr->meta_desc.desc.space_id;
        args.store_name = &st_name;
        args.store_id = gr->meta_desc.desc.store_id;

        Store *st = Cluster::StaticFindStore(args);
        if ( st ) {
            sp->GetAcs().SetObj(st);
        } else {
            sp = 0;
        }
    } else {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ << " add proxy failed name: " << gr->meta_desc.desc.store_name <<
            " id: " << gr->meta_desc.desc.store_id << " th_id: " << cr_pid_t << SoeLogger::LogError() << endl;
    }

    return sp;
}

int AsyncIOSession::sendInboundStreamEventError(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev,
    uint32_t req_type, RcsdbFacade::ProxyNameBase &error_proxy, RcsdbFacade::Exception::eStatus sts)
{
    uint32_t rsp_type = eMsgInvalidRsp;  // invalid rsp type

    switch ( req_type ) {
    case eMsgAssignMMChannelReq:
        rsp_type = eMsgAssignMMChannelRsp;
        break;
    case eMsgAssignMMChannelRsp:
        break;
    case eMsgCreateClusterReq:
        rsp_type = eMsgCreateClusterRsp;
        break;
    case eMsgCreateClusterRsp:
        break;
    case eMsgCreateSpaceReq:
        rsp_type = eMsgCreateClusterRsp;
        break;
    case eMsgCreateSpaceRsp:
        break;
    case eMsgCreateStoreReq:
        rsp_type = eMsgCreateClusterRsp;
        break;
    case eMsgCreateStoreRsp:
        break;
    case eMsgDeleteClusterReq:
        rsp_type = eMsgDeleteClusterRsp;
        break;
    case eMsgDeleteClusterRsp:
        break;
    case eMsgDeleteSpaceReq:
        rsp_type = eMsgDeleteSpaceRsp;
        break;
    case eMsgDeleteSpaceRsp:
        break;
    case eMsgDeleteStoreReq:
        rsp_type = eMsgDeleteStoreRsp;
        break;
    case eMsgDeleteStoreRsp:
        break;
    case eMsgCreateBackupReq:
        rsp_type = eMsgCreateBackupRsp;
        break;
    case eMsgCreateBackupRsp:
        break;
    case eMsgRestoreBackupReq:
        rsp_type = eMsgRestoreBackupRsp;
        break;
    case eMsgRestoreBackupRsp:
        break;
    case eMsgDeleteBackupReq:
        rsp_type = eMsgRestoreBackupRsp;
        break;
    case eMsgDeleteBackupRsp:
        break;
    case eMsgListBackupsReq:
        rsp_type = eMsgListBackupsRsp;
        break;
    case eMsgListBackupsRsp:
        break;
    case eMsgCreateSnapshotReq:
        rsp_type = eMsgCreateSnapshotRsp;
        break;
    case eMsgCreateSnapshotRsp:
        break;
    case eMsgDeleteSnapshotReq:
        rsp_type = eMsgDeleteSnapshotRsp;
        break;
    case eMsgDeleteSnapshotRsp:
        break;
    case eMsgListSnapshotsReq:
        rsp_type = eMsgListSnapshotsRsp;
        break;
    case eMsgListSnapshotsRsp:
        break;
    case eMsgOpenStoreReq:
        rsp_type = eMsgOpenStoreRsp;
        break;
    case eMsgOpenStoreRsp:
        break;
    case eMsgCloseStoreReq:
        rsp_type = eMsgCloseStoreRsp;
        break;
    case eMsgCloseStoreRsp:
        break;
    case eMsgGetReq:
        rsp_type = eMsgGetRsp;
        break;
    case eMsgGetRsp:
        break;
    case eMsgPutReq:
        rsp_type = eMsgPutRsp;
        break;
    case eMsgPutRsp:
        break;
    case eMsgDeleteReq:
        rsp_type = eMsgDeleteRsp;
        break;
    case eMsgDeleteRsp:
        break;
    case eMsgGetRangeReq:
        rsp_type = eMsgGetRangeRsp;
        break;
    case eMsgGetRangeRsp:
        break;
    case eMsgMergeReq:
        rsp_type = eMsgMergeRsp;
        break;
    case eMsgMergeRsp:
        break;
    case eMsgGetReqNT:
        rsp_type = eMsgGetRspNT;
        break;
    case eMsgGetRspNT:
        break;
    case eMsgPutReqNT:
        rsp_type = eMsgPutRspNT;
        break;
    case eMsgPutRspNT:
        break;
    case eMsgDeleteReqNT:
        rsp_type = eMsgDeleteRspNT;
        break;
    case eMsgDeleteRspNT:
        break;
    case eMsgGetRangeReqNT:
        rsp_type = eMsgGetRangeRspNT;
        break;
    case eMsgGetRangeRspNT:
        break;
    case eMsgMergeReqNT:
        rsp_type = eMsgMergeRspNT;
        break;
    case eMsgMergeRspNT:
        break;
    case eMsgBeginTransactionReq:
        rsp_type = eMsgBeginTransactionRsp;
        break;
    case eMsgBeginTransactionRsp:
        break;
    case eMsgCommitTransactionReq:
        rsp_type = eMsgCommitTransactionRsp;
        break;
    case eMsgCommitTransactionRsp:
        break;
    case eMsgAbortTransactionReq:
        rsp_type = eMsgAbortTransactionRsp;
        break;
    case eMsgAbortTransactionRsp:
        break;
    case eMsgCreateIteratorReq:
        rsp_type = eMsgCreateIteratorRsp;
        break;
    case eMsgCreateIteratorRsp:
        break;
    case eMsgDeleteIteratorReq:
        rsp_type = eMsgDeleteIteratorRsp;
        break;
    case eMsgDeleteIteratorRsp:
        break;
    case eMsgIterateReq:
        rsp_type = eMsgIterateRsp;
        break;
    case eMsgIterateRsp:
        break;
    case eMsgCreateGroupReq:
        rsp_type = eMsgCreateGroupRsp;
        break;
    case eMsgCreateGroupRsp:
        break;
    case eMsgDeleteGroupReq:
        rsp_type = eMsgDeleteGroupRsp;
        break;
    case eMsgDeleteGroupRsp:
        break;
    case eMsgWriteGroupReq:
        rsp_type = eMsgWriteGroupRsp;
        break;
    case eMsgWriteGroupRsp:
        break;
    case eMsgMetaAd:
    default:
        if ( sess_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
        sts = RcsdbFacade::Exception::eInvalidArgument;
        break;
    }

    GetRsp<TestIov> *gp = MetaMsgs::GetRsp<TestIov>::poolT.get();
    LocalArgsDescriptor args;
    InitDesc(args);
    StoreProxyType &e_pr = dynamic_cast<StoreProxyType &>(error_proxy);
    string cluster_n = e_pr.GetClusterName();
    string space_n = e_pr.GetSpaceName();
    string store_n = e_pr.GetName();
    args.cluster_name = &cluster_n;
    args.space_name = &space_n;
    args.store_name = &store_n;
    args.cluster_id = dynamic_cast<StoreProxyType &>(error_proxy).GetClusterId();
    args.space_id = dynamic_cast<StoreProxyType &>(error_proxy).GetSpaceId();
    args.store_id = dynamic_cast<StoreProxyType &>(error_proxy).GetId();
    (void )dynamic_cast<StoreProxyType &>(error_proxy).PrepareSendIov(args, gp);

    // status/type field
    gp->SetCombinedStatus(static_cast<uint32_t>(sts), static_cast<uint32_t>(false));
    gp->iovhdr.hdr.type = rsp_type;

    int ret = event_point.SendIov(*gp);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "send failed type: " << gp->iovhdr.hdr.type << " ret: " << ret << SoeLogger::LogError() << endl;
    }

    msg_sent_count++;
    bytes_sent_count += static_cast<uint64_t>(gp->iovhdr.hdr.total_length);

    MetaMsgs::GetRsp<TestIov>::poolT.put(gp);

    if ( sess_debug == true && !(msg_recv_count%print_divider) ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "(" << session_id << ")msg_sent_count: " << msg_sent_count <<
            " msg_recv_count: " << msg_recv_count <<
            " bytes_sent_count: " << bytes_sent_count <<
            " bytes_recv_count: " << bytes_recv_count << SoeLogger::LogDebug() << endl;
    }

    return ret > 0 ? 0 : ret;
}

//
//
//
int AsyncIOSession::processInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev)
{
    // enqueue in event
    in_idle_ttl = 0;
    int ret = EventSrvAsyncSession::processInboundStreamEvent(buf, from_addr, ev);
    if ( ret ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " failed enqueue ret:" << ret << SoeLogger::LogError() << endl;
    }
    if ( sess_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got and enqueued ev" << SoeLogger::LogDebug() << endl;
    return ret;
}

int AsyncIOSession::FindProxy(const GetReq<TestIov> *gr, MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev, uint32_t req_type, StatusBits &s_bits)
{
    SoeLogger::Logger logger;

    // set the store proxy if not set yet
    if ( !store_proxy ) {
        store_proxy = dynamic_cast<StoreProxy1 *>(SetStoreProxy(gr)); // find store
        if ( store_proxy ) {
            if ( store_proxy->state == StoreProxyType::eDeleted ) {
                string cl(gr->meta_desc.desc.cluster_name);
                string sp(gr->meta_desc.desc.space_name);
                string st(gr->meta_desc.desc.store_name);
                StoreProxyType error_proxy(cl, sp, st,
                    gr->meta_desc.desc.cluster_id,
                    gr->meta_desc.desc.space_id,
                    gr->meta_desc.desc.store_id,
                    0);
                (void )sendInboundStreamEventError(buf, dst_addr, ev, req_type, error_proxy, RcsdbFacade::Exception::eNoFunction);
                return -1;
            }
        } else {
            if ( sess_debug == true ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) <<
                "Not found store: " << gr->meta_desc.desc.store_name <<
                " space: " << gr->meta_desc.desc.space_name <<
                " cluster: " << gr->meta_desc.desc.cluster_name << SoeLogger::LogDebug() << endl;
            }

            string cl(gr->meta_desc.desc.cluster_name);
            string sp(gr->meta_desc.desc.space_name);
            string st(gr->meta_desc.desc.store_name);
            StoreProxyType error_proxy(cl, sp, st,
                gr->meta_desc.desc.cluster_id,
                gr->meta_desc.desc.space_id,
                gr->meta_desc.desc.store_id,
                0);
            (void )sendInboundStreamEventError(buf, dst_addr, ev, req_type, error_proxy, RcsdbFacade::Exception::eNoFunction);
            return -1;
        }
    }

    // we have a proxy here, check the async_io_session bit
    s_bits.status_bits = gr->iovhdr.descr.status;
    if ( !s_bits.bits.async_io_session ) {
        type = Type::eSyncAPISession;
        if ( !store_proxy->io_session ) {
            if ( store_proxy ) {
                store_proxy->io_session = this;
                this->adm_session_id = store_proxy->adm_session_id;
            } else {
                string cl(gr->meta_desc.desc.cluster_name);
                string sp(gr->meta_desc.desc.space_name);
                string st(gr->meta_desc.desc.store_name);
                StoreProxyType error_proxy(cl, sp, st,
                    gr->meta_desc.desc.cluster_id,
                    gr->meta_desc.desc.space_id,
                    gr->meta_desc.desc.store_id,
                    0);
                (void )sendInboundStreamEventError(buf, dst_addr, ev, req_type, error_proxy, RcsdbFacade::Exception::eInternalError);
                return -1;
            }
        }
    } else {
        if ( !store_proxy->io_session_async ) {
            if ( store_proxy ) {
                store_proxy->io_session_async = this;
                this->adm_session_id = store_proxy->adm_session_id;
            } else {
                string cl(gr->meta_desc.desc.cluster_name);
                string sp(gr->meta_desc.desc.space_name);
                string st(gr->meta_desc.desc.store_name);
                StoreProxyType error_proxy(cl, sp, st,
                    gr->meta_desc.desc.cluster_id,
                    gr->meta_desc.desc.space_id,
                    gr->meta_desc.desc.store_id,
                    0);
                (void )sendInboundStreamEventError(buf, dst_addr, ev, req_type, error_proxy, RcsdbFacade::Exception::eInternalError);
                return -1;
            }
        }
    }

    return 0;
}

//
// Multiple event processors are enqueuing events but there is only one dequeuing thread so all events for a given
// session are processed single-threaded
//
int AsyncIOSession::sendInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev)
{
    SoeLogger::Logger logger;

    // dequeue in event
    int ret = EventSrvAsyncSession::sendInboundStreamEvent(buf, dst_addr, ev);
    if ( ret ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "failed dequeue ret:" << ret << SoeLogger::LogError() << endl;
        delete ev;
        return ret;
    }

    if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " (" << session_id << ")Got and dequeued ev" << SoeLogger::LogDebug() << endl;

    // Check if it's a Close event that has ben queued up as a reult of HUP
    // All store close reqs should be coming through here
    if ( ev->ev.events & EPOLLRDHUP ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Disconnect on: " << event_point.GetDesc() <<
            " open_close_ref_count: " << OpenCloseRefCountGet() << " type: " << type <<
            " store_proxy: " << store_proxy <<
            " this: " << this << SoeLogger::LogDebug() << endl;

        bool s_ret = false;
        for ( uint32_t jj = 0 ; jj < 100 ; jj++ ) {
            if ( AsyncIOSession::global_lock.try_lock() == false ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Lock denied " <<
                    " open_close_ref_count: " << OpenCloseRefCountGet() << " type: " << type << SoeLogger::LogDebug() << endl;
                ::usleep(100000);
            } else {
                s_ret = true;
                break;
            }
        }
        if ( s_ret == false ) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "Lock failed " <<
                " open_close_ref_count: " << OpenCloseRefCountGet() << " type: " << type << SoeLogger::LogError() << endl;
            delete ev;
            return MetaMsgs::MsgStatus::eMsgDisconnect;
        }

#if 0
        if ( store_proxy ) {
            cout << "AsyncIOSession::" << __FUNCTION__ << " HUP fd=" << event_point.GetDesc() << " type=" << type <<
                " s_p=" << store_proxy->GetName() << "/" << store_proxy->GetId() << " o_c_r_c=" << OpenCloseRefCountGet() << " state=" << state <<
                " acs->state=" << store_proxy->GetAcs().get_state() << " acs->rcsdb_r_c=" << store_proxy->GetAcs().get_ref_count() << endl;
        } else {
            cout << "AsyncIOSession::" << __FUNCTION__ << " HUP fd=" << event_point.GetDesc() << " state=" << state << " type=" << type <<
                " s_p=" << store_proxy << " o_c_r_c=" << OpenCloseRefCountGet() << endl;
        }
#endif

        // Normally we should be asserting on store_proxy != 0 but we might be getting close event prior to setting store_proxy
        if ( store_proxy &&
            (store_proxy->state ==  StoreProxy1::State::eCreated || store_proxy->state == StoreProxy1::State::eOpened) &&
            /*(state == AsyncIOSession::eCreated || state == AsyncIOSession::eDestroyed) &&*/
            OpenCloseRefCountGet() ) {
            // must do try/catch cause OnDisconnect can throw
            uint32_t tmp_cnt = OpenCloseRefCountGet();

            // in case client does something stupid which would block the server
            if ( tmp_cnt > 64 ) {
                OpenCloseRefCountSet(64);
            }
            try {
                for ( uint32_t i = 0 ; i < tmp_cnt ; i++ ) {
                    store_proxy->OnDisconnectClose();
                }
            } catch (const RcsdbFacade::Exception &ex) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Closing store ... ignoring what: " << ex.what() << SoeLogger::LogInfo() << endl;
            }
        }

        deinitialise(true);
        state = AsyncIOSession::State::eDefunct;

        delete ev;
        AsyncIOSession::global_lock.unlock();
        return ret;
    }

    if ( state == AsyncIOSession::State::eDefunct ) {
        delete ev;
        return MetaMsgs::MsgStatus::eMsgDisconnect;
    }

    // proceed with normal msg processing
    GetReq<TestIov> *gr = MetaMsgs::GetReq<TestIov>::poolT.get();

    Point *pt = reinterpret_cast<Point *>(ev->ev.data.ptr);
    ret = pt->RecvIov(*gr);
    //cout << "> io req size: " << ret << " type: " << MsgTypeInfo::ToString(static_cast<eMsgType>(gr->iovhdr.hdr.type)) << endl;
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            this->state = AsyncIOSession::State::eDestroyed;
            if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Cli disconnected fd: " << pt->GetDesc() << SoeLogger::LogDebug() << endl;
        } else {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "recv failed type:" << gr->iovhdr.hdr.type << " ret: " << ret << SoeLogger::LogError() << endl;
        }

        // No action here, event processor will do the cleanup
        MetaMsgs::GetReq<TestIov>::poolT.put(gr);
        delete ev;
        return ret;
    }

    // decode ids
    gr->UnHitchIds();

    // decode bits
    StatusBits s_bits;
    s_bits.status_bits = 0;
    gr->UnhitchStatusBits(s_bits);

    ret = event_lis.RearmAsyncPoint(pt, ev->epoll_fd);
    if ( ret ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "RearmAsyncPoint failed ret: " << ret << " errno: " << errno << SoeLogger::LogError() << endl;
        MetaMsgs::GetReq<TestIov>::poolT.put(gr);
        delete ev;
        return ret;
    }

    msg_recv_count++;
    bytes_recv_count += static_cast<uint64_t>(gr->iovhdr.hdr.total_length);

    if ( sess_debug == true ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "(" << session_id << ")Got GetReq<TestIov> req_type: " << gr->iovhdr.hdr.type <<
        " cluster: " << gr->meta_desc.desc.cluster_name << " cluster_id: " << gr->meta_desc.desc.cluster_id <<
        " space: " << gr->meta_desc.desc.space_name << " space_id: " << gr->meta_desc.desc.space_id <<
        " store: " << gr->meta_desc.desc.store_name << " store_id: " << gr->meta_desc.desc.store_id <<
        SoeLogger::LogDebug() << endl;
    }

    RcsdbFacade::Exception::eStatus sts = RcsdbFacade::Exception::eOk;
    uint32_t req_type = gr->iovhdr.hdr.type;

    // need these two for finding Proxy
    if ( gr->GetIovSize(GetReq<TestIov>::end_key_idx, gr->meta_desc.desc.end_key_len) ) {
        MetaMsgs::GetReq<TestIov>::poolT.put(gr);
        delete ev;
        return -1;
    }

    if ( gr->GetIovSize(GetReq<TestIov>::buf_idx, gr->meta_desc.desc.buf_len) ) {
        MetaMsgs::GetReq<TestIov>::poolT.put(gr);
        delete ev;
        return -1;
    }

    ret = FindProxy(gr, buf, dst_addr, ev, req_type, s_bits);
    if ( ret ) {
        MetaMsgs::GetReq<TestIov>::poolT.put(gr);
        delete ev;
        return ret;
    }

    if ( sess_debug == true ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "(" << store_proxy->GetThreadId() <<
        ")Got GetReq<TestIov> req_type: " << gr->iovhdr.hdr.type <<
        " cluster: " << gr->meta_desc.desc.cluster_name << " cluster_id: " << gr->meta_desc.desc.cluster_id <<
        " space: " << gr->meta_desc.desc.space_name << " space_id: " << gr->meta_desc.desc.space_id <<
        " store: " << gr->meta_desc.desc.store_name << " store_id: " << gr->meta_desc.desc.store_id <<
        SoeLogger::LogDebug() << endl;
    }

    // now that we have a proxy we can create a descriptor and process an i/o
    LocalArgsDescriptor args;
    InitDesc(args);

    // set status bits
    args.overwrite_dups = static_cast<bool>(s_bits.bits.override_dups);
    switch ( s_bits.bits.range_type ) {
    case STORE_RANGE_DELETE:
        args.iterator_idx = Store_range_delete;
        break;
    case SUBSET_RANGE_DELETE:
        args.iterator_idx = Subset_range_delete;
        break;
    case STORE_RANGE_PUT:
        args.iterator_idx = Store_range_put;
        break;
    case SUBSET_RANGE_PUT:
        args.iterator_idx = Subset_range_put;
        break;
    case STORE_RANGE_GET:
        args.iterator_idx = Store_range_get;
        break;
    case SUBSET_RANGE_GET:
        args.iterator_idx = Subset_range_get;
        break;
    case STORE_RANGE_MERGE:
        args.iterator_idx = Store_range_merge;
        break;
    case SUBSET_RANGE_MERGE:
        args.iterator_idx = Subset_range_merge;
        break;
    default:
        break; // no change
    }

    // sequence number that determines if it's an async ( > 0) or sync op (== 0)
    args.seq_number = gr->iovhdr.seq;

    ret = store_proxy->PrepareRecvDesc(args, gr);
    if ( ret ) {
        MetaMsgs::GetReq<TestIov>::poolT.put(gr);
        if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Failed PrepareRecvDesc " << SoeLogger::LogDebug() << endl;
        string cl(gr->meta_desc.desc.cluster_name);
        string sp(gr->meta_desc.desc.space_name);
        string st(gr->meta_desc.desc.store_name);
        StoreProxyType error_proxy(cl, sp, st,
            gr->meta_desc.desc.cluster_id,
            gr->meta_desc.desc.space_id,
            gr->meta_desc.desc.store_id,
            0);

        if ( ret == -2 ) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "Duplicate id or name detected, cluster:  " << cl << " cluster_id: " << gr->meta_desc.desc.cluster_id <<
                " space: " << sp << " space_id: " << gr->meta_desc.desc.space_id <<
                " store: " << st << " store_id: " << gr->meta_desc.desc.store_id << SoeLogger::LogError() << endl;

            int er_ret = sendInboundStreamEventError(buf, dst_addr, ev, req_type, error_proxy, RcsdbFacade::Exception::eDuplicateIdDetected);
            delete ev;
            return er_ret;
        } else {
            int er_ret = sendInboundStreamEventError(buf, dst_addr, ev, req_type, error_proxy, RcsdbFacade::Exception::eNetworkError);
            delete ev;
            return er_ret;
        }
    }

    if ( sess_debug == true ) {
        char print_data[128];
        char print_key[128];
        char print_end_key[128];
        ::memset(print_data, '\0', sizeof(print_data));
        ::memset(print_key, '\0', sizeof(print_key));
        ::memset(print_end_key, '\0', sizeof(print_end_key));
        ::memcpy(print_data, args.data, MIN(args.data_len, sizeof(print_data)));
        print_data[sizeof(print_data) - 1] = '\0';
        ::memcpy(print_key, args.key, MIN(args.key_len, sizeof(print_key)));
        print_key[sizeof(print_key) - 1] = '\0';
        ::memcpy(print_end_key, args.end_key, MIN(args.end_key_len, sizeof(print_end_key)));
        print_end_key[sizeof(print_end_key) - 1] = '\0';

        logger.Clear(CLEAR_DEFAULT_ARGS) << "(" << session_id << ")Data: " << gr->iovhdr.hdr.type <<
        " key/end_key: " << print_key << "/" << print_end_key <<
        " key_len/end_key_len: " << args.key_len << "/" << args.end_key_len <<
        " data: " << print_data << " data_len: " << args.data_len <<
        " buf_len: " << args.buf_len <<
        SoeLogger::LogDebug() << endl;
    }

    uint32_t rsp_type = eMsgInvalidRsp;  // invalid type

    // handle async vs sync ops separately
    if ( !args.seq_number ) {
        rsp_type = store_proxy->processSyncInboundOp(args, req_type, sts);

        if ( sess_debug == true ) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "about to send " << rsp_type << SoeLogger::LogDebug() << endl;
        }

        GetRsp<TestIov> *gp = MetaMsgs::GetRsp<TestIov>::poolT.get();
        ret = store_proxy->PrepareSendIov(args, gp);
        if ( ret ) {
            MetaMsgs::GetReq<TestIov>::poolT.put(gr);
            MetaMsgs::GetRsp<TestIov>::poolT.put(gp);
            if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Failed PrepareSendIov " << SoeLogger::LogDebug() << endl;
            delete ev;
            return ret;
        }

        // status/type field
        gp->SetCombinedStatus(static_cast<uint32_t>(sts), static_cast<uint32_t>(args.transaction_db));
        gp->iovhdr.hdr.type = rsp_type;

        ret = event_point.SendIov(*gp);
        //cout << "< io rsp size: " << ret << " type: " << MsgTypeInfo::ToString(static_cast<eMsgType>(gp->iovhdr.hdr.type)) << endl;
        if ( ret < 0 ) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "send failed type: " << gp->iovhdr.hdr.type << " ret: " << ret << SoeLogger::LogError() << endl;
            if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
                this->state = AsyncIOSession::State::eDestroyed;
            }
        }

        msg_sent_count++;
        bytes_sent_count += static_cast<uint64_t>(gp->iovhdr.hdr.total_length);

        MetaMsgs::GetRsp<TestIov>::poolT.put(gp);

        if ( sess_debug == true && !(msg_recv_count%print_divider) ) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "(" << session_id << ")msg_sent_count: " << msg_sent_count <<
                " msg_recv_count: " << msg_recv_count <<
                " bytes_sent_count: " << bytes_sent_count <<
                " bytes_recv_count: " << bytes_recv_count << SoeLogger::LogDebug() << endl;
        }
    } else {
#ifdef __DEBUG1_ASYNC_IO__
        {
            WriteLock io_w_lock(AsyncIOSession::io_debug_lock);
            if ( args.end_key_len == 2*sizeof(uint64_t) ) {
                uint64_t a_ptrs[2];
                a_ptrs[0] = *reinterpret_cast<const uint64_t *>(args.end_key);
                a_ptrs[1] = *reinterpret_cast<const uint64_t *>(args.end_key + sizeof(uint64_t));
                cerr << __FUNCTION__ << " s_n=" << *args.store_name << " io_s=" << hex << this << dec <<
                    " seq=0x" << hex << args.seq_number << dec << " pos=" <<
                    a_ptrs[0] << " th=0x" << hex << a_ptrs[1] << dec << endl;
            }
        }
#endif

        rsp_type = store_proxy->processAsyncInboundOp(args, req_type, sts);

        if ( sess_debug == true ) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "about to send " << rsp_type << SoeLogger::LogDebug() << endl;
        }

        GetAsyncRsp<TestIov> *gp = MetaMsgs::GetAsyncRsp<TestIov>::poolT.get();
        ret = store_proxy->PrepareSendIov(args, gp);
        if ( ret ) {
            MetaMsgs::GetReq<TestIov>::poolT.put(gr);
            MetaMsgs::GetAsyncRsp<TestIov>::poolT.put(gp);
            if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "PrepareSendIov for async io failed" << SoeLogger::LogError() << endl;
            delete ev;
            return ret;
        }

        // status/type field
        gp->SetCombinedStatus(static_cast<uint32_t>(sts), static_cast<uint32_t>(args.transaction_db));
        gp->iovhdr.hdr.type = rsp_type;

        ret = event_point.SendIov(*gp);
        if ( ret < 0 ) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "send failed type: " << gp->iovhdr.hdr.type << " ret: " << ret << SoeLogger::LogError() << endl;
            if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
                this->state = AsyncIOSession::State::eDestroyed;
            }
        }

        msg_sent_count++;
        bytes_sent_count += static_cast<uint64_t>(gp->iovhdr.hdr.total_length);

        MetaMsgs::GetAsyncRsp<TestIov>::poolT.put(gp);

        if ( sess_debug == true && !(msg_recv_count%print_divider) ) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "(" << session_id << ")msg_sent_count: " << msg_sent_count <<
                " msg_recv_count: " << msg_recv_count <<
                " bytes_sent_count: " << bytes_sent_count <<
                " bytes_recv_count: " << bytes_recv_count << SoeLogger::LogDebug() << endl;
        }
    }

    MetaMsgs::GetReq<TestIov>::poolT.put(gr);

    delete ev;

    return ret > 0 ? 0 : ret;
}

int AsyncIOSession::processOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev)
{
    // enqueue out event
    int ret = EventSrvAsyncSession::processOutboundStreamEvent(buf, from_addr, ev);
    if ( ret ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed enqueue ret:" << ret << SoeLogger::LogError() << endl;
        return ret;
    }
    if ( sess_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got and enqueued ev" << SoeLogger::LogDebug() << endl;
    return ret;
}

int AsyncIOSession::sendOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev)
{
    // dequeue out event
    int ret = EventSrvAsyncSession::sendOutboundStreamEvent(buf, dst_addr, ev);
    if ( ret ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "failed dequeue ret:" << ret << SoeLogger::LogError() << endl;
        return ret;
    }
    if ( sess_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got and dequeued ev: " << ev->epoll_fd << SoeLogger::LogDebug() << endl;

    delete ev;
    return ret > 0 ? 0 : ret;
}

void AsyncIOSession::inLoop()
{
    if ( sess_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Start inLoop" << SoeLogger::LogDebug() << endl;
    EventSrvAsyncSession::inLoop();
}

void AsyncIOSession::outLoop()
{
    if ( sess_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Start outLoop" << SoeLogger::LogDebug() << endl;
    EventSrvAsyncSession::outLoop();
}

void AsyncIOSession::CloseAllStores()
{
    SoeLogger::Logger logger;

    // discard disconnected i/o sessions that have been idle for > 5 ticks
    if ( store_proxy && type == Type::eSyncAPISession ) {
        if ( sess_debug == true ) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << " name: " << store_proxy->GetName() << " id: " << store_proxy->GetId() <<
                " open_close_ref_count: " << OpenCloseRefCountGet() << SoeLogger::LogDebug() << endl;
        }

        // queue up a close event
        MetaNet::IoEvent *ev = MetaNet::IoEvent::RDHUOEvent();
        MetaMsgs::MsgBufferBase buf;
        IPv4Address from_addr;
        processInboundStreamEvent(buf, from_addr, ev);
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Enqueued close event st_pr->adi=" << store_proxy->adm_session_id << SoeLogger::LogDebug() << endl;
    }
}

// Close all stores on adm session disconnect event
int AsyncIOSession::CloseAllSessionStores(uint64_t _adm_s_id)
{
    SoeLogger::Logger logger;

    if ( AsyncIOSession::global_lock.try_lock() == false ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "lock denied" << _adm_s_id << SoeLogger::LogDebug() << endl;
        return 1;
    }

    for ( uint32_t i = 0 ; i < AsyncIOSession::max_sessions ; i++ ) {
        // no i/o session
        if ( !AsyncIOSession::sessions[i].get() ) {
            continue;
        }

        AsyncIOSession *io_s = AsyncIOSession::sessions[i].get();
        if ( io_s->type == AsyncIOSession::Type::eAsyncAPISession ) {
            continue;
        }

        // i/o session has a proxy ?
        if ( _adm_s_id == AsyncIOSession::invalid_adm_session_id || io_s->adm_session_id == AsyncIOSession::invalid_adm_session_id ||
            io_s->adm_session_id != _adm_s_id ) {
            continue;
        }

        //if ( io_s->store_proxy && (io_s->store_proxy->GetName() == string("") || !io_s->store_proxy->GetId()) ) {
            //delete io_s->store_proxy;
            //io_s->store_proxy = 0;
        //}

        // we have a session that belongs to the _adm_s_id
        // enqueue close event to close session
        if ( io_s->state != AsyncIOSession::State::eDefunct && io_s->store_proxy ) {
            io_s->CloseAllStores();
        }
        io_s->deinitialise(true);
        io_s->state = AsyncIOSession::State::eDefunct;
    }

    AsyncIOSession::global_lock.unlock();

    return 0;
}

// Purge-close all stale stores on adm session timer event
// This will not affect current i/os goinf to opened stores
void AsyncIOSession::PurgeAllSessionsAndStores()
{
    SoeLogger::Logger logger;

    if ( AsyncIOSession::global_lock.try_lock() == false ) {
        return;
    }

    for ( uint32_t i = 0 ; i < AsyncIOSession::max_sessions ; i++ ) {
        // i/o session has a proxy
        if ( AsyncIOSession::sessions[i].get() ) {
            AsyncIOSession *io_s = AsyncIOSession::sessions[i].get();

            //cout << "AsyncIOSession::" << __FUNCTION__ << " sock: " << io_s->event_point.GetDesc() <<
                //" state: " << io_s->state << " proxy: " << io_s->store_proxy << " ttl: " << io_s->in_destroyed_ttl << endl;

            if ( io_s->state != AsyncIOSession::State::eDefunct ) {
                io_s->in_idle_ttl++;

                // check if Created session has been idle for too long and might be dead already
                if ( io_s->in_idle_ttl > 100 ) {
                    // TODO: find socket and its state in the map
                    //io_s->deinitialise(true);
                    //io_s->state = AsyncIOSession::State::eDefunct;
                }
                continue;
            }

            if ( io_s->in_defunct_ttl++ < 20 ) {
                continue;
            }

            // discard disconnected i/o sessions that have been idle for > 20 ticks
            if ( io_s->state != AsyncIOSession::State::eCreated ) {
                //if ( !io_s->store_proxy ) {
                    //continue;
                //} else {
                    //cout << "AsyncIOSession::" << __FUNCTION__ << " resetting ... sock: " << io_s->event_point.GetDesc() <<
                        //" state: " << io_s->state << endl;
                    //AsyncIOSession::sessions[i].reset();
                    //continue;
                //}

                //if ( io_s->store_proxy->GetName() == string("") || !io_s->store_proxy->GetId() ) {
                    //continue;
                //}

                try {
                    //StoreProxyType::State sess_state = io_s->store_proxy->state;
                    //if ( sess_state != StoreProxyType::State::eDeleted && sess_state != StoreProxyType::State::eCreated && io_s->state == State::eDefunct ) {
                        //logger.Clear(CLEAR_DEFAULT_ARGS) << " Closing store state=" << io_s->store_proxy->state <<
                            //" adm_session_id: " << io_s->store_proxy->adm_session_id << SoeLogger::LogDebug() << endl;
                        //io_s->CloseAllStores();
                    //}

                    logger.Clear(CLEAR_DEFAULT_ARGS) << " Deleting stale session sock: " << i <<
                        " cross check sock: " << io_s->event_point.GetDesc() << SoeLogger::LogDebug() << endl;

                    //cout << "AsyncIOSession::" << __FUNCTION__ << " sock: " << io_s->event_point.GetDesc() <<
                        //" type: " << io_s->type <<
                        //" state: " << io_s->state << endl;
                    AsyncIOSession::sessions[i].reset();
                    continue;
                } catch (const RcsdbFacade::Exception &ex) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Purging stale session failed what: " << ex.what() << SoeLogger::LogError() << endl;
                }
            }
        }
    }

    AsyncIOSession::global_lock.unlock();
}

}


