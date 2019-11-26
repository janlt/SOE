/**
 * @file    metadbsrvadmsession.cpp
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
 * metadbsrvadmsession.cpp
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
#include "msgneteventclisession.hpp"
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

namespace Metadbsrv {

PROXY_TYPES

uint64_t AdmSession::session_id_counter = 1;
shared_ptr<AdmSession> AdmSession::sessions[max_sessions];
Lock AdmSession::global_lock;

//
// Adm Session
//
AdmSession::AdmSession(TcpSocket &soc, bool _test)
    :IPv4TCPSrvSession(soc, _test),
     counter(0),
     bytes_received(0),
     bytes_sent(0),
     sess_debug(true),
     session_id(AdmSession::session_id_counter++)
{
    my_addr = IPv4Address::findNodeNoLocalAddr();
}

AdmSession::~AdmSession() {}

void AdmSession::SetSessDebug(bool _sess_debug)
{
    sess_debug = _sess_debug;
}

int AdmSession::processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Invalid/suspect msg" << SoeLogger::LogError() << endl;
    return -1;
}

void AdmSession::InitDesc(LocalArgsDescriptor &args) const
{
    ::memset(&args, '\0', sizeof(args));
}

void AdmSession::InitAdmDesc(LocalAdmArgsDescriptor &args) const
{
    ::memset(&args, '\0', sizeof(args));
}

int AdmSession::processStreamMessage(MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    WriteLock w_lock(AdmSession::adm_op_lock);

    SoeLogger::Logger logger;

    if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " (" << session_id << ")Got msg" << SoeLogger::LogDebug() << endl;

    uint64_t backup_id = 2222;
    uint64_t snapshot_id = 4444;

    MetaMsgs::AdmMsgBase base_msg;
    MetaMsgs::AdmMsgBuffer buf(true);
    int ret = base_msg.recv(this->tcp_sock.GetDesc(), buf);
    if ( ret < 0 ) {
        if ( static_cast<MetaMsgs::MsgStatus>(ret) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
            if (  sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Client disconnected" << SoeLogger::LogDebug() << endl;
        } else {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't receive AdmMsgBase hdr ret: " << ret << SoeLogger::LogError() << endl;
        }
        return ret;
    }
    bytes_received += ret;

    if ( debug == true ) {
        if ( !(counter%0x1000) ) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << " counter: " << counter <<
                " received: " << bytes_received <<
                " sent: " << bytes_sent <<
                " sess->soc: " << this->tcp_sock.GetDesc() << SoeLogger::LogDebug() << endl;
        }
    }

    try {
        MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
        if ( !recv_msg ) {
            if ( static_cast<MetaMsgs::MsgStatus>(-errno) == MetaMsgs::MsgStatus::eMsgDisconnect ) {
                if (  sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Client disconnected" << SoeLogger::LogDebug() << endl;
            } else {
                logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't receive AdmMsgBase hdr errno: " << errno << SoeLogger::LogError() << endl;
            }
            return -errno;
        }
        recv_msg->hdr = base_msg.hdr;

        switch ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) ) {
        case eMsgAssignMMChannelReq:
        {
            MetaMsgs::AdmMsgAssignMMChannelReq *mm_req = dynamic_cast<MetaMsgs::AdmMsgAssignMMChannelReq *>(recv_msg);
            ret = mm_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgAssignMMChannelReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgAssignMMChannelReq req" <<
                    " pid: " << mm_req->pid <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }



            MetaMsgs::AdmMsgAssignMMChannelRsp *mm_rsp = MetaMsgs::AdmMsgAssignMMChannelRsp::poolT.get();
            mm_rsp->path = "/tmp/soemmdir_dont_touch/mmpoint_file_";
            mm_rsp->pid = mm_req->pid;
            ret = mm_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgAssignMMChannelRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgAssignMMChannelRsp::poolT.put(mm_rsp);
        }
        break;
        case eMsgCreateClusterReq:
        {
            MetaMsgs::AdmMsgCreateClusterReq *cc_req = dynamic_cast<MetaMsgs::AdmMsgCreateClusterReq *>(recv_msg);
            ret = cc_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgCreateClusterReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgCreateClusterReq req" <<
                    " cluster_name: " << cc_req->cluster_name << " og: " << cc_req->only_get_by_name <<
                    " json " << cc_req->json <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalArgsDescriptor args;
            InitDesc(args);
            args.cluster_id = ClusterProxyType::invalid_id;
            args.cluster_name = &cc_req->cluster_name;
            args.range_callback_arg = (cc_req->only_get_by_name ? (char *)LD_GetByName : 0);
            AdmMsgJsonParser json_parser(cc_req->json);
            int p_ret = json_parser.Parse();
            if ( !p_ret ) {
                json_parser.GetBoolTagValue(string("debug"), sess_debug);
            }

            try {
                if ( cc_req->only_get_by_name ) {
                    args.cluster_id = ClusterProxyType::GetClusterByName(args);
                } else {
                    args.cluster_id = ClusterProxyType::CreateCluster(args);
                }
            } catch (RcsdbFacade::Exception &ex) {
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Cluster::CreateCluster/GetClusterByName failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.key_len = static_cast<uint32_t>(ex.status);
            }

            MetaMsgs::AdmMsgCreateClusterRsp *cc_rsp = MetaMsgs::AdmMsgCreateClusterRsp::poolT.get();
            cc_rsp->cluster_id = args.cluster_id;
            cc_rsp->status = args.key_len; // overloaded
            ret = cc_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgCreateClusterRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgCreateClusterRsp::poolT.put(cc_rsp);
        }
        break;
        case eMsgCreateSpaceReq:
        {
            MetaMsgs::AdmMsgCreateSpaceReq *cs_req = dynamic_cast<MetaMsgs::AdmMsgCreateSpaceReq *>(recv_msg);
            ret = cs_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgCreateSpaceReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgCreateSpaceReq req" <<
                    " cluster_name: " << cs_req->cluster_name <<
                    " space_name: " << cs_req->space_name << " og: " << cs_req->only_get_by_name <<
                    " json " << cs_req->json <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalArgsDescriptor args;
            InitDesc(args);
            args.space_id = ClusterProxyType::invalid_id;
            args.cluster_name = &cs_req->cluster_name;
            args.space_name = &cs_req->space_name;
            args.cluster_id = cs_req->cluster_id;
            args.range_callback_arg = (cs_req->only_get_by_name ? (char *)LD_GetByName : 0);
            AdmMsgJsonParser json_parser(cs_req->json);
            int p_ret = json_parser.Parse();
            if ( !p_ret ) {
                json_parser.GetBoolTagValue(string("debug"), sess_debug);
            }

            try {
                if ( cs_req->only_get_by_name ) {
                    args.space_id = SpaceProxyType::GetSpaceByName(args);
                } else {
                    args.space_id = SpaceProxyType::CreateSpace(args);
                }
            } catch (RcsdbFacade::Exception &ex) {
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " SpaceProxyType::CreateSpace/GetSpaceByName failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.space_id = ClusterProxyType::invalid_id;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.key_len = static_cast<uint32_t>(ex.status);
            }

            MetaMsgs::AdmMsgCreateSpaceRsp *cs_rsp = MetaMsgs::AdmMsgCreateSpaceRsp::poolT.get();
            cs_rsp->cluster_id = args.cluster_id;
            cs_rsp->space_id = args.space_id;
            cs_rsp->status = args.key_len; // overloaded
            ret = cs_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgCreateSpaceRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgCreateSpaceRsp::poolT.put(cs_rsp);
        }
        break;
        case eMsgCreateStoreReq:
        {
            MetaMsgs::AdmMsgCreateStoreReq *ct_req = dynamic_cast<MetaMsgs::AdmMsgCreateStoreReq *>(recv_msg);
            ret = ct_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgCreateStoreReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgCreateStoreReq req" <<
                    " cluster_name: " << ct_req->cluster_name << " cl_id: " << ct_req->cluster_id <<
                    " space_name: " << ct_req->space_name << " sp_id: " << ct_req->space_id <<
                    " store_name: " << ct_req->store_name << " og: " << ct_req->only_get_by_name <<
                    " tr_db: " << ct_req->flags.bit_fields.transactional <<
                    " ov_dups: " << ct_req->flags.bit_fields.overwrite_dups <<
                    " bl_bts: " << ct_req->flags.bit_fields.bloom_bits <<
                    " snc_mod: " << ct_req->flags.bit_fields.sync_mode <<
                    " json: " << ct_req->json <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalArgsDescriptor args;
            InitDesc(args);
            args.store_id = ClusterProxyType::invalid_id;
            args.cluster_name = &ct_req->cluster_name;
            args.space_name = &ct_req->space_name;
            args.store_name = &ct_req->store_name;
            args.cluster_id = ct_req->cluster_id;
            args.space_id = ct_req->space_id;
            args.group_idx = ct_req->cr_thread_id;
            args.transaction_db = ct_req->flags.bit_fields.transactional;
            args.overwrite_dups = ct_req->flags.bit_fields.overwrite_dups;
            args.bloom_bits = ct_req->flags.bit_fields.bloom_bits;  // no overflow must be ensured on 8-bit field
            args.sync_mode = ct_req->flags.bit_fields.sync_mode;
            args.range_callback_arg = (ct_req->only_get_by_name ? (char *)LD_GetByName : 0);
            AdmMsgJsonParser json_parser(ct_req->json);
            int p_ret = json_parser.Parse();
            if ( !p_ret ) {
                json_parser.GetBoolTagValue(string("debug"), sess_debug);
            }

            try {
                if ( ct_req->only_get_by_name ) {
                    args.store_id = StoreProxyType::GetStoreByName(args, from_addr, session_id);
                } else {
                    args.store_id = StoreProxyType::CreateStore(args, from_addr, pt, session_id);
                }
            } catch (RcsdbFacade::Exception &ex) {
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " StoreProxyType::CreateStore/GetStoreByName failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.store_id = ClusterProxyType::invalid_id;
                args.space_id = ClusterProxyType::invalid_id;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.key_len = static_cast<uint32_t>(ex.status);
            }

            MetaMsgs::AdmMsgCreateStoreRsp *ct_rsp = MetaMsgs::AdmMsgCreateStoreRsp::poolT.get();
            ct_rsp->cluster_id = args.cluster_id;
            ct_rsp->space_id = args.space_id;
            ct_rsp->store_id = args.store_id;
            ct_rsp->status = args.key_len; // overloaded
            ret = ct_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgCreateStoreRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgCreateStoreRsp::poolT.put(ct_rsp);
        }
        break;
        case eMsgDeleteClusterReq:
        {
            MetaMsgs::AdmMsgDeleteClusterReq *dc_req = dynamic_cast<MetaMsgs::AdmMsgDeleteClusterReq *>(recv_msg);
            ret = dc_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgDeleteClusterReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgDeleteClusterReq req" <<
                    " cluster_name: " << dc_req->cluster_name <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalArgsDescriptor args;
            InitDesc(args);
            args.cluster_id = dc_req->cluster_id;
            args.cluster_name = &dc_req->cluster_name;
            try {
                ClusterProxyType::DestroyCluster(args);
            } catch (RcsdbFacade::Exception &ex) {
                if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Cluster::DestroyCluster failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.key_len = static_cast<uint32_t>(ex.status);
            }

            MetaMsgs::AdmMsgDeleteClusterRsp *dc_rsp = MetaMsgs::AdmMsgDeleteClusterRsp::poolT.get();
            dc_rsp->cluster_id = args.cluster_id;
            dc_rsp->status = args.key_len; // overloaded
            ret = dc_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgDeleteClusterRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgDeleteClusterRsp::poolT.put(dc_rsp);
        }
        break;
        case eMsgDeleteSpaceReq:
        {
            MetaMsgs::AdmMsgDeleteSpaceReq *ds_req = dynamic_cast<MetaMsgs::AdmMsgDeleteSpaceReq *>(recv_msg);
            ret = ds_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgDeleteSpaceReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgDeleteSpaceReq req" <<
                    " cluster_name: " << ds_req->cluster_name <<
                    " space_name: " << ds_req->space_name <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalArgsDescriptor args;
            InitDesc(args);
            args.cluster_id = ds_req->cluster_id;
            args.space_id = ds_req->space_id;
            args.cluster_name = &ds_req->cluster_name;
            args.space_name = &ds_req->space_name;
            try {
                SpaceProxyType::DestroySpace(args);
            } catch (RcsdbFacade::Exception &ex) {
                if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " SpaceProxyType::DestroySpace failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.space_id = ClusterProxyType::invalid_id;
                args.key_len = static_cast<uint32_t>(ex.status);
            }

            MetaMsgs::AdmMsgDeleteSpaceRsp *ds_rsp = MetaMsgs::AdmMsgDeleteSpaceRsp::poolT.get();
            ds_rsp->cluster_id = args.cluster_id;
            ds_rsp->space_id = args.space_id;
            ds_rsp->status = args.key_len; // overloaded
            ret = ds_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgDeleteSpaceRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgDeleteSpaceRsp::poolT.put(ds_rsp);
        }
        break;
        case eMsgDeleteStoreReq:
        {
            MetaMsgs::AdmMsgDeleteStoreReq *dt_req = dynamic_cast<MetaMsgs::AdmMsgDeleteStoreReq *>(recv_msg);
            ret = dt_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgDeleteStoreReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgDeleteStoreReq req" <<
                    " cluster_name: " << dt_req->cluster_name <<
                    " space_name: " << dt_req->space_name <<
                    " store_name: " << dt_req->store_name <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalArgsDescriptor args;
            InitDesc(args);
            args.cluster_id = dt_req->cluster_id;
            args.space_id = dt_req->space_id;
            args.store_id = dt_req->store_id;
            args.cluster_name = &dt_req->cluster_name;
            args.space_name = &dt_req->space_name;
            args.store_name = &dt_req->store_name;
            args.group_idx = dt_req->cr_thread_id;
            try {
                StoreProxyType::DestroyStore(args, from_addr);
            } catch (RcsdbFacade::Exception &ex) {
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " StoreProxyType::DestroyStore failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.space_id = ClusterProxyType::invalid_id;
                args.store_id = ClusterProxyType::invalid_id;
                args.key_len = static_cast<uint32_t>(ex.status);
            }

            MetaMsgs::AdmMsgDeleteStoreRsp *dt_rsp = MetaMsgs::AdmMsgDeleteStoreRsp::poolT.get();
            dt_rsp->cluster_id = args.cluster_id;
            dt_rsp->space_id = args.space_id;
            dt_rsp->store_id = args.store_id;
            dt_rsp->status = args.key_len; // overloaded
            ret = dt_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgDeleteStoreRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgDeleteStoreRsp::poolT.put(dt_rsp);
        }
        break;
        case eMsgRepairStoreReq:
        {
            MetaMsgs::AdmMsgRepairStoreReq *rt_req = dynamic_cast<MetaMsgs::AdmMsgRepairStoreReq *>(recv_msg);
            ret = rt_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgRepairStoreReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgRepairStoreReq req" <<
                    " cluster_name: " << rt_req->cluster_name << " cl_id: " << rt_req->cluster_id <<
                    " space_name: " << rt_req->space_name << " sp_id: " << rt_req->space_id <<
                    " store_name: " << rt_req->store_name << " og: " << rt_req->only_get_by_name <<
                    " json: " << rt_req->json <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalArgsDescriptor args;
            InitDesc(args);
            args.store_id = ClusterProxyType::invalid_id;
            args.cluster_name = &rt_req->cluster_name;
            args.space_name = &rt_req->space_name;
            args.store_name = &rt_req->store_name;
            args.cluster_id = rt_req->cluster_id;
            args.space_id = rt_req->space_id;
            args.store_id = rt_req->store_id;
            args.group_idx = rt_req->cr_thread_id;
            AdmMsgJsonParser json_parser(rt_req->json);
            int p_ret = json_parser.Parse();
            if ( !p_ret ) {
                json_parser.GetBoolTagValue(string("debug"), sess_debug);
            }

            try {
                args.store_id = StoreProxyType::RepairStore(args, from_addr, session_id);
                args.key_len = args.status;
            } catch (RcsdbFacade::Exception &ex) {
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " StoreProxyType::CreateStore/GetStoreByName failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.store_id = ClusterProxyType::invalid_id;
                args.space_id = ClusterProxyType::invalid_id;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.key_len = static_cast<uint32_t>(ex.status);
            }

            MetaMsgs::AdmMsgRepairStoreRsp *rt_rsp = MetaMsgs::AdmMsgRepairStoreRsp::poolT.get();
            rt_rsp->cluster_id = args.cluster_id;
            rt_rsp->space_id = args.space_id;
            rt_rsp->store_id = args.store_id;
            rt_rsp->status = args.key_len; // overloaded
            ret = rt_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgRepairStoreRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgRepairStoreRsp::poolT.put(rt_rsp);
        }
        break;
        case eMsgListClustersReq:
        {
            MetaMsgs::AdmMsgListClustersReq *lc_req = dynamic_cast<MetaMsgs::AdmMsgListClustersReq *>(recv_msg);
            ret = lc_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgListClustersReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgListClustersReq req" <<
                    " json: " << lc_req->json <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalArgsDescriptor args;
            InitDesc(args);

            if ( lc_req->backup_req ) {
                args.key = lc_req->backup_path.c_str();
                args.key_len = lc_req->backup_path.length();
            }

            MetaMsgs::AdmMsgListClustersRsp *lc_rsp = MetaMsgs::AdmMsgListClustersRsp::poolT.get();
            args.list = &lc_rsp->clusters;
            AdmMsgJsonParser json_parser(lc_req->json);
            int p_ret = json_parser.Parse();
            if ( !p_ret ) {
                json_parser.GetBoolTagValue(string("debug"), sess_debug);
            }

            try {
                if ( !lc_req->backup_req ) {
                    ClusterProxyType::ListClusters(args, from_addr, session_id);
                } else {
                    ClusterProxyType::ListBackupClusters(args, from_addr, session_id);
                }
                args.key_len = args.status;
                lc_rsp->num_clusters = static_cast<uint32_t>(lc_rsp->clusters.size());
            } catch (RcsdbFacade::Exception &ex) {
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " StoreProxyType::ListClusters failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.store_id = ClusterProxyType::invalid_id;
                args.space_id = ClusterProxyType::invalid_id;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.key_len = static_cast<uint32_t>(ex.status);
            }

            lc_rsp->status = args.key_len; // overloaded
            ret = lc_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgListClustersRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            lc_rsp->clusters.clear();
            lc_rsp->num_clusters = 0;
            MetaMsgs::AdmMsgListClustersRsp::poolT.put(lc_rsp);
        }
        break;
        case eMsgListSpacesReq:
        {
            MetaMsgs::AdmMsgListSpacesReq *ls_req = dynamic_cast<MetaMsgs::AdmMsgListSpacesReq *>(recv_msg);
            ret = ls_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgListSpacesReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgListSpacesReq req" <<
                    " cluster_name: " << ls_req->cluster_name << " cl_id: " << ls_req->cluster_id <<
                    " json: " << ls_req->json <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalArgsDescriptor args;
            InitDesc(args);
            args.cluster_name = &ls_req->cluster_name;
            args.cluster_id = ls_req->cluster_id;
            if ( ls_req->backup_req ) {
                args.key = ls_req->backup_path.c_str();
                args.key_len = ls_req->backup_path.length();
            }

            MetaMsgs::AdmMsgListSpacesRsp *ls_rsp = MetaMsgs::AdmMsgListSpacesRsp::poolT.get();
            args.list = &ls_rsp->spaces;
            AdmMsgJsonParser json_parser(ls_req->json);
            int p_ret = json_parser.Parse();
            if ( !p_ret ) {
                json_parser.GetBoolTagValue(string("debug"), sess_debug);
            }

            try {
                if ( !ls_req->backup_req ) {
                    ClusterProxyType::ListSpaces(args, from_addr, session_id);
                } else {
                    ClusterProxyType::ListBackupSpaces(args, from_addr, session_id);
                }
                args.key_len = args.status;
                ls_rsp->num_spaces = static_cast<uint32_t>(ls_rsp->spaces.size());
            } catch (RcsdbFacade::Exception &ex) {
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " StoreProxyType::ListSpaces failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.store_id = ClusterProxyType::invalid_id;
                args.space_id = ClusterProxyType::invalid_id;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.key_len = static_cast<uint32_t>(ex.status);
            }

            ls_rsp->status = args.key_len; // overloaded
            ret = ls_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgListSpacesRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            ls_rsp->spaces.clear();
            ls_rsp->num_spaces = 0;
            MetaMsgs::AdmMsgListSpacesRsp::poolT.put(ls_rsp);
        }
        break;
        case eMsgListStoresReq:
        {
            MetaMsgs::AdmMsgListStoresReq *lt_req = dynamic_cast<MetaMsgs::AdmMsgListStoresReq *>(recv_msg);
            ret = lt_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgListStoresReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgListStoresReq req" <<
                    " cluster_name: " << lt_req->cluster_name << " cl_id: " << lt_req->cluster_id <<
                    " space_name: " << lt_req->space_name << " sp_id: " << lt_req->space_id <<
                    " json: " << lt_req->json <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalArgsDescriptor args;
            InitDesc(args);
            args.cluster_name = &lt_req->cluster_name;
            args.space_name = &lt_req->space_name;
            args.cluster_id = lt_req->cluster_id;
            args.space_id = lt_req->space_id;
            if ( lt_req->backup_req ) {
                args.key = lt_req->backup_path.c_str();
                args.key_len = lt_req->backup_path.length();
            }

            MetaMsgs::AdmMsgListStoresRsp *lt_rsp = MetaMsgs::AdmMsgListStoresRsp::poolT.get();
            args.list = &lt_rsp->stores;
            AdmMsgJsonParser json_parser(lt_req->json);
            int p_ret = json_parser.Parse();
            if ( !p_ret ) {
                json_parser.GetBoolTagValue(string("debug"), sess_debug);
            }

            try {
                if ( !lt_req->backup_req ) {
                    ClusterProxyType::ListStores(args, from_addr, session_id);
                } else {
                    ClusterProxyType::ListBackupStores(args, from_addr, session_id);
                }
                args.key_len = args.status;
                lt_rsp->num_stores = static_cast<uint32_t>(lt_rsp->stores.size());
            } catch (RcsdbFacade::Exception &ex) {
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " StoreProxyType::ListStores failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.store_id = ClusterProxyType::invalid_id;
                args.space_id = ClusterProxyType::invalid_id;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.key_len = static_cast<uint32_t>(ex.status);
            }

            lt_rsp->status = args.key_len; // overloaded
            ret = lt_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgListSpacesRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            lt_rsp->stores.clear();
            lt_rsp->num_stores = 0;
            MetaMsgs::AdmMsgListStoresRsp::poolT.put(lt_rsp);
        }
        break;
        case eMsgListSubsetsReq:
        {
            MetaMsgs::AdmMsgListSubsetsReq *lu_req = dynamic_cast<MetaMsgs::AdmMsgListSubsetsReq *>(recv_msg);
            ret = lu_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgListSubsetsReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgListSubsetsReq req" <<
                    " cluster_name: " << lu_req->cluster_name << " cl_id: " << lu_req->cluster_id <<
                    " space_name: " << lu_req->space_name << " sp_id: " << lu_req->space_id <<
                    " json: " << lu_req->json <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalArgsDescriptor args;
            InitDesc(args);
            args.cluster_name = &lu_req->cluster_name;
            args.space_name = &lu_req->space_name;
            args.store_name = &lu_req->store_name;
            args.cluster_id = lu_req->cluster_id;
            args.space_id = lu_req->space_id;
            args.store_id = lu_req->store_id;

            MetaMsgs::AdmMsgListSubsetsRsp *lu_rsp = MetaMsgs::AdmMsgListSubsetsRsp::poolT.get();
            args.list = &lu_rsp->subsets;
            AdmMsgJsonParser json_parser(lu_req->json);
            int p_ret = json_parser.Parse();
            if ( !p_ret ) {
                json_parser.GetBoolTagValue(string("debug"), sess_debug);
            }

            try {
                ClusterProxyType::ListSubsets(args, from_addr, session_id);
                args.key_len = args.status;
                lu_rsp->num_subsets = static_cast<uint32_t>(lu_rsp->subsets.size());
            } catch (RcsdbFacade::Exception &ex) {
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " StoreProxyType::ListStores failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.store_id = ClusterProxyType::invalid_id;
                args.space_id = ClusterProxyType::invalid_id;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.key_len = static_cast<uint32_t>(ex.status);
            }

            lu_rsp->status = args.key_len; // overloaded
            ret = lu_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgListSubsetsRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            lu_rsp->subsets.clear();
            lu_rsp->num_subsets = 0;
            MetaMsgs::AdmMsgListSubsetsRsp::poolT.put(lu_rsp);
        }
        break;
        case eMsgCreateBackupReq:
        {
            MetaMsgs::AdmMsgCreateBackupReq *cb_req = dynamic_cast<MetaMsgs::AdmMsgCreateBackupReq *>(recv_msg);
            ret = cb_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgCreateBackupReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgCreateBackupReq req" <<
                    " cluster_name: " << cb_req->cluster_name <<
                    " space_name: " << cb_req->space_name <<
                    " store_name: " << cb_req->store_name <<
                    " backup_engine_name: " << cb_req->backup_engine_name <<
                    " backup_engien_idx:" << cb_req->backup_engine_idx <<
                    " backup_engine_id: " << cb_req->backup_engine_id <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalAdmArgsDescriptor args;
            InitAdmDesc(args);
            args.cluster_name = &cb_req->cluster_name;
            args.space_name = &cb_req->space_name;
            args.store_name = &cb_req->store_name;
            args.cluster_id = cb_req->cluster_id;
            args.space_id = cb_req->space_id;
            args.store_id = cb_req->store_id;
            args.sync_mode = cb_req->sess_thread_id;
            args.backup_name = &cb_req->backup_engine_name;
            args.backup_id = cb_req->backup_engine_id;
            args.backup_idx = cb_req->backup_engine_idx;

            try {
                StoreProxyType::StaticCreateBackup(args, from_addr, session_id);
            } catch (RcsdbFacade::Exception &ex) {
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " StoreProxyType::StaticCreateBackup failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.store_id = ClusterProxyType::invalid_id;
                args.space_id = ClusterProxyType::invalid_id;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.backup_idx = static_cast<uint32_t>(ex.status);
            }

            MetaMsgs::AdmMsgCreateBackupRsp *cb_rsp = MetaMsgs::AdmMsgCreateBackupRsp::poolT.get();
            cb_rsp->backup_id = args.backup_id;
            if ( args.backup_idx == cb_req->backup_engine_idx ) {
                cb_rsp->status = 0;
            } else {
                cb_rsp->status = args.backup_idx;
            }
            ret = cb_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgCreateBackupRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgCreateBackupRsp::poolT.put(cb_rsp);
        }
        break;
        case eMsgRestoreBackupReq:
        {
            MetaMsgs::AdmMsgRestoreBackupReq *rb_req = dynamic_cast<MetaMsgs::AdmMsgRestoreBackupReq *>(recv_msg);
            ret = rb_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgCreateBackupReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgCreateBackupReq req" <<
                    " cluster_name: " << rb_req->cluster_name <<
                    " space_name: " << rb_req->space_name <<
                    " store_name: " << rb_req->store_name <<
                    " backup_name: " << rb_req->backup_name <<
                    " backup_engien_idx:" << rb_req->backup_engine_idx <<
                    " backup_id: " << rb_req->backup_id <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalAdmArgsDescriptor args;
            InitAdmDesc(args);
            args.cluster_name = &rb_req->cluster_name;
            args.space_name = &rb_req->space_name;
            args.store_name = &rb_req->store_name;
            args.cluster_id = rb_req->cluster_id;
            args.space_id = rb_req->space_id;
            args.store_id = rb_req->store_id;
            args.sync_mode = rb_req->sess_thread_id;
            args.backup_name = &rb_req->backup_name;
            args.backup_id = rb_req->backup_id;
            args.backup_idx = rb_req->backup_engine_idx;
            std::vector<std::pair<std::string, uint32_t> > restore_names;
            if ( rb_req->restore_cluster_name != "" && rb_req->restore_space_name != "" && rb_req->restore_store_name != "" ) {
                restore_names.push_back(std::pair<std::string, uint32_t>(rb_req->restore_cluster_name, 0));
                restore_names.push_back(std::pair<std::string, uint32_t>(rb_req->restore_space_name, 0));
                restore_names.push_back(std::pair<std::string, uint32_t>(rb_req->restore_store_name, 0));
                args.backups = &restore_names;
            }

            try {
                StoreProxyType::StaticRestoreBackup(args, from_addr);
            } catch (RcsdbFacade::Exception &ex) {
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " StoreProxyType::StaticRestoreBackup failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.store_id = ClusterProxyType::invalid_id;
                args.space_id = ClusterProxyType::invalid_id;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.backup_idx = static_cast<uint32_t>(ex.status);
            }

            MetaMsgs::AdmMsgRestoreBackupRsp *rb_rsp = MetaMsgs::AdmMsgRestoreBackupRsp::poolT.get();
            rb_rsp->backup_id = args.backup_id;
            if ( args.backup_idx == rb_req->backup_engine_idx ) {
                rb_rsp->status = 0;
            } else {
                rb_rsp->status = args.backup_idx;
            }
            ret = rb_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgRestoreBackupRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgRestoreBackupRsp::poolT.put(rb_rsp);
        }
        break;
        case eMsgDeleteBackupReq:
        {
            MetaMsgs::AdmMsgDeleteBackupReq *db_req = dynamic_cast<MetaMsgs::AdmMsgDeleteBackupReq *>(recv_msg);
            ret = db_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgDeleteBackupReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgDeleteBackupReq req" <<
                    " cluster_name: " << db_req->cluster_name <<
                    " space_name: " << db_req->space_name <<
                    " store_name: " << db_req->store_name <<
                    " backup_name: " << db_req->backup_name <<
                    " backup_id: " << db_req->backup_id <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalAdmArgsDescriptor args;
            InitAdmDesc(args);
            args.cluster_name = &db_req->cluster_name;
            args.space_name = &db_req->space_name;
            args.store_name = &db_req->store_name;
            args.cluster_id = db_req->cluster_id;
            args.space_id = db_req->space_id;
            args.store_id = db_req->store_id;
            args.sync_mode = db_req->sess_thread_id;
            args.backup_name = &db_req->backup_name; // used for backup root dir
            args.backup_id = db_req->backup_id;

            try {
                StoreProxyType::StaticDestroyBackup(args, from_addr);
            } catch (RcsdbFacade::Exception &ex) {
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " StoreProxyType::StaticDestroyBackup failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.store_id = ClusterProxyType::invalid_id;
                args.space_id = ClusterProxyType::invalid_id;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.backup_idx = static_cast<uint32_t>(ex.status);
            }

            MetaMsgs::AdmMsgDeleteBackupRsp *db_rsp = MetaMsgs::AdmMsgDeleteBackupRsp::poolT.get();
            db_rsp->backup_id = backup_id;
            if ( args.backup_id == db_req->backup_id ) {
                db_rsp->status = 0;
            } else {
                db_rsp->status = args.backup_idx;
            }
            ret = db_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgDeleteBackupRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgDeleteBackupRsp::poolT.put(db_rsp);
        }
        break;
        case eMsgVerifyBackupReq:
        {
            MetaMsgs::AdmMsgVerifyBackupReq *vb_req = dynamic_cast<MetaMsgs::AdmMsgVerifyBackupReq *>(recv_msg);
            ret = vb_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgVerifyBackupReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgVerifyBackupReq req" <<
                    " cluster_name: " << vb_req->cluster_name <<
                    " space_name: " << vb_req->space_name <<
                    " store_name: " << vb_req->store_name <<
                    " backup_name: " << vb_req->backup_name <<
                    " backup_id: " << vb_req->backup_id <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }

            LocalAdmArgsDescriptor args;
            InitAdmDesc(args);
            args.cluster_name = &vb_req->cluster_name;
            args.space_name = &vb_req->space_name;
            args.store_name = &vb_req->store_name;
            args.cluster_id = vb_req->cluster_id;
            args.space_id = vb_req->space_id;
            args.store_id = vb_req->store_id;
            args.sync_mode = vb_req->sess_thread_id;
            args.backup_name = &vb_req->backup_name; // used for backup root dir
            args.backup_id = vb_req->backup_id;

            try {
                StoreProxyType::StaticVerifyBackup(args, from_addr);
            } catch (RcsdbFacade::Exception &ex) {
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " StoreProxyType::StaticVerifyBackup failed: " << ex.what() << SoeLogger::LogDebug() << endl;
                args.store_id = ClusterProxyType::invalid_id;
                args.space_id = ClusterProxyType::invalid_id;
                args.cluster_id = ClusterProxyType::invalid_id;
                args.backup_idx = static_cast<uint32_t>(ex.status);
            }

            MetaMsgs::AdmMsgVerifyBackupRsp *vb_rsp = MetaMsgs::AdmMsgVerifyBackupRsp::poolT.get();
            vb_rsp->backup_id = backup_id;
            if ( args.backup_id == vb_rsp->backup_id ) {
                vb_rsp->status = 0;
            } else {
                vb_rsp->status = args.backup_idx;
            }
            ret = vb_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgVerifyBackupRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgVerifyBackupRsp::poolT.put(vb_rsp);
        }
        break;
        case eMsgListBackupsReq:
        {
            MetaMsgs::AdmMsgListBackupsReq *lbks_req = dynamic_cast<MetaMsgs::AdmMsgListBackupsReq *>(recv_msg);
            ret = lbks_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgListBackupsReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgListBackupsReq req" <<
                    " cluster_name: " << lbks_req->cluster_name <<
                    " space_name: " << lbks_req->space_name <<
                    " store_name: " << lbks_req->store_name <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }



            MetaMsgs::AdmMsgListBackupsRsp *lbks_rsp = MetaMsgs::AdmMsgListBackupsRsp::poolT.get();
            string backup_name = "bbbbeeeeckup_name_";
            lbks_rsp->num_backups = 128;
            for ( uint32_t i = 0 ; i < lbks_rsp->num_backups ; i++ ) {
                lbks_rsp->backups.push_back(pair<uint64_t, string>(backup_id + i, backup_name + to_string(i)));
            }
            ret = lbks_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgListBackupsRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            lbks_rsp->backups.erase(lbks_rsp->backups.begin(), lbks_rsp->backups.end());
            lbks_rsp->num_backups = 0;
            MetaMsgs::AdmMsgListBackupsRsp::poolT.put(lbks_rsp);
        }
        break;
        case eMsgCreateSnapshotReq:
        {
            MetaMsgs::AdmMsgCreateSnapshotReq *csn_req = dynamic_cast<MetaMsgs::AdmMsgCreateSnapshotReq *>(recv_msg);
            ret = csn_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgCreateSnapshotReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgCreateSnapshotReq req" <<
                    " cluster_name: " << csn_req->cluster_name <<
                    " space_name: " << csn_req->space_name <<
                    " store_name: " << csn_req->store_name <<
                    " snapshot_engine_name: " << csn_req->snapshot_engine_name <<
                    " snapshot_engine_idx: " << csn_req->snapshot_engine_idx <<
                    " snapshot_engine_id: " << csn_req->snapshot_engine_id <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }



            MetaMsgs::AdmMsgCreateSnapshotRsp *csn_rsp = MetaMsgs::AdmMsgCreateSnapshotRsp::poolT.get();
            csn_rsp->snapshot_id = snapshot_id;
            ret = csn_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgCreateSnapshotRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgCreateSnapshotRsp::poolT.put(csn_rsp);
        }
        break;
        case eMsgDeleteSnapshotReq:
        {
            MetaMsgs::AdmMsgDeleteSnapshotReq *dsn_req = dynamic_cast<MetaMsgs::AdmMsgDeleteSnapshotReq *>(recv_msg);
            ret = dsn_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgDeleteSnapshotReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgDeleteSnapshotReq req" <<
                    " cluster_name: " << dsn_req->cluster_name <<
                    " space_name: " << dsn_req->space_name <<
                    " store_name: " << dsn_req->store_name <<
                    " snapshot_name: " << dsn_req->snapshot_name <<
                    " snapshot_id: " << dsn_req->snapshot_id <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }



            MetaMsgs::AdmMsgDeleteSnapshotRsp *dsn_rsp = MetaMsgs::AdmMsgDeleteSnapshotRsp::poolT.get();
            dsn_rsp->snapshot_id = snapshot_id;
            ret = dsn_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgDeleteSnapshotRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgDeleteSnapshotRsp::poolT.put(dsn_rsp);
        }
        break;
        case eMsgListSnapshotsReq:
        {
            MetaMsgs::AdmMsgListSnapshotsReq *lsns_req = dynamic_cast<MetaMsgs::AdmMsgListSnapshotsReq *>(recv_msg);
            ret = lsns_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgListSnapshotsReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgListSnapshotsReq req" <<
                    " cluster_name: " << lsns_req->cluster_name <<
                    " space_name: " << lsns_req->space_name <<
                    " store_name: " << lsns_req->store_name <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }



            MetaMsgs::AdmMsgListSnapshotsRsp *lsns_rsp = MetaMsgs::AdmMsgListSnapshotsRsp::poolT.get();
            string snapshot_name = "snapppshot_name_";
            lsns_rsp->num_snapshots = 128;
            for ( uint32_t i = 0 ; i < lsns_rsp->num_snapshots ; i++ ) {
                lsns_rsp->snapshots.push_back(pair<uint64_t, string>(snapshot_id + i, snapshot_name + to_string(i)));
            }
            ret = lsns_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgListSnapshotsRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            lsns_rsp->snapshots.erase(lsns_rsp->snapshots.begin(), lsns_rsp->snapshots.end());
            lsns_rsp->num_snapshots = 0;
            MetaMsgs::AdmMsgListSnapshotsRsp::poolT.put(lsns_rsp);
        }
        break;
        case eMsgOpenStoreReq:
        {
            MetaMsgs::AdmMsgOpenStoreReq *os_req = dynamic_cast<MetaMsgs::AdmMsgOpenStoreReq *>(recv_msg);
            ret = os_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgOpenStoreReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgOpenStoreReq req" <<
                    " cluster_name: " << os_req->cluster_name <<
                    " space_name: " << os_req->space_name <<
                    " store_name: " << os_req->store_name <<
                    " store_id: " << os_req->store_id <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }



            MetaMsgs::AdmMsgOpenStoreRsp *os_rsp = MetaMsgs::AdmMsgOpenStoreRsp::poolT.get();
            os_rsp->store_id = os_req->store_id;
            ret = os_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgOpenStoreRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgOpenStoreRsp::poolT.put(os_rsp);
        }
        break;
        case eMsgCloseStoreReq:
        {
            MetaMsgs::AdmMsgCloseStoreReq *cs_req = dynamic_cast<MetaMsgs::AdmMsgCloseStoreReq *>(recv_msg);
            ret = cs_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Can't receive AdmMsgCloseStoreReq" << SoeLogger::LogError() << endl;
            } else {
                counter++;
                bytes_received += ret;
                if ( sess_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Got AdmMsgCloseStoreReq req" <<
                    " cluster_name: " << cs_req->cluster_name <<
                    " space_name: " << cs_req->space_name <<
                    " store_name: " << cs_req->store_name <<
                    " store_id: " << cs_req->store_id <<
                    " counter: " << counter << SoeLogger::LogDebug() << endl;
            }



            MetaMsgs::AdmMsgCloseStoreRsp *cs_rsp = MetaMsgs::AdmMsgCloseStoreRsp::poolT.get();
            cs_rsp->store_id = cs_req->store_id;
            ret = cs_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                logger.Clear(CLEAR_DEFAULT_ARGS) << " Failed sending AdmMsgCloseStoreRsp: " << ret << SoeLogger::LogError() << endl;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgCloseStoreRsp::poolT.put(cs_rsp);
        }
        break;
        default:
            logger.Clear(CLEAR_DEFAULT_ARGS) << " Receive invalid AdmMsgBase hdr" << SoeLogger::LogError() << endl;
            return -1;
        }

        MetaMsgs::AdmMsgFactory::getInstance()->put(recv_msg);

    } catch (const runtime_error &ex) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Failed getting msg from factory: " << ex.what() << SoeLogger::LogError() << endl;
    }

    return ret;
}

int AdmSession::sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Invalid invocation" << SoeLogger::LogDebug() << endl;
    return -1;
}

int AdmSession::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Sending msg" << SoeLogger::LogDebug() << endl;
    return 0;
}

void AdmSession::CloseAllStores()
{
    bool s_ret = false;

    for ( uint32_t i = 0 ; i < 100 ; i++ ) {
        if ( AsyncIOSession::CloseAllSessionStores(session_id) ) {
            ::usleep(10000);
        } else {
            s_ret = true;
            break;
        }
    }

    if ( s_ret == false ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "CloseAllSessionStores() not done" << SoeLogger::LogError() << endl;
    }
}

void AdmSession::PurgeStaleStores()
{
    AsyncIOSession::PurgeAllSessionsAndStores();
}

void AdmSession::PruneArchive()
{
    ClusterProxyType::PruneArchive();
}

}
