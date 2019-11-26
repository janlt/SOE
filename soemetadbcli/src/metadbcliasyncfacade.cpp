/**
 * @file    metadbcliasyncfacade.cpp
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
 * metadbcliasyncfacade.cpp
 *
 *  Created on: Dec 4, 2017
 *      Author: Jan Lisowiec
 */

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include <vector>
#include <map>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

extern "C" {
#include "soeutil.h"
}

#include "soelogger.hpp"
using namespace SoeLogger;

#include "soe_bind_api.hpp"
#include "rcsdbargdefs.hpp"
#include "rcsdbfutureapi.hpp"
#include "rcsdbfacadeapi.hpp"
#include "rcsdbfacadebase.hpp"
#include "rcsdbfunctorbase.hpp"

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

#include "metadbcliargdefs.hpp"
#include "metadbclifacadeapi.hpp"
#include "metadbcliiterator.hpp"
#include "metadbclifacade.hpp"
#include "metadbclimcastprocessor.hpp"
#include "metadbclisessionmgr.hpp"

//
// This is implementation of async iov interface that Soe clients invoke via Soe async (futures) API.
// The requests are routed to RunAsyncOp() which in turn enqueues them to sendOutboundAsyncOp(),
// which sends and receives messages. Upon receipt of an inbound message, which contains a future identifier,
// a LocalArgsDescriptor is created and a corresponding future is located. Future's API is invoked
// using future_signaller callback and LocalArgsDescriptor is passed to the future.
//

namespace MetadbcliFacade {

void StoreProxy::InitDesc(LocalArgsDescriptor &args) const
{
    ::memset(&args, '\0', sizeof(args));
    args.group_idx = -1;
    args.transaction_idx = -1;
    args.iterator_idx = -1;
}

#ifdef __DEBUG_ASYNC_IO__
Lock StoreProxy::io_debug_lock;
#endif

//
// Send async i/o, i.e. when regs.seq_numeber > 0
//
uint32_t StoreProxy::RunAsyncOp(LocalArgsDescriptor &args, eMsgType op_type)
{
    uint32_t op_sts = RcsdbFacade::Exception::eOk;

    if ( state == StoreProxy::State::eDeleted ) {
        return op_sts;
    }

    if ( !future_signaller ) {
        future_signaller = args.future_signaller;
    }

    if ( args.status == 1 ) { // debug ON
        ClusterProxy::msg_debug = true;
        SessionMgr::GetInstance()->McastSetDebug(true);
    } else if ( args.status == 2 ) {
        ClusterProxy::msg_debug = false;
        SessionMgr::GetInstance()->McastSetDebug(false);
    } else {
        ;
    }

    IoEpollEvent epoll_ev;
    epoll_ev.data.ptr = this;
    IoEvent *ev = new IoEvent(epoll_ev);
    args.status = op_type;
    ev->io = &args;

#ifdef __DEBUG1_ASYNC_IO__
    {
        WriteLock io_w_lock(StoreProxy::io_debug_lock);
        cerr << __FUNCTION__ << " seq=0x" << hex << args.seq_number << dec <<
            " pos: " << *reinterpret_cast<const size_t *>(args.end_key) << " th: " << args.group_idx << endl;
    }
#endif

    // enqueue an i/o to the outbound worker thread
    assert(io_session_async);
    MetaMsgs::MsgBufferBase buf;
    IPv4Address from_addr;
    int ret = io_session_async->processOutboundStreamEvent(buf, from_addr, ev);

    return !ret ? RcsdbFacade::Exception::eOk : RcsdbFacade::Exception::eIOError;
}

//
// Send async i/o from the context of out_thread, i.e. after out_fifo FIFO
//
// This method will send async i/o's in batches and receive rsp's, that may or may not be a response
// to the one sent. The received message's seq number will not be checked against the
// sent message's seq number. However, any received async message must correspond to an
// existing "future".
//
uint32_t StoreProxy::SendOutboundAsyncOp(Point &event_point, IoEvent *events[], uint32_t ev_count)
{
    // First, send all the events
    uint32_t thread_id_marker[2];
    thread_id_marker[1] = 0xa5a5a5a5;

    int ret = 0;
    for ( uint32_t i = 0 ; i < ev_count ; i++ ) {
        IoEvent *ev = events[i];

        assert(ev->io);

        GetAsyncReq<TestIov> *gr = MetaMsgs::GetAsyncReq<TestIov>::poolT.get();
        LocalArgsDescriptor *args = reinterpret_cast<LocalArgsDescriptor *>(ev->io);

        // the first msg that goes out has to have the thread_id set so the metadbsrv can
        // identify its StoreProxy
        thread_id_marker[0] = args->group_idx;

        args->buf = reinterpret_cast<char *>(thread_id_marker);
        args->buf_len = sizeof(thread_id_marker); // both fields overloaded, now buf is supposed to have client's thread_id in it

        if ( async_in_loop_started == false ) {
            async_in_thread = new boost::thread(&StoreProxy::RecvInLoop, this);
            async_in_loop_started = true;
        }

        gr->iovhdr.hdr.type = static_cast<eMsgType>(args->status);
        int ret = PrepareSendAsyncIov(*args, gr);

        // async i/o's always have async_io_session bit set for easier identification
        StatusBits s_bits;
        s_bits.status_bits = gr->iovhdr.descr.status;
        s_bits.bits.async_io_session = 1;
        gr->iovhdr.descr.status = s_bits.status_bits;

        if ( ret ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " PrepareSendIov failed " << SoeLogger::LogError() << endl;
            }
            MetaMsgs::GetAsyncReq<TestIov>::poolT.put(gr);
            delete args;
            return RcsdbFacade::Exception::eInvalidArgument;
        }

#ifdef __DEBUG_ASYNC_IO__
    {
        WriteLock io_w_lock(StoreProxy::io_debug_lock);
        uint64_t a_ptrs[2];
        a_ptrs[0] = *reinterpret_cast<const uint64_t *>(args->end_key);
        a_ptrs[1] = *reinterpret_cast<const uint64_t *>(args->end_key + sizeof(uint64_t));
        cerr << __FUNCTION__ << " s_n=" << *args->store_name << " io_s=" << hex << this << dec <<
            " seq=0x" << hex << args->seq_number << dec << " pos=" <<
            a_ptrs[0] << " th=0x" << hex << a_ptrs[1] << dec << " e_p=" << event_point.GetDesc() << endl;
    }
#endif

        {
            ret = event_point.SendIov(*gr);
            if ( ret < 0 ) {
                if ( ClusterProxy::msg_debug == true ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " send failed type:" << gr->iovhdr.hdr.type << SoeLogger::LogError() << endl;
                }
                MetaMsgs::GetAsyncReq<TestIov>::poolT.put(gr);
                delete args;
                return RcsdbFacade::Exception::eIOError;
            }
        }

        MetaMsgs::GetAsyncReq<TestIov>::poolT.put(gr);

        delete args;
    }

    return ret >= 0 ? RcsdbFacade::Exception::eOk : RcsdbFacade::Exception::eIOError;
}

void StoreProxy::RecvInLoop()
{
    for ( ;; ) {
        LocalArgsDescriptor async_args;
        uint32_t sts = RecvAsyncOp(async_args, eMsgInvalid);
        if ( sts ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Terminate in loop sts: " << sts << SoeLogger::LogError() << endl;
            break;
        }
    }
}

//
// Receive async i/o
//
uint32_t StoreProxy::RecvAsyncOp(LocalArgsDescriptor &args, eMsgType op_type)
{
    int ret = 0;

    GetAsyncRsp<TestIov> *gp = MetaMsgs::GetAsyncRsp<TestIov>::poolT.get();

    ret = io_session_async->event_point.RecvIov(*gp);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected errno: " << errno << SoeLogger::LogError() << endl;
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " recv ret: " << ret << " failed type:" << gp->iovhdr.hdr.type << SoeLogger::LogError() << endl;
        }
        MetaMsgs::GetAsyncRsp<TestIov>::poolT.put(gp);
        return RcsdbFacade::Exception::eNetworkError;
    }


    uint32_t rsp_type = gp->iovhdr.hdr.type;

    if ( gp->isAsync() == true ) {
        InitDesc(args);

        ret = PrepareRecvAsyncIov(args, gp);
        if ( ret ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " PrepareRecvAsyncIov failed " << SoeLogger::LogDebug() << endl;
            }
            MetaMsgs::GetAsyncRsp<TestIov>::poolT.put(gp);
            return RcsdbFacade::Exception::eIncomplete;
        }

        uint32_t it_valid;
        gp->GetCombinedStatus(args.status, it_valid);
        args.transaction_db = static_cast<bool>(it_valid);

#ifdef __DEBUG_ASYNC_IO__
        {
            WriteLock io_w_lock(StoreProxy::io_debug_lock);
            uint64_t a_ptrs[2];
            a_ptrs[0] = *reinterpret_cast<const uint64_t *>(args.end_key);
            a_ptrs[1] = *reinterpret_cast<const uint64_t *>(args.end_key + sizeof(uint64_t));
            cerr << __FUNCTION__ << " s_n=" << *args.store_name << " io_s=" << hex << this << dec <<
                " seq=0x" << hex << args.seq_number << dec << " pos=" <<
                a_ptrs[0] << " th=0x" << hex << a_ptrs[1] << dec << " e_p=" << io_session_async->event_point.GetDesc() << endl;
        }
#endif

        // handle rsp as "future"
        FinalizeAsyncRsp(args, rsp_type);
    } else {
        // stray sync response, we don't know which sync req it corresponds to, we stash it
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Received stray sync rsp " << SoeLogger::LogError() << endl;
    }

    MetaMsgs::GetAsyncRsp<TestIov>::poolT.put(gp);

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got GetAsyncRsp<TestIov> sts: " <<
        hex << gp->iovhdr.descr.status << dec <<
        " ret " << ret <<  " th_id: " << thread_id <<
        " seq: " << gp->seq <<
        " rsp_type: " << rsp_type << " total_length: " << gp->iovhdr.hdr.total_length <<
        " cluster: " << gp->meta_desc.desc.cluster_name << " cluster_id: " << gp->meta_desc.desc.cluster_id <<
        " space: " << gp->meta_desc.desc.space_name << " space_id: " << gp->meta_desc.desc.space_id <<
        " store: " << gp->meta_desc.desc.store_name << " store_id: " << gp->meta_desc.desc.store_id << SoeLogger::LogDebug() << endl;
    }

    return ret >= 0 ? RcsdbFacade::Exception::eOk : RcsdbFacade::Exception::eIOError;
}

int StoreProxy::PrepareSendAsyncIov(const LocalArgsDescriptor &args, IovMsgBase *iov)
{
    GetAsyncReq<TestIov> *gr = dynamic_cast<GetAsyncReq<TestIov> *>(iov);

    // set seq number
    gr->seq = args.seq_number;
    gr->iovhdr.seq = args.seq_number;

    return PrepareSendIov(args, iov);
}

int StoreProxy::PrepareRecvAsyncIov(LocalArgsDescriptor &args, IovMsgBase *iov)
{
    GetAsyncRsp<TestIov> *gp = dynamic_cast<GetAsyncRsp<TestIov> *>(iov);

    // set seq number
    gp->seq = gp->iovhdr.seq;
    args.seq_number = gp->seq;

    return PrepareRecvIov(args, iov);
}

void StoreProxy::FinalizeAsyncRsp(LocalArgsDescriptor &args, uint32_t rsp_type)
{
    if ( future_signaller ) {
        RcsdbFacade::FutureSignal sig;
        sig.future = reinterpret_cast<void *>(args.seq_number);
        sig.args = &args;
        future_signaller(&sig);
    }
}

}
