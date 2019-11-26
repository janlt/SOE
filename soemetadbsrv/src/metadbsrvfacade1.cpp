/**
 * @file    metadbsrvfacade1.cpp
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
 *  soe_soe
 *
 * @section Source
 *
 *    
 *    
 *
 */

/*
 * metadbsrvfacade1.cpp
 *
 *  Created on: Nov 24, 2017
 *      Author: Jan Lisowiec
 */

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include <vector>
#include <map>
#include <fstream>
#include <string>

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

#include "soe_bind_api.hpp"
#include "rcsdbargdefs.hpp"
#include "rcsdbfacadeapi.hpp"
#include "rcsdbproxyfacadebase.hpp"

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

#include "msgnetadkeys.hpp"
#include "msgnetioevent.hpp"
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
#include "msgnetlistener.hpp"
#include "msgneteventlistener.hpp"
#include "msgnetsession.hpp"
#include "msgnetipv4tcpsession.hpp"
#include "msgnetipv4tcpclisession.hpp"
#include "msgneteventsession.hpp"
#include "msgneteventclisession.hpp"
#include "msgneteventsession.hpp"
#include "msgneteventsrvsession.hpp"
#include "msgnetasynceventsrvsession.hpp"

using namespace MetaMsgs;
using namespace MetaNet;
using namespace RcsdbFacade;

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
#include "rcsdbidgener.hpp"
#include "rcsdbfacadeapi.hpp"
#include "rcsdbfacadebase.hpp"
#include "rcsdbconfigbase.hpp"
#include "rcsdbconfig.hpp"
#include "rcsdbfilters.hpp"

#include "metadbsrvrcsdbname.hpp"
#include "metadbsrvrcsdbstoreiterator.hpp"
#include "metadbsrvrangecontext.hpp"
#include "metadbsrvrcsdbstore.hpp"
#include "metadbsrvrcsdbspace.hpp"
#include "metadbsrvrcsdbcluster.hpp"
#include "metadbsrvrcsdbstorebackup.hpp"
#include "metadbsrvrcsdbsnapshot.hpp"
#include "metadbsrvaccessors1.hpp"
#include "metadbsrvfacadeapi.hpp"
#include "metadbsrvasynciosession.hpp"
#include "metadbsrvdefaultstore.hpp"
#include "metadbsrvfacade1.hpp"

using namespace std;
using namespace MetaNet;

void __attribute__ ((constructor)) Metadbsrv::StaticInitializerEnter1(void);
void __attribute__ ((destructor)) Metadbsrv::StaticInitializerExit1(void);

namespace Metadbsrv {

#ifdef __DEBUG_ASYNC_IO__
Lock StoreProxy1::io_debug_lock;
#endif

//
// StoreProxy1
//
RcsdbAccessors1<Metadbsrv::Store, Metadbsrv::DefaultStore>  StoreProxy1::global_acs;

void StoreProxy1::GarbageCollectStore(StoreProxy1 *&st_new, uint64_t t_id, LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    WriteLock w_lock(ClusterProxy1::static_op_lock);
    StoreProxy1::GarbageCollectStoreNoLock(st_new, t_id, args);
}

void StoreProxy1::GarbageCollectStoreNoLock(StoreProxy1 *&st_new, uint64_t t_id, LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    SoeLogger::Logger logger;

    if ( ClusterProxy1::global_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Found possibly stale StoreProxy1 state: " <<
        st_new->state << " name: " << *args.store_name << " id: " << t_id << SoeLogger::LogDebug() << endl;

    bool garbage_collect = false;

    switch ( st_new->state ) {
    case StoreProxy1::State::eCreated:
    case StoreProxy1::State::eOpened:
        break;
    case StoreProxy1::State::eClosed:
    case StoreProxy1::State::eDeleted:
        garbage_collect = true;
        break;
    }

    if ( garbage_collect == true ) {
        //cout << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ << " pxy->name: " << st_new->name <<
            //" pxy->thread_id: " << st_new->thread_id << " t_id: " << t_id << " group_idx: " << args.group_idx << endl;

        // if stale proxy has an io_session check its state
        if ( st_new->io_session ) {
            if ( st_new->io_session->state == AsyncIOSession::State::eDefunct ) {
                if ( ClusterProxy1::global_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " stale io_session state: " <<
                    st_new->io_session->state << SoeLogger::LogDebug() << endl;
            }
        }

        if ( st_new->io_session_async ) {
            if ( st_new->io_session_async->state == AsyncIOSession::State::eDefunct ) {
                if ( ClusterProxy1::global_debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " stale io_session_async state: " <<
                    st_new->io_session_async->state << SoeLogger::LogDebug() << endl;
            }
        }

        // archive stale proxy if state == eClosed or eDeleted
        st_new = ClusterProxy1::StaticRemoveStoreProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name),
            ProxyMapKey1(args.space_id, ClusterProxy1::no_thread_id, *args.space_name),
            ProxyMapKey1(st_new->id, st_new->thread_id, st_new->name));
        if ( !st_new ) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << " no such proxy " << SoeLogger::LogError() << endl;
        } else {
            ClusterProxy1::ArchiveStoreProxy(*st_new);
            st_new = 0;
        }
    }
}

uint64_t StoreProxy1::CreateStore(LocalArgsDescriptor &args, const IPv4Address &from_addr, const MetaNet::Point *pt, uint64_t _adm_s_id) throw(RcsdbFacade::Exception)
{
    SoeLogger::Logger logger;

    WriteLock w_lock(ClusterProxy1::static_op_lock);

    //cout << __FUNCTION__ << " ##### st_name: " << *args.store_name << " st_id: " << args.store_id << endl;

    uint64_t t_id = StoreProxy1::global_acs.create_fun(args);
    if ( ClusterProxy1::global_debug == true ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) <<
            " t_id: " << t_id << " name: " << *args.store_name << " th_id: " << args.group_idx <<
            " cluster: " << *args.cluster_name << " space: " << *args.space_name << SoeLogger::LogDebug() << SoeLogger::LogDebug() << endl;
    }

    if ( t_id == StoreProxy1::invalid_id ) {
        return t_id;
    }

    // find out if there is a stale proxy and clean it up
    StoreProxy1 *st_new = ClusterProxy1::StaticFindStoreProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name),
            ProxyMapKey1(args.space_id, ClusterProxy1::no_thread_id, *args.space_name),
            ProxyMapKey1(t_id, args.group_idx, *args.store_name));

    // there is a stale proxy only if the state is eClosed or eDeleted
    // garbage collect stale proxy if needed
    if ( st_new ) {
        StoreProxy1::GarbageCollectStoreNoLock(st_new, t_id, args);
    }

    // recreate proxy and insert it, io_session will be created upon connect
    if ( !st_new ) {
        st_new = new StoreProxy1(*args.cluster_name,
            *args.space_name,
            *args.store_name,
            args.cluster_id,
            args.space_id,
            t_id,
            args.group_idx,
            ClusterProxy1::global_debug,
            _adm_s_id);
        bool ins_ret = ClusterProxy1::StaticAddStoreProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name),
            ProxyMapKey1(args.space_id, ClusterProxy1::no_thread_id, *args.space_name),
            *st_new);
        if ( ins_ret == false ) {
            logger.Clear(CLEAR_DEFAULT_ARGS) << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ << " add proxy failed name: " << st_new->name <<
                " id: " << st_new->id << " th_id: " << st_new->thread_id << SoeLogger::LogError() << endl;
            t_id = StoreProxy1::invalid_id;
            delete st_new;
        }
    } else {
        if ( st_new->state == StoreProxy1::State::eClosed ||
            st_new->state == StoreProxy1::State::eDeleted ) {
            st_new->state = StoreProxy1::State::eCreated;
        }
    }

    return t_id;
}

uint64_t StoreProxy1::RepairStore(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception)
{
    SoeLogger::Logger logger;

    WriteLock w_lock(ClusterProxy1::static_op_lock);

    //cout << __FUNCTION__ << " ##### st_name: " << *args.store_name << " st_id: " << args.store_id << endl;

    uint64_t t_id = StoreProxy1::global_acs.repair_fun(args);
    if ( ClusterProxy1::global_debug == true ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) <<
            " t_id: " << t_id << " name: " << *args.store_name << " th_id: " << args.group_idx <<
            " cluster: " << *args.cluster_name << " space: " << *args.space_name << SoeLogger::LogDebug() << SoeLogger::LogDebug() << endl;
    }

    return t_id;
}

void StoreProxy1::DestroyStore(LocalArgsDescriptor &args, const IPv4Address &from_addr) throw(RcsdbFacade::Exception)
{
    SoeLogger::Logger logger;

    logger.Clear(CLEAR_DEFAULT_ARGS) << "StoreProxy1::" << __FUNCTION__ << " st_name: " << *args.store_name <<
        " st_id: " << args.store_id << SoeLogger::LogDebug() << endl;

    WriteLock w_lock(ClusterProxy1::static_op_lock);

    StoreProxy1::global_acs.destroy_fun(args);
    if ( ClusterProxy1::global_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) <<
            " cluster: " << *args.cluster_name << " c_id: " << args.cluster_id <<
            " space: " << *args.space_name <<  " s_id: " << args.space_id <<
            " name: " << *args.store_name << " t_id: " << args.store_id << " th_id: " << args.group_idx << SoeLogger::LogDebug() << endl;
    }

    // now remove all proxies matching name and id and put them on removed_list
    std::list<StoreProxy1 *> removed_proxies;
    bool des_ret = ClusterProxy1::StaticFindAllStoreProxies(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name),
        ProxyMapKey1(args.space_id, ClusterProxy1::no_thread_id, *args.space_name),
        ProxyMapKey1(args.store_id, ClusterProxy1::no_thread_id, *args.store_name),
        removed_proxies);

    if ( des_ret == false ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ << " removed_proxies.size(): " << removed_proxies.size() <<
            " args.group_idx: " << args.group_idx << SoeLogger::LogDebug() << endl;
        return;
    }

    for ( std::list<StoreProxy1 *>::iterator it = removed_proxies.begin() ; it != removed_proxies.end() ; it++ ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ << " removed_proxies: name: " <<
            (*it)->name << " id: " << (*it)->id << " thread_id: " << (*it)->thread_id << " state: " << (*it)->state <<
            " args.store_name: " << *args.store_name << " args.thread_id: " << args.group_idx << SoeLogger::LogDebug() << endl;
    }

    // garbage collect all except this one
    for ( std::list<StoreProxy1 *>::iterator it = removed_proxies.begin() ; it != removed_proxies.end() ; it++ ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ << " deactivating ... name: " <<
            (*it)->name << " id: " << (*it)->id << " thread_id: " << (*it)->thread_id << " state: " << (*it)->state <<
            SoeLogger::LogDebug() << endl;

        if ( (*it)->name == *args.store_name && (*it)->id == args.store_id && (*it)->thread_id == args.group_idx ) {
            (*it)->state = StoreProxy1::State::eDeleted;
            (*it)->GetAcs().ResetObj();
            StoreProxy1 *st_to_del = *it;
            StoreProxy1::GarbageCollectStoreNoLock(st_to_del, st_to_del->id, args);
            continue;
        }

        logger.Clear(CLEAR_DEFAULT_ARGS)  << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ << " to be deleted proxy name: " << (*it)->name <<
            " id: " << (*it)->id << " th_id: " << (*it)->thread_id << SoeLogger::LogDebug() << endl;

        if ( (*it)->state != StoreProxy1::eClosed && (*it)->state != StoreProxy1::eDeleted ) {
            try {
                (*it)->CloseStores();
                (*it)->state = StoreProxy1::State::eDeleted;
                (*it)->GetAcs().ResetObj();
                StoreProxy1 *st_to_del = *it;
                StoreProxy1::GarbageCollectStoreNoLock(st_to_del, st_to_del->id, args);
                if ( !st_to_del ) {
                    logger.Clear(CLEAR_DEFAULT_ARGS) << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ << " garbage collected ok name: " <<
                        (*it)->name << " id: " << (*it)->id << " thread_id: " << (*it)->thread_id << SoeLogger::LogDebug() << endl;
                } else {
                    logger.Clear(CLEAR_DEFAULT_ARGS) << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ << " garbage collected failed name: " <<
                        (*it)->name << " id: " << (*it)->id << " thread_id: " << (*it)->thread_id << SoeLogger::LogDebug() << endl;
                }
            } catch ( ... ) {
                logger.Clear(CLEAR_DEFAULT_ARGS)  << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ <<
                    " exception caugth ... ignoring " << SoeLogger::LogDebug() << endl;
                continue;
            }
        }
    }
}

uint64_t StoreProxy1::GetStoreByName(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception)
{
    SoeLogger::Logger logger;

    //cout << __FUNCTION__ << " ##### st_name: " << *args.store_name << " st_id: " << args.store_id << endl;

    WriteLock w_lock(ClusterProxy1::static_op_lock);

    uint64_t t_id = StoreProxy1::global_acs.get_by_name_fun(args);
    if ( ClusterProxy1::global_debug == true ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " t_id: " << t_id << " name: " << *args.store_name <<
            " space: " << *args.space_name << " c_id: " << args.space_id <<
            " cluster: " << *args.cluster_name << " c_id: " << args.cluster_id <<
            " rc: " << (args.range_callback_arg == (char *)LD_GetByName) << SoeLogger::LogDebug() << endl;
    }

    // create StoreProxy1 if space exists
    if ( t_id != StoreProxy1::invalid_id ) {
        StoreProxy1 *t_pr = ClusterProxy1::StaticFindStoreProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name),
            ProxyMapKey1(args.space_id, ClusterProxy1::no_thread_id, *args.space_name),
            ProxyMapKey1(t_id, args.group_idx, *args.store_name));

        if ( t_pr )  {
            // there is a stale proxy only if the state is eClosed or eDeleted
            // garbage collect stale proxy if needed
            StoreProxy1::GarbageCollectStoreNoLock(t_pr, t_id, args);
        }

        if ( !t_pr ) {
            StoreProxy1 *st_new = new StoreProxy1(*args.cluster_name,
                *args.space_name,
                *args.store_name,
                args.cluster_id,
                args.space_id,
                t_id,
                args.group_idx, // pid_t of the calling thread
                ClusterProxy1::global_debug,
                _adm_s_id);

            // Check if there is cluster proxy and create it
            ClusterProxy1 *c_pr = ClusterProxy1::StaticFindClusterProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name));
            if ( !c_pr ) {
                ClusterProxy1 *cl_new = new ClusterProxy1(*args.cluster_name, args.cluster_id, ClusterProxy1::global_debug);
                bool ins_ret = ClusterProxy1::StaticAddClusterProxy(*cl_new);
                if ( ins_ret == false ) {
                    delete cl_new;
                    t_id = StoreProxy1::invalid_id;
                }
            }

            if ( t_id != StoreProxy1::invalid_id ) {
                // Check if there is SpaceProxy1 and create it
                SpaceProxy1 *s_pr = ClusterProxy1::StaticFindSpaceProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name),
                    ProxyMapKey1(args.space_id, ClusterProxy1::no_thread_id, *args.space_name));
                if ( !s_pr ) {
                    SpaceProxy1 *sp_new = new SpaceProxy1(*args.cluster_name,
                        *args.space_name,
                        args.cluster_id,
                        args.space_id,
                        ClusterProxy1::global_debug);
                    bool ins_ret = ClusterProxy1::StaticAddSpaceProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name), *sp_new);
                    if ( ins_ret == false ) {
                        delete sp_new;
                        t_id = StoreProxy1::invalid_id;
                    }
                }

                if ( t_id != StoreProxy1::invalid_id ) {
                    bool ins_ret = ClusterProxy1::StaticAddStoreProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name),
                        ProxyMapKey1(args.space_id, ClusterProxy1::no_thread_id, *args.space_name), *st_new);
                    if ( ins_ret == false ) {
                        //cout << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ << " add proxy failed name: " << st_new->name <<
                            //" id: " << st_new->id << " th_id: " << st_new->thread_id << endl;
                        delete st_new;
                        t_id = StoreProxy1::invalid_id;
                    } //else {
                        //cout << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ << " add proxy ok name: " << st_new->name <<
                            //" id: " << st_new->id << " th_id: " << st_new->thread_id << endl;
                    //}
                }
            }
        } else {
            if ( t_pr->state == StoreProxy1::State::eClosed ||
                t_pr->state == StoreProxy1::State::eDeleted ) {
                t_pr->state = StoreProxy1::State::eCreated;
            }
        }
    }

    return t_id;
}

uint32_t StoreProxy1::StaticCreateBackup(LocalAdmArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception)
{
    SoeLogger::Logger logger;

    WriteLock w_lock(ClusterProxy1::static_op_lock);

    //cout << __FUNCTION__ << " ##### st_name: " << *args.store_name << " st_id: " << args.store_id << endl;

    // find out if there is a stale proxy and clean it up
    StoreProxy1 *st_new = ClusterProxy1::StaticFindStoreProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name),
            ProxyMapKey1(args.space_id, ClusterProxy1::no_thread_id, *args.space_name),
            ProxyMapKey1(args.store_id, args.sync_mode, *args.store_name));

    // store not found
    if ( !st_new ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ <<
            " no proxy found cluster: " << *args.cluster_name << " cluster_id: " << args.cluster_id <<
            " space: " << *args.space_name << " space_id: " << args.space_id <<
            " store: " << *args.store_name << " store_id: " << args.store_id << SoeLogger::LogError() << endl;
        args.backup_idx = StoreProxy1::invalid_32_id;
        return args.backup_idx;
    }

    st_new->acs.create_backup(args);

    return args.backup_id;
}

uint32_t StoreProxy1::StaticRestoreBackup(LocalAdmArgsDescriptor &args, const IPv4Address &from_addr) throw(RcsdbFacade::Exception)
{
    SoeLogger::Logger logger;

    WriteLock w_lock(ClusterProxy1::static_op_lock);

    //cout << __FUNCTION__ << " ##### st_name: " << *args.store_name << " st_id: " << args.store_id << endl;

    // find out if there is a stale proxy and clean it up
    StoreProxy1 *st_new = ClusterProxy1::StaticFindStoreProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name),
            ProxyMapKey1(args.space_id, ClusterProxy1::no_thread_id, *args.space_name),
            ProxyMapKey1(args.store_id, args.sync_mode, *args.store_name));

    // store not found
    if ( !st_new ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ <<
            " no proxy found cluster: " << *args.cluster_name << " cluster_id: " << args.cluster_id <<
            " space: " << *args.space_name << " space_id: " << args.space_id <<
            " store: " << *args.store_name << " store_id: " << args.store_id << SoeLogger::LogError() << endl;
        args.backup_idx = StoreProxy1::invalid_32_id;
        return args.backup_idx;
    }

    st_new->acs.restore_backup(args);

    return args.backup_id;
}

uint32_t StoreProxy1::StaticDestroyBackup(LocalAdmArgsDescriptor &args, const IPv4Address &from_addr) throw(RcsdbFacade::Exception)
{
    SoeLogger::Logger logger;

    WriteLock w_lock(ClusterProxy1::static_op_lock);

    //cout << __FUNCTION__ << " ##### st_name: " << *args.store_name << " st_id: " << args.store_id << endl;

    // find out if there is a stale proxy and clean it up
    StoreProxy1 *st_new = ClusterProxy1::StaticFindStoreProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name),
            ProxyMapKey1(args.space_id, ClusterProxy1::no_thread_id, *args.space_name),
            ProxyMapKey1(args.store_id, args.sync_mode, *args.store_name));

    // store not found
    if ( !st_new ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ <<
            " no proxy found cluster: " << *args.cluster_name << " cluster_id: " << args.cluster_id <<
            " space: " << *args.space_name << " space_id: " << args.space_id <<
            " store: " << *args.store_name << " store_id: " << args.store_id << SoeLogger::LogError() << endl;
        args.backup_idx = StoreProxy1::invalid_32_id;
        return args.backup_idx;
    }

    st_new->acs.destroy_backup(args);

    return args.backup_id;
}

uint32_t StoreProxy1::StaticVerifyBackup(LocalAdmArgsDescriptor &args, const IPv4Address &from_addr) throw(RcsdbFacade::Exception)
{
    SoeLogger::Logger logger;

    WriteLock w_lock(ClusterProxy1::static_op_lock);

    //cout << __FUNCTION__ << " ##### st_name: " << *args.store_name << " st_id: " << args.store_id << endl;

    // find out if there is a stale proxy and clean it up
    StoreProxy1 *st_new = ClusterProxy1::StaticFindStoreProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name),
            ProxyMapKey1(args.space_id, ClusterProxy1::no_thread_id, *args.space_name),
            ProxyMapKey1(args.store_id, args.sync_mode, *args.store_name));

    // store not found
    if ( !st_new ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "StoreProxy1::" << __FUNCTION__ << "/" << __LINE__ <<
            " no proxy found cluster: " << *args.cluster_name << " cluster_id: " << args.cluster_id <<
            " space: " << *args.space_name << " space_id: " << args.space_id <<
            " store: " << *args.store_name << " store_id: " << args.store_id << SoeLogger::LogError() << endl;
        args.backup_idx = StoreProxy1::invalid_32_id;
        return args.backup_idx;
    }

    st_new->acs.verify_backup(args);

    return args.backup_id;
}

uint32_t StoreProxy1::StaticCreateSnapshot(LocalAdmArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception)
{
    return StoreProxy1::invalid_32_id;
}

uint32_t StoreProxy1::StaticRestoreSnapshot(LocalAdmArgsDescriptor &args, const IPv4Address &from_addr) throw(RcsdbFacade::Exception)
{
    return StoreProxy1::invalid_32_id;
}

uint32_t StoreProxy1::StaticDestroySnapshot(LocalAdmArgsDescriptor &args, const IPv4Address &from_addr) throw(RcsdbFacade::Exception)
{
    return StoreProxy1::invalid_32_id;
}

void StoreProxy1::CloseStores()
{
    //cout << "AsyncIOSession::" << __FUNCTION__ << " name: " << name << " id: " << id <<
        //" open_close_ref_count: " << io_session->open_close_ref_count << endl;
    SoeLogger::Logger logger;

    if ( AsyncIOSession::global_lock.try_lock() == false ) {
        return;
    }

    if ( adm_session_id == StoreProxy1::invalid_id || !io_session || io_session->state != AsyncIOSession::State::eDefunct ) {
        AsyncIOSession::global_lock.unlock();
        return;
    }

    uint32_t tmp_cnt = io_session->OpenCloseRefCountGetNoLock();
    if ( tmp_cnt > 0 && tmp_cnt < 64 ) {
        for ( uint32_t i = 0 ; i < tmp_cnt ; i++ ) {
            MetaNet::IoEvent *ev = MetaNet::IoEvent::RDHUOEvent();
            MetaMsgs::MsgBufferBase buf;
            IPv4Address from_addr;
            io_session->processInboundStreamEvent(buf, from_addr, ev);
            logger.Clear(CLEAR_DEFAULT_ARGS) << "Enqueued close event st_pr->adi=" << adm_session_id << SoeLogger::LogDebug() << endl;
        }
    }

    AsyncIOSession::global_lock.unlock();
}

int StoreProxy1::PrepareSendIov(const LocalArgsDescriptor &args, IovMsgBase *iov)
{
    SoeLogger::Logger logger;
    IovMsgRsp<TestIov> *gp = dynamic_cast<IovMsgRsp<TestIov> *>(iov);

    gp->iovhdr.seq = args.seq_number;

    // Cluster/Space/Stroe elements
    gp->meta_desc.desc.cluster_id = cluster_id;
    if ( gp->SetIovSize(GetReq<TestIov>::cluster_name_idx, cluster.size()) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " cluster_name too big" << SoeLogger::LogError() << endl;
        return -1;
    }
    ::memcpy(gp->meta_desc.desc.cluster_name, const_cast<char *>(cluster.data()), cluster.size());

    gp->meta_desc.desc.space_id = space_id;
    if ( gp->SetIovSize(GetReq<TestIov>::space_name_idx, space.size()) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " space_name too big" << SoeLogger::LogError() << endl;
        return -1;
    }
    ::memcpy(gp->meta_desc.desc.space_name, const_cast<char *>(space.data()), space.size());

    gp->meta_desc.desc.store_id = id;
    if ( gp->SetIovSize(GetReq<TestIov>::store_name_idx, name.size()) ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " store_name too big" << SoeLogger::LogError() << endl;
        return -1;
    }
    ::memcpy(gp->meta_desc.desc.store_name, const_cast<char *>(name.data()), name.size());

    // Data elements
    gp->meta_desc.desc.key_len = static_cast<uint32_t>(args.key_len);
    if ( gp->SetIovSize(GetReq<TestIov>::key_idx, gp->meta_desc.desc.key_len) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " key too big" << SoeLogger::LogError() << endl;
        return -1;
    }
    ::memcpy(gp->meta_desc.desc.key, const_cast<char *>(args.key) ? const_cast<char *>(args.key) : "", args.key_len);

    gp->meta_desc.desc.end_key_len = static_cast<uint32_t>(args.end_key_len);
    if ( gp->SetIovSize(GetReq<TestIov>::end_key_idx, gp->meta_desc.desc.end_key_len) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " end_key too big" << SoeLogger::LogError() << endl;
        return -1;
    }
    ::memcpy(gp->meta_desc.desc.end_key, const_cast<char *>(args.end_key) ? const_cast<char *>(args.end_key) : "", args.end_key_len);

    gp->meta_desc.desc.data_len = static_cast<uint32_t>(args.data_len);
    if ( gp->SetIovSize(GetReq<TestIov>::data_idx, gp->meta_desc.desc.data_len) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " Data too big" << SoeLogger::LogError() << endl;
        return -1;
    }
    ::memcpy(gp->meta_desc.desc.data, const_cast<char *>(args.data) ? const_cast<char *>(args.data) : "", args.data_len);

    gp->meta_desc.desc.buf_len = static_cast<uint32_t>(args.buf_len);
    if ( gp->SetIovSize(GetReq<TestIov>::buf_idx, gp->meta_desc.desc.buf_len) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " buf too big" << SoeLogger::LogError() << endl;
        return -1;
    }
    ::memcpy(gp->meta_desc.desc.buf, const_cast<char *>(args.buf) ? const_cast<char *>(args.buf) : "", args.buf_len);

    gp->HitchIds();

    return 0;
}

int StoreProxy1::PrepareRecvDesc(LocalArgsDescriptor &args, IovMsgBase *iov)
{
    SoeLogger::Logger logger;
    IovMsgReq<TestIov> *gr = dynamic_cast<IovMsgReq<TestIov> *>(iov);

    args.seq_number = gr->iovhdr.seq;

    // Cluster/Space/Store elements
    if ( !gr->meta_desc.desc.cluster_name || strcmp(gr->meta_desc.desc.cluster_name, cluster.c_str()) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " invalid args.cluster_name: " <<
            gr->meta_desc.desc.cluster_name << " cluster: " << cluster << SoeLogger::LogError() << endl;
        return -2;
    }
    args.cluster_name = &cluster;

    if ( gr->meta_desc.desc.cluster_id != cluster_id ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " invalid args.cluster_id: " <<
            gr->meta_desc.desc.cluster_id << " cluster_id: " << cluster_id << SoeLogger::LogError() << endl;
        return -2;
    }
    args.cluster_id = gr->meta_desc.desc.cluster_id;

    if ( !gr->meta_desc.desc.space_name || strcmp(gr->meta_desc.desc.space_name, space.c_str()) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " invalid args.space_name: " <<
            gr->meta_desc.desc.space_name << " space: " << space << SoeLogger::LogError() << endl;
        return -2;
    }
    args.space_name = &space;

    if ( gr->meta_desc.desc.space_id != space_id ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " invalid args.space_id: " <<
            gr->meta_desc.desc.space_id << " space_id: " << space_id << SoeLogger::LogError() << endl;
        return -2;
    }
    args.space_id = gr->meta_desc.desc.space_id;

    if ( !gr->meta_desc.desc.store_name || strcmp(gr->meta_desc.desc.store_name, name.c_str()) ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " invalid args.store_name: " <<
            gr->meta_desc.desc.store_name << " name: " << name << SoeLogger::LogError() << endl;
        return -2;
    }
    args.store_name = &name;

    if ( gr->meta_desc.desc.store_id != id ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << " invalid args.store_id: " <<
            gr->meta_desc.desc.store_id << " id: " << id << SoeLogger::LogError() << endl;
        return -2;
    }
    args.store_id = gr->meta_desc.desc.store_id;

    // Data elements
    if ( gr->GetIovSize(GetReq<TestIov>::key_idx, gr->meta_desc.desc.key_len) ) {
        return -1;
    }
    args.key_len = static_cast<size_t>(gr->meta_desc.desc.key_len);
    args.key = gr->meta_desc.desc.key;

    if ( gr->GetIovSize(GetReq<TestIov>::end_key_idx, gr->meta_desc.desc.end_key_len) ) {
        return -1;
    }
    args.end_key_len = gr->meta_desc.desc.end_key_len;
    args.end_key = gr->meta_desc.desc.end_key;

    if ( gr->GetIovSize(GetReq<TestIov>::data_idx, gr->meta_desc.desc.data_len) ) {
        return -1;
    }
    args.data_len = static_cast<size_t>(gr->meta_desc.desc.data_len);
    args.data = gr->meta_desc.desc.data;

    if ( gr->GetIovSize(GetReq<TestIov>::buf_idx, gr->meta_desc.desc.buf_len) ) {
        return -1;
    }
    args.buf = gr->meta_desc.desc.buf;
    args.buf_len = static_cast<size_t>(gr->meta_desc.desc.buf_len);  // indicate there is that much buffer available

    return 0;
}

uint32_t StoreProxy1::processAsyncInboundOp(LocalArgsDescriptor &args, uint32_t req_type, RcsdbFacade::Exception::eStatus &sts) throw(RcsdbFacade::Exception)
{
    uint32_t rsp_type = eMsgInvalidRsp;
    SoeLogger::Logger logger;

    try {
        if ( state == StoreProxy1::State::eDeleted ) {
            StoreProxy1 *garbage_proxy = this;
            StoreProxy1::GarbageCollectStore(garbage_proxy, GetId(), args);
            throw RcsdbFacade::Exception("Store already closed or deleted", RcsdbFacade::Exception::eStatus::eOpAborted);
        }

        switch ( req_type ) {
        case eMsgGetAsyncReq:
        {
            args.buf_len = IovMsgBase::iov_buf_size;
            rsp_type = eMsgGetAsyncRsp;
            Get(args);
        }
        break;
        case eMsgGetAsyncRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgPutAsyncReq:
        {
            rsp_type = eMsgPutAsyncRsp;
            Put(args);
        }
        break;
        case eMsgPutAsyncRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgDeleteAsyncReq:
        {
            rsp_type = eMsgDeleteAsyncRsp;
            Delete(args);
        }
        break;
        case eMsgDeleteAsyncRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgGetRangeAsyncReq:
        {
            rsp_type = eMsgGetRangeAsyncRsp;
            GetRange(args);
        }
        break;
        case eMsgGetRangeAsyncRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgMergeAsyncReq:
        {
            rsp_type = eMsgMergeAsyncRsp;
            Merge(args);
        }
        break;
        case eMsgMergeAsyncRsp:
        case eMsgGetAsyncReqNT:
        case eMsgGetAsyncRspNT:
        case eMsgPutAsyncReqNT:
        case eMsgPutAsyncRspNT:
        case eMsgDeleteAsyncReqNT:
        case eMsgDeleteAsyncRspNT:
        case eMsgGetRangeAsyncReqNT:
        case eMsgGetRangeAsyncRspNT:
        case eMsgMergeAsyncReqNT:
        case eMsgMergeAsyncRspNT:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        default:
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
            break;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( req_type == eMsgIterateReq ) {
            *reinterpret_cast<uint32_t *>(const_cast<char *>(args.data)) = static_cast<uint32_t>(args.transaction_db);
            args.data_len = sizeof(uint32_t);
        }
        if ( req_type == eMsgPutReq ) {
            args.data_len = 0;
        }

        if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Op failed type: " << req_type <<
            " what: " << ex.what() << " sts: " << ex.status << " args.transaction_db: " << args.transaction_db <<
            " args.data_len: " << args.data_len << SoeLogger::LogDebug() << endl;
        sts = ex.status;
    } catch ( ... ) {
        sts = RcsdbFacade::Exception::eInternalError;
    }

#ifdef __DEBUG1_ASYNC_IO__
    {
        WriteLock io_w_lock(StoreProxy1::io_debug_lock);
        if ( args.end_key_len == 2*sizeof(uint64_t) ) {
            uint64_t a_ptrs[2];
            a_ptrs[0] = *reinterpret_cast<const uint64_t *>(args.end_key);
            a_ptrs[1] = *reinterpret_cast<const uint64_t *>(args.end_key + sizeof(uint64_t));
            cerr << __FUNCTION__ << " seq=0x" << hex << args.seq_number << dec << " pos: " << a_ptrs[0] << " th: " << a_ptrs[1] << endl;
        }
    }
#endif

    return rsp_type;
}

uint32_t StoreProxy1::processSyncInboundOp(LocalArgsDescriptor &args, uint32_t req_type, RcsdbFacade::Exception::eStatus &sts) throw(RcsdbFacade::Exception)
{
    uint32_t rsp_type = eMsgInvalidRsp;
    SoeLogger::Logger logger;

    try {
        if ( state == StoreProxy1::State::eDeleted ) {
            StoreProxy1 *garbage_proxy = this;
            StoreProxy1::GarbageCollectStore(garbage_proxy, GetId(), args);
            throw RcsdbFacade::Exception("Store already closed or deleted", RcsdbFacade::Exception::eStatus::eOpAborted);
        }

        switch ( req_type ) {
        case eMsgAssignMMChannelReq:
        case eMsgAssignMMChannelRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgCreateClusterReq:
        case eMsgCreateClusterRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgCreateSpaceReq:
        case eMsgCreateSpaceRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgCreateStoreReq:
        case eMsgCreateStoreRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgDeleteClusterReq:
        case eMsgDeleteClusterRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgDeleteSpaceReq:
        case eMsgDeleteSpaceRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgDeleteStoreReq:
        case eMsgDeleteStoreRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgCreateBackupEngineReq:
        {
            rsp_type = eMsgCreateBackupEngineRsp;
            LocalAdmArgsDescriptor adm_args;
            ::memset(&adm_args, '\0', sizeof(adm_args));
            MakeAdmArgsFromStdArgs(adm_args, args);
            string be(args.key, args.key_len);
            adm_args.snapshot_name = &be; // bckp_path
            adm_args.cluster_idx = args.sync_mode; // incremental
            string bn(args.end_key, args.end_key_len);
            adm_args.backup_name = &bn;
            if ( args.data_len >= sizeof(adm_args.sync_mode) ) {
                adm_args.sync_mode = *reinterpret_cast<const uint32_t *>(args.data);
            }

            CreateBackupEngine(adm_args);

            // the result is sent back as data (backup_idx, backup_id)
            *reinterpret_cast<uint32_t *>(const_cast<char *>(args.data)) = static_cast<uint32_t>(adm_args.backup_idx);
            args.data_len = sizeof(adm_args.backup_idx);
            *reinterpret_cast<uint32_t *>(const_cast<char *>(args.data) + args.data_len) = static_cast<uint32_t>(adm_args.backup_id);
            args.data_len += sizeof(adm_args.backup_id);
        }
        break;
        case eMsgCreateBackupEngineRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgDeleteBackupEngineReq:
        {
            rsp_type = eMsgDeleteBackupEngineRsp;
            LocalAdmArgsDescriptor adm_args;
            ::memset(&adm_args, '\0', sizeof(adm_args));
            MakeAdmArgsFromStdArgs(adm_args, args);
            string be(args.key, args.key_len);
            adm_args.snapshot_name = &be; // bckp_path
            if ( args.data_len >= sizeof(adm_args.backup_idx) + sizeof(adm_args.backup_id) ) {
                adm_args.backup_idx = *reinterpret_cast<const uint32_t *>(args.data);
                adm_args.backup_id = *reinterpret_cast<const uint32_t *>(args.data + sizeof(adm_args.backup_idx));
            }
            string bn(args.end_key, args.end_key_len);
            adm_args.backup_name = &bn;

            DestroyBackupEngine(adm_args);
        }
        break;
        case eMsgDeleteBackupEngineRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgCreateBackupReq:
        case eMsgCreateBackupRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgDeleteBackupReq:
        case eMsgDeleteBackupRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgListBackupsReq:
        case eMsgListBackupsRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgCreateSnapshotEngineReq:
        case eMsgCreateSnapshotEngineRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgCreateSnapshotReq:
        case eMsgCreateSnapshotRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgDeleteSnapshotReq:
        case eMsgDeleteSnapshotRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgListSnapshotsReq:
        case eMsgListSnapshotsRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgOpenStoreReq:
        {
            rsp_type = eMsgOpenStoreRsp;
            Open(args);
            (void )io_session->IncOpenCloseRefCount();
        }
        break;
        case eMsgOpenStoreRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgCloseStoreReq:
        {
            rsp_type = eMsgCloseStoreRsp;
            Close(args);
            (void )io_session->DecOpenCloseRefCount();
        }
        break;
        case eMsgCloseStoreRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgGetReq:
        {
            args.buf_len = IovMsgBase::iov_buf_size;
            rsp_type = eMsgGetRsp;
            Get(args);
        }
        break;
        case eMsgGetRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgPutReq:
        {
            rsp_type = eMsgPutRsp;
            Put(args);
        }
        break;
        case eMsgPutRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgDeleteReq:
        {
            rsp_type = eMsgDeleteRsp;
            Delete(args);
        }
        break;
        case eMsgDeleteRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgGetRangeReq:
        {
            rsp_type = eMsgGetRangeRsp;
            GetRange(args);
        }
        break;
        case eMsgGetRangeRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgMergeReq:
        {
            rsp_type = eMsgMergeRsp;
            Merge(args);
        }
        break;
        case eMsgMergeRsp:
        case eMsgGetReqNT:
        case eMsgGetRspNT:
        case eMsgPutReqNT:
        case eMsgPutRspNT:
        case eMsgDeleteReqNT:
        case eMsgDeleteRspNT:
        case eMsgGetRangeReqNT:
        case eMsgGetRangeRspNT:
        case eMsgMergeReqNT:
        case eMsgMergeRspNT:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgBeginTransactionReq:
        {
            rsp_type = eMsgBeginTransactionRsp;
            LocalAdmArgsDescriptor adm_args;
            ::memset(&adm_args, '\0', sizeof(adm_args));
            MakeAdmArgsFromStdArgs(adm_args, args);

            StartTransaction(adm_args);

            args.transaction_idx = adm_args.transaction_idx;

            // the result is sent back as data
            *reinterpret_cast<uint32_t *>(const_cast<char *>(args.data)) = static_cast<uint32_t>(args.transaction_idx);
            args.data_len = sizeof(args.transaction_idx);
        }
        break;
        case eMsgBeginTransactionRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgCommitTransactionReq:
        {
            rsp_type = eMsgCommitTransactionRsp;
            LocalAdmArgsDescriptor adm_args;
            ::memset(&adm_args, '\0', sizeof(adm_args));
            MakeAdmArgsFromStdArgs(adm_args, args);

            // Transaction Idx, sent as data
            args.transaction_idx = *reinterpret_cast<const uint32_t *>(args.data + args.data_len - sizeof(uint32_t));
            args.data_len -= sizeof(uint32_t);

            adm_args.transaction_idx = args.transaction_idx;

            CommitTransaction(adm_args);
        }
        break;
        case eMsgCommitTransactionRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << " Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgAbortTransactionReq:
        {
            rsp_type = eMsgAbortTransactionRsp;
            LocalAdmArgsDescriptor adm_args;
            ::memset(&adm_args, '\0', sizeof(adm_args));
            MakeAdmArgsFromStdArgs(adm_args, args);

            // Transaction Idx, sent as data
            args.transaction_idx = *reinterpret_cast<const uint32_t *>(args.data + args.data_len - sizeof(uint32_t));
            args.data_len -= sizeof(uint32_t);

            adm_args.transaction_idx = args.transaction_idx;

            RollbackTransaction(adm_args);
        }
        break;
        case eMsgAbortTransactionRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgMetaAd:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgCreateIteratorReq:
        {
            rsp_type = eMsgCreateIteratorRsp;
            CreateIterator(args);
        }
        break;
        case eMsgCreateIteratorRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgDeleteIteratorReq:
        {
            rsp_type = eMsgDeleteIteratorRsp;
            DestroyIterator(args);
        }
        break;
        case eMsgDeleteIteratorRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgIterateReq:
        {
            rsp_type = eMsgIterateRsp;
            Iterate(args);
        }
        break;
        case eMsgIterateRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgCreateGroupReq:
        {
            rsp_type = eMsgCreateGroupRsp;
            CreateGroup(args);
        }
        break;
        case eMsgCreateGroupRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgDeleteGroupReq:
        {
            rsp_type = eMsgDeleteGroupRsp;
            DestroyGroup(args);
        }
        break;
        case eMsgDeleteGroupRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        case eMsgWriteGroupReq:
        {
            rsp_type = eMsgWriteGroupRsp;
            WriteGroup(args);
        }
        break;
        case eMsgWriteGroupRsp:
        {
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
        }
        break;
        default:
            if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Invalid type: " << req_type << SoeLogger::LogDebug() << endl;
            sts = RcsdbFacade::Exception::eInvalidArgument;
            break;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( req_type == eMsgIterateReq ) {
            *reinterpret_cast<uint32_t *>(const_cast<char *>(args.data)) = static_cast<uint32_t>(args.transaction_db);
            args.data_len = sizeof(uint32_t);
        }
        if ( req_type == eMsgPutReq ) {
            args.data_len = 0;
        }

        if ( debug == true ) logger.Clear(CLEAR_DEFAULT_ARGS) << "Op failed type: " << req_type <<
            " what: " << ex.what() << " sts: " << ex.status << " args.transaction_db: " << args.transaction_db <<
            " args.data_len: " << args.data_len << SoeLogger::LogDebug() << endl;
        sts = ex.status;
    } catch ( ... ) {
        sts = RcsdbFacade::Exception::eInternalError;
    }

    return rsp_type;
}

//
// Dynamic StoreProxy1
//
void StoreProxy1::Open(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name <<
            " id: " << id << " space: " << space << " space_id: " << space_id <<
            " cluster: " << cluster << " cluster_id: " << cluster_id << SoeLogger::LogDebug() << endl;
    }

    acs.open(args);
    state = StoreProxy1::State::eOpened;
}

void StoreProxy1::OnDisconnectClose() throw(RcsdbFacade::Exception)
{
    LocalArgsDescriptor args;
    ::memset(&args, '\0', sizeof(args));
    args.group_idx = -1;
    args.transaction_idx = -1;
    args.iterator_idx = -1;

    Close(args);
}

void StoreProxy1::Close(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << name <<
            " id: " << id << " space: " << space << " space_id: " << space_id <<
            " cluster: " << cluster << " cluster_id: " << cluster_id << SoeLogger::LogDebug() << endl;
    }

    acs.close(args);
    state = StoreProxy1::State::eClosed;
}

void StoreProxy1::Repair(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    acs.repair(args);
}

void StoreProxy1::DecodeGroupTransactionIdx(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    // Check for group or transaction Idx
    if ( args.buf_len == 2*sizeof(uint32_t) ) {
        uint32_t *idx_buf = reinterpret_cast<uint32_t *>(args.buf);
        args.buf_len -= 2*sizeof(uint32_t);
        if ( idx_buf[1] != 0xa5a5a5a5 ) {
            if ( idx_buf[0] == LD_group_type_idx ) {
                args.group_idx = idx_buf[1];
            } else if ( idx_buf[0] == LD_transaction_type_idx ) {
                args.transaction_idx = idx_buf[1];
            } else {
                // If either value is !0 - error
                if ( idx_buf[0] || idx_buf[1] ) {
                    throw RcsdbFacade::Exception("Invalid idx type", RcsdbFacade::Exception::eInvalidArgument);
                }
            }
        }
    }
}

void StoreProxy1::Put(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }

    // Decode for group or transaction Idx
    DecodeGroupTransactionIdx(args);

    acs.put(args);
}

void StoreProxy1::Get(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    acs.get(args);
}

void StoreProxy1::Delete(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)

{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }

    // Decode for group or transaction Idx
    DecodeGroupTransactionIdx(args);

    acs.del(args);
}

void StoreProxy1::GetRange(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }

    acs.get_range(args);
}

void StoreProxy1::Merge(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }

    // Decode for group or transaction Idx
    DecodeGroupTransactionIdx(args);

    acs.merge(args);
}

void StoreProxy1::Iterate(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }

    // reset keys to empty if sent as LD_Unbounded_key
    if ( args.key && *reinterpret_cast<const uint64_t *>(args.key) == LD_Unbounded_key ) {
        args.key_len = 0;
        *const_cast<char *>(args.key) = '\0';
    }

    // Iterator Op type and Idx, sent as data
    args.sync_mode = *reinterpret_cast<const uint32_t *>(args.data + args.data_len - sizeof(args.sync_mode));
    args.data_len -= sizeof(args.sync_mode);
    args.iterator_idx = *reinterpret_cast<const uint32_t *>(args.data + args.data_len - sizeof(args.iterator_idx));
    args.data_len -= sizeof(args.iterator_idx);

    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " s_m " << args.sync_mode << " d_l " << args.data_len <<
            " it_idx " << args.iterator_idx << " k_l " << args.key_len << SoeLogger::LogDebug() << endl;
    }

    acs.iterate(args);

    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " tr_db " << args.transaction_db << SoeLogger::LogDebug() << endl;
    }

    *reinterpret_cast<uint32_t *>(const_cast<char *>(args.data)) = static_cast<uint32_t>(args.transaction_db);
    args.data_len = sizeof(uint32_t);
}

void StoreProxy1::CreateGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }

    acs.create_group(args);

    // the result is sent back as data
    *reinterpret_cast<uint32_t *>(const_cast<char *>(args.data)) = static_cast<uint32_t>(args.group_idx);
    args.data_len = sizeof(args.group_idx);
}

void StoreProxy1::DestroyGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }

    // Group Idx, sent as data
    args.group_idx = *reinterpret_cast<const uint32_t *>(args.data + args.data_len - sizeof(args.group_idx));
    args.data_len -= sizeof(args.group_idx);

    acs.destroy_group(args);
}

void StoreProxy1::WriteGroup(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }

    // Group Idx, sent as data
    args.group_idx = *reinterpret_cast<const uint32_t *>(args.data + args.data_len - sizeof(args.group_idx));
    args.data_len -= sizeof(args.group_idx);

    acs.write_group(args);
}

void StoreProxy1::MakeAdmArgsFromStdArgs(LocalAdmArgsDescriptor &adm_args, const LocalArgsDescriptor &args)
{
    adm_args.cluster_name = args.cluster_name;
    adm_args.cluster_id = args.cluster_id;
    adm_args.cluster_idx = args.cluster_idx;

    adm_args.space_name = args.space_name;
    adm_args.space_id = args.space_id;
    adm_args.space_idx = args.space_idx;

    adm_args.store_name = args.store_name;
    adm_args.store_id = args.store_id;
    adm_args.store_idx = args.store_idx;

    adm_args.sync_mode = args.sync_mode;

    adm_args.transaction_idx = args.transaction_idx;
}

void StoreProxy1::StartTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    acs.start_transaction(args);
}

void StoreProxy1::CommitTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    acs.commit_transaction(args);
}

void StoreProxy1::RollbackTransaction(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    acs.rollback_transaction(args);
}

void StoreProxy1::CreateSnapshotEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    acs.create_snapshot_engine(args);
}

void StoreProxy1::CreateSnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    acs.create_snapshot(args);
}

void StoreProxy1::DestroySnapshot(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    acs.destroy_snapshot(args);
}

void StoreProxy1::ListSnapshots(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    acs.list_snapshots(args);
}

void StoreProxy1::CreateBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    acs.create_backup_engine(args);
}

void StoreProxy1::CreateBackup(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    acs.create_backup(args);
}

void StoreProxy1::DestroyBackupEngine(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    acs.destroy_backup_engine(args);
}

void StoreProxy1::ListBackups(LocalAdmArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }
    acs.list_backups(args);
}

void StoreProxy1::CreateIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }

    // reset key/end_key to empty if sent as LD_Unbounded_key
    if ( args.key && *reinterpret_cast<const uint64_t *>(args.key) == LD_Unbounded_key ) {
        args.key_len = 0;
        *const_cast<char *>(args.key) = '\0';
    }
    if ( args.end_key && *reinterpret_cast<const uint64_t *>(args.end_key) == LD_Unbounded_key ) {
        args.end_key_len = 0;
        *const_cast<char *>(args.end_key) = '\0';
    }

    // Iterator dir and type
    args.overwrite_dups = static_cast<bool>(*reinterpret_cast<uint32_t *>(args.buf + args.buf_len - sizeof(uint32_t)));
    args.buf_len -= sizeof(uint32_t);
    args.cluster_idx = *reinterpret_cast<uint32_t *>(args.buf + args.buf_len - sizeof(uint32_t));
    args.buf_len -= sizeof(uint32_t);

    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " o_d " << args.overwrite_dups << " d_l " << args.data_len <<
            " cl_idx " << args.cluster_idx << " k_l " << args.key_len << " e_k_l " << args.end_key_len << SoeLogger::LogDebug() << endl;
    }

    acs.create_iterator(args);

    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " it_idx " << args.iterator_idx << SoeLogger::LogDebug() << endl;
    }

    // the result is sent back as data
    *reinterpret_cast<uint32_t *>(const_cast<char *>(args.data)) = static_cast<uint32_t>(args.iterator_idx);
    args.data_len = sizeof(args.iterator_idx);
}

void StoreProxy1::DestroyIterator(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
    }

    // Iterator Id
    args.iterator_idx = *reinterpret_cast<const uint32_t *>(args.data);

    acs.destroy_iterator(args);
}

void StoreProxy1::ResetAcs()
{
}

void StoreProxy1::BindApiFunctions() throw(RcsdbFacade::Exception)
{
}

void StoreProxy1::BindStaticApiFunctions() throw(RcsdbFacade::Exception)
{
}




//
// Static SpaceProxy1
//
RcsdbAccessors1<Metadbsrv::Space, Metadbsrv::Space>  SpaceProxy1::global_acs;

uint64_t SpaceProxy1::CreateSpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    SoeLogger::Logger logger;

    uint64_t s_id = SpaceProxy1::global_acs.create_fun(args);
    if ( ClusterProxy1::global_debug == true ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Space create: " << s_id << " name: " << *args.space_name <<
            " cluster: " << *args.cluster_name << SoeLogger::LogDebug() << endl;
    }

    if ( s_id != SpaceProxy1::invalid_id ) {
        SpaceProxy1 *sp_new = new SpaceProxy1(*args.cluster_name,
            *args.space_name,
            args.cluster_id,
            s_id,
            ClusterProxy1::global_debug);
        bool ins_ret = ClusterProxy1::StaticAddSpaceProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name), *sp_new);
        if ( ins_ret == false ) {
            delete sp_new;
            s_id = SpaceProxy1::invalid_id;
        }
    }

    return s_id;
}

void SpaceProxy1::DestroySpace(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    SpaceProxy1::global_acs.destroy_fun(args);
    if ( ClusterProxy1::global_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Space destroy: name: " << *args.space_name <<
            " cluster: " << *args.cluster_name << SoeLogger::LogDebug() << endl;
    }

    bool del_ret = ClusterProxy1::StaticDeleteSpaceProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name),
        ProxyMapKey1(args.space_id, ClusterProxy1::no_thread_id, *args.space_name));
    if ( del_ret == false ) {
        args.space_id = SpaceProxy1::invalid_id;
    }
}

uint64_t SpaceProxy1::GetSpaceByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    SoeLogger::Logger logger;

    uint64_t s_id = SpaceProxy1::global_acs.get_by_name_fun(args);
    if ( ClusterProxy1::global_debug == true ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Space get_by_name: " << s_id << " name: " << *args.space_name <<
            " cluster: " << *args.cluster_name << SoeLogger::LogDebug() << endl;
    }

    // create SpaceProxy1 if space exists
    if ( s_id != SpaceProxy1::invalid_id ) {
        SpaceProxy1 *s_pr = ClusterProxy1::StaticFindSpaceProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name),
            ProxyMapKey1(s_id, ClusterProxy1::no_thread_id, *args.space_name));
        if ( !s_pr ) {
            SpaceProxy1 *sp_new = new SpaceProxy1(*args.cluster_name,
                *args.space_name,
                args.cluster_id,
                s_id,
                ClusterProxy1::global_debug);
            bool ins_ret = ClusterProxy1::StaticAddSpaceProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name), *sp_new);
            if ( ins_ret == false ) {
                delete sp_new;
                s_id = SpaceProxy1::invalid_id;
            }
        }
    }

    return s_id;
}

void SpaceProxy1::BindStaticApiFunctions() throw(RcsdbFacade::Exception)
{
}

bool SpaceProxy1::AddStoreProxy(StoreProxy1 &st)
{
    WriteLock w_lock(SpaceProxy1::map_lock);

    ProxyMapKey1 map_key(st.id, st.thread_id, st.name);
    pair<map<ProxyMapKey1, StoreProxy1 *>::iterator, bool> ret;
    ret = stores.insert(pair<ProxyMapKey1, StoreProxy1 *>(map_key, &st));
    if ( ret.second == false ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Store already exists id: " <<
            st.id << " th_id: " << st.thread_id << " name: " << st.name << SoeLogger::LogError() << endl;
        return false;
    }

    return true;
}

StoreProxy1 *SpaceProxy1::RemoveStoreProxy(const ProxyMapKey1 &key)
{
    WriteLock w_lock(SpaceProxy1::map_lock);

    map<ProxyMapKey1, StoreProxy1 *>::iterator it;
    it = stores.find(key);
    if ( it == stores.end() ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Store not found id: " <<
            key.i_val << " th_id: " << key.t_tid << " name: " << key.s_val << SoeLogger::LogError() << endl;
        return 0;
    }

    SpaceProxy1::stores.erase(it);

    return (*it).second;
}

bool SpaceProxy1::DeleteStoreProxy(const ProxyMapKey1 &key)
{
    WriteLock w_lock(SpaceProxy1::map_lock);

    map<ProxyMapKey1, StoreProxy1 *>::iterator it;
    it = stores.find(key);
    if ( it == stores.end() ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Store not found id: " <<
            key.i_val << " th_id: " << key.t_tid << " name: " << key.s_val << SoeLogger::LogError() << endl;
        return false;
    }

    SpaceProxy1::stores.erase(it);
    delete (*it).second;
    return true;
}

StoreProxy1 *SpaceProxy1::FindStoreProxy(const ProxyMapKey1 &key)
{
    WriteLock w_lock(SpaceProxy1::map_lock);

    map<ProxyMapKey1, StoreProxy1 *>::iterator it;
    it = stores.find(key);
    if ( it == stores.end() ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Store not found id: " <<
            key.i_val << " th_id: " << key.t_tid << " name: " << key.s_val << SoeLogger::LogError() << endl;
        return 0;
    }

    return (*it).second;
}

bool SpaceProxy1::FindAllStoreProxies(const ProxyMapKey1 &st_key, std::list<StoreProxy1 *> &removed_proxies)
{
    WriteLock w_lock(SpaceProxy1::map_lock);

    uint32_t count = 0;
    for ( std::map<ProxyMapKey1, StoreProxy1 *>::iterator it = stores.begin() ; it != stores.end() ; it++ ) {
        if ( (*it).second->name == st_key.s_val && (*it).second->id == st_key.i_val ) {
            removed_proxies.push_back((*it).second);
            count++;
        }
    }

    return count ? true : false;
}



ClusterProxy1::Initializer *static_initializer_ptr1 = 0;

bool ClusterProxy1::Initializer::initialized = false;
Lock ClusterProxy1::global_lock;
Lock ClusterProxy1::static_op_lock;
ClusterProxy1::Initializer initalized1;

void StaticInitializerEnter1(void)
{
    static_initializer_ptr1 = &initalized1;
}

void StaticInitializerExit1(void)
{
    static_initializer_ptr1 = 0;
}

RcsdbAccessors1<Metadbsrv::Cluster, Metadbsrv::Cluster>  ClusterProxy1::global_acs;

map<ProxyMapKey1, ClusterProxy1 &> ClusterProxy1::clusters;
Lock ClusterProxy1::clusters_map_lock;

std::list<StoreProxy1 *> ClusterProxy1::archive;

ClusterProxy1::Initializer::Initializer()
{
    WriteLock w_lock(ClusterProxy1::global_lock);

    if ( Initializer::initialized == false ) {
        try {
            ClusterProxy1::BindStaticApiFunctions();
            SpaceProxy1::BindStaticApiFunctions();
            StoreProxy1::BindStaticApiFunctions();
        } catch (const RcsdbFacade::Exception &ex) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Registerstatic functions failed: " << SoeLogger::LogError() << endl;
        }
    }
    Initializer::initialized = true;
}

ClusterProxy1::Initializer::~Initializer()
{
    WriteLock w_lock(ClusterProxy1::global_lock);

    if ( Initializer::initialized == true ) {
        if ( ClusterProxy1::global_debug == true ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
        }
    }
    Initializer::initialized = false;
}

bool ClusterProxy1::global_debug = false;
bool ClusterProxy1::msg_debug = false;
uint64_t ClusterProxy1::adm_msg_counter = 0;

//
// Static ClusterProxy1
//
uint64_t ClusterProxy1::CreateCluster(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    uint64_t c_id = ClusterProxy1::global_acs.create_fun(args);
    if ( ClusterProxy1::global_debug ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Cluster create: " << c_id << " name: " << *args.cluster_name << SoeLogger::LogDebug() << endl;
    }

    if ( c_id != ClusterProxy1::invalid_id ) {
        ClusterProxy1 *cl_new = new ClusterProxy1(*args.cluster_name, c_id, ClusterProxy1::global_debug);
        bool ins_ret = ClusterProxy1::StaticAddClusterProxy(*cl_new);
        if ( ins_ret == false ) {
            delete cl_new;
            c_id = ClusterProxy1::invalid_id;
        }
    }

    return c_id;
}

void ClusterProxy1::DestroyCluster(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
   ClusterProxy1::global_acs.destroy_fun(args);
    if ( ClusterProxy1::global_debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Cluster destroy: name: " << *args.cluster_name << SoeLogger::LogDebug() << endl;
    }

    bool del_ret = ClusterProxy1::StaticDeleteClusterProxy(ProxyMapKey1(args.cluster_id, ClusterProxy1::no_thread_id, *args.cluster_name));
    if ( del_ret == false ) {
        args.cluster_id = ClusterProxy1::invalid_id;
    }
}

uint64_t ClusterProxy1::GetClusterByName(LocalArgsDescriptor &args) throw(RcsdbFacade::Exception)
{
    SoeLogger::Logger logger;

   uint64_t c_id = ClusterProxy1::global_acs.get_by_name_fun(args);
    if ( ClusterProxy1::global_debug == true ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Cluster get_by_name: " << c_id << " name: " << *args.cluster_name << SoeLogger::LogDebug() << endl;
    }

    // create ClusterProxy1 if cluster exists
    if ( c_id != ClusterProxy1::invalid_id ) {
        ClusterProxy1 *c_pr = StaticFindClusterProxy(ProxyMapKey1(c_id, ClusterProxy1::no_thread_id, *args.cluster_name));
        if ( !c_pr ) {
            ClusterProxy1 *cl_new = new ClusterProxy1(*args.cluster_name, c_id, ClusterProxy1::global_debug);
            bool ins_ret = ClusterProxy1::StaticAddClusterProxy(*cl_new);
            if ( ins_ret == false ) {
                delete cl_new;
                c_id = ClusterProxy1::invalid_id;
            }
        }
    }

    return c_id;
}

void ClusterProxy1::BindStaticApiFunctions() throw(RcsdbFacade::Exception)
{
}

void ClusterProxy1::ListSubsets(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception)
{
    Cluster::ListSubsets(args);
}

void ClusterProxy1::ListStores(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception)
{
    Cluster::ListStores(args);
}

void ClusterProxy1::ListSpaces(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception)
{
    Cluster::ListSpaces(args);
}

void ClusterProxy1::ListClusters(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception)
{
    Cluster::ListClusters(args);
}

void ClusterProxy1::ListBackupStores(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception)
{
    Cluster::ListBackupStores(args);
}

void ClusterProxy1::ListBackupSpaces(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception)
{
    Cluster::ListBackupSpaces(args);
}

void ClusterProxy1::ListBackupClusters(LocalArgsDescriptor &args, const IPv4Address &from_addr, uint64_t _adm_s_id) throw(RcsdbFacade::Exception)
{
    Cluster::ListBackupClusters(args);
}

//
// -----------------------------------------------
//
bool ClusterProxy1::AddSpaceProxy(SpaceProxy1 &sp)
{
    WriteLock w_lock(ClusterProxy1::map_lock);

    ProxyMapKey1 map_key(sp.id, ClusterProxy1::no_thread_id, sp.name);
    pair<map<ProxyMapKey1, SpaceProxy1 &>::iterator, bool> ret;
    ret = spaces.insert(pair<ProxyMapKey1, SpaceProxy1 &>(map_key, sp));
    if ( ret.second == false ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Space already exists id: " <<
             sp.id << " td_id: " << ClusterProxy1::no_thread_id << " name: " << sp.name << SoeLogger::LogError() << endl;
        return false;
    }

    return true;
}

bool ClusterProxy1::DeleteSpaceProxy(const ProxyMapKey1 &key)
{
    WriteLock w_lock(ClusterProxy1::map_lock);

    map<ProxyMapKey1, SpaceProxy1 &>::iterator it;
    it = spaces.find(key);
    if ( it == spaces.end() ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Space not found id: " <<
            key.i_val << " th_id: " << key.t_tid << " name: " << key.s_val << SoeLogger::LogError() << endl;
        return false;
    }

    ClusterProxy1::spaces.erase(it);
    return true;
}

SpaceProxy1 *ClusterProxy1::FindSpaceProxy(const ProxyMapKey1 &key)
{
    WriteLock w_lock(ClusterProxy1::map_lock);

    map<ProxyMapKey1, SpaceProxy1 &>::iterator it;
    it = spaces.find(key);
    if ( it == spaces.end() ) {
        if ( key.t_tid != ClusterProxy1::no_thread_id ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Space not found " << key.i_val << " th_id: " <<
                key.t_tid << " name: " << key.s_val << SoeLogger::LogError() << endl;
        }
        return 0;
    }

    return &(*it).second;
}

//
// -----------------------------------------------
//
bool ClusterProxy1::StaticAddClusterProxy1NoLock(ClusterProxy1 &cp)
{
    ProxyMapKey1 map_key(cp.id, ClusterProxy1::no_thread_id, cp.name);
    pair<map<ProxyMapKey1, ClusterProxy1 &>::iterator, bool> ret;
    ret = ClusterProxy1::clusters.insert(pair<ProxyMapKey1, ClusterProxy1 &>(map_key, cp));
    if ( ret.second == false ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster already exists id: " <<
            cp.id << " th_id: " << ClusterProxy1::no_thread_id << " name: " << cp.name << SoeLogger::LogError() << endl;
        return false;
    }

    return true;
}

bool ClusterProxy1::StaticAddClusterProxy(ClusterProxy1 &cp, bool lock_clusters_map)
{
    if ( lock_clusters_map == true ) {
        WriteLock w_lock(ClusterProxy1::clusters_map_lock);
        return ClusterProxy1::StaticAddClusterProxy1NoLock(cp);
    }
    return ClusterProxy1::StaticAddClusterProxy1NoLock(cp);
}


bool ClusterProxy1::StaticDeleteClusterProxy1NoLock(const ProxyMapKey1 &key)
{
    map<ProxyMapKey1, ClusterProxy1 &>::iterator it;
    it = ClusterProxy1::clusters.find(key);
    if ( it == ClusterProxy1::clusters.end() ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found id: " <<
            key.i_val << " th_id: " << key.t_tid << " name: " << key.s_val << SoeLogger::LogError() << endl;
        return false;
    }

    ClusterProxy1::clusters.erase(it);
    return true;
}

bool ClusterProxy1::StaticDeleteClusterProxy(const ProxyMapKey1 &key, bool lock_clusters_map)
{
    if ( lock_clusters_map == true ) {
        WriteLock w_lock(ClusterProxy1::clusters_map_lock);
        return ClusterProxy1::StaticDeleteClusterProxy1NoLock(key);
    }
    return ClusterProxy1::StaticDeleteClusterProxy1NoLock(key);
}


ClusterProxy1 *ClusterProxy1::StaticFindClusterProxy1NoLock(const ProxyMapKey1 &key)
{
    map<ProxyMapKey1, ClusterProxy1 &>::iterator it;
    it = ClusterProxy1::clusters.find(key);
    if ( it == ClusterProxy1::clusters.end() ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found id: "
            << key.i_val << " th_id: " << key.t_tid << " name: " << key.s_val << SoeLogger::LogError() << endl;
        return 0;
    }

    return &(*it).second;
}

ClusterProxy1 *ClusterProxy1::StaticFindClusterProxy(const ProxyMapKey1 &key, bool lock_clusters_map)
{
    if ( lock_clusters_map == true ) {
        WriteLock w_lock(ClusterProxy1::clusters_map_lock);
        return ClusterProxy1::StaticFindClusterProxy1NoLock(key);
    }
    return ClusterProxy1::StaticFindClusterProxy1NoLock(key);
}

//
// -----------------------------------------------
//
bool ClusterProxy1::StaticAddSpaceProxy(const ProxyMapKey1 &cl_key, SpaceProxy1 &sp)
{
    WriteLock w_lock(ClusterProxy1::clusters_map_lock);

    ClusterProxy1 *cl_p = ClusterProxy1::StaticFindClusterProxy(cl_key, false);
    if ( !cl_p ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found id: " <<
            cl_key.i_val << " th_id: " << cl_key.t_tid << " name: " << cl_key.s_val << SoeLogger::LogError() << endl;
        return false;
    }

    sp.parent = cl_p;
    return cl_p->AddSpaceProxy(sp);
}

bool ClusterProxy1::StaticDeleteSpaceProxy(const ProxyMapKey1 &cl_key, const ProxyMapKey1 &sp_key)
{
    WriteLock w_lock(ClusterProxy1::clusters_map_lock);

    ClusterProxy1 *cl_p = ClusterProxy1::StaticFindClusterProxy(cl_key, false);
    if ( !cl_p ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found id: " <<
            cl_key.i_val << " th_id: " << cl_key.t_tid << " name: " << cl_key.s_val << SoeLogger::LogError() << endl;
        return false;
    }

    return cl_p->DeleteSpaceProxy(sp_key);
}

SpaceProxy1 *ClusterProxy1::StaticFindSpaceProxy(const ProxyMapKey1 &cl_key, const ProxyMapKey1 &sp_key)
{
    WriteLock w_lock(ClusterProxy1::clusters_map_lock);

    ClusterProxy1 *cl_p = ClusterProxy1::StaticFindClusterProxy(cl_key, false);
    if ( !cl_p ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found id: " <<
            cl_key.i_val << " th_id: " << cl_key.t_tid << " name: " << cl_key.s_val << SoeLogger::LogError() << endl;
        return 0;
    }

    return cl_p->FindSpaceProxy(sp_key);
}

//
// -----------------------------------------------
//
bool ClusterProxy1::StaticAddStoreProxy(const ProxyMapKey1 &cl_key, const ProxyMapKey1 &sp_key, StoreProxy1 &st)
{
    WriteLock w_lock(ClusterProxy1::clusters_map_lock);

    ClusterProxy1 *f_cl = ClusterProxy1::StaticFindClusterProxy(cl_key, false);
    if ( !f_cl ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found for id: " << cl_key.i_val <<
            " th_id: " << cl_key.t_tid << " name: " << cl_key.s_val << SoeLogger::LogError() << endl;
        return false;
    }

    SpaceProxy1 *f_sp = f_cl->FindSpaceProxy(sp_key);
    if ( !f_sp ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Space not found for id: " << sp_key.i_val <<
            " th_id: " << sp_key.t_tid << " name: " << sp_key.s_val << SoeLogger::LogError() << endl;
        return false;
    }

    st.parent = f_sp;
    return f_sp->AddStoreProxy(st);
}

bool ClusterProxy1::StaticDeleteStoreProxy(const ProxyMapKey1 &cl_key, const ProxyMapKey1 &sp_key, const ProxyMapKey1 &st_key)
{
    WriteLock w_lock(ClusterProxy1::clusters_map_lock);

    ClusterProxy1 *f_cl = ClusterProxy1::StaticFindClusterProxy(cl_key, false);
    if ( !f_cl ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found for id: " << cl_key.i_val <<
            " th_id: " << cl_key.t_tid << " name: " << cl_key.s_val << SoeLogger::LogError() << endl;
        return false;
    }

    SpaceProxy1 *f_sp = f_cl->FindSpaceProxy(sp_key);
    if ( !f_sp ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Space not found for id: " << sp_key.i_val <<
            " th_id: " << sp_key.t_tid << " name: " << sp_key.s_val << SoeLogger::LogError() << endl;
        return false;
    }

    return f_sp->DeleteStoreProxy(st_key);
}

StoreProxy1 *ClusterProxy1::StaticRemoveStoreProxy(const ProxyMapKey1 &cl_key, const ProxyMapKey1 &sp_key, const ProxyMapKey1 &st_key)
{
    WriteLock w_lock(ClusterProxy1::clusters_map_lock);

    ClusterProxy1 *f_cl = ClusterProxy1::StaticFindClusterProxy(cl_key, false);
    if ( !f_cl ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found for id: " << cl_key.i_val <<
            " th_id: " << cl_key.t_tid << " name: " << cl_key.s_val << SoeLogger::LogError() << endl;
        return 0;
    }

    SpaceProxy1 *f_sp = f_cl->FindSpaceProxy(sp_key);
    if ( !f_sp ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Space not found for id: " << sp_key.i_val <<
            " th_id: " << sp_key.t_tid << " name: " << sp_key.s_val << SoeLogger::LogError() << endl;
        return 0;
    }

    return f_sp->RemoveStoreProxy(st_key);
}

bool ClusterProxy1::StaticFindAllStoreProxies(const ProxyMapKey1 &cl_key, const ProxyMapKey1 &sp_key, const ProxyMapKey1 &st_key,
    std::list<StoreProxy1 *> &removed_proxies)
{
    WriteLock w_lock(ClusterProxy1::clusters_map_lock);

    ClusterProxy1 *f_cl = ClusterProxy1::StaticFindClusterProxy(cl_key, false);
    if ( !f_cl ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found for id: " << cl_key.i_val <<
            " th_id: " << cl_key.t_tid << " name: " << cl_key.s_val << SoeLogger::LogError() << endl;
        return 0;
    }

    SpaceProxy1 *f_sp = f_cl->FindSpaceProxy(sp_key);
    if ( !f_sp ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Space not found for id: " << sp_key.i_val <<
            " th_id: " << sp_key.t_tid << " name: " << sp_key.s_val << SoeLogger::LogError() << endl;
        return 0;
    }

    return f_sp->FindAllStoreProxies(st_key, removed_proxies);
}

StoreProxy1 *ClusterProxy1::StaticFindStoreProxy(const ProxyMapKey1 &cl_key, const ProxyMapKey1 &sp_key, const ProxyMapKey1 &st_key)
{
    WriteLock w_lock(ClusterProxy1::clusters_map_lock);

    ClusterProxy1 *f_cl = ClusterProxy1::StaticFindClusterProxy(cl_key, false);
    if ( !f_cl ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Cluster not found for id: " << cl_key.i_val <<
            " th_id: " << cl_key.t_tid << " name: " << cl_key.s_val << SoeLogger::LogError() << endl;
        return 0;
    }

    SpaceProxy1 *f_sp = f_cl->FindSpaceProxy(sp_key);
    if ( !f_sp ) {
        if ( ClusterProxy1::msg_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Space not found for id: " << sp_key.i_val <<
            " th_id: " << sp_key.t_tid << " name: " << sp_key.s_val << SoeLogger::LogError() << endl;
        return 0;
    }

    return f_sp->FindStoreProxy(st_key);
}

void ClusterProxy1::PruneArchive()
{
#ifdef _GARBAGE_COLLECT_STORES_
    //vector<StoreProxy1 *> to_delete;
    StoreProxy1 *to_delete[100];
    uint32_t to_delete_cnt = 0;
    {
        WriteLock w_lock(ClusterProxy1::global_lock);

        if ( ClusterProxy1::archive.size() ) {
            //cout << "ClusterProxy1::" << __FUNCTION__ << " archive.size: " << ClusterProxy1::archive.size() << endl;
        }

        bool found_some = true;
        while ( found_some == true ) {
            found_some = false;
            for ( std::list<StoreProxy1 *>::iterator it = ClusterProxy1::archive.begin() ; it != ClusterProxy1::archive.end() ; it++ ) {
                if ( (*it)->in_archive_ttl++ > 20 ) {
                    //cout << "ClusterProxy1::" << __FUNCTION__ << " moving to delete container ... name: " << (*it)->name << " state: " <<
                        //(*it)->state << " in_archive_ttl: " << (*it)->in_archive_ttl <<  endl;

                    // remove from list and add to to_delete
                    (*it)->io_session = 0;
                    (*it)->io_session_async = 0;
                    ClusterProxy1::archive.erase(it);
                    //to_delete.push_back(*it);
                    to_delete[to_delete_cnt++] = *it;
                    if ( to_delete_cnt >= 100 ) {
                        found_some = false;
                    } else {
                        found_some = true;
                    }
                    break;
                }
            }
        }
    }

    // delete outside of Cluster::archive_lock to avoid deadlock, using the to_delte vector
    //for ( uint32_t i = 0 ; i < to_delete.size() ; i++ ) {
    for ( uint32_t i = 0 ; i < to_delete_cnt ; i++ ) {
        //cout << "ClusterProxy1::" << __FUNCTION__ << " deleting ... name: " << to_delete[i]->name << " id: " << to_delete[i]->id << endl;
        delete to_delete[i];
    }
    //to_delete.clear();
#endif
}

void ClusterProxy1::ArchiveStoreProxy(const StoreProxy1 &st)
{
    WriteLock w_lock(ClusterProxy1::global_lock);

    bool found = false;
    for ( std::list<StoreProxy1 *>::iterator it = ClusterProxy1::archive.begin() ; it != ClusterProxy1::archive.end() ; it++ ) {
        if ( st.name == (*it)->name &&
            st.space == (*it)->space &&
            st.id == (*it)->id &&
            st.space == (*it)->space &&
            st.space_id == (*it)->space_id &&
            st.cluster == (*it)->cluster &&
            st.cluster_id == (*it)->cluster_id &&
            st.thread_id == (*it)->thread_id ) {
            found = true;
            break;
        }
    }

    // if found we can't insert it so it becomes a leak
    if ( found == false ) {
        ClusterProxy1::archive.push_back(const_cast<StoreProxy1 *>(&st));
    }
}

}


