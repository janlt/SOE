/**
 * @file    metadbclifacade.cpp
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
 * metadbclifacade.cpp
 *
 *  Created on: Feb 4, 2017
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

void __attribute__ ((constructor)) MetadbcliFacade::StaticInitializerEnter(void);
void __attribute__ ((destructor)) MetadbcliFacade::StaticInitializerExit(void);

#ifdef _METADBCLI_DEBUG_ON_
#define METADBCLI_DEBUG(a) (a)
#else
#define METADBCLI_DEBUG(a)
#endif

namespace MetadbcliFacade {

string StoreProxy::default_arg = "";
RcsdbFacade::FunctorBase   StoreProxy::static_ftors;
Lock StoreProxy::adm_op_lock;

Lock StoreProxy::garbage_lock;
list<StoreProxy *> StoreProxy::garbage;

uint32_t StoreProxy::GetIter(uint32_t store_iter)
{
    if ( store_iter > StoreProxy::max_iters ) {
        return StoreProxy::invalid_iter_id;
    }

    WriteLock w_lock(StoreProxy::iters_lock);

    if ( !iterators[store_iter] ) {
        iterators[store_iter] = new ProxyIterator(store_iter);
        return store_iter;
    }

    return StoreProxy::invalid_iter_id;
}

uint32_t StoreProxy::PutIter(uint32_t proxy_iter)
{
    if ( proxy_iter > StoreProxy::max_iters ) {
        return StoreProxy::invalid_iter_id;
    }

    WriteLock w_lock(StoreProxy::iters_lock);

    if ( iterators[proxy_iter] ) {
        if ( iterators[proxy_iter]->id != proxy_iter ) {
            return StoreProxy::invalid_iter_id;
        }
        delete iterators[proxy_iter];
        iterators[proxy_iter] = 0;
        return proxy_iter;
    }

    return StoreProxy::invalid_iter_id;
}

uint64_t StoreProxy::AddAndRegisterStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    // find if there is cluster proxy
    ClusterProxy *cl_p = ClusterProxy::StaticFindClusterProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name));
    if ( !cl_p ) {
        if ( args.cluster_id != ClusterProxy::invalid_id ) {
            args.cluster_id = ClusterProxy::AddAndRegisterCluster(args);   // valid cluster_id found on the server
        } else {
            args.store_id = StoreProxy::invalid_id;  // no store_id found on the server
        }
    }

    // find if there is space proxy
    SpaceProxy *sp_p = ClusterProxy::StaticFindSpaceProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name),
        ProxyMapKey(args.space_id, ClusterProxy::no_thread_id, *args.space_name));
    if ( !sp_p ) {
        if ( args.space_id != SpaceProxy::invalid_id ) {
            args.space_id = SpaceProxy::AddAndRegisterSpace(args);   // valid space_id found on the server
        } else {
            args.store_id = StoreProxy::invalid_id;  // no store_id found on the server
        }
    }

    // find out if we have defunct StoreProxy which needs to be cleaned up
    StoreProxy *st_new = ClusterProxy::StaticFindStoreProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name),
        ProxyMapKey(args.space_id, ClusterProxy::no_thread_id, *args.space_name),
        ProxyMapKey(args.store_id, args.group_idx, *args.store_name));
    if ( st_new ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Found possibly stale StoreProxy state: " <<
            st_new->state << " name: " << *args.store_name << " id: " << args.store_id << SoeLogger::LogDebug() << endl;

        if ( st_new->io_session ) {
            if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " io_session state: " <<
                st_new->io_session->state << SoeLogger::LogDebug() << endl;

            if ( st_new->io_session->state == AsyncIOSession::State::eDefunct ) {
                if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Found stale AsyncIOSession state: " <<
                    st_new->io_session->state << SoeLogger::LogDebug() << endl;

                bool del_ret = ClusterProxy::StaticDeleteStoreProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name),
                    ProxyMapKey(args.space_id, ClusterProxy::no_thread_id, *args.space_name),
                    ProxyMapKey(st_new->id, st_new->thread_id, st_new->name));
                if ( del_ret == false ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Fatal error " << SoeLogger::LogError() << endl;
                    abort();
                }

                delete st_new;
                st_new = 0;
            }
        }

        if ( st_new && st_new->io_session_async ) {
            if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " io_session_async state: " <<
                st_new->io_session_async->state << SoeLogger::LogDebug() << endl;

            if ( st_new->io_session_async->state == AsyncIOSession::State::eDefunct ) {
                if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Found stale AsyncIOSession state: " <<
                    st_new->io_session_async->state << SoeLogger::LogDebug() << endl;

                bool del_ret = ClusterProxy::StaticDeleteStoreProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name),
                    ProxyMapKey(args.space_id, ClusterProxy::no_thread_id, *args.space_name),
                    ProxyMapKey(st_new->id, st_new->thread_id, st_new->name));
                if ( del_ret == false ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Proxy for delete not found for io_session_async " << SoeLogger::LogDebug() << endl;
                } else {
                    delete st_new;
                    st_new = 0;
                }
            }
        }
    }

    // create new store proxy and add it
    st_new = new StoreProxy(*args.cluster_name,
        *args.space_name,
        *args.store_name,
        args.cluster_id,
        args.space_id,
        args.store_id,
        args.group_idx, // pid_t of the calling thread
        ClusterProxy::msg_debug);
    bool ins_ret = ClusterProxy::StaticAddStoreProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name),
        ProxyMapKey(args.space_id, ClusterProxy::no_thread_id, *args.space_name),
        *st_new);
    if ( ins_ret == false ) {
        delete st_new;
        args.store_id = StoreProxy::invalid_id;
    } else {
        // Currently, only TCP IPv4 based io sessions are created
        IPv4Address default_start_addr;
        default_start_addr.addr.sin_family = AF_INET;
        default_start_addr.addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if ( string(AdmSession::GetAdmSession()->own_addr.GetNTOASinAddr()) == string(AdmSession::GetAdmSession()->peer_addr.GetNTOASinAddr()) ||
            string(default_start_addr.GetNTOASinAddr()) == string(AdmSession::GetAdmSession()->peer_addr.GetNTOASinAddr()) ) {
            //st_new->io_session = AdmSession::GetAdmSession()->CreateLocalUnixSockAsyncIOSession(AdmSession::GetAdmSession()->peer_addr);
            st_new->io_session = AdmSession::GetAdmSession()->CreateRemoteAsyncIOSession(AdmSession::GetAdmSession()->peer_addr, false, false);
            st_new->io_session_async = AdmSession::GetAdmSession()->CreateRemoteAsyncIOSession(AdmSession::GetAdmSession()->peer_addr, false, true);
        }
        // Might be duplicates so try/catch and ignore
        try {
            st_new->RegisterStaticApiFunctions();
        } catch (const SoeApi::Exception &ex) {
            if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got SoeApi exception(RegisterStatic) ignored ... " <<
               " what: " << ex.what() << SoeLogger::LogDebug() << endl;
        }

        // Exception on this is an error
        try {
            st_new->RegisterApiFunctions();
            st_new->IncRefCount();
        } catch (const SoeApi::Exception &ex) {
            if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got SoeApi exception" <<
               " what: " << ex.what() << SoeLogger::LogError() << endl;
            args.store_id = StoreProxy::invalid_id;
        }
    }

    return args.store_id;
}

void StoreProxy::RemoveAndDeregisterStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    // find if there is cluster proxy
    ClusterProxy *cl_p = ClusterProxy::StaticFindClusterProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name));
    if ( !cl_p ) {
        if ( args.cluster_id != ClusterProxy::invalid_id ) {
            args.cluster_id = ClusterProxy::AddAndRegisterCluster(args);   // valid cluster_id found on the server
        } else {
            args.store_id = StoreProxy::invalid_id;  // no store_id found on the server
        }
    }

    // find if there is space proxy
    SpaceProxy *sp_p = ClusterProxy::StaticFindSpaceProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name),
        ProxyMapKey(args.space_id, ClusterProxy::no_thread_id, *args.space_name));
    if ( !sp_p ) {
        if ( args.space_id != SpaceProxy::invalid_id ) {
            args.space_id = SpaceProxy::AddAndRegisterSpace(args);   // valid space_id found on the server
        } else {
            args.store_id = StoreProxy::invalid_id;  // no store_id found on the server
        }
    }

    // find out if we have defunct StoreProxy which needs to be cleaned up
    StoreProxy *st_new = ClusterProxy::StaticFindStoreProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name),
        ProxyMapKey(args.space_id, ClusterProxy::no_thread_id, *args.space_name),
        ProxyMapKey(args.store_id, args.group_idx, *args.store_name));
    if ( st_new ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Found possibly stale StoreProxy state: " <<
            st_new->state << " name: " << *args.store_name << " id: " << args.store_id << SoeLogger::LogDebug() << endl;

        if ( st_new->io_session ) {
            if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " io_session state: " <<
                st_new->io_session->state << SoeLogger::LogDebug() << endl;

            if ( st_new->io_session->state == AsyncIOSession::State::eDefunct ) {
                if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Found stale AsyncIOSession state: " <<
                    st_new->io_session->state << SoeLogger::LogDebug() << endl;

                // Exception on this is an error
                try {
                    st_new->DeregisterApiFunctions();
                } catch (const SoeApi::Exception &ex) {
                    if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got SoeApi exception" <<
                       " what: " << ex.what() << SoeLogger::LogError() << endl;
                }

                bool del_ret = ClusterProxy::StaticDeleteStoreProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name),
                    ProxyMapKey(args.space_id, ClusterProxy::no_thread_id, *args.space_name),
                    ProxyMapKey(st_new->id, st_new->thread_id, st_new->name));
                if ( del_ret == false ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Fatal error " << SoeLogger::LogError() << endl;
                    abort();
                }

                Archive(st_new);
            }
        }
    }
}

void StoreProxy::Archive(StoreProxy *_pr)
{
    WriteLock w_lock(StoreProxy::garbage_lock);

    StoreProxy::garbage.push_front(_pr);

    bool removed = false;
    for( ;; ) {
        removed = false;
        for ( StoreProxy * pr : StoreProxy::garbage ) {
            if ( pr->ttl_in_defunct++ > 20 ) {
                //std::cout << "@@@@ identified proxy for deletion " << pr->name << std::endl;
                pr->CloseIOSession();
                StoreProxy::garbage.remove(pr);
                delete pr;
                removed = true;
                break;
            }
        }

        if ( removed == false ) {
            break;
        }
    }
}

//
// StoreProxy
//
StoreProxy  *StoreProxy::reg_api_instance = 0;

uint64_t StoreProxy::CreateStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;
    bool get_by_name = false;

    if ( args.cluster_name && args.space_name && args.store_name ) {
        METADBCLI_DEBUG(cout << "StoreProxy::" << __FUNCTION__ << " cn: " << *args.cluster_name <<
                " sn: " << *args.space_name << " tn: " << *args.store_name << endl);
    }

    WriteLock w_lock(ClusterProxy::adm_op_lock);

    SessionMgr::StartMgr();

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock || tcp_sock->Created() == false ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgCreateStoreReq *ct_req = MetaMsgs::AdmMsgCreateStoreReq::poolT.get();
    if ( args.status == 1 ) { // debug ON
        ClusterProxy::msg_debug = true;
        ClusterProxy::global_debug = true;
        ct_req->json = string("{ \"debug\": true}");
        SessionMgr::GetInstance()->McastSetDebug(true);
    } else if ( args.status == 2 ) {
        ClusterProxy::msg_debug = false;
        ClusterProxy::global_debug = false;
        ct_req->json = string("{ \"debug\": false}");
        SessionMgr::GetInstance()->McastSetDebug(false);
    } else {
        ;
    }

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " w: " << reinterpret_cast<uint64_t>(args.range_callback_arg) << SoeLogger::LogDebug() << endl;
    }

    ct_req->cluster_name = *args.cluster_name;
    ct_req->space_name = *args.space_name;
    ct_req->store_name = *args.store_name;
    ct_req->cluster_id = args.cluster_id;
    ct_req->space_id = args.space_id;
    ct_req->cr_thread_id = args.group_idx;  // overloaded field
    ct_req->only_get_by_name = (args.range_callback_arg == (char *)LD_GetByName ? 1 : 0);
    ct_req->flags.bit_fields.transactional = args.transaction_db;
    ct_req->flags.bit_fields.overwrite_dups = args.overwrite_dups;
    ct_req->flags.bit_fields.bloom_bits = args.bloom_bits;  // no overflow must be ensured on 8-bit field
    ct_req->flags.bit_fields.sync_mode = args.sync_mode;
    get_by_name = ct_req->only_get_by_name;
    args.range_callback_arg = 0;

    ret = ct_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't send AdmMsgCreateStoreReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgCreateStoreReq::poolT.put(ct_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgCreateStoreReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgCreateStoreReq::poolT.put(ct_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
            }
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgCreateStoreRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgCreateStoreRsp *ct_rsp = dynamic_cast<MetaMsgs::AdmMsgCreateStoreRsp *>(recv_msg);
    ret = ct_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgCreateStoreRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgCreateStoreRsp::poolT.put(ct_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgCreateStoreRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgCreateStoreRsp " <<
            " cluster_id: " << ct_rsp->cluster_id <<
            " space_id: " << ct_rsp->space_id <<
            " store_id: " << ct_rsp->store_id << " th_id: " << args.group_idx << " sts: " << ct_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
        }
    }
    args.cluster_id = ct_rsp->cluster_id;
    args.space_id = ct_rsp->space_id;
    args.store_id = ct_rsp->store_id;
    uint32_t sts = ct_rsp->status;
    MetaMsgs::AdmMsgFactory::getInstance()->put(recv_msg);

    if ( sts ) {
        throw RcsdbFacade::Exception("Received error from srv ", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }

    if ( get_by_name == false ) {
        args.store_id = AddAndRegisterStore(args);
    } else {
        StoreProxy *st_p = ClusterProxy::StaticFindStoreProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name),
            ProxyMapKey(args.space_id, ClusterProxy::no_thread_id, *args.space_name),
            ProxyMapKey(args.store_id, args.group_idx, *args.store_name));
        if ( !st_p ) {
            if ( args.store_id != StoreProxy::invalid_id ) {
                args.store_id = AddAndRegisterStore(args);   // valid store_id found on the server
            } else {
                args.store_id = StoreProxy::invalid_id;  // no store_id found on the server
            }
        } else {
            st_p->IncRefCount();
        }
    }

    return args.store_id;
}

void StoreProxy::DestroyStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;

    if ( args.cluster_name && args.space_name && args.store_name ) {
        METADBCLI_DEBUG(cout << "StoreProxy::" << __FUNCTION__ << " cn: " << *args.cluster_name <<
                " sn: " << *args.space_name << " tn: " << *args.store_name << endl);
    }

    WriteLock w_lock(ClusterProxy::adm_op_lock);

    SessionMgr::StartMgr();

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " w: " << reinterpret_cast<uint64_t>(args.range_callback_arg) << SoeLogger::LogDebug() << endl;
    }

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock || tcp_sock->Created() == false ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgDeleteStoreReq *dt_req = MetaMsgs::AdmMsgDeleteStoreReq::poolT.get();
    dt_req->cluster_name = *args.cluster_name;
    dt_req->space_name = *args.space_name;
    dt_req->store_name = *args.store_name;
    dt_req->cluster_id = args.cluster_id;
    dt_req->space_id = args.space_id;
    dt_req->store_id = args.store_id;
    dt_req->cr_thread_id = args.group_idx;  // overloaded field

    ret = dt_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't send AdmMsgDeleteStoreReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgDeleteStoreReq::poolT.put(dt_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgDeleteStoreReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgDeleteStoreReq::poolT.put(dt_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
            }
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgDeleteStoreRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgDeleteStoreRsp *dt_rsp = dynamic_cast<MetaMsgs::AdmMsgDeleteStoreRsp *>(recv_msg);
    ret = dt_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgDeleteStoreRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgDeleteStoreRsp::poolT.put(dt_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgDeleteStoreRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgDeleteStoreRsp " <<
            " cluster_id: " << dt_rsp->cluster_id <<
            " space_id: " << dt_rsp->space_id <<
            " store_id: " << dt_rsp->store_id << " sts: " << dt_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
        }
    }
    args.cluster_id = dt_rsp->cluster_id;
    args.space_id = dt_rsp->space_id;
    args.store_id = dt_rsp->store_id;
    uint32_t sts = dt_rsp->status;
    MetaMsgs::AdmMsgDeleteStoreRsp::poolT.put(dt_rsp);

    if ( sts ) {
        throw RcsdbFacade::Exception("Received error from srv ", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }

    StoreProxy *st_p = ClusterProxy::StaticFindStoreProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name),
                ProxyMapKey(args.space_id, ClusterProxy::no_thread_id, *args.space_name),
                ProxyMapKey(args.store_id, args.group_idx, *args.store_name));

    if ( st_p ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " removing StoreProxy cl_id: " << args.cluster_id << " cl_name: " << *args.cluster_name <<
                " sp_id: " << args.space_id << " sp_name: " << *args.space_name <<
                " st_id: " << args.store_id << " th_id: " << args.group_idx << " st_name: " << *args.store_name << SoeLogger::LogDebug() << endl;
        }

        st_p->state = StoreProxy::State::eDeleted;
    }

    bool del_ret = ClusterProxy::StaticDeleteStoreProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name),
        ProxyMapKey(args.space_id, ClusterProxy::no_thread_id, *args.space_name),
        ProxyMapKey(args.store_id, args.group_idx, *args.store_name));
    if ( del_ret == false ) {
        args.space_id = SpaceProxy::invalid_id;
        args.store_id = StoreProxy::invalid_id;
    }

    if ( st_p ) {
        st_p->DeregisterApiFunctions();
    }
}

uint64_t StoreProxy::RepairStore(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;

    if ( args.cluster_name && args.space_name && args.store_name ) {
        METADBCLI_DEBUG(cout << "StoreProxy::" << __FUNCTION__ << " cn: " << *args.cluster_name <<
                " sn: " << *args.space_name << " tn: " << *args.store_name << endl);
    }

    WriteLock w_lock(ClusterProxy::adm_op_lock);

    SessionMgr::StartMgr();

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " w: " << reinterpret_cast<uint64_t>(args.range_callback_arg) << SoeLogger::LogDebug() << endl;
    }

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock || tcp_sock->Created() == false ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgRepairStoreReq *rt_req = MetaMsgs::AdmMsgRepairStoreReq::poolT.get();
    rt_req->cluster_name = *args.cluster_name;
    rt_req->space_name = *args.space_name;
    rt_req->store_name = *args.store_name;
    rt_req->cluster_id = args.cluster_id;
    rt_req->space_id = args.space_id;
    rt_req->store_id = args.store_id;
    rt_req->cr_thread_id = args.group_idx;  // overloaded field

    ret = rt_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't send AdmMsgRepairStoreReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgRepairStoreReq::poolT.put(rt_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgRepairStoreReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgRepairStoreReq::poolT.put(rt_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
            }
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgRepairStoreRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgRepairStoreRsp *rt_rsp = dynamic_cast<MetaMsgs::AdmMsgRepairStoreRsp *>(recv_msg);
    ret = rt_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgRepairStoreRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgRepairStoreRsp::poolT.put(rt_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgRepairStoreRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgRepairStoreRsp " <<
            " cluster_id: " << rt_rsp->cluster_id <<
            " space_id: " << rt_rsp->space_id <<
            " store_id: " << rt_rsp->store_id << " sts: " << rt_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
        }
    }
    args.cluster_id = rt_rsp->cluster_id;
    args.space_id = rt_rsp->space_id;
    args.store_id = rt_rsp->store_id;
    args.status = rt_rsp->status;
    MetaMsgs::AdmMsgRepairStoreRsp::poolT.put(rt_rsp);

    return args.store_id;
}

uint64_t StoreProxy::GetStoreByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    args.range_callback_arg = (char *)LD_GetByName;
    return StoreProxy::CreateStore(args);
}

int StoreProxy::PrepareSendIov(const LocalArgsDescriptor &args, IovMsgBase *iov)
{
    IovMsgReq<TestIov> *gr = dynamic_cast<IovMsgReq<TestIov> *>(iov);

    // Cluster/Space/Store elements
    gr->meta_desc.desc.cluster_id = cluster_id;
    if ( gr->SetIovSize(GetReq<TestIov>::cluster_name_idx, cluster.size()) ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " cluster_name too big" << SoeLogger::LogError() << endl;
        return -1;
    }
    ::memcpy(gr->meta_desc.desc.cluster_name, const_cast<char *>(cluster.data()), cluster.size());

    gr->meta_desc.desc.space_id = space_id;
    if ( gr->SetIovSize(GetReq<TestIov>::space_name_idx, space.size()) ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " space_name too big" << SoeLogger::LogError() << endl;
        return -1;
    }
    ::memcpy(gr->meta_desc.desc.space_name, const_cast<char *>(space.data()), space.size());

    gr->meta_desc.desc.store_id = id;
    if ( gr->SetIovSize(GetReq<TestIov>::store_name_idx, name.size()) ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " store_name too big" << SoeLogger::LogError() << endl;
        return -1;
    }
    ::memcpy(gr->meta_desc.desc.store_name, const_cast<char *>(name.data()), name.size());

    // Data elements
    gr->meta_desc.desc.key_len = static_cast<uint32_t>(args.key_len);
    if ( gr->SetIovSize(GetReq<TestIov>::key_idx, gr->meta_desc.desc.key_len) ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " key too big" << SoeLogger::LogError() << endl;
        return -1;
    }
    ::memcpy(gr->meta_desc.desc.key, const_cast<char *>(args.key) ? const_cast<char *>(args.key) : "", args.key_len);

    gr->meta_desc.desc.end_key_len = static_cast<uint32_t>(args.end_key_len);
    if ( gr->SetIovSize(GetReq<TestIov>::end_key_idx, gr->meta_desc.desc.end_key_len) ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " end_key too big" << SoeLogger::LogError() << endl;
        return -1;
    }
    ::memcpy(gr->meta_desc.desc.end_key, const_cast<char *>(args.end_key) ? const_cast<char *>(args.end_key) : "", args.end_key_len);

    gr->meta_desc.desc.data_len = static_cast<uint32_t>(args.data_len);
    if ( gr->SetIovSize(GetReq<TestIov>::data_idx, gr->meta_desc.desc.data_len) ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Data too big" << SoeLogger::LogError() << endl;
        return -1;
    }
    ::memcpy(gr->meta_desc.desc.data, const_cast<char *>(args.data) ? const_cast<char *>(args.data) : "", args.data_len);

    gr->meta_desc.desc.buf_len = static_cast<uint32_t>(args.buf_len);
    if ( gr->SetIovSize(GetReq<TestIov>::buf_idx, gr->meta_desc.desc.buf_len) ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " buf_len too big" << SoeLogger::LogError() << endl;
        return -1;
    }
    ::memcpy(gr->meta_desc.desc.buf, const_cast<char *>(args.buf) ? const_cast<char *>(args.buf) : "", args.buf_len);

    gr->HitchIds();

    StatusBits s_bits;
    s_bits.status_bits = 0;
    s_bits.bits.override_dups = static_cast<uint32_t>(args.overwrite_dups);
    switch ( args.iterator_idx ) {
    case Store_range_delete:
        s_bits.bits.range_type = STORE_RANGE_DELETE;
        break;
    case Subset_range_delete:
        s_bits.bits.range_type = SUBSET_RANGE_DELETE;
        break;
    case Store_range_get:
        s_bits.bits.range_type = STORE_RANGE_GET;
        break;
    case Subset_range_get:
        s_bits.bits.range_type = SUBSET_RANGE_GET;
        break;
    case Store_range_put:
        s_bits.bits.range_type = STORE_RANGE_PUT;
        break;
    case Subset_range_put:
        s_bits.bits.range_type = SUBSET_RANGE_PUT;
        break;
    case Store_range_merge:
        s_bits.bits.range_type = STORE_RANGE_MERGE;
        break;
    case Subset_range_merge:
        s_bits.bits.range_type = SUBSET_RANGE_MERGE;
        break;
    default:
        break; // no change
    }
    gr->HitchStatusBits(s_bits);

    return 0;
}

int StoreProxy::PrepareRecvIov(LocalArgsDescriptor &args, IovMsgBase *iov)
{
    IovMsgRsp<TestIov> *gp = dynamic_cast<IovMsgRsp<TestIov> *>(iov);

    // Cluster/Space/Store elements
    (void )gp->UnHitchIds();

    if ( !gp->meta_desc.desc.cluster_name || strcmp(gp->meta_desc.desc.cluster_name, cluster.c_str()) ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " invalid cluster_name: " <<
            gp->meta_desc.desc.cluster_name << SoeLogger::LogError() << endl;
        return -1;
    }
    args.cluster_name = &cluster;

    if ( gp->meta_desc.desc.cluster_id != cluster_id ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " invalid cluster_id: " <<
            gp->meta_desc.desc.cluster_id << SoeLogger::LogError() << endl;
        return -1;
    }
    args.cluster_id = gp->meta_desc.desc.cluster_id;

    if ( !gp->meta_desc.desc.space_name || strcmp(gp->meta_desc.desc.space_name, space.c_str()) ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " invalid space_name: " <<
            gp->meta_desc.desc.space_name << SoeLogger::LogError() << endl;
        return -1;
    }
    args.space_name = &space;

    if ( gp->meta_desc.desc.space_id != space_id ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " invalid space_id: " <<
            gp->meta_desc.desc.space_id << SoeLogger::LogError() << endl;
        return -1;
    }
    args.space_id = gp->meta_desc.desc.space_id;

    if ( !gp->meta_desc.desc.store_name || strcmp(gp->meta_desc.desc.store_name, name.c_str()) ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " invalid store_name: " <<
            gp->meta_desc.desc.store_name << SoeLogger::LogError() << endl;
        return -1;
    }
    args.store_name = &name;

    if ( gp->meta_desc.desc.store_id != id ) {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " invalid store_id: " <<
            gp->meta_desc.desc.store_id << SoeLogger::LogError() << endl;
        return -1;
    }
    args.store_id = gp->meta_desc.desc.store_id;

    // Data elements
    // key
    if ( gp->GetIovSize(GetReq<TestIov>::key_idx, gp->meta_desc.desc.key_len) ) {
        return -1;
    }
    args.key_len = static_cast<size_t>(gp->meta_desc.desc.key_len);
    args.key = gp->meta_desc.desc.key;

    // end_key
    if ( gp->GetIovSize(GetReq<TestIov>::end_key_idx, gp->meta_desc.desc.end_key_len) ) {
        return -1;
    }
    args.end_key_len = static_cast<size_t>(gp->meta_desc.desc.end_key_len);
    args.end_key = gp->meta_desc.desc.end_key;

    // data
    if ( gp->GetIovSize(GetReq<TestIov>::data_idx, gp->meta_desc.desc.data_len) ) {
        return -1;
    }
    args.data = gp->meta_desc.desc.data;
    args.data_len = static_cast<size_t>(gp->meta_desc.desc.data_len);

    // buf
    if ( gp->GetIovSize(GetReq<TestIov>::buf_idx, gp->meta_desc.desc.buf_len) ) {
        return -1;
    }
    args.buf = gp->meta_desc.desc.buf;
    args.buf_len = static_cast<size_t>(gp->meta_desc.desc.buf_len);

    return 0;
}

//
// Send and receive an i/o messages synchronously.
//
// Since we are sending and receiving synchronous messages on the same session, i.e. same connection,
// it ensures that the response is for the message sent, because we lock the send/receive section.
// Check if the response is a sync message, cause there might be async messages coming. If the received
// message is async handle it appropriately.
//
uint32_t StoreProxy::RunOp(LocalArgsDescriptor &args, eMsgType op_type)
{
    uint32_t op_sts = RcsdbFacade::Exception::eOk;

    if ( state == StoreProxy::State::eDeleted ) {
        return op_sts;
    }

    GetReq<TestIov> *gr = MetaMsgs::GetReq<TestIov>::poolT.get();

    if ( args.status == 1 ) { // debug ON
        ClusterProxy::msg_debug = true;
        SessionMgr::GetInstance()->McastSetDebug(true);
    } else if ( args.status == 2 ) {
        ClusterProxy::msg_debug = false;
        SessionMgr::GetInstance()->McastSetDebug(false);
    } else {
        ;
    }

    int ret = PrepareSendIov(args, gr);
    if ( ret ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " PrepareSendIov failed " << SoeLogger::LogError() << endl;
        }
        MetaMsgs::GetReq<TestIov>::poolT.put(gr);
        return RcsdbFacade::Exception::eInvalidArgument;
    }

    // Lock the entire iov send/recv section in case Soe session is being used concurently
    // by multiple threads
    // Same needs to be done for asynchronous API, although async msgs
    GetRsp<TestIov> *gp = MetaMsgs::GetRsp<TestIov>::poolT.get();
    uint32_t req_type;
    {
        WriteLock iov_lock(iov_send_recv_op_lock);

        gr->iovhdr.hdr.type = op_type;
        ret = io_session->event_point.SendIov(*gr);
        //cout << "> io req size: " << ret << " type: " << MsgTypeInfo::ToString(static_cast<eMsgType>(gr->iovhdr.hdr.type)) << endl;
        if ( ret < 0 ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " send failed type:" << gr->iovhdr.hdr.type << SoeLogger::LogError() << endl;
            }
            MetaMsgs::GetReq<TestIov>::poolT.put(gr);
            MetaMsgs::GetRsp<TestIov>::poolT.put(gp);
            return RcsdbFacade::Exception::eIOError;
        }

        req_type = gr->iovhdr.hdr.type;

        ret = io_session->event_point.RecvIov(*gp);
        //cout << "< io rsp size: " << ret << " type: " << MsgTypeInfo::ToString(static_cast<eMsgType>(gp->iovhdr.hdr.type)) << endl;
        if ( ret < 0 ) {
            if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
                if ( ClusterProxy::msg_debug == true ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected " << SoeLogger::LogError() << endl;
                }
            } else {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " recv failed type:" << gp->iovhdr.hdr.type << SoeLogger::LogError() << endl;
            }
            MetaMsgs::GetReq<TestIov>::poolT.put(gr);
            MetaMsgs::GetRsp<TestIov>::poolT.put(gp);
            return RcsdbFacade::Exception::eIOError;
        }
    }

    MetaMsgs::GetReq<TestIov>::poolT.put(gr);

    // this might be an async message received in the body of a sync message, handle it separately
    uint32_t rsp_type;
    if ( gp->isAsync() == true ) {
        LocalArgsDescriptor async_args;
        InitDesc(async_args);
        async_args.seq_number = gp->iovhdr.seq;

        ret = PrepareRecvIov(async_args, gp);
        if ( ret ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " PrepareRecvAsyncIov failed " << SoeLogger::LogDebug() << endl;
            }
            MetaMsgs::GetRsp<TestIov>::poolT.put(gp);
            return RcsdbFacade::Exception::eIncomplete;
        }

        rsp_type = gp->iovhdr.hdr.type;
        uint32_t it_valid;
        gp->GetCombinedStatus(op_sts, it_valid);
        async_args.transaction_db = static_cast<bool>(it_valid);

        FinalizeAsyncRsp(async_args, rsp_type);
    } else {
        ret = PrepareRecvIov(args, gp);
        if ( ret ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " PrepareRecvAsyncIov failed " << SoeLogger::LogDebug() << endl;
            }
            MetaMsgs::GetRsp<TestIov>::poolT.put(gp);
            return RcsdbFacade::Exception::eIncomplete;
        }

        rsp_type = gp->iovhdr.hdr.type;
        uint32_t it_valid;
        gp->GetCombinedStatus(op_sts, it_valid);
        args.transaction_db = static_cast<bool>(it_valid);
    }

    MetaMsgs::GetRsp<TestIov>::poolT.put(gp);

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got GetRsp<TestIov> sts: " << hex << gp->iovhdr.descr.status << dec <<
        " op_sts " << op_sts <<  " th_id: " << thread_id <<
        " req_type/rsp_type: " << req_type << "/" << rsp_type << " total_length: " << gp->iovhdr.hdr.total_length <<
        " cluster: " << gp->meta_desc.desc.cluster_name << " cluster_id: " << gp->meta_desc.desc.cluster_id <<
        " space: " << gp->meta_desc.desc.space_name << " space_id: " << gp->meta_desc.desc.space_id <<
        " store: " << gp->meta_desc.desc.store_name << " store_id: " << gp->meta_desc.desc.store_id << SoeLogger::LogDebug() << endl;
    }

    return op_sts;
}

void StoreProxy::CloseIOSession()
{
    // NOP
    if ( io_session ) {
        io_session->deinitialise(true);
        delete io_session;
        io_session = 0;
    }

    if ( io_session_async ) {
        io_session_async->deinitialise(true);
        delete io_session_async;
        io_session_async = 0;
    }
}

//
// Adm ops are handled via adm session
//
uint32_t StoreProxy::RunAdmOp(LocalAdmArgsDescriptor &args, eMsgType op_type)
{
    return 0;
}

void StoreProxy::IncRefCount()
{
    WriteLock w_lock(StoreProxy::ref_count_lock);
    ref_count++;
}
//
// Dynamic StoreProxy
//
void StoreProxy::Open(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( args.cluster_name && args.space_name && args.store_name ) {
        METADBCLI_DEBUG(cout << "StoreProxy::" << __FUNCTION__ << " cn: " << *args.cluster_name <<
                " sn: " << *args.space_name << " tn: " << *args.store_name << endl);
    }

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster << SoeLogger::LogDebug() << endl;
    }

    const char *client_end_key = args.end_key;
    size_t client_end_key_len = args.end_key_len;

    uint32_t thread_id_marker[2];
    thread_id_marker[0] = args.group_idx;
    thread_id_marker[1] = 0xa5a5a5a5;
    args.end_key = reinterpret_cast<char *>(thread_id_marker);
    args.end_key_len = sizeof(thread_id_marker); // both fields overloaded, now end_key_len is supposed to have client's thread_id in it

    uint32_t sts = RunOp(args, eMsgOpenStoreReq);

    args.end_key = client_end_key;
    args.end_key_len = client_end_key_len;

    if ( sts == RcsdbFacade::Exception::eOk ) {
        state = StoreProxy::State::eOpened;
    }

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::Close(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( args.cluster_name && args.space_name && args.store_name ) {
        METADBCLI_DEBUG(cout << "StoreProxy::" << __FUNCTION__ << " cn: " << *args.cluster_name <<
            " sn: " << *args.space_name << " tn: " << *args.store_name <<
            " p_cnt: " << put_op_counter << " g_cnt: " << get_op_counter << " d_cnt: " << delete_op_counter << " m_cnt: " << merge_op_counter <<
            " in_bts: " << in_bytes_counter << " out_bts: " << out_bytes_counter << endl);
    }

    bool still_keep = false;

    {
        WriteLock w_lock(StoreProxy::ref_count_lock);

        if ( ref_count == 1 ) {
            put_op_counter = 0;
            get_op_counter = 0;
            delete_op_counter = 0;
            merge_op_counter = 0;
            in_bytes_counter = 0;
            out_bytes_counter = 0;
        }
    }

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster << SoeLogger::LogDebug() << endl;
    }

    uint32_t sts = RunOp(args, eMsgCloseStoreReq);

    {
        WriteLock w_lock(StoreProxy::ref_count_lock);
        ref_count--;
        if ( ref_count ) {
            still_keep = true;
        }
    }

    if ( still_keep == false && sts == RcsdbFacade::Exception::eOk ) {
        state = StoreProxy::State::eClosed;
        io_session->state = AsyncIOSession::State::eDefunct;
        io_session_async->state = AsyncIOSession::State::eDefunct;
    }

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }

    if ( still_keep == false ) {
        StoreProxy::RemoveAndDeregisterStore(args);
    }
}

void StoreProxy::Repair(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster << SoeLogger::LogDebug() << endl;
    }

    const char *client_end_key = args.end_key;
    size_t client_end_key_len = args.end_key_len;

    uint32_t thread_id_marker[2];
    thread_id_marker[0] = args.group_idx;
    thread_id_marker[1] = 0xa5a5a5a5;
    args.end_key = reinterpret_cast<char *>(thread_id_marker);
    args.end_key_len = sizeof(thread_id_marker); // both fields overloaded, now end_key_len is supposed to have client's thread_id in it

    uint64_t st_id = StoreProxy::RepairStore(args);
    uint32_t sts = args.status;

    args.end_key = client_end_key;
    args.end_key_len = client_end_key_len;

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }

    if ( st_id == StoreProxy::invalid_id ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", RcsdbFacade::Exception::eInvalidArgument);
    }
}

void StoreProxy::Put(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster << SoeLogger::LogDebug() << endl;
    }

    put_op_counter++;
    out_bytes_counter += static_cast<uint64_t>(args.data_len);

    // Can't be both group and transaction Put
    if ( args.group_idx != invalid_resource_id && args.transaction_idx != invalid_resource_id ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " truncated buffer ", RcsdbFacade::Exception::eInvalidArgument);
    }

    // transaction_idx used only for sync msgs
    uint32_t idx_buf[4] = {0, 0};

    // Check if it's a group Put
    if ( args.group_idx != invalid_resource_id ) {
        idx_buf[0] = LD_group_type_idx;
        idx_buf[1] = args.group_idx;
    }

    // Check if it's a transaction Put
    if ( args.transaction_idx != invalid_resource_id ) {
        idx_buf[0] = LD_transaction_type_idx;
        idx_buf[1] = args.transaction_idx;
    }

    args.buf = reinterpret_cast<char *>(idx_buf);
    args.buf_len = sizeof(sizeof(idx_buf));

    uint32_t sts;
    if ( args.seq_number ) {
        sts = RunAsyncOp(args, eMsgPutAsyncReq);
    } else {
        sts = RunOp(args, eMsgPutReq);
    }

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::Get(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster << SoeLogger::LogDebug() << endl;
    }

    get_op_counter++;

    char *client_buf = args.buf;
    size_t client_buf_size = args.buf_len;
    args.buf_len = 0;

    uint32_t sts;
    if ( args.seq_number ) {
        sts = RunAsyncOp(args, eMsgGetAsyncReq);
    } else {
        sts = RunOp(args, eMsgGetReq);
    }

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        args.buf = client_buf;
        args.buf_len = client_buf_size;
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }

    if ( args.buf_len > client_buf_size ) {
        args.buf = client_buf;
        args.buf_len = client_buf_size;
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " truncated buffer ", RcsdbFacade::Exception::eTruncatedValue);
    }

    in_bytes_counter += static_cast<uint64_t>(args.buf_len);

    ::memcpy(client_buf, args.buf, args.buf_len);
    args.buf = client_buf;
}

void StoreProxy::Delete(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)

{
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster << SoeLogger::LogDebug() << endl;
    }

    delete_op_counter++;

    // Can't be both group and transaction Put
    if ( args.group_idx != invalid_resource_id && args.transaction_idx != invalid_resource_id ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " truncated buffer ", RcsdbFacade::Exception::eInvalidArgument);
    }

    uint32_t idx_buf[2] = {0, 0};

    // Check if it's a group Put
    if ( args.group_idx != invalid_resource_id ) {
        idx_buf[0] = LD_group_type_idx;
        idx_buf[1] = args.group_idx;
    }

    // Check if it's a transaction Put
    if ( args.transaction_idx != invalid_resource_id ) {
        idx_buf[0] = LD_transaction_type_idx;
        idx_buf[1] = args.transaction_idx;
    }

    args.buf = reinterpret_cast<char *>(idx_buf);
    args.buf_len = sizeof(sizeof(idx_buf));

    uint32_t sts;
    if ( args.seq_number ) {
        sts = RunAsyncOp(args, eMsgDeleteAsyncReq);
    } else {
        sts = RunOp(args, eMsgDeleteReq);
    }

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::GetRange(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster << SoeLogger::LogDebug() << endl;
    }

    get_op_counter++;

    uint32_t sts = RunOp(args, eMsgGetRangeReq);

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::Merge(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster << SoeLogger::LogDebug() << endl;
    }

    merge_op_counter++;
    out_bytes_counter += static_cast<uint64_t>(args.data_len);

    // Can't be both group and transaction Put
    if ( args.group_idx != invalid_resource_id && args.transaction_idx != invalid_resource_id ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " truncated buffer ", RcsdbFacade::Exception::eInvalidArgument);
    }

    uint32_t idx_buf[2] = {0, 0};

    // Check if it's a group Put
    if ( args.group_idx != invalid_resource_id ) {
        idx_buf[0] = LD_group_type_idx;
        idx_buf[1] = args.group_idx;
    }

    // Check if it's a transaction Put
    if ( args.transaction_idx != invalid_resource_id ) {
        idx_buf[0] = LD_transaction_type_idx;
        idx_buf[1] = args.transaction_idx;
    }

    args.buf = reinterpret_cast<char *>(idx_buf);
    args.buf_len = sizeof(sizeof(idx_buf));

    uint32_t sts;
    if ( args.seq_number ) {
        sts = RunAsyncOp(args, eMsgMergeAsyncReq);
    } else {
        sts = RunOp(args, eMsgMergeReq);
    }

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

//
//
// The extra information required to create and clean up resources (groups, transactions, iterators, etc)
// needs to be passed between clients and the srver:
//
// 1. SessionIterator
//
// CreateIterator:
//    args.key = start_key;
//    args.key_len = start_key_size;
//    args.end_key = end_key;
//    args.end_key_len = end_key_size;
//    args.overwrite_dups = (dir == Iterator::eIteratorDir::eIteratorDirForward ? true : false);
//    args.cluster_idx = Iterator::eType::eIteratorTypeContainer;
//
//    return args.iterator_idx = iterator_no;
//
// DestroyIterator:
//    args.iterator_idx
//
//    return args.iterator_idx = iterator_no;
//
// Iterator::First
//    args.key = key;
//    args.key_len = key_size;
//    args.sync_mode = LD_Iterator_First; // overloaded field
//
//    return Session::Iterate Exception
//
// Iterator::Next
//    args.sync_mode = LD_Iterator_Next; // overloaded field
//
//    return Session::Iterate Exception
//
// Iterator::Last
//    args.sync_mode = LD_Iterator_Last; // overloaded field
//
//    return Session::Iterate Exception
//
// Iterator::Valid
//    args.sync_mode = LD_Iterator_IsValid; // overloaded field
//    sess->Iterate(args);
//
//    return args.transaction_db; // overloaded field
//
// Iterator::Key
//    return Duple(args.key, args.key_len); // overloaded fields
//
// Iterator::Value
//    return Duple(args.buf, args.buf_len);
//
void StoreProxy::Iterate(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster <<
            " s_m " << args.sync_mode <<
            " key/key_len " << /*args.key*/"PH" << "/" << args.key_len <<
            " end_key_len " << args.end_key_len << SoeLogger::LogDebug() << endl;
    }

    get_op_counter += 3;

    uint64_t unbounded_key = LD_Unbounded_key;

    // In case key is empty we have to use special way of telling the server
    if ( args.sync_mode == LD_Iterator_First ) {
        if ( !args.key_len ) {
            args.key_len = sizeof(unbounded_key);
            args.key = reinterpret_cast<const char *>(&unbounded_key);
        }
    }

    // Iterator Id and Op
    uint32_t buf[2];
    buf[0] = args.iterator_idx;
    buf[1] = args.sync_mode;
    args.data = reinterpret_cast<const char *>(buf);
    args.data_len = sizeof(args.iterator_idx) + sizeof(args.sync_mode);

    uint32_t sts = RunOp(args, eMsgIterateReq);

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }

    // Iterator valid ?
    args.transaction_db = static_cast<bool>(*reinterpret_cast<const uint32_t *>(args.data + args.data_len - sizeof(uint32_t)));
    args.data_len -= sizeof(uint32_t);

    if ( true /*args.sync_mode == LD_Iterator_IsValid*/ ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts <<
            " tr_db " << args.transaction_db << " s_mo " << args.sync_mode <<
            " args.key " << /*args.key*/"PH" << " args.key_len " << args.key_len <<
            " args.end_key " << /*args.end_key*/"PH" << " args.end_key_len " << args.end_key_len <<
            " args.buf " << /*args.buf*/"PH" << " args.buf_len " << args.buf_len <<
            SoeLogger::LogDebug() << endl;
        }
    }

    // If "valid" was requested, restore client_buf and client_key and return
    if ( args.sync_mode == LD_Iterator_IsValid ) {
        return;
    }

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts <<
        " tr_db " << args.transaction_db << " s_mo " << args.sync_mode <<
        " args.key " << /*args.key*/"PH" << " args.key_len " << args.key_len <<
        " args.end_key " << /*args.end_key*/"PH" << " args.end_key_len " << args.end_key_len <<
        " args.buf " << /*args.buf*/"PH" << " args.buf_len " << args.buf_len <<
        SoeLogger::LogDebug() << endl;
    }

    in_bytes_counter += static_cast<uint64_t>(args.buf_len);
}

void StoreProxy::CreateGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster << SoeLogger::LogDebug() << endl;
    }

    uint32_t sts = RunOp(args, eMsgCreateGroupReq);

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    // Group Idx
    args.group_idx = *reinterpret_cast<const uint32_t *>(args.data + args.data_len - sizeof(uint32_t));
    args.data_len -= sizeof(uint32_t);

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::DestroyGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster <<
            " g_idx " << args.group_idx << SoeLogger::LogDebug() << endl;
    }

    // Group Idx
    uint32_t idx_buf[1];
    idx_buf[0] = args.group_idx;
    args.data = reinterpret_cast<const char *>(idx_buf);
    args.data_len = sizeof(idx_buf);

    uint32_t sts = RunOp(args, eMsgDeleteGroupReq);

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::WriteGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster <<
            " g_idx " << args.group_idx << SoeLogger::LogDebug() << endl;
    }

    // Group Idx
    uint32_t idx_buf[1];
    idx_buf[0] = args.group_idx;
    args.data = reinterpret_cast<const char *>(idx_buf);
    args.data_len = sizeof(idx_buf);

    uint32_t sts = RunOp(args, eMsgWriteGroupReq);

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::MakeArgsFromAdmArgs(const LocalAdmArgsDescriptor &args, LocalArgsDescriptor &std_args)
{
    std_args.cluster_name = args.cluster_name;
    std_args.cluster_id = args.cluster_id;
    std_args.cluster_idx = args.cluster_idx;

    std_args.space_name = args.space_name;
    std_args.space_id = args.space_id;
    std_args.space_idx = args.space_idx;

    std_args.store_name = args.store_name;
    std_args.store_id = args.store_id;
    std_args.store_idx = args.store_idx;

    std_args.sync_mode = args.sync_mode;

    std_args.transaction_idx = args.transaction_idx;
}

void StoreProxy::StartTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster << SoeLogger::LogDebug() << endl;
    }

    LocalArgsDescriptor std_args;
    ::memset(&std_args, '\0', sizeof(std_args));
    MakeArgsFromAdmArgs(args, std_args);

    uint32_t sts = RunOp(std_args, eMsgBeginTransactionReq);

    // Transaction Idx
    std_args.transaction_idx = *reinterpret_cast<const uint32_t *>(std_args.data + std_args.data_len - sizeof(std_args.transaction_idx));
    std_args.data_len -= sizeof(std_args.transaction_idx);
    args.transaction_idx = std_args.transaction_idx;

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::CommitTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster << SoeLogger::LogDebug() << endl;
    }

    LocalArgsDescriptor std_args;
    ::memset(&std_args, '\0', sizeof(std_args));
    MakeArgsFromAdmArgs(args, std_args);

    // Transaction Idx
    uint32_t idx_buf[1];
    idx_buf[0] = std_args.transaction_idx;
    std_args.data = reinterpret_cast<const char *>(idx_buf);
    std_args.data_len = sizeof(idx_buf);

    uint32_t sts = RunOp(std_args, eMsgCommitTransactionReq);

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::RollbackTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster << SoeLogger::LogDebug() << endl;
    }

    LocalArgsDescriptor std_args;
    ::memset(&std_args, '\0', sizeof(std_args));
    MakeArgsFromAdmArgs(args, std_args);

    // Transaction Idx
    uint32_t idx_buf[1];
    idx_buf[0] = std_args.transaction_idx;
    std_args.data = reinterpret_cast<const char *>(idx_buf);
    std_args.data_len = sizeof(idx_buf);

    uint32_t sts = RunOp(std_args, eMsgAbortTransactionReq);

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::CreateSnapshotEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    throw RcsdbFacade::Exception("Operation not supported", RcsdbFacade::Exception::eOpNotSupported);
}

void StoreProxy::CreateSnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    throw RcsdbFacade::Exception("Operation not supported", RcsdbFacade::Exception::eOpNotSupported);
}

void StoreProxy::DestroySnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    throw RcsdbFacade::Exception("Operation not supported", RcsdbFacade::Exception::eOpNotSupported);
}

void StoreProxy::ListSnapshots(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    throw RcsdbFacade::Exception("Operation not supported", RcsdbFacade::Exception::eOpNotSupported);
}

void StoreProxy::CreateBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster <<
            " backup_engine: " << args.backup_name <<
            " backup_pth: " << args.snapshot_name <<
            " incremental: " << args.backup_idx << SoeLogger::LogDebug() << endl;
    }

    LocalArgsDescriptor std_args;
    ::memset(&std_args, '\0', sizeof(std_args));
    MakeArgsFromAdmArgs(args, std_args);

    // backup path, incremental
    if ( args.snapshot_name ) {
        std_args.key = args.snapshot_name->c_str();
        std_args.key_len = args.snapshot_name->length();
    }
    std_args.sync_mode = args.backup_idx;

    uint32_t tmp_buf[2];
    tmp_buf[0] = std_args.sync_mode;
    tmp_buf[1] = 0x5f5f5f5f;
    std_args.data = reinterpret_cast<const char *>(tmp_buf);
    std_args.data_len = sizeof(tmp_buf);

    // backup name
    if ( args.backup_name ) {
        std_args.end_key = args.backup_name->c_str();
        std_args.end_key_len = args.backup_name->length();
    }

    uint32_t sts = RunOp(std_args, eMsgCreateBackupEngineReq);

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    // BackupEngine Idx, Id
    if ( std_args.data_len < sizeof(args.backup_idx) + sizeof(args.backup_id) ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", RcsdbFacade::Exception::eInvalidArgument);
    }

    args.backup_idx = *reinterpret_cast<const uint32_t *>(std_args.data);
    args.backup_id = *reinterpret_cast<const uint32_t *>(std_args.data + sizeof(args.backup_idx));

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::CreateBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;

    if ( args.cluster_name && args.space_name && args.store_name ) {
        METADBCLI_DEBUG(cout << "StoreProxy::" << __FUNCTION__ << " cn: " << *args.cluster_name <<
                " sn: " << *args.space_name << " tn: " << *args.store_name << endl);
    }

    WriteLock w_lock(ClusterProxy::adm_op_lock);

    SessionMgr::StartMgr();

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock || tcp_sock->Created() == false ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgCreateBackupReq *cb_req = MetaMsgs::AdmMsgCreateBackupReq::poolT.get();

    cb_req->cluster_name = *args.cluster_name;
    cb_req->space_name = *args.space_name;
    cb_req->store_name = *args.store_name;
    cb_req->cluster_id = args.cluster_id;
    cb_req->space_id = args.space_id;
    cb_req->store_id = args.store_id;
    cb_req->backup_engine_name = *args.backup_name;
    cb_req->backup_engine_idx = args.backup_idx;
    cb_req->backup_engine_id = args.backup_id;
    cb_req->sess_thread_id = args.sync_mode; // overloaded field

    ret = cb_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't send AdmMsgCreateBackupReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgCreateBackupReq::poolT.put(cb_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgCreateBackupReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgCreateBackupReq::poolT.put(cb_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
            }
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgCreateBackupRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgCreateBackupRsp *cbr_rsp = dynamic_cast<MetaMsgs::AdmMsgCreateBackupRsp *>(recv_msg);
    ret = cbr_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgCreateBackupRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgCreateBackupRsp::poolT.put(cbr_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgCreateBackupRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgCreateBackupRsp " <<
            " backup_id: " << cbr_rsp->backup_id <<
            " sts: " << cbr_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
        }
    }
    uint32_t sts = cbr_rsp->status;
    args.backup_id = cbr_rsp->backup_id;
    MetaMsgs::AdmMsgFactory::getInstance()->put(recv_msg);

    if ( sts ) {
        throw RcsdbFacade::Exception("Received error from srv ", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::RestoreBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;

    if ( args.cluster_name && args.space_name && args.store_name ) {
        METADBCLI_DEBUG(cout << "StoreProxy::" << __FUNCTION__ << " cn: " << *args.cluster_name <<
                " sn: " << *args.space_name << " tn: " << *args.store_name << endl);
    }

    WriteLock w_lock(ClusterProxy::adm_op_lock);

    SessionMgr::StartMgr();

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock || tcp_sock->Created() == false ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgRestoreBackupReq *rb_req = MetaMsgs::AdmMsgRestoreBackupReq::poolT.get();

    rb_req->cluster_name = *args.cluster_name;
    rb_req->space_name = *args.space_name;
    rb_req->store_name = *args.store_name;
    rb_req->cluster_id = args.cluster_id;
    rb_req->space_id = args.space_id;
    rb_req->store_id = args.store_id;
    rb_req->backup_name = *args.backup_name;
    rb_req->backup_engine_idx = args.backup_idx;
    rb_req->backup_id = args.backup_id;
    rb_req->sess_thread_id = args.sync_mode; // overloaded field

    if ( args.backups && args.backups->size() == 3 ) {
        rb_req->restore_cluster_name = (*args.backups)[2].first;
        rb_req->restore_space_name = (*args.backups)[1].first;
        rb_req->restore_store_name = (*args.backups)[0].first;
    }

    ret = rb_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't send AdmMsgRestoreBackupReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgRestoreBackupReq::poolT.put(rb_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgRestoreBackupReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgRestoreBackupReq::poolT.put(rb_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
            }
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgRestoreBackupRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgRestoreBackupRsp *rbr_rsp = dynamic_cast<MetaMsgs::AdmMsgRestoreBackupRsp *>(recv_msg);
    ret = rbr_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgRestoreBackupRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgRestoreBackupRsp::poolT.put(rbr_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgRestoreBackupRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgRestoreBackupRsp " <<
            " backup_id: " << rbr_rsp->backup_id <<
            " sts: " << rbr_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
        }
    }
    uint32_t sts = rbr_rsp->status;
    args.backup_id = rbr_rsp->backup_id;
    MetaMsgs::AdmMsgFactory::getInstance()->put(recv_msg);

    if ( sts ) {
        throw RcsdbFacade::Exception("Received error from srv ", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::DestroyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;

    if ( args.cluster_name && args.space_name && args.store_name ) {
        METADBCLI_DEBUG(cout << "StoreProxy::" << __FUNCTION__ << " cn: " << *args.cluster_name <<
                " sn: " << *args.space_name << " tn: " << *args.store_name << endl);
    }

    WriteLock w_lock(ClusterProxy::adm_op_lock);

    SessionMgr::StartMgr();

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock || tcp_sock->Created() == false ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgDeleteBackupReq *db_req = MetaMsgs::AdmMsgDeleteBackupReq::poolT.get();

    db_req->cluster_name = *args.cluster_name;
    db_req->space_name = *args.space_name;
    db_req->store_name = *args.store_name;
    db_req->cluster_id = args.cluster_id;
    db_req->space_id = args.space_id;
    db_req->store_id = args.store_id;
    db_req->backup_name = *args.backup_name; // overloaded, used for backup root dir
    db_req->backup_id = args.backup_id;
    db_req->sess_thread_id = args.sync_mode; // overloaded field

    ret = db_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't send AdmMsgDeleteBackupReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgDeleteBackupReq::poolT.put(db_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgDeleteBackupReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgDeleteBackupReq::poolT.put(db_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
            }
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgDeleteBackupRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgDeleteBackupRsp *dbr_rsp = dynamic_cast<MetaMsgs::AdmMsgDeleteBackupRsp *>(recv_msg);
    ret = dbr_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgDeleteBackupRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgDeleteBackupRsp::poolT.put(dbr_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgDeleteBackupRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgDeleteBackupRsp " <<
            " backup_id: " << dbr_rsp->backup_id <<
            " sts: " << dbr_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
        }
    }
    uint32_t sts = dbr_rsp->status;
    args.backup_id = dbr_rsp->backup_id;
    MetaMsgs::AdmMsgFactory::getInstance()->put(recv_msg);

    if ( sts ) {
        throw RcsdbFacade::Exception("Received error from srv ", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::VerifyBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;

    if ( args.cluster_name && args.space_name && args.store_name ) {
        METADBCLI_DEBUG(cout << "StoreProxy::" << __FUNCTION__ << " cn: " << *args.cluster_name <<
                " sn: " << *args.space_name << " tn: " << *args.store_name << endl);
    }

    WriteLock w_lock(ClusterProxy::adm_op_lock);

    SessionMgr::StartMgr();

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock || tcp_sock->Created() == false ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgVerifyBackupReq *vb_req = MetaMsgs::AdmMsgVerifyBackupReq::poolT.get();

    vb_req->cluster_name = *args.cluster_name;
    vb_req->space_name = *args.space_name;
    vb_req->store_name = *args.store_name;
    vb_req->cluster_id = args.cluster_id;
    vb_req->space_id = args.space_id;
    vb_req->store_id = args.store_id;
    vb_req->backup_name = *args.backup_name; // overloaded, used for backup root dir
    vb_req->backup_id = args.backup_id;
    vb_req->sess_thread_id = args.sync_mode; // overloaded field

    ret = vb_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't send AdmMsgVerifyBackupReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgVerifyBackupReq::poolT.put(vb_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgVerifyBackupReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgVerifyBackupReq::poolT.put(vb_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
            }
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgVerifyBackupRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgVerifyBackupRsp *vbr_rsp = dynamic_cast<MetaMsgs::AdmMsgVerifyBackupRsp *>(recv_msg);
    ret = vbr_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgVerifyBackupRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgVerifyBackupRsp::poolT.put(vbr_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgVerifyBackupRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgVerifyBackupRsp " <<
            " backup_id: " << vbr_rsp->backup_id <<
            " sts: " << vbr_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
        }
    }
    uint32_t sts = vbr_rsp->status;
    args.backup_id = vbr_rsp->backup_id;
    MetaMsgs::AdmMsgFactory::getInstance()->put(recv_msg);

    if ( sts ) {
        throw RcsdbFacade::Exception("Received error from srv ", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster <<
            " backup_engine: " << args.backup_name <<
            " backup_pth: " << args.snapshot_name <<
            " incremental: " << args.backup_idx << SoeLogger::LogDebug() << endl;
    }

    LocalArgsDescriptor std_args;
    ::memset(&std_args, '\0', sizeof(std_args));
    MakeArgsFromAdmArgs(args, std_args);

    // backup path, incremental
    if ( args.snapshot_name ) {
        std_args.key = args.snapshot_name->c_str();
        std_args.key_len = args.snapshot_name->length();
    }
    uint32_t id_buf[2];
    id_buf[0] = args.backup_idx;
    id_buf[1] = args.backup_id;
    std_args.data = reinterpret_cast<const char *>(id_buf);
    std_args.data_len = sizeof(id_buf);

    // backup name
    if ( args.backup_name ) {
        std_args.end_key = args.backup_name->c_str();
        std_args.end_key_len = args.backup_name->length();
    }

    uint32_t sts = RunOp(std_args, eMsgDeleteBackupEngineReq);

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    throw RcsdbFacade::Exception("Operation not supported", RcsdbFacade::Exception::eOpNotSupported);
}

void StoreProxy::CreateIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster <<
            " o_d/c_idx " << args.overwrite_dups << "/" << args.cluster_idx <<
            " key/key_len " << /*args.key*/"PH" << "/" << args.key_len <<
            " end_key/end_key_len " << /*args.end_key*/"PH" << "/" << args.end_key_len << SoeLogger::LogDebug() << endl;
    }

    uint64_t unbounded_key = LD_Unbounded_key;;

    // In case key/end_key are empty we have to use special way of telling the server
    if ( !args.key_len ) {
        args.key_len = sizeof(unbounded_key);
        args.key = reinterpret_cast<const char *>(&unbounded_key);
    }
    if ( !args.end_key_len ) {
        args.end_key_len = sizeof(unbounded_key);
        args.end_key = reinterpret_cast<const char *>(&unbounded_key);
    }

    // Iterator type and dir
    uint32_t buf[2];
    buf[0] = args.cluster_idx;
    buf[1] = args.overwrite_dups;
    args.buf = reinterpret_cast<char *>(buf);
    args.buf_len = 2*sizeof(uint32_t);

    uint32_t sts = RunOp(args, eMsgCreateIteratorReq);

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }

    // Iterator Idx
    args.iterator_idx = *reinterpret_cast<const uint32_t *>(args.data + args.data_len - sizeof(args.iterator_idx));
    args.data_len -= sizeof(args.iterator_idx);

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }
}

void StoreProxy::DestroyIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name << " id: " << id << " th_id: " << thread_id <<
            " space: " << space <<
            " cluster: " << cluster << SoeLogger::LogDebug() << endl;
    }

    if ( args.buf ) {
        args.buf = 0;
        args.buf_len = 0;
    }

    if ( args.key ) {
        args.key = 0;
        args.key_len = 0;
    }

    // Iterator Id
    args.data = reinterpret_cast<char *>(&args.iterator_idx);
    args.data_len = sizeof(args.iterator_idx);

    uint32_t sts = RunOp(args, eMsgDeleteIteratorReq);

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " RunOp() sts: " << sts << SoeLogger::LogDebug() << endl;
    }

    if ( sts != RcsdbFacade::Exception::eOk ) {
        throw RcsdbFacade::Exception(string(__FUNCTION__) + " failed", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }
}

void StoreProxy::RegisterApiFunctions() throw(SoeApi::Exception)
{
    // bind functions to boost functors
    boost::function<void(LocalArgsDescriptor &)> StoreOpen =
        boost::bind(&MetadbcliFacade::StoreProxy::Open, this, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreClose =
        boost::bind(&MetadbcliFacade::StoreProxy::Close, this, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreRepair =
        boost::bind(&MetadbcliFacade::StoreProxy::Repair, this, _1);

    // I/O
    boost::function<void(LocalArgsDescriptor &)> StorePut =
        boost::bind(&MetadbcliFacade::StoreProxy::Put, this, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreGet =
        boost::bind(&MetadbcliFacade::StoreProxy::Get, this, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreDelete =
        boost::bind(&MetadbcliFacade::StoreProxy::Delete, this, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreGetRange =
        boost::bind(&MetadbcliFacade::StoreProxy::GetRange, this, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreMerge =
        boost::bind(&MetadbcliFacade::StoreProxy::Merge, this, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreIterate =
        boost::bind(&MetadbcliFacade::StoreProxy::Iterate, this, _1);

    // Group I/Os
    boost::function<void(LocalArgsDescriptor &)> StoreCreateGroup =
        boost::bind(&MetadbcliFacade::StoreProxy::CreateGroup, this, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreDestroyGroup =
        boost::bind(&MetadbcliFacade::StoreProxy::DestroyGroup, this, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreWriteGroup =
        boost::bind(&MetadbcliFacade::StoreProxy::WriteGroup, this, _1);

    // Transaction
    boost::function<void(LocalAdmArgsDescriptor &)> StartTransaction =
        boost::bind(&MetadbcliFacade::StoreProxy::StartTransaction, this, _1);

    boost::function<void(LocalAdmArgsDescriptor &)> CommitTransaction =
        boost::bind(&MetadbcliFacade::StoreProxy::CommitTransaction, this, _1);

    boost::function<void(LocalAdmArgsDescriptor &)> RollbackTransaction =
        boost::bind(&MetadbcliFacade::StoreProxy::RollbackTransaction, this, _1);

    // Snapshot
    boost::function<void(LocalAdmArgsDescriptor &)> CreateSnapshotEngine =
        boost::bind(&MetadbcliFacade::StoreProxy::CreateSnapshotEngine, this, _1);

    boost::function<void(LocalAdmArgsDescriptor &)> CreateSnapshot =
        boost::bind(&MetadbcliFacade::StoreProxy::CreateSnapshot, this, _1);

    boost::function<void(LocalAdmArgsDescriptor &)> DestroySnapshot =
        boost::bind(&MetadbcliFacade::StoreProxy::DestroySnapshot, this, _1);

    boost::function<void(LocalAdmArgsDescriptor &)> ListSnapshots =
        boost::bind(&MetadbcliFacade::StoreProxy::ListSnapshots, this, _1);

    // Backups
    boost::function<void(LocalAdmArgsDescriptor &)> CreateBackupEngine =
        boost::bind(&MetadbcliFacade::StoreProxy::CreateBackupEngine, this, _1);

    boost::function<void(LocalAdmArgsDescriptor &)> CreateBackup =
        boost::bind(&MetadbcliFacade::StoreProxy::CreateBackup, this, _1);

    boost::function<void(LocalAdmArgsDescriptor &)> RestoreBackup =
        boost::bind(&MetadbcliFacade::StoreProxy::RestoreBackup, this, _1);

    boost::function<void(LocalAdmArgsDescriptor &)> DestroyBackup =
        boost::bind(&MetadbcliFacade::StoreProxy::DestroyBackup, this, _1);

    boost::function<void(LocalAdmArgsDescriptor &)> VerifyBackup =
        boost::bind(&MetadbcliFacade::StoreProxy::VerifyBackup, this, _1);

    boost::function<void(LocalAdmArgsDescriptor &)> DestroyBackupEngine =
        boost::bind(&MetadbcliFacade::StoreProxy::DestroyBackupEngine, this, _1);

    boost::function<void(LocalAdmArgsDescriptor &)> ListBackups =
        boost::bind(&MetadbcliFacade::StoreProxy::ListBackups, this, _1);

    // Iterators
    boost::function<void(LocalArgsDescriptor &)> CreateIterator =
        boost::bind(&MetadbcliFacade::StoreProxy::CreateIterator, this, _1);

    boost::function<void(LocalArgsDescriptor &)> DestroyIterator =
        boost::bind(&MetadbcliFacade::StoreProxy::DestroyIterator, this, _1);

    // register via SoeApi


    // Open/Close
    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreOpen, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_OPEN), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreRepair, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_REPAIR), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
         StoreClose, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_CLOSE), ClusterProxy::global_debug));

    // I/O
    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StorePut, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_PUT), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreGet, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_GET), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreDelete, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_DELETE), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreGetRange, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_GET_RANGE), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreMerge, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_MERGE), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
            StoreIterate, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_ITERATE), ClusterProxy::global_debug));

    // Group I/Os
    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreCreateGroup, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_CREATE_GROUP), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreDestroyGroup, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_DESTROY_GROUP), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreWriteGroup, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_WRITE_GROUP), ClusterProxy::global_debug));

    // Transactions
    ftors.Register(new SoeApi::Api<void(LocalAdmArgsDescriptor &)>(
        StartTransaction, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_START_TRANSACTION), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalAdmArgsDescriptor &)>(
        CommitTransaction, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_COMMIT_TRANSACTION), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalAdmArgsDescriptor &)>(
        RollbackTransaction, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_ROLLBACK_TRANSACTION), ClusterProxy::global_debug));

    // Snapshots
    ftors.Register(new SoeApi::Api<void(LocalAdmArgsDescriptor &)>(
        CreateSnapshotEngine, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_CREATE_SNAPSHOT_ENGINE), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalAdmArgsDescriptor &)>(
        CreateSnapshot, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_CREATE_SNAPSHOT), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalAdmArgsDescriptor &)>(
        DestroySnapshot, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_DESTROY_SNAPSHOT), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalAdmArgsDescriptor &)>(
        ListSnapshots, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_LIST_SNAPSHOTS), ClusterProxy::global_debug));

    // Backups
    ftors.Register(new SoeApi::Api<void(LocalAdmArgsDescriptor &)>(
        CreateBackupEngine, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_CREATE_BACKUP_ENGINE), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalAdmArgsDescriptor &)>(
        CreateBackup, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_CREATE_BACKUP), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalAdmArgsDescriptor &)>(
        RestoreBackup, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_RESTORE_BACKUP), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalAdmArgsDescriptor &)>(
        DestroyBackup, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_DESTROY_BACKUP), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalAdmArgsDescriptor &)>(
        VerifyBackup, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_VERIFY_BACKUP), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalAdmArgsDescriptor &)>(
        DestroyBackupEngine, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_DESTROY_BACKUP_ENGINE), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalAdmArgsDescriptor &)>(
        ListBackups, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_LIST_BACKUPS), ClusterProxy::global_debug));

    // Iterators
    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        CreateIterator, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_CREATE_ITERATOR), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        DestroyIterator, MetadbcliFacade::CreateLocalName(cluster, space, name, thread_id, FUNCTION_DESTROY_ITERATOR), ClusterProxy::global_debug));
}

void StoreProxy::DeregisterApiFunctions() throw(SoeApi::Exception)
{
    ftors.DeregisterAll();
}

void StoreProxy::RegisterStaticApiFunctions() throw(SoeApi::Exception)
{
    // bind functions to boost functors
    boost::function<uint64_t(LocalArgsDescriptor &)> StoreCreate =
        boost::bind(&MetadbcliFacade::StoreProxy::CreateStore, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreDestroy =
        boost::bind(&MetadbcliFacade::StoreProxy::DestroyStore, _1);

    boost::function<uint64_t(LocalArgsDescriptor &)> StoreGetByName =
        boost::bind(&MetadbcliFacade::StoreProxy::GetStoreByName, _1);


    // register via SoeApi
    static_ftors.Register(new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
        StoreCreate, MetadbcliFacade::CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE), ClusterProxy::global_debug));

    static_ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreDestroy, MetadbcliFacade::CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY), ClusterProxy::global_debug));

    static_ftors.Register(new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
        StoreGetByName, MetadbcliFacade::CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME), ClusterProxy::global_debug));
}

void StoreProxy::DeregisterStaticApiFunctions() throw(SoeApi::Exception)
{

}




uint64_t SpaceProxy::AddAndRegisterSpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    SpaceProxy *sp_new = new SpaceProxy(*args.cluster_name, *args.space_name, args.space_id, ClusterProxy::msg_debug);
    bool ins_ret = ClusterProxy::StaticAddSpaceProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name), *sp_new);
    if ( ins_ret == false ) {
        delete sp_new;
        args.space_id = SpaceProxy::invalid_id;
    } else {
        // Might be duplicates so try/catch and ignore
        try {
            sp_new->RegisterStaticApiFunctions();
        } catch (const SoeApi::Exception &ex) {
            if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got SoeApi exception(RegisterStatic) ignored ... " <<
               " what: " << ex.what() << SoeLogger::LogError() << endl;
        }

        // Exception on this is an error
        try {
            sp_new->RegisterApiFunctions();
        } catch (const SoeApi::Exception &ex) {
            if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got SoeApi exception" <<
               " what: " << ex.what() << SoeLogger::LogError() << endl;
            args.space_id = SpaceProxy::invalid_id;
        }
    }

    return args.space_id;
}

//
// Static SpaceProxy
//
SpaceProxy  *SpaceProxy::reg_api_instance = 0;
RcsdbFacade::FunctorBase   SpaceProxy::static_ftors;
Lock SpaceProxy::adm_op_lock;

uint64_t SpaceProxy::CreateSpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;
    bool get_by_name = false;

    WriteLock w_lock(ClusterProxy::adm_op_lock);

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgCreateSpaceReq *cs_req = MetaMsgs::AdmMsgCreateSpaceReq::poolT.get();
    if ( args.status == 1 ) { // debug ON
        ClusterProxy::msg_debug = true;
        cs_req->json = string("{ \"debug\": true}");
        SessionMgr::GetInstance()->McastSetDebug(true);
    } else if ( args.status == 2 ) {
        ClusterProxy::msg_debug = false;
        cs_req->json = string("{ \"debug\": false}");
        SessionMgr::GetInstance()->McastSetDebug(false);
    } else {
        ;
    }

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " w: " << reinterpret_cast<uint64_t>(args.range_callback_arg) << SoeLogger::LogDebug() << endl;
    }

    cs_req->cluster_name = *args.cluster_name;
    cs_req->space_name = *args.space_name;
    cs_req->cluster_id = args.cluster_id;
    cs_req->only_get_by_name = (args.range_callback_arg == (char *)LD_GetByName ? 1 : 0);
    get_by_name = cs_req->only_get_by_name;
    args.range_callback_arg = 0;

    ret = cs_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgCreateClusterReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgCreateSpaceReq::poolT.put(cs_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgCreateSpaceReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgCreateSpaceReq::poolT.put(cs_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgCreateSpaceRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgCreateSpaceRsp *cs_rsp = dynamic_cast<MetaMsgs::AdmMsgCreateSpaceRsp *>(recv_msg);
    ret = cs_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgCreateSpaceRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgCreateSpaceRsp::poolT.put(cs_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgCreateSpaceRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgCreateSpaceRsp " <<
            " cluster_id: " << cs_rsp->cluster_id <<
            " space_id: " << cs_rsp->space_id << " sts: " << cs_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
    }
    args.space_id = cs_rsp->space_id;
    uint32_t sts = cs_rsp->status;
    MetaMsgs::AdmMsgCreateSpaceRsp::poolT.put(cs_rsp);

    if ( sts ) {
        throw RcsdbFacade::Exception("Received error from srv ", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }

    if ( get_by_name == false ) {
        args.space_id = AddAndRegisterSpace(args);
    } else {
        SpaceProxy *sp_f = ClusterProxy::StaticFindSpaceProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name),
            ProxyMapKey(args.space_id, ClusterProxy::no_thread_id, *args.space_name));
        if ( !sp_f ) {
            if ( args.space_id != SpaceProxy::invalid_id ) { // valid space_id received
                args.space_id = AddAndRegisterSpace(args);
            } else {
                args.space_id = SpaceProxy::invalid_id; // no space_id found
            }
        }
    }

    return args.space_id;
}

void SpaceProxy::DestroySpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;

    WriteLock w_lock(ClusterProxy::adm_op_lock);

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " w: " << reinterpret_cast<uint64_t>(args.range_callback_arg) << SoeLogger::LogDebug() << endl;
    }

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgDeleteSpaceReq *ds_req = MetaMsgs::AdmMsgDeleteSpaceReq::poolT.get();
    ds_req->cluster_name = *args.cluster_name;
    ds_req->space_name = *args.space_name;
    ds_req->cluster_id = args.cluster_id;
    ds_req->space_id = args.space_id;

    ret = ds_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgDeleteSpaceReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgDeleteSpaceReq::poolT.put(ds_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgDeleteSpaceReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgDeleteSpaceReq::poolT.put(ds_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgDeleteSpaceRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgDeleteSpaceRsp *ds_rsp = dynamic_cast<MetaMsgs::AdmMsgDeleteSpaceRsp *>(recv_msg);
    ret = ds_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgDeleteSpaceRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgDeleteSpaceRsp::poolT.put(ds_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgDeleteSpaceRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgDeleteSpaceRsp " <<
            " cluster_id: " << ds_rsp->cluster_id <<
            " space_id: " << ds_rsp->space_id << " sts: " << ds_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
        }
    }
    args.cluster_id = ds_rsp->cluster_id;
    args.space_id = ds_rsp->space_id;
    uint32_t sts = ds_rsp->status;
    MetaMsgs::AdmMsgDeleteSpaceRsp::poolT.put(ds_rsp);

    if ( sts ) {
        throw RcsdbFacade::Exception("Received error from srv ", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }

    bool del_ret = ClusterProxy::StaticDeleteSpaceProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name),
        ProxyMapKey(args.space_id, ClusterProxy::no_thread_id, *args.space_name));
    if ( del_ret == false ) {
        args.space_id = SpaceProxy::invalid_id;
    }
}

uint64_t SpaceProxy::GetSpaceByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    args.range_callback_arg = (char *)LD_GetByName;  // LD_GetByName overloaded field
    return SpaceProxy::CreateSpace(args);
}

void SpaceProxy::RegisterStaticApiFunctions() throw(SoeApi::Exception)
{
    // bind functions to boost functors
    boost::function<uint64_t(LocalArgsDescriptor &)> SpaceCreate =
        boost::bind(&MetadbcliFacade::SpaceProxy::CreateSpace, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceDestroy =
        boost::bind(&MetadbcliFacade::SpaceProxy::DestroySpace, _1);

    boost::function<uint64_t(LocalArgsDescriptor &)> SpaceGetByName =
        boost::bind(&MetadbcliFacade::SpaceProxy::GetSpaceByName, _1);


    // register via SoeApi
    static_ftors.Register(new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
        SpaceCreate, MetadbcliFacade::CreateLocalName(STATIC_CLUSTER, "", "", 0, FUNCTION_CREATE), ClusterProxy::global_debug));

    static_ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceDestroy, MetadbcliFacade::CreateLocalName(STATIC_CLUSTER, "", "", 0, FUNCTION_DESTROY), ClusterProxy::global_debug));

    static_ftors.Register(new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
        SpaceGetByName, MetadbcliFacade::CreateLocalName(STATIC_CLUSTER, "", "", 0, FUNCTION_GET_BY_NAME), ClusterProxy::global_debug));
}

void SpaceProxy::DeregisterStaticApiFunctions() throw(SoeApi::Exception)
{
    static_ftors.DeregisterAll();
}

void SpaceProxy::RegisterApiFunctions()  throw(SoeApi::Exception)
{
    // bind functions to boost functors
    boost::function<void(LocalArgsDescriptor &)> SpaceLocalOpen =
        boost::bind(&MetadbcliFacade::SpaceProxy::Open, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceLocalClose =
        boost::bind(&MetadbcliFacade::SpaceProxy::Close, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceLocalPut =
        boost::bind(&MetadbcliFacade::SpaceProxy::Put, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceLocalGet =
        boost::bind(&MetadbcliFacade::SpaceProxy::Get, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceLocalDel =
        boost::bind(&MetadbcliFacade::SpaceProxy::Delete, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceLocalGetRange =
        boost::bind(&MetadbcliFacade::SpaceProxy::GetRange, this, _1);


    // register via SoeApi
    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceLocalOpen, MetadbcliFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_OPEN), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceLocalClose, MetadbcliFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_CLOSE), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceLocalPut, MetadbcliFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_PUT), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceLocalGet, MetadbcliFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_GET), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceLocalDel, MetadbcliFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_DELETE), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceLocalGetRange, MetadbcliFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_GET_RANGE), ClusterProxy::global_debug));
}

void SpaceProxy::DeregisterApiFunctions()  throw(SoeApi::Exception)
{
    ftors.DeregisterAll();
}

bool SpaceProxy::AddStoreProxy(StoreProxy &st)
{
    WriteLock w_lock(SpaceProxy::map_lock);

    ProxyMapKey map_key(st.id, st.thread_id, st.name);
    pair<map<ProxyMapKey, StoreProxy &>::iterator, bool> ret;
    ret = stores.insert(pair<ProxyMapKey, StoreProxy &>(map_key, st));
    if ( ret.second == false ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Store already exists id: " <<
            st.id << " th_id: " << st.thread_id << " name: " << st.name << SoeLogger::LogError() << endl;
        }
        return false;
    }

    return true;
}

bool SpaceProxy::DeleteStoreProxy(const ProxyMapKey &key)
{
    WriteLock w_lock(SpaceProxy::map_lock);

    map<ProxyMapKey, StoreProxy &>::iterator it;
    it = stores.find(key);
    if ( it == stores.end() ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Store not found id: " <<
            key.i_val << " th_id: " << key.t_tid << " name: " << key.s_val << SoeLogger::LogError() << endl;
        }
        return false;
    }

    SpaceProxy::stores.erase(it);
    return true;
}

StoreProxy *SpaceProxy::FindStoreProxy(const ProxyMapKey &key)
{
    WriteLock w_lock(SpaceProxy::map_lock);

    map<ProxyMapKey, StoreProxy &>::iterator it;
    it = stores.find(key);
    if ( it == stores.end() ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Store not found id: " <<
            key.i_val << " th_id: " << key.t_tid << " name: " << key.s_val << SoeLogger::LogError() << endl;
        }
        return 0;
    }

    return &(*it).second;
}




ClusterProxy::Initializer *static_initializer_ptr = 0;

bool ClusterProxy::Initializer::initialized = false;
Lock ClusterProxy::global_lock;
ClusterProxy::Initializer initalized;
ClusterProxy  *ClusterProxy::reg_api_instance = 0;
Lock ClusterProxy::adm_op_lock;

void StaticInitializerEnter(void)
{
    static_initializer_ptr = &initalized;
}

void StaticInitializerExit(void)
{
    static_initializer_ptr = 0;
}

ClusterProxy::Initializer::Initializer()
{
    WriteLock w_lock(ClusterProxy::global_lock);

    if ( Initializer::initialized == false ) {
        try {
            ClusterProxy::RegisterStaticApiFunctions();
            ClusterProxy::reg_api_instance = new ClusterProxy("static_cluster_name",
                ClusterProxy::invalid_id, false);
            ClusterProxy::reg_api_instance->RegisterApiFunctions();

            SpaceProxy::RegisterStaticApiFunctions();
            SpaceProxy::reg_api_instance = new SpaceProxy("static_cluster_name", "static_space_name",
                SpaceProxy::invalid_id, false);
            SpaceProxy::reg_api_instance->parent = ClusterProxy::reg_api_instance;
            SpaceProxy::reg_api_instance->RegisterApiFunctions();

            StoreProxy::RegisterStaticApiFunctions();
            StoreProxy::reg_api_instance = new StoreProxy("static_cluster_name", "static_space_name", "static_store_name",
                    ClusterProxy::invalid_id, SpaceProxy::invalid_id, StoreProxy::invalid_id, false);
            StoreProxy::reg_api_instance->parent = SpaceProxy::reg_api_instance;
            StoreProxy::reg_api_instance->RegisterApiFunctions();
        } catch (const SoeApi::Exception &ex) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Registerstatic functions failed: " << SoeLogger::LogError() << endl;
        }
    }
    Initializer::initialized = true;
}

ClusterProxy::Initializer::~Initializer()
{
    WriteLock w_lock(ClusterProxy::global_lock);

    if ( Initializer::initialized == true ) {
        if ( ClusterProxy::global_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
        }

        if ( ClusterProxy::reg_api_instance ) {
            delete ClusterProxy::reg_api_instance;
        }

        if ( SpaceProxy::reg_api_instance ) {
            delete SpaceProxy::reg_api_instance;
        }

        if ( StoreProxy::reg_api_instance ) {
            delete StoreProxy::reg_api_instance;
        }
    }
    Initializer::initialized = false;
}

bool ClusterProxy::global_debug = false;
bool ClusterProxy::msg_debug = false;
uint64_t ClusterProxy::adm_msg_counter = 0;
RcsdbFacade::FunctorBase   ClusterProxy::static_ftors;

map<ProxyMapKey, ClusterProxy &> ClusterProxy::clusters;
Lock ClusterProxy::clusters_map_lock;

uint64_t ClusterProxy::AddAndRegisterCluster(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    ClusterProxy *cl_new = new ClusterProxy(*args.cluster_name, args.cluster_id, ClusterProxy::msg_debug);
    bool ins_ret = ClusterProxy::StaticAddClusterProxy(*cl_new);
    if ( ins_ret == false ) {
        delete cl_new;
        args.cluster_id = ClusterProxy::invalid_id;
    } else {
        // Might be duplicates so try/catch and ignore
        try {
            cl_new->RegisterStaticApiFunctions();
        } catch (const SoeApi::Exception &ex) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got SoeApi exception(RegisterStatic) ignored .... " <<
               " what: " << ex.what() << SoeLogger::LogDebug() << endl;
            }
        }

        // Exception on this is an error
        try {
            cl_new->RegisterApiFunctions();
        } catch (const SoeApi::Exception &ex) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got SoeApi exception" <<
               " what: " << ex.what() << SoeLogger::LogError() << endl;
            }
            args.cluster_id = ClusterProxy::invalid_id;
        }
    }

    return args.cluster_id;
}


//
// Static ClusterProxy
//
uint64_t ClusterProxy::CreateCluster(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;
    bool get_by_name = false;

    WriteLock w_lock(ClusterProxy::adm_op_lock);

    SessionMgr::StartMgr();

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgCreateClusterReq *cc_req = MetaMsgs::AdmMsgCreateClusterReq::poolT.get();

    if ( args.status == 1 ) { // debug ON
        ClusterProxy::msg_debug = true;
        cc_req->json = string("{ \"debug\": true}");
        SessionMgr::GetInstance()->McastSetDebug(true);
    } else if ( args.status == 2 ){
        ClusterProxy::msg_debug = false;
        cc_req->json = string("{ \"debug\": false}");
        SessionMgr::GetInstance()->McastSetDebug(false);
    } else {
        ;
    }

    // print overloaded field range_callback_arg if it's get_by_name() or create()
    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " w: " << reinterpret_cast<uint64_t>(args.range_callback_arg) << SoeLogger::LogDebug() << endl;
    }

    cc_req->cluster_name = *args.cluster_name;
    cc_req->only_get_by_name = (args.range_callback_arg == (char *)LD_GetByName ? 1 : 0);
    get_by_name = cc_req->only_get_by_name;
    args.range_callback_arg = 0;

    ret = cc_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgCreateClusterReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgCreateClusterReq::poolT.put(cc_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgCreateClusterReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgCreateClusterReq::poolT.put(cc_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
            }
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgCreateClusterRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgCreateClusterRsp *cc_rsp = dynamic_cast<MetaMsgs::AdmMsgCreateClusterRsp *>(recv_msg);
    ret = cc_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgCreateClusterRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgCreateClusterRsp::poolT.put(cc_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgCreateClusterRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgCreateClusterRsp " <<
            " cluster_id: " << cc_rsp->cluster_id << " sts: " << cc_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
    }
    args.cluster_id = cc_rsp->cluster_id;
    uint32_t sts = cc_rsp->status;
    MetaMsgs::AdmMsgCreateClusterRsp::poolT.put(cc_rsp);

    if ( sts ) {
        throw RcsdbFacade::Exception("Received error from srv ", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }

    if ( get_by_name == false ) {
        args.cluster_id = AddAndRegisterCluster(args);
    } else {
        ClusterProxy *cl_p = ClusterProxy::StaticFindClusterProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name));
        if ( !cl_p ) {
            // Got valid cluster id
            if ( args.cluster_id != ClusterProxy::invalid_id ) { // valid cluster_id
                args.cluster_id = AddAndRegisterCluster(args);
            } else {
                args.cluster_id = ClusterProxy::invalid_id; // no cluster found
            }
        }
    }

    return args.cluster_id;
}

void ClusterProxy::DestroyCluster(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;

    WriteLock w_lock(ClusterProxy::adm_op_lock);

    SessionMgr::StartMgr();

    if ( args.overwrite_dups == true ) {
        ClusterProxy::msg_debug = true;
    }

    if ( ClusterProxy::msg_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " w: " << reinterpret_cast<uint64_t>(args.range_callback_arg) << SoeLogger::LogDebug() << endl;
    }

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgDeleteClusterReq *dc_req = MetaMsgs::AdmMsgDeleteClusterReq::poolT.get();
    dc_req->cluster_name = *args.cluster_name;
    dc_req->cluster_id = args.cluster_id;

    ret = dc_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgDeleteClusterReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgDeleteClusterReq::poolT.put(dc_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgDeleteClusterReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgDeleteClusterReq::poolT.put(dc_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
            }
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgDeleteClusterRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgDeleteClusterRsp *dc_rsp = dynamic_cast<MetaMsgs::AdmMsgDeleteClusterRsp *>(recv_msg);
    ret = dc_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgCreateClusterRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgDeleteClusterRsp::poolT.put(dc_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgDeleteClusterRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgDeleteClusterRsp " <<
            " cluster_id: " << dc_rsp->cluster_id << " sts: " << dc_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
        }
    }
    args.cluster_id = dc_rsp->cluster_id;
    uint32_t sts = dc_rsp->status;
    MetaMsgs::AdmMsgDeleteClusterRsp::poolT.put(dc_rsp);

    if ( sts ) {
        throw RcsdbFacade::Exception("Received error from srv ", static_cast<RcsdbFacade::Exception::eStatus>(sts));
    }

    bool del_ret = ClusterProxy::StaticDeleteClusterProxy(ProxyMapKey(args.cluster_id, ClusterProxy::no_thread_id, *args.cluster_name));
    if ( del_ret == false ) {
        args.cluster_id = ClusterProxy::invalid_id;
    }
}

uint64_t ClusterProxy::GetClusterByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    args.range_callback_arg = reinterpret_cast<char *>(LD_GetByName);
    return ClusterProxy::CreateCluster(args);
}

void ClusterProxy::SwapNameList(std::vector<std::pair<uint64_t, std::string> > &from_list, std::vector<std::pair<uint64_t, std::string> > &to_list)
{
    to_list.swap(from_list);
}

void ClusterProxy::ListSubsets(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;

    if ( !args.list ) {
        throw RcsdbFacade::Exception("NULL list passed", RcsdbFacade::Exception::eInvalidArgument);
    }

    SessionMgr::StartMgr();

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgListSubsetsReq *lu_req = MetaMsgs::AdmMsgListSubsetsReq::poolT.get();

    if ( args.status == 1 ) { // debug ON
        ClusterProxy::msg_debug = true;
        lu_req->json = string("{ \"debug\": true}");
        SessionMgr::GetInstance()->McastSetDebug(true);
    } else if ( args.status == 2 ){
        ClusterProxy::msg_debug = false;
        lu_req->json = string("{ \"debug\": false}");
        SessionMgr::GetInstance()->McastSetDebug(false);
    } else {
        ;
    }

    lu_req->cluster_name = *args.cluster_name;
    lu_req->cluster_id = args.cluster_id;
    lu_req->space_name = *args.space_name;
    lu_req->space_id = args.space_id;
    lu_req->store_name = *args.store_name;
    lu_req->store_id = args.store_id;

    ret = lu_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgListSubsetsReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgListSubsetsReq::poolT.put(lu_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgListSubsetsReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgListSubsetsReq::poolT.put(lu_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
            }
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgListSubsetsRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgListSubsetsRsp *lu_rsp = dynamic_cast<MetaMsgs::AdmMsgListSubsetsRsp *>(recv_msg);
    ret = lu_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgListSubsetsRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgListSubsetsRsp::poolT.put(lu_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgListSubsetsRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgListSubsetsRsp " <<
            " sts: " << lu_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
    }

    SwapNameList(lu_rsp->subsets, *args.list);
    lu_rsp->num_subsets = 0;
    args.status = lu_rsp->status;
    MetaMsgs::AdmMsgListSubsetsRsp::poolT.put(lu_rsp);

    return;
}

void ClusterProxy::ListStores(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;

    if ( !args.list ) {
        throw RcsdbFacade::Exception("NULL list passed", RcsdbFacade::Exception::eInvalidArgument);
    }

    SessionMgr::StartMgr();

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgListStoresReq *lt_req = MetaMsgs::AdmMsgListStoresReq::poolT.get();

    if ( args.status == 1 ) { // debug ON
        ClusterProxy::msg_debug = true;
        lt_req->json = string("{ \"debug\": true}");
        SessionMgr::GetInstance()->McastSetDebug(true);
    } else if ( args.status == 2 ){
        ClusterProxy::msg_debug = false;
        lt_req->json = string("{ \"debug\": false}");
        SessionMgr::GetInstance()->McastSetDebug(false);
    } else {
        ;
    }

    lt_req->cluster_name = *args.cluster_name;
    lt_req->cluster_id = args.cluster_id;
    lt_req->space_name = *args.space_name;
    lt_req->space_id = args.space_id;
    if ( args.key_len ) {
        lt_req->backup_path = string(args.key, args.key_len);
        lt_req->backup_req = 1;
    }

    ret = lt_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgListStoresReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgListStoresReq::poolT.put(lt_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgListStoresReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgListStoresReq::poolT.put(lt_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
            }
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgListStoresRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgListStoresRsp *lt_rsp = dynamic_cast<MetaMsgs::AdmMsgListStoresRsp *>(recv_msg);
    ret = lt_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgListStoresRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgListStoresRsp::poolT.put(lt_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgListStoresRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgListStoresRsp " <<
            " sts: " << lt_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
    }

    SwapNameList(lt_rsp->stores, *args.list);
    lt_rsp->num_stores = 0;
    args.status = lt_rsp->status;
    MetaMsgs::AdmMsgListStoresRsp::poolT.put(lt_rsp);

    return;
}

void ClusterProxy::ListSpaces(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;

    if ( !args.list ) {
        throw RcsdbFacade::Exception("NULL list passed", RcsdbFacade::Exception::eInvalidArgument);
    }

    SessionMgr::StartMgr();

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgListSpacesReq *ls_req = MetaMsgs::AdmMsgListSpacesReq::poolT.get();

    if ( args.status == 1 ) { // debug ON
        ClusterProxy::msg_debug = true;
        ls_req->json = string("{ \"debug\": true}");
        SessionMgr::GetInstance()->McastSetDebug(true);
    } else if ( args.status == 2 ){
        ClusterProxy::msg_debug = false;
        ls_req->json = string("{ \"debug\": false}");
        SessionMgr::GetInstance()->McastSetDebug(false);
    } else {
        ;
    }

    ls_req->cluster_name = *args.cluster_name;
    ls_req->cluster_id = args.cluster_id;
    if ( args.key_len ) {
        ls_req->backup_path = string(args.key, args.key_len);
        ls_req->backup_req = 1;
    }

    ret = ls_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgListSpacesReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgListSpacesReq::poolT.put(ls_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgListSpacesReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgListSpacesReq::poolT.put(ls_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
            }
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgListSpacesRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgListSpacesRsp *ls_rsp = dynamic_cast<MetaMsgs::AdmMsgListSpacesRsp *>(recv_msg);
    ret = ls_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgListSpacesRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgListSpacesRsp::poolT.put(ls_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgListSpacesRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgListSpacesRsp " <<
            " sts: " << ls_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
    }

    SwapNameList(ls_rsp->spaces, *args.list);
    ls_rsp->num_spaces = 0;
    args.status = ls_rsp->status;
    MetaMsgs::AdmMsgListSpacesRsp::poolT.put(ls_rsp);

    return;
}

void ClusterProxy::ListClusters(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    int ret = 0;

    if ( !args.list ) {
        throw RcsdbFacade::Exception("NULL list passed", RcsdbFacade::Exception::eInvalidArgument);
    }

    SessionMgr::StartMgr();

    TcpSocket *tcp_sock = AdmSession::GetAdmSessionSock();
    if ( !tcp_sock ) {
        throw RcsdbFacade::Exception("Metasrv not reachable", RcsdbFacade::Exception::eProviderUnreachable);
    }

    MetaMsgs::AdmMsgBuffer buf(true);

    // Send Req
    MetaMsgs::AdmMsgListClustersReq *lc_req = MetaMsgs::AdmMsgListClustersReq::poolT.get();

    if ( args.status == 1 ) { // debug ON
        ClusterProxy::msg_debug = true;
        lc_req->json = string("{ \"debug\": true}");
        SessionMgr::GetInstance()->McastSetDebug(true);
    } else if ( args.status == 2 ){
        ClusterProxy::msg_debug = false;
        lc_req->json = string("{ \"debug\": false}");
        SessionMgr::GetInstance()->McastSetDebug(false);
    } else {
        ;
    }

    if ( args.key_len ) {
        lc_req->backup_path = string(args.key, args.key_len);
        lc_req->backup_req = 1;
    }

    ret = lc_req->send(tcp_sock->GetDesc());
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgListClustersReq: " << ret << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgListClustersReq::poolT.put(lc_req);
        throw RcsdbFacade::Exception("Can't send AdmMsgListClustersReq", RcsdbFacade::Exception::eNetworkError);
    }
    MetaMsgs::AdmMsgListClustersReq::poolT.put(lc_req);

    // Receive AdmMsgBase
    MetaMsgs::AdmMsgBase base_msg;
    ret = base_msg.recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if ( ClusterProxy::msg_debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Srv disconnected" << SoeLogger::LogError() << endl;
            }
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        throw RcsdbFacade::Exception("Can't recv AdmMsgBase hdr", RcsdbFacade::Exception::eNetworkError);
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't get Factory" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Can't get Factory", RcsdbFacade::Exception::eInternalError);
    }
    recv_msg->hdr = base_msg.hdr;

    if ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) != eMsgListClustersRsp ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid rsp" << SoeLogger::LogError() << endl;
        throw RcsdbFacade::Exception("Invalid rsp", RcsdbFacade::Exception::eInvalidArgument);
    }

    // Receive Rsp
    MetaMsgs::AdmMsgListClustersRsp *lc_rsp = dynamic_cast<MetaMsgs::AdmMsgListClustersRsp *>(recv_msg);
    ret = lc_rsp->recv(tcp_sock->GetDesc(), buf);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgListClustersRsp" << SoeLogger::LogError() << endl;
        MetaMsgs::AdmMsgListClustersRsp::poolT.put(lc_rsp);
        throw RcsdbFacade::Exception("Can't recv AdmMsgListClustersRsp", RcsdbFacade::Exception::eNetworkError);
    } else {
        if ( ClusterProxy::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgListClustersRsp " <<
            " sts: " << lc_rsp->status <<
            " adm_msg_counter: " << ClusterProxy::adm_msg_counter++ << SoeLogger::LogDebug() << endl;
    }

    SwapNameList(lc_rsp->clusters, *args.list);
    lc_rsp->num_clusters = 0;
    args.status = lc_rsp->status;
    MetaMsgs::AdmMsgListClustersRsp::poolT.put(lc_rsp);

    return;
}

void ClusterProxy::RegisterStaticApiFunctions() throw(SoeApi::Exception)
{
    // bind functions to boost functors
    boost::function<uint64_t(LocalArgsDescriptor &)> ClusterLocalCreate =
        boost::bind(&MetadbcliFacade::ClusterProxy::CreateCluster, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalDestroy =
        boost::bind(&MetadbcliFacade::ClusterProxy::DestroyCluster, _1);

    boost::function<uint64_t(LocalArgsDescriptor &)> ClusterLocalGetByName =
        boost::bind(&MetadbcliFacade::ClusterProxy::GetClusterByName, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalListClusters =
        boost::bind(&MetadbcliFacade::ClusterProxy::ListClusters, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalListSpaces =
        boost::bind(&MetadbcliFacade::ClusterProxy::ListSpaces, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalListStores =
        boost::bind(&MetadbcliFacade::ClusterProxy::ListStores, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalListSubsets =
        boost::bind(&MetadbcliFacade::ClusterProxy::ListSubsets, _1);

    // register via SoeApi
    static_ftors.Register(new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
        ClusterLocalCreate, MetadbcliFacade::CreateLocalName("", "", "", 0, FUNCTION_CREATE), ClusterProxy::global_debug));

    static_ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalDestroy, MetadbcliFacade::CreateLocalName("", "", "", 0, FUNCTION_DESTROY), ClusterProxy::global_debug));

    static_ftors.Register(new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
        ClusterLocalGetByName, MetadbcliFacade::CreateLocalName("", "", "", 0, FUNCTION_GET_BY_NAME), ClusterProxy::global_debug));

    static_ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalListClusters, MetadbcliFacade::CreateLocalName("", "", "", 0, FUNCTION_LIST_CLUSTERS), ClusterProxy::global_debug));

    static_ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalListSpaces, MetadbcliFacade::CreateLocalName("", "", "", 0, FUNCTION_LIST_SPACES), ClusterProxy::global_debug));

    static_ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalListStores, MetadbcliFacade::CreateLocalName("", "", "", 0, FUNCTION_LIST_STORES), ClusterProxy::global_debug));

    static_ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalListSubsets, MetadbcliFacade::CreateLocalName("", "", "", 0, FUNCTION_LIST_SUBSETS), ClusterProxy::global_debug));
}

void ClusterProxy::DeregisterStaticApiFunctions() throw(SoeApi::Exception)
{
    static_ftors.DeregisterAll();
}

void ClusterProxy::RegisterApiFunctions()  throw(SoeApi::Exception)
{
    // bind functions to boost functors
    boost::function<void(LocalArgsDescriptor &)> ClusterLocalOpen =
        boost::bind(&MetadbcliFacade::ClusterProxy::Open, this, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalClose =
        boost::bind(&MetadbcliFacade::ClusterProxy::Close, this, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalPut =
        boost::bind(&MetadbcliFacade::ClusterProxy::Put, this, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalGet =
        boost::bind(&MetadbcliFacade::ClusterProxy::Get, this, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalDel =
        boost::bind(&MetadbcliFacade::ClusterProxy::Delete, this, _1);

    boost::function<void(LocalArgsDescriptor &)> ClusterLocalGetRange =
        boost::bind(&MetadbcliFacade::ClusterProxy::GetRange, this, _1);


    // register via SoeApi
    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalOpen, MetadbcliFacade::CreateLocalName(name, "", "", 0, FUNCTION_OPEN), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalClose, MetadbcliFacade::CreateLocalName(name, "", "", 0, FUNCTION_CLOSE), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalPut, MetadbcliFacade::CreateLocalName(name, "", "", 0, FUNCTION_PUT), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalGet, MetadbcliFacade::CreateLocalName(name, "", "", 0, FUNCTION_GET), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalDel, MetadbcliFacade::CreateLocalName(name, "", "", 0, FUNCTION_DELETE), ClusterProxy::global_debug));

    ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        ClusterLocalGetRange, MetadbcliFacade::CreateLocalName(name, "", "", 0, FUNCTION_GET_RANGE), ClusterProxy::global_debug));
}

void ClusterProxy::DeregisterApiFunctions()  throw(SoeApi::Exception)
{
    ftors.DeregisterAll();
}

//
// -----------------------------------------------
//
bool ClusterProxy::AddSpaceProxy(SpaceProxy &sp)
{
    WriteLock w_lock(ClusterProxy::map_lock);

    ProxyMapKey map_key(sp.id, ClusterProxy::no_thread_id, sp.name);
    pair<map<ProxyMapKey, SpaceProxy &>::iterator, bool> ret;
    ret = spaces.insert(pair<ProxyMapKey, SpaceProxy &>(map_key, sp));
    if ( ret.second == false ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Space already exists id: " <<
             sp.id << " th_id: " << ClusterProxy::no_thread_id << " name: " << sp.name << SoeLogger::LogError() << endl;
        }
        return false;
    }

    return true;
}

bool ClusterProxy::DeleteSpaceProxy(const ProxyMapKey &key)
{
    WriteLock w_lock(ClusterProxy::map_lock);

    map<ProxyMapKey, SpaceProxy &>::iterator it;
    it = spaces.find(key);
    if ( it == spaces.end() ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Space not found id: " <<
            key.i_val << " th_id: " << key.t_tid << " name: " << key.s_val << SoeLogger::LogError() << endl;
        }
        return false;
    }

    ClusterProxy::spaces.erase(it);
    return true;
}

SpaceProxy *ClusterProxy::FindSpaceProxy(const ProxyMapKey &key)
{
    WriteLock w_lock(ClusterProxy::map_lock);

    map<ProxyMapKey, SpaceProxy &>::iterator it;
    it = spaces.find(key);
    if ( it == spaces.end() ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Space not found " <<
            key.i_val << " th_id: " << key.t_tid << " name: " << key.s_val << SoeLogger::LogError() << endl;
        }
        return 0;
    }

    return &(*it).second;
}

//
// -----------------------------------------------
//
bool ClusterProxy::StaticAddClusterProxyNoLock(ClusterProxy &cp)
{
    ProxyMapKey map_key(cp.id, ClusterProxy::no_thread_id, cp.name);
    pair<map<ProxyMapKey, ClusterProxy &>::iterator, bool> ret;
    ret = ClusterProxy::clusters.insert(pair<ProxyMapKey, ClusterProxy &>(map_key, cp));
    if ( ret.second == false ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster already exists id: " <<
            cp.id << " th_id: " << ClusterProxy::no_thread_id << " name: " << cp.name << SoeLogger::LogError() << endl;
        }
        return false;
    }

    return true;
}

bool ClusterProxy::StaticAddClusterProxy(ClusterProxy &cp, bool lock_clusters_map)
{
    if ( lock_clusters_map == true ) {
        WriteLock w_lock(ClusterProxy::clusters_map_lock);
        return ClusterProxy::StaticAddClusterProxyNoLock(cp);
    }
    return ClusterProxy::StaticAddClusterProxyNoLock(cp);
}


bool ClusterProxy::StaticDeleteClusterProxyNoLock(const ProxyMapKey &key)
{
    map<ProxyMapKey, ClusterProxy &>::iterator it;
    it = ClusterProxy::clusters.find(key);
    if ( it == ClusterProxy::clusters.end() ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found id: " <<
            key.i_val << " th_id: " << key.t_tid << " name: " << key.s_val << SoeLogger::LogError() << endl;
        }
        return false;
    }

    ClusterProxy::clusters.erase(it);
    return true;
}

bool ClusterProxy::StaticDeleteClusterProxy(const ProxyMapKey &key, bool lock_clusters_map)
{
    if ( lock_clusters_map == true ) {
        WriteLock w_lock(ClusterProxy::clusters_map_lock);
        return ClusterProxy::StaticDeleteClusterProxyNoLock(key);
    }
    return ClusterProxy::StaticDeleteClusterProxyNoLock(key);
}


ClusterProxy *ClusterProxy::StaticFindClusterProxyNoLock(const ProxyMapKey &key)
{
    map<ProxyMapKey, ClusterProxy &>::iterator it;
    it = ClusterProxy::clusters.find(key);
    if ( it == ClusterProxy::clusters.end() ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found id: "
            << key.i_val << " th_id: " << key.t_tid << " name: " << key.s_val << SoeLogger::LogError() << endl;
        }
        return 0;
    }

    return &(*it).second;
}

ClusterProxy *ClusterProxy::StaticFindClusterProxy(const ProxyMapKey &key, bool lock_clusters_map)
{
    if ( lock_clusters_map == true ) {
        WriteLock w_lock(ClusterProxy::clusters_map_lock);
        return ClusterProxy::StaticFindClusterProxyNoLock(key);
    }
    return ClusterProxy::StaticFindClusterProxyNoLock(key);
}

//
// -----------------------------------------------
//
bool ClusterProxy::StaticAddSpaceProxy(const ProxyMapKey &cl_key, SpaceProxy &sp)
{
    WriteLock w_lock(ClusterProxy::clusters_map_lock);

    ClusterProxy *cl_p = ClusterProxy::StaticFindClusterProxy(cl_key, false);
    if ( !cl_p ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found id: " <<
            cl_key.i_val << " th_id: " << cl_key.t_tid << " name: " << cl_key.s_val << SoeLogger::LogError() << endl;
        }
        return false;
    }

    sp.parent = cl_p;
    return cl_p->AddSpaceProxy(sp);
}

bool ClusterProxy::StaticDeleteSpaceProxy(const ProxyMapKey &cl_key, const ProxyMapKey &sp_key)
{
    WriteLock w_lock(ClusterProxy::clusters_map_lock);

    ClusterProxy *cl_p = ClusterProxy::StaticFindClusterProxy(cl_key, false);
    if ( !cl_p ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found id: " <<
            cl_key.i_val << " th_id: " << cl_key.t_tid << " name: " << cl_key.s_val << SoeLogger::LogError() << endl;
        }
        return false;
    }

    return cl_p->DeleteSpaceProxy(sp_key);
}

SpaceProxy *ClusterProxy::StaticFindSpaceProxy(const ProxyMapKey &cl_key, const ProxyMapKey &sp_key)
{
    WriteLock w_lock(ClusterProxy::clusters_map_lock);

    ClusterProxy *cl_p = ClusterProxy::StaticFindClusterProxy(cl_key, false);
    if ( !cl_p ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found id: " <<
            cl_key.i_val << " th_id: " << cl_key.t_tid << " name: " << cl_key.s_val << SoeLogger::LogError() << endl;
        }
        return 0;
    }

    return cl_p->FindSpaceProxy(sp_key);
}

//
// -----------------------------------------------
//
bool ClusterProxy::StaticAddStoreProxy(const ProxyMapKey &cl_key, const ProxyMapKey &sp_key, StoreProxy &st)
{
    WriteLock w_lock(ClusterProxy::clusters_map_lock);

    ClusterProxy *f_cl = ClusterProxy::StaticFindClusterProxy(cl_key, false);
    if ( !f_cl ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found for id: " << cl_key.i_val <<
            " th_id: " << cl_key.t_tid << " name: " << cl_key.s_val << SoeLogger::LogError() << endl;
        }
        return false;
    }

    SpaceProxy *f_sp = f_cl->FindSpaceProxy(sp_key);
    if ( !f_sp ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Space not found for id: " << sp_key.i_val <<
            " th_id: " << sp_key.t_tid << " name: " << sp_key.s_val << SoeLogger::LogError() << endl;
        }
        return false;
    }

    st.parent = f_sp;
    return f_sp->AddStoreProxy(st);
}

bool ClusterProxy::StaticDeleteStoreProxy(const ProxyMapKey &cl_key, const ProxyMapKey &sp_key, const ProxyMapKey &st_key)
{
    WriteLock w_lock(ClusterProxy::clusters_map_lock);

    ClusterProxy *f_cl = ClusterProxy::StaticFindClusterProxy(cl_key, false);
    if ( !f_cl ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found for id: " << cl_key.i_val <<
            " th_id: " << cl_key.t_tid << " name: " << cl_key.s_val << SoeLogger::LogError() << endl;
        }
        return false;
    }

    SpaceProxy *f_sp = f_cl->FindSpaceProxy(sp_key);
    if ( !f_sp ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Space not found for id: " << sp_key.i_val <<
            " th_id: " << sp_key.t_tid << " name: " << sp_key.s_val << SoeLogger::LogError() << endl;
        }
        return false;
    }

    return f_sp->DeleteStoreProxy(st_key);
}

StoreProxy *ClusterProxy::StaticFindStoreProxy(const ProxyMapKey &cl_key, const ProxyMapKey &sp_key, const ProxyMapKey &st_key)
{
    WriteLock w_lock(ClusterProxy::clusters_map_lock);

    ClusterProxy *f_cl = ClusterProxy::StaticFindClusterProxy(cl_key, false);
    if ( !f_cl ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found for id: " << cl_key.i_val <<
            " th_id: " << cl_key.t_tid << " name: " << cl_key.s_val << SoeLogger::LogError() << endl;
        }
        return 0;
    }

    SpaceProxy *f_sp = f_cl->FindSpaceProxy(sp_key);
    if ( !f_sp ) {
        if ( ClusterProxy::msg_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Space not found for id: " << sp_key.i_val <<
            " th_id: " << sp_key.t_tid << " name: " << sp_key.s_val << SoeLogger::LogError() << endl;
        }
        return 0;
    }

    return f_sp->FindStoreProxy(st_key);
}

}

