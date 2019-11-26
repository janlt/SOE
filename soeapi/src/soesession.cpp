/**
 * @file    soesession.cpp
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
 * soesession.cpp
 *
 *  Created on: Feb 3, 2017
 *      Author: Jan Lisowiec
 */



#include <sys/syscall.h>
#include <string.h>
#include <dlfcn.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <new>

#include "soeutil.h"
#include "soe_soe.hpp"

#define gettid() syscall(SYS_gettid)

using namespace std;
using namespace KvsFacade;
using namespace RcsdbFacade;

namespace Soe {

const SoeSessionStatusText Session::sts_texts[] = { SOE_SESSION_STATUS_TEXT_A };

const char *Session::StatusToText(Status sts)
{
    return sts_texts[sts].sts_text;
}

/**
 * @brief Session destructor
 */
Session::~Session()
{
    Close();

    // take care of futures by checking if there are still some not discarded yet
    // Although we close this session, if this is a metadbcli session, the client may not be aware
    // of it and still try to signal unsignelled futures
    if ( garbage_futures.size() > 0 ) {
        WriteLock w_lock(Session::global_garbage_futures_lock);

        size_t coll;
        for ( ;; ) {
            coll = 0;
            for ( auto it = garbage_futures.begin() ; it != garbage_futures.end() ; it++ ) {
                if ( dynamic_cast<SimpleFuture *>(*it) ) {
                    if ( (*it)->GetReceived() == true ) {
                        if ( (*it)->GetReceived() == true ) {
                            delete (*it);
                        } else {
                            Session::global_garbage_futures.push_back(*it);
                        }
                        garbage_futures.erase(it);
                    }
                } else if ( dynamic_cast<SetFuture *>(*it) ) {
                    SetFuture *set_f = dynamic_cast<SetFuture *>(*it);
                    size_t coll = DestroySetFuture(set_f, true);
                    if ( coll ) {
                        break;
                    }

                    // if all children have been received it's safe to delete fut, otherwise garbage collect it
                    if ( set_f->GetNumProcessed() >= set_f->GetNumRequested() ) {
                        delete (*it);
                        garbage_futures.erase(it);
                    }
                } else {
                    ostringstream str_buf;
                    str_buf << __FUNCTION__ << ":" << __LINE__ << " Unknown Soe future " << typeid(*it).name() << endl << flush;
                    logErrMsg(str_buf.str().c_str());
                }
            }
            if ( !coll ) {
                break;
            }
        }
    }

    DEBUG_FUTURES(cout << "Session::" << __FUNCTION__ << " pending_num_futures: " << pending_num_futures << endl);
}

Session::Status Session::TranslateConstExceptionStatus(const RcsdbFacade::Exception &ex) const
{
    return const_cast<Session *>(this)->TranslateExceptionStatus(ex);
}

Session::Status Session::TranslateExceptionStatus(const RcsdbFacade::Exception &ex)
{
    Session::Status sts = Session::Status::eOsError;
    switch ( ex.status ) {
    case RcsdbFacade::Exception::eOk:
        sts = Session::Status::eOk;
        break;
    case RcsdbFacade::Exception::eDuplicateKey:
        sts = Session::Status::eDuplicateKey;
        break;
    case RcsdbFacade::Exception::eNotFoundKey:
        sts = Session::Status::eNotFoundKey;
        break;
    case RcsdbFacade::Exception::eTruncatedValue:
        sts = Session::Status::eTruncatedValue;
        break;
    case RcsdbFacade::Exception::eStoreCorruption:
        sts = Session::Status::eStoreCorruption;
        break;
    case RcsdbFacade::Exception::eOpNotSupported:
        sts = Session::Status::eOpNotSupported;
        break;
    case RcsdbFacade::Exception::eInvalidArgument:
        sts = Session::Status::eInvalidArgument;
        break;
    case RcsdbFacade::Exception::eIOError:
        sts = Session::Status::eIOError;
        break;
    case RcsdbFacade::Exception::eMergeInProgress:
        sts = Session::Status::eMergeInProgress;
        break;
    case RcsdbFacade::Exception::eIncomplete:
        sts = Session::Status::eIncomplete;
        break;
    case RcsdbFacade::Exception::eShutdownInProgress:
        sts = Session::Status::eShutdownInProgress;
        break;
    case RcsdbFacade::Exception::eOpTimeout:
        sts = Session::Status::eOpTimeout;
        break;
    case RcsdbFacade::Exception::eOpAborted:
        sts = Session::Status::eOpAborted;
        break;
    case RcsdbFacade::Exception::eStoreBusy:
        sts = Session::Status::eStoreBusy;
        break;
    case RcsdbFacade::Exception::eItemExpired:
        sts = Session::Status::eItemExpired;
        break;
    case RcsdbFacade::Exception::eTryAgain:
        sts = Session::Status::eTryAgain;
        break;
    case RcsdbFacade::Exception::eInternalError:
        sts = Session::Status::eOsError;
        break;
    case RcsdbFacade::Exception::eOsError:
        sts = Session::Status::eOsError;
        break;
    case RcsdbFacade::Exception::eFoobar:
        sts = Session::Status::eFoobar;
        break;
    case RcsdbFacade::Exception::eOsResourceExhausted:
        sts = Session::Status::eOsResourceExhausted;
        break;
    case RcsdbFacade::Exception::eProviderResourceExhausted:
        sts = Session::Status::eProviderResourceExhausted;
        break;
    case RcsdbFacade::Exception::eInvalidStoreState:
        sts = Session::Status::eInvalidStoreState;
        break;
    case RcsdbFacade::Exception::eProviderUnreachable:
        sts = Session::Status::eProviderUnreachable;
        break;
    case RcsdbFacade::Exception::eChannelError:
        sts = Session::Status::eChannelError;
        break;
    case RcsdbFacade::Exception::eNetworkError:
        sts = Session::Status::eNetworkError;
        break;
    case RcsdbFacade::Exception::eNoFunction:
        sts = Session::Status::eNoFunction;
        break;
    case RcsdbFacade::Exception::eDuplicateIdDetected:
        sts = Session::Status::eDuplicateIdDetected;
        break;
    default:
        break;

    }
    //cout << "============== |" << ex.error << "|" << ex.status << "|" << sts << endl;
    return sts;
}


/**
 * @brief Session ok status
 *
 * Check session status
 *
 * @param[in,out]  sts        Pointer to Status value
 *                            If non-NULL it'll be populated on return with the status of last operation
 * @param[out] bool           "true" if session is eOk
 */
bool Session::ok(Status *sts) const
{
    if ( sts ) {
        *sts = (open_status == true ? Session::Status::eOk : Session::Status::eInternalError);
    }
    return open_status;
}

void Session::InitDesc(LocalArgsDescriptor &args) const
{
    ::memset(&args, '\0', sizeof(args));

    args.cluster_name = &cluster_name;
    args.cluster_id = cluster_id;
    args.space_name = &space_name;
    args.space_id = space_id;
    args.store_name = &container_name;
    args.store_id = container_id;
    args.transaction_db = transaction_db;
    args.group_idx = Group::invalid_group;
    args.transaction_idx = Transaction::invalid_transaction;
    args.sync_mode = sync_mode;
    args.bloom_bits = 8;
}

void Session::InitAdmDesc(LocalAdmArgsDescriptor &args) const
{
    ::memset(&args, '\0', sizeof(args));

    args.cluster_name = &cluster_name;
    args.cluster_id = cluster_id;
    args.space_name = &space_name;
    args.space_id = space_id;
    args.store_name = &container_name;
    args.store_id = container_id;
    args.snapshot_name = 0;
    args.transaction_idx = Transaction::invalid_transaction;
    args.sync_mode = sync_mode;
}

/**
 * @brief Session constructor
 *
 * Create an instance of Session
 *
 * @param[in]  c_n            Cluster name
 * @param[in]  s_n            Space name
 * @param[in]  cn_n           Container (store) name
 * @param[in]  pr             Provider name
 * @param[in]  transaction_db Transaction store ( "true" - transactions supported, "false" - transactions not supported)
 * @param[in]  debug          Print debug info
 * @param[in]  no_throw       Suppress exceptions
 * @param[in]  s_m            Sync mode (see soesession.hpp)
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Session(const string &c_n,
    const string &s_n,
    const string &cn_n,
    const string &pr,
    bool _transation_db,
    bool _debug,
    bool _no_throw,
    Session::eSyncMode s_m) throw(runtime_error)
        :provider(pr),
        cluster_name(c_n),
        space_name(s_n),
        container_name(cn_n),
        cluster_id(Session::invalid_64_id),
        space_id(Session::invalid_64_id),
        container_id(Session::invalid_64_id),
        cr_thread_id(static_cast<uint32_t>(gettid())),
        transaction_db(_transation_db),
        no_throw(_no_throw),
        debug(_debug),
        sync_mode(s_m),
        create_name_fun(0),
        pending_num_futures(0),
        open_status(false),
        last_status(Session::Status::eOk)
{
    this_sess = shared_ptr<Session>(this, Session::Deleter());
    create_name_fun = provider.CheckProvider();

    if ( reinterpret_cast<uint64_t>(create_name_fun) == 0 ) {
        if ( no_throw == true ) {
            create_name_fun = Provider::InvalidCreateLocalNameFun;
            return;
        } else {
            throw runtime_error((string("Invalid provider: ") + pr).c_str());
        }
        create_name_fun = Provider::InvalidCreateLocalNameFun;
    }
}

/**
 * @brief Session Open
 *
 * Open session to an existing {cluster,space,store,provider}
 *
 * @param[in]  c_n            Cluster name
 * @param[in]  s_n            Space name
 * @param[in]  cn_n           Container (store) name
 * @param[in]  pr             Provider name
 * @param[in]  transaction_db Transaction store ( "true" - transactions supported, "false" - transactions not supported)
 * @param[in]  debug          Print debug info
 * @param[in]  no_throw       Suppress exceptions
 * @param[in]  s_m            Sync mode (see soesession.hpp)
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::Open(const string &c_n,
    const string &s_n,
    const string &cn_n,
    const string &pr,
    bool _transaction_db,
    bool _debug,
    bool _no_throw,
    Session::eSyncMode s_m) throw(runtime_error)
{
    provider = Provider(pr);
    cluster_name = c_n;
    space_name = s_n;
    container_name = cn_n;
    no_throw = _no_throw;
    transaction_db = _transaction_db;
    debug = _debug;
    open_status = false;
    sync_mode = s_m;

    ostringstream str_buf;
    Session::Status ret = Session::Status::eOsError;

    create_name_fun = provider.CheckProvider();
    if ( !create_name_fun ) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << "No such provider: " << pr << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("No such provider: ") + pr).c_str());
        } else {
            last_status = Session::Status::eInvalidProvider;
            return Session::Status::eInvalidProvider;
        }
    }

    try {
        ret = OpenSession();
    } catch (const SoeApi::Exception &ex) {
        if ( no_throw != true ) {
            throw runtime_error((string("No Api function found: ") + ex.what()).c_str());
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Open failed(RcsdbFacade::Exception): " << ex.what() <<
                "sts: " << ex.status << " errno: " << ex.os_errno << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw ex;
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( no_throw != true ) {
            throw ex;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( no_throw != true ) {
            throw runtime_error((string("Create failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( ret == Session::Status::eOk ) {
        open_status = true;
    }
    if ( debug == true ) {
        str_buf << __FUNCTION__ << ":" << __LINE__ << "Open() sts: " << ret << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

/**
 * @brief Session Close
 *
 * Close session
 *
 */
void Session::Close()
{
    ostringstream str_buf;

    try {
        if ( acs.close ) {
            LocalArgsDescriptor args;
            InitDesc(args);
            args.group_idx = cr_thread_id;

            acs.close(args);
        } else {
            last_status = Session::Status::eNoFunction;
            if ( debug == true ) {
                str_buf << __FUNCTION__ << ":" << __LINE__ << "Close() no such function" << endl << flush;
                logErrMsg(str_buf.str().c_str());
            }
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Close failed(SoeApi): " << container_name << endl << flush;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        last_status = TranslateExceptionStatus(ex);
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Close failed(RcsdbFacade::Exception): " << ex.what() << endl << flush;
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Close failed(runtime_error): " << container_name << endl << flush;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Close failed(...): " << container_name << endl << flush;
        }
    }

    if ( debug == true ) {
        //cout << __FUNCTION__ << ":" << __LINE__ << "Closed: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }
}

/*
 * @brief Session Open
 *
 * Repair store associated with a session
 *
 * @param[out] status         return status (see soesession.hpp)
 *
 */
Session::Status Session::Repair() throw(runtime_error)
{
    ostringstream str_buf;
    Session::Status ret = Session::Status::eOsError;

    try {
        if ( acs.repair ) {
            LocalArgsDescriptor args;
            InitDesc(args);

            acs.repair(args);
            RcsdbFacade::Exception sts_ex(std::string(""), static_cast<RcsdbFacade::Exception::eStatus>(args.status));
            ret = TranslateExceptionStatus(sts_ex);
        } else {
            if ( debug == true ) {
                str_buf << __FUNCTION__ << ":" << __LINE__ << "Repair() no such function" << endl << flush;
                logErrMsg(str_buf.str().c_str());
            }
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Repair failed(SoeApi): " << container_name << endl << flush;
        }
        if ( no_throw != true ) {
            throw runtime_error(string("Cluster acs function failed " + ex.what()).c_str());
        } else {
            ret = Status::eFoobar;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Repair failed(RcsdbFacade::Exception): " << ex.what() <<
                " sts: " << ex.status << " errno: " << ex.os_errno << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string("Cluster acs function failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Repair failed(runtime_error): " << container_name << endl << flush;
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Status::eFoobar;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Repair failed(...): " << container_name << endl << flush;
        }
        if ( no_throw != true ) {
            throw runtime_error("Repair failed(...)failed ");
        } else {
            ret = Status::eFoobar;
        }
    }

    if ( debug == true ) {
        //cout << __FUNCTION__ << ":" << __LINE__ << "Repair: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

/*
 * @brief Session - list all the clusters for a given provider
 *
 * Return a list of all the clusters
 *
 * @param[in/out]  clusters       vector of clusters on return
 * @param[out]     status         return status (see soesession.hpp)
 *
 */
Session::Status Session::ListClusters(std::vector<std::pair<uint64_t, std::string> > &clusters, std::string backup_path) throw(std::runtime_error)
{
    Session::Status ret = Session::Status::eOsError;

    try {
        // get local static functions
        cluster_acs.list_clusters_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun("", "", "", 0, FUNCTION_LIST_CLUSTERS));

        LocalArgsDescriptor args;
        InitDesc(args);
        args.list = &clusters;
        if ( backup_path != "" ) {
            args.key = backup_path.c_str();
            args.key_len = backup_path.length();
        }
        args.status = static_cast<uint32_t>(debug);
        cluster_acs.list_clusters_fun(args);
        RcsdbFacade::Exception sts_ex(std::string(""), static_cast<RcsdbFacade::Exception::eStatus>(args.status));
        ret = TranslateExceptionStatus(sts_ex);

        if ( debug ) {
            ostringstream str_buf;
            str_buf << "Cluster avaialble for ops id: " << cluster_id << " name: " << cluster_name << endl << flush;
            logDbgMsg(str_buf.str().c_str());
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function: " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw runtime_error(string("Cluster acs function failed " + ex.what()).c_str());
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed(RcsdbFacade::Exception): " << ex.what() <<
                " status: " << ex.status <<
                " error: " << ex.error << " errno: " << ex.os_errno << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }

        if ( no_throw != true ) {
            throw runtime_error(string("Cluster acs function failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed " << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw;
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed " << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw runtime_error("Cluster acs function failed ");
    }

    return ret;
}

/*
 * @brief Session list spaces for a given cluster
 *
 * Return a list of all the clusters
 *
 * @param[in]      c_n            cluster name
 * @param[in]      c_id           cluster id
 * @param[in/out]  spaces         vector of spaces on return
 * @param[out]     status         return status (see soesession.hpp)
 *
 */
Session::Status Session::ListSpaces(const std::string &c_n, uint64_t c_id, std::vector<std::pair<uint64_t, std::string> > &spaces, std::string backup_path) throw(std::runtime_error)
{
    Session::Status ret = Session::Status::eOsError;

    try {
        // get local static functions
        cluster_acs.list_spaces_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun("", "", "", 0, FUNCTION_LIST_SPACES));

        LocalArgsDescriptor args;
        InitDesc(args);
        args.cluster_name = &c_n;
        args.cluster_id = c_id;
        args.list = &spaces;
        if ( backup_path != "" ) {
            args.key = backup_path.c_str();
            args.key_len = backup_path.length();
        }
        args.status = static_cast<uint32_t>(debug);
        cluster_acs.list_spaces_fun(args);
        RcsdbFacade::Exception sts_ex(std::string(""), static_cast<RcsdbFacade::Exception::eStatus>(args.status));
        ret = TranslateExceptionStatus(sts_ex);

        if ( debug ) {
            ostringstream str_buf;
            str_buf << "Cluster avaialble for ops id: " << cluster_id << " name: " << cluster_name << endl << flush;
            logDbgMsg(str_buf.str().c_str());
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function: " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw runtime_error(string("Cluster acs function failed " + ex.what()).c_str());
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed(RcsdbFacade::Exception): " << ex.what() <<
                " status: " << ex.status <<
                " error: " << ex.error << " errno: " << ex.os_errno << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }

        if ( no_throw != true ) {
            throw runtime_error(string("Cluster acs function failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed " << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw;
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed " << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw runtime_error("Cluster acs function failed ");
    }

    return ret;
}

/*
 * @brief Session list stores for a given cluster and space
 *
 * Return a list of all the clusters
 *
 * @param[in]      c_n            cluster name
 * @param[in]      c_id           cluster id
 * @param[in]      s_n            space name
 * @param[in]      s_id           space id
 * @param[in/out]  stores         vector of stores on return
 * @param[out]     status         return status (see soesession.hpp)
 *
 */
Session::Status Session::ListStores(const std::string &c_n, uint64_t c_id, const std::string &s_n, uint64_t s_id, std::vector<std::pair<uint64_t, std::string> > &stores, std::string backup_path) throw(std::runtime_error)
{
    Session::Status ret = Session::Status::eOsError;

    try {
        // get local static functions
        cluster_acs.list_stores_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun("", "", "", 0, FUNCTION_LIST_STORES));

        LocalArgsDescriptor args;
        InitDesc(args);
        args.cluster_name = &c_n;
        args.cluster_id = c_id;
        args.space_name = &s_n;
        args.space_id = s_id;
        args.list = &stores;
        if ( backup_path != "" ) {
            args.key = backup_path.c_str();
            args.key_len = backup_path.length();
        }
        args.status = static_cast<uint32_t>(debug);
        cluster_acs.list_stores_fun(args);
        RcsdbFacade::Exception sts_ex(std::string(""), static_cast<RcsdbFacade::Exception::eStatus>(args.status));
        ret = TranslateExceptionStatus(sts_ex);

        if ( debug ) {
            ostringstream str_buf;
            str_buf << "Cluster avaialble for ops id: " << cluster_id << " name: " << cluster_name << endl << flush;
            logDbgMsg(str_buf.str().c_str());
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function: " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw runtime_error(string("Cluster acs function failed " + ex.what()).c_str());
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed(RcsdbFacade::Exception): " << ex.what() <<
                " status: " << ex.status <<
                " error: " << ex.error << " errno: " << ex.os_errno << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }

        if ( no_throw != true ) {
            throw runtime_error(string("Cluster acs function failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed " << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw;
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed " << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw runtime_error("Cluster acs function failed ");
    }

    return ret;
}

/*
 * @brief Session list subsets for a given cluster, space and store
 *
 * Return a list of all the clusters
 *
 * @param[in]      c_n            cluster name
 * @param[in]      c_id           cluster id
 * @param[in]      s_n            space name
 * @param[in]      s_id           space id
 * @param[in]      t_n            store name
 * @param[in]      t_id           store id
 * @param[in/out]  subsets        vector of subsets on return
 * @param[out]     status         return status (see soesession.hpp)
 *
 */
Session::Status Session::ListSubsets(const std::string &c_n, uint64_t c_id, const std::string &s_n, uint64_t s_id, const std::string &t_n, uint64_t t_id, std::vector<std::pair<uint64_t, std::string> > &subsets) throw(std::runtime_error)
{
    Session::Status ret = Session::Status::eOsError;

    try {
        // get local static functions
        cluster_acs.list_subsets_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun("", "", "", 0, FUNCTION_LIST_SUBSETS));

        LocalArgsDescriptor args;
        InitDesc(args);
        args.cluster_name = &c_n;
        args.cluster_id = c_id;
        args.space_name = &s_n;
        args.space_id = s_id;
        args.store_name = &t_n;
        args.store_id = t_id;
        args.list = &subsets;
        args.status = static_cast<uint32_t>(debug);
        cluster_acs.list_subsets_fun(args);
        RcsdbFacade::Exception sts_ex(std::string(""), static_cast<RcsdbFacade::Exception::eStatus>(args.status));
        ret = TranslateExceptionStatus(sts_ex);

        if ( debug ) {
            ostringstream str_buf;
            str_buf << "Cluster avaialble for ops id: " << cluster_id << " name: " << cluster_name << endl << flush;
            logDbgMsg(str_buf.str().c_str());
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function: " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw runtime_error(string("Cluster acs function failed " + ex.what()).c_str());
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed(RcsdbFacade::Exception): " << ex.what() <<
                " status: " << ex.status <<
                " error: " << ex.error << " errno: " << ex.os_errno << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }

        if ( no_throw != true ) {
            throw runtime_error(string("Cluster acs function failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed " << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw;
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed " << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw runtime_error("Cluster acs function failed ");
    }

    return ret;
}

void Session::CreateCluster() throw(runtime_error)
{
    try {
        // get local static functions to find cluster by name and create one if not found
        cluster_acs.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(create_name_fun("", "", "", 0, FUNCTION_CREATE));
        cluster_acs.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun("", "", "", 0, FUNCTION_DESTROY));
        cluster_acs.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(create_name_fun("", "", "", 0, FUNCTION_GET_BY_NAME));

        LocalArgsDescriptor args;
        InitDesc(args);
        args.status = static_cast<uint32_t>(debug);
        cluster_id = cluster_acs.get_by_name_fun(args);
        if ( cluster_id == Session::invalid_64_id ) {
            args.cluster_id = Session::invalid_64_id;
            cluster_id = cluster_acs.create_fun(args);
        }
        if ( debug ) {
            ostringstream str_buf;
            str_buf << "Cluster avaialble for ops id: " << cluster_id << " name: " << cluster_name << endl << flush;
            logDbgMsg(str_buf.str().c_str());
        }

        // get object functions to manipulate a cluster
        string bind_cluster_name = cluster_name;
        if ( provider.GetName() != Provider::GetLocalName() ) {
            bind_cluster_name = "static_cluster_name";
        }
        cluster_acs.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(bind_cluster_name, "", "", 0, FUNCTION_OPEN));
        cluster_acs.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(bind_cluster_name, "", "", 0, FUNCTION_CLOSE));
        cluster_acs.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(bind_cluster_name, "", "", 0, FUNCTION_PUT));
        cluster_acs.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(bind_cluster_name, "", "", 0, FUNCTION_GET));
        cluster_acs.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(bind_cluster_name, "", "", 0, FUNCTION_DELETE));
        cluster_acs.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(bind_cluster_name, "", "", 0, FUNCTION_GET_RANGE));
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function: " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw runtime_error(string("Cluster acs function failed " + ex.what()).c_str());
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed(RcsdbFacade::Exception): " << ex.what() <<
                " status: " << ex.status <<
                " error: " << ex.error << " errno: " << ex.os_errno << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        char errno_buf[32];
        sprintf(errno_buf, "%d %u", ex.os_errno, ex.status);
        throw runtime_error(string(errno_buf).c_str());
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed " << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw;
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Cluster acs function failed " << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw runtime_error("Cluster acs function failed ");
    }
}

void Session::CreateSpace() throw(runtime_error)
{
    try {
        // get local static functions to find space by name and create one if not found
        space_acs.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(STATIC_CLUSTER, "", "", 0, FUNCTION_CREATE));
        space_acs.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(STATIC_CLUSTER, "", "", 0, FUNCTION_DESTROY));
        space_acs.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(STATIC_CLUSTER, "", "", 0, FUNCTION_GET_BY_NAME));

        LocalArgsDescriptor args;
        InitDesc(args);
        args.status = static_cast<uint32_t>(debug);
        space_id = space_acs.get_by_name_fun(args);
        if ( space_id == Session::invalid_64_id ) {
            args.space_id = Session::invalid_64_id;
            space_id = space_acs.create_fun(args);
        }
        if ( debug ) {
            ostringstream str_buf;
            str_buf << "Space avaialble for ops id: " << space_id << " name: " << space_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }

        // get object functions to manipulate a space
        string bind_cluster_name = cluster_name;
        string bind_space_name = space_name;
        if ( provider.GetName() != Provider::GetLocalName() ) {
            bind_cluster_name = "static_cluster_name";
            bind_space_name = "static_space_name";
        }
        space_acs.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(bind_cluster_name, bind_space_name, "", 0, FUNCTION_OPEN));
        space_acs.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(bind_cluster_name, bind_space_name, "", 0, FUNCTION_CLOSE));
        space_acs.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(bind_cluster_name, bind_space_name, "", 0, FUNCTION_PUT));
        space_acs.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(bind_cluster_name, bind_space_name, "", 0, FUNCTION_GET));
        space_acs.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(bind_cluster_name, bind_space_name, "", 0, FUNCTION_DELETE));
        space_acs.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(bind_cluster_name, bind_space_name, "", 0, FUNCTION_GET_RANGE));
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Space acs function: " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw runtime_error(string(" Space acs function failed " + ex.what()).c_str());
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Space acs function failed(RcsdbFacade::Exception): " << ex.what() <<
                " status: " << ex.status <<
                " error: " << ex.error << " errno: " << ex.os_errno << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        char errno_buf[16];
        sprintf(errno_buf, "%d %u", ex.os_errno, ex.status);
        throw runtime_error(string(errno_buf).c_str());
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Space acs function failed " << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw;
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Space acs function failed " << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        throw runtime_error("Space acs function failed ");
    }
}

void Session::CreateStore(bool create_con) throw(runtime_error)
{
    ostringstream str_buf;

    // get local static functions
    try {
        acs.create_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs create_fun: " << ex.what() << endl << flush; }
    }

    try {
        acs.destroy_fun = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs destroy_fun: " << ex.what() << endl << flush; }
    }

    try {
        acs.get_by_name_fun = SoeApi::Api<uint64_t(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs get_by_name_fun: " << ex.what() << endl << flush; }
    }

    // we need to handle possible RcsdbFacade::Exception here since the exception specification is too narrow
    try {
        if ( create_con == true ) {
            LocalArgsDescriptor args;
            InitDesc(args);
            args.group_idx = cr_thread_id; // overloaded field
            args.status = static_cast<uint32_t>(debug);
            container_id = acs.get_by_name_fun(args);
            args.group_idx = Group::invalid_group;
            if ( container_id == Session::invalid_64_id ) {
                args.store_id = Session::invalid_64_id;
                args.group_idx = cr_thread_id; // overloaded field
                container_id = acs.create_fun(args);
                args.group_idx = Group::invalid_group;
                //soe_log(SOE_DEBUG, "######## args.s_id(%lu) args.s_id(%lu) args.c_id(%lu)\n", args.cluster_id, args.space_id, args.store_id);
                cluster_id = args.cluster_id;
                space_id = args.space_id;
                container_id = args.store_id;
            }
            if ( debug ) {
                if ( debug ) { str_buf << "Store avaialble for ops id: " << container_id << " name: " << container_name << endl << flush; }
            }
        } else {
            LocalArgsDescriptor args;
            InitDesc(args);
            args.group_idx = cr_thread_id; // overloaded field
            (void )acs.get_by_name_fun(args);
            args.group_idx = Group::invalid_group;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        throw runtime_error(string(" CreateStore failed " + ex.what()).c_str());
    }


    // get object functions to manipulate a container
    try {
        acs.open = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_OPEN));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs open: " << ex.what() << endl << flush; }
    }

    try {
        acs.close = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_CLOSE));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs close: " << ex.what() << endl << flush; }
    }

    try {
        acs.repair = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_REPAIR));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs repair: " << ex.what() << endl << flush; }
    }

    try {
        acs.put = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_PUT));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs put: " << ex.what() << endl << flush; }
    }

    try {
        acs.get = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_GET));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs get: " << ex.what() << endl << flush; }
    }

    try {
        acs.del = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_DELETE));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs del: " << ex.what() << endl << flush; }
    }

    try {
        acs.get_range = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_GET_RANGE));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf<< __FUNCTION__ << ":" << __LINE__ << " Acs get_range: " << ex.what() << endl << flush; }
    }

    try {
        acs.merge = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_MERGE));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs merge: " << ex.what() << endl << flush; }
    }

    try {
        acs.iterate = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_ITERATE));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs iterate: " << ex.what() << endl << flush; }
    }

    try {
        acs.start_transaction = SoeApi::Api<void(LocalAdmArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_START_TRANSACTION));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs start_transaction: " << ex.what() << endl << flush; }
    }

    try {
        acs.commit_transaction = SoeApi::Api<void(LocalAdmArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_COMMIT_TRANSACTION));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs commit_transaction: " << ex.what() << endl << flush; }
    }

    try {
        acs.rollback_transaction = SoeApi::Api<void(LocalAdmArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_ROLLBACK_TRANSACTION));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs rollback_transaction: " << ex.what() << endl << flush; }
    }

    try {
        acs.create_snapshot_engine = SoeApi::Api<void(LocalAdmArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_CREATE_SNAPSHOT_ENGINE));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs create_snapshot engine: " << ex.what() << endl << flush; }
    }

    try {
        acs.create_snapshot = SoeApi::Api<void(LocalAdmArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_CREATE_SNAPSHOT));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs create_snapshot: " << ex.what() << endl << flush; }
    }

    try {
        acs.destroy_snapshot = SoeApi::Api<void(LocalAdmArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_DESTROY_SNAPSHOT));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs destroy_snapshot: " << ex.what() << endl << flush; }
    }

    try {
        acs.list_snapshots = SoeApi::Api<void(LocalAdmArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_LIST_SNAPSHOTS));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs list_snapshots: " << ex.what() << endl << flush; }
    }

    try {
        acs.create_backup_engine = SoeApi::Api<void(LocalAdmArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_CREATE_BACKUP_ENGINE));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs create_backup_engine: " << ex.what() << endl << flush; }
    }

    try {
        acs.create_backup = SoeApi::Api<void(LocalAdmArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_CREATE_BACKUP));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs create_backup: " << ex.what() << endl << flush; }
    }

    try {
        acs.restore_backup = SoeApi::Api<void(LocalAdmArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_RESTORE_BACKUP));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs restore_backup: " << ex.what() << endl << flush; }
    }

    try {
        acs.destroy_backup = SoeApi::Api<void(LocalAdmArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_DESTROY_BACKUP));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs destroy_backup: " << ex.what() << endl << flush; }
    }

    try {
        acs.verify_backup = SoeApi::Api<void(LocalAdmArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_VERIFY_BACKUP));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs verify_backup: " << ex.what() << endl << flush; }
    }

    try {
        acs.destroy_backup_engine = SoeApi::Api<void(LocalAdmArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_DESTROY_BACKUP_ENGINE));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs destroy_backup_engine: " << ex.what() << endl << flush; }
    }

    try {
        acs.list_backups = SoeApi::Api<void(LocalAdmArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_LIST_BACKUPS));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs list_backup_engines: " << ex.what() << endl << flush; }
    }

    try {
        acs.create_group = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_CREATE_GROUP));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs create_group: " << ex.what() << endl << flush; }
    }

    try {
        acs.destroy_group = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_DESTROY_GROUP));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs destroy_group: " << ex.what() << endl << flush; }
    }

    try {
        acs.write_group = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_WRITE_GROUP));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs write_group: " << ex.what() << endl << flush; }
    }

    try {
        acs.create_iterator = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_CREATE_ITERATOR));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs create_iterator: " << ex.what() << endl << flush; }
    }

    try {
        acs.destroy_iterator = SoeApi::Api<void(LocalArgsDescriptor &)>::SearchFunction(create_name_fun(cluster_name, space_name, container_name, cr_thread_id, FUNCTION_DESTROY_ITERATOR));
    } catch (const SoeApi::Exception &ex) {
        if ( debug ) { str_buf << __FUNCTION__ << ":" << __LINE__ << " Acs destroy_iterator: " << ex.what() << endl << flush; }
    }

    if ( debug ) {
        logDbgMsg(str_buf.str().c_str());
    }
}

Session::Status Session::OpenSession() throw(runtime_error)
{
    Session::Status ret = Session::Status::eOsError;
    ostringstream str_buf;

    if ( cluster_id != Session::invalid_64_id && space_id != Session::invalid_64_id && container_id != Session::invalid_64_id ) {
        return Session::Status::eInvalidArgument;
    }

    try {
        CreateStore(false);

        if ( acs.open ) {
            LocalArgsDescriptor args;
            InitDesc(args);
            args.group_idx = cr_thread_id; // overloaded field
            args.status = static_cast<uint32_t>(debug); // overloaded field

            acs.open(args);
            args.group_idx = Group::invalid_group;
            //soe_log(SOE_DEBUG, "######## args.s_id(%lu) args.s_id(%lu) args.c_id(%lu)\n", args.cluster_id, args.space_id, args.store_id);
            cluster_id = args.cluster_id;
            space_id = args.space_id;
            container_id = args.store_id;
            ret = Session::Status::eOk;
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Open failed(SoeApi): " << container_name << endl << flush;
        }
        if ( no_throw != true ) {
            throw runtime_error((string("No Api function found: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Open failed(RcsdbFacade::Exception): " << ex.what() <<
                " error: " << ex.error << " errno: " << ex.os_errno << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" Open failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Open failed(runtime_error): " << container_name << endl << flush;
        }
        if ( no_throw != true ) {
            throw ex;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Open failed(...): " << container_name << endl << flush;
        }
        if ( no_throw != true ) {
            throw runtime_error((string("Create failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        str_buf << __FUNCTION__ << ":" << __LINE__ << "OpenSession() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

/**
 * @brief Session Create
 *
 * Create session to a new {cluster,space,store,provider}
 * If {cluster,space,store,provider} exists, it's equivalent to Open()
 *
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::Create() throw(runtime_error)
{
    Session::Status ret = Session::Status::eOsError;
    ostringstream str_buf;

    try {
        CreateCluster();
        CreateSpace();
        CreateStore();

        LocalArgsDescriptor args;
        InitDesc(args);
        args.group_idx = cr_thread_id; // overloaded field
        args.status = static_cast<uint32_t>(debug);

        acs.open(args);
        args.group_idx = Group::invalid_group;
        ret = Session::eOk;
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Create failed(SoeApi): " << ex.what() << endl << flush;
        }
        if ( no_throw != true ) {
            throw runtime_error((string("Create failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Create failed(runtime_error): " << ex.what() << endl << flush;
        }
        if ( no_throw != true ) {
            throw;
        } else {
            // parse the error to get the status
            int errno_ret;
            int ret_status;
            sscanf(ex.what(), "%d %d", &errno_ret, &ret_status);
            ret = static_cast<Session::Status>(ret_status);
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Create failed(runtime_error): " << ex.what() <<
                " error: " << ex.error << " errno: " << ex.os_errno << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" Create failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch ( ... ) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Create failed(...): " << container_name << endl << flush;
        }
        if ( no_throw != true ) {
            throw runtime_error((string("Create failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( ret == Session::eOk ) {
        open_status = true;
    }

    if ( debug == true ) {
        str_buf << __FUNCTION__ << ":" << __LINE__ << "Create() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

/**
 * @brief Session DestroyStore
 *
 * Destroy store associated with a session
 * Physically deletes store files. No active session may be present at
 * the time a store is being destroyed.
 *
 * @param[out] status         return status (see soesession.hpp)
 *
 */
Session::Status Session::DestroyStore() throw(runtime_error)
{
    Session::Status ret = Session::Status::eOsError;
    ostringstream str_buf;

    try {
        LocalArgsDescriptor args;
        InitDesc(args);
        args.group_idx = cr_thread_id; // overloaded field
        args.space_id = space_id;

        if ( acs.destroy_fun ) {
            acs.destroy_fun(args);
            args.group_idx = Group::invalid_group;
            ret = Session::eOk;
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyStore failed(SoeApi): " << ex.what() << endl << flush;
        }
        if ( no_throw != true ) {
            throw runtime_error((string("DestroyStore failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyStore failed(runtime_error): " << ex.what() << endl << flush;
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyStore failed(RcsdbFacade::Exception): " << ex.what() <<
                " error: " << ex.error << " errno: " << ex.os_errno << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" DestroyStore failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch ( ... ) {
        if ( debug == true ) {
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyStore failed(...): " << container_name << endl << flush;
        }
        if ( no_throw != true ) {
            throw runtime_error((string("DestroyStore failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( ret == Session::eOk ) {
        try {
            (void )Close();
        } catch ( ... ) {
            ret = Session::Status::eOk;
        }
        open_status = false;
    }

    if ( debug == true ) {
        str_buf << __FUNCTION__ << ":" << __LINE__ << "Destroy() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

/**
 * @brief Session Put
 *
 * Put KV pair (item) to a store associated with session
 * If a key doesn't exist it'll be created, if it does eDuplicateKey will be returned
 *
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[in]  data           Data
 * @param[in]  data_size      Data length
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::Put(const char *key, size_t key_size, const char *data, size_t data_size, bool over_dups) throw(runtime_error)
{
    LocalArgsDescriptor args;
    InitDesc(args);
    args.key = key;
    args.key_len = key_size;
    args.data = data;
    args.data_len = data_size;
    args.overwrite_dups = over_dups;

    return Put(args);
}

/**
 * @brief Session Get
 *
 * Get KV pair (item) from a store associated with session
 *
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[in]  buf            Buffer for data, in case of eTruncatedValue return status will
 *                            be filled in with portion of the value since some applications can work with that
 * @param[in]  buf_size       Buffer size pointer
 *                            It'll be set the the length of the value when the return status is either eOk or eTruncatedValue
 *                            The caller has to check the return status and decide if it' ok to proceed with truncated value
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::Get(const char *key, size_t key_size, char *buf, size_t *buf_size) throw(runtime_error)
{
    LocalArgsDescriptor args;
    InitDesc(args);
    args.key = key;
    args.key_len = key_size;
    args.buf = buf;
    args.buf_len = *buf_size;

    Status ret = Get(args);
    if ( ret == Session::eOk || ret == Session::eTruncatedValue ) {
        *buf_size = args.buf_len;
    }
    last_status = ret;
    return ret;
}

/**
 * @brief Session Delete
 *
 * Delete KV pair (item) from a store associated with session
 *
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::Delete(const char *key, size_t key_size) throw(runtime_error)
{
    LocalArgsDescriptor args;
    InitDesc(args);
    args.key = key;
    args.key_len = key_size;

    return Delete(args);
}

/**
 * @brief Session Merge
 *
 * Merge KV pair (item) with store items based on merge filter
 *
 * The result of this operation for default filter is the same as Put, i.e.
 * when a key doesn't exist it'll be created, it it does it'l be updated
 *
 *
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[in]  data           Data
 * @param[in]  data_size      Data length
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::Merge(const char *key, size_t key_size, const char *data, size_t data_size) throw(runtime_error)
{
    LocalArgsDescriptor args;
    InitDesc(args);
    args.key = key;
    args.key_len = key_size;
    args.data = data;
    args.data_len = data_size;

    return Merge(args);
}

/**
 * @brief Session GetRange
 *
 * Get range of KV pairs (items) falling within range key to end_key
 *
 * @param[in]  start_key      Start key
 * @param[in]  start_key_size Start key length
 * @param[in]  end_key        End key
 * @param[in]  end_key_size   End key length
 * @param[in]  buf            Buffer for callback
 * @param[in]  buf_size       Buffer size
 * @param[in]  r_cb           Range callback (called by Soe on each item within the range)
 * @param[in]  ctx            Context passed to callback
 * @param[in]  dir            Iteration direction
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::GetRange(const char *start_key,
    size_t start_key_size,
    const char *end_key,
    size_t end_key_size,
    char *buf,
    size_t *buf_size,
    RangeCallback r_cb,
    void *ctx,
    Iterator::eIteratorDir dir) throw(runtime_error)
{
    LocalArgsDescriptor args;
    InitDesc(args);
    args.key = start_key;
    args.end_key = end_key;
    args.key_len = start_key_size;
    args.end_key_len = end_key_size;
    args.data = start_key;
    args.data_len = start_key_size;
    args.range_callback = r_cb;
    args.range_callback_arg = static_cast<char *>(ctx);
    args.overwrite_dups = (dir == Iterator::eIteratorDir::eIteratorDirForward ? true : false);
    args.iterator_idx = (ctx == this ? Store_range_delete : Subset_range_delete);

    return GetRange(args);
}

/**
 * @brief Session GetSet
 *
 * Get set of KV pairs (items) whose keys are passed in as a vector
 *
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_nfound  Specifies whether or not to fail the function on the first
 *                                   not found key
 * @param[in,out]  fail_pos          Position within items with not found key that caused a failure
 * @param[out] status                Return status (see soesession.hpp)
 *                                   If items.size() == 0 the return status is eInvalidArgument
 *                                   If fail_on_first_nfound == false and items.size() != 0 the return status is eOk
 *                                   if fail_on_first_nfound == true and items.size() != 0 and a key in keys is not found
 *                                   the return status is eNotFoundKey and fail_pos is set to first failed index
 */
Session::Status Session::GetSet(vector<pair<Duple, Duple>> &items,
    bool fail_on_first_nfound,
    size_t *fail_pos) throw(runtime_error)
{
    Session::Status sts = Session::Status::eInvalidArgument;

    if ( fail_pos ) {
        *fail_pos= 0;
    }
    for ( vector<pair<Duple, Duple>>::iterator it = items.begin() ; it != items.end() ; ++it ) {
        sts = Get((*it).first.data(), (*it).first.size(), (*it).second.buffer(), (*it).second.buffer_size());
        if ( sts != Session::Status::eOk ) {
            if ( sts == Session::Status::eNotFoundKey ) {
                if ( fail_on_first_nfound == true ) {
                    break;
                } else {
                    if ( fail_pos ) {
                        (*fail_pos)++;
                    }
                    continue;
                }
            }
            break;
        }
        if ( fail_pos ) {
            (*fail_pos)++;
        }
    }

    if ( sts == Session::Status::eNotFoundKey ) {
        if ( fail_on_first_nfound == false ) {
            sts = Session::Status::eOk;
        }
    }

    last_status = sts;
    return sts;
}

/**
 * @brief Session PutSet
 *
 * Put set of KV pairs (items) whose keys and values are passed in as a vector
 *
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_dup     Specifies whether or not to fail the function on the first duplicate key
 * @param[in,out] first_pos          Position within items with not found key that caused a failure
 * @param[out] status                Return status (see soesession.hpp)
 *                                   If items.size() == 0 the return status is eInvalidArgument
 *                                   If fail_on_first_nfound == false and items.size() != 0 the return status is eOk
 *                                   if fail_on_first_nfound == true and items.size() != 0 and a key in keys is not found
 *                                   the return status is eDuplicateKey and fail_pos is set to first failed index
 */
Session::Status Session::PutSet(vector<pair<Duple, Duple>> &items,
    bool fail_on_first_dup,
    size_t *fail_pos) throw(runtime_error)
{
    Session::Status sts = Session::Status::eInvalidArgument;

    if ( fail_pos ) {
        *fail_pos= 0;
    }
    for ( vector<pair<Duple, Duple>>::iterator it = items.begin() ; it != items.end() ; ++it ) {
        sts = Put((*it).first.data(), (*it).first.size(), (*it).second.data(), (*it).second.size());
        if ( sts != Session::Status::eOk ) {
            if ( sts == Session::Status::eDuplicateKey ) {
                if ( fail_on_first_dup == true ) {
                    break;
                } else {
                    if ( fail_pos ) {
                        (*fail_pos)++;
                    }
                    continue;
                }
            }
            break;
        }
        if ( fail_pos ) {
            (*fail_pos)++;
        }
    }

    if ( sts == Session::Status::eDuplicateKey ) {
        if ( fail_on_first_dup == false ) {
            sts = Session::Status::eOk;
        }
    }

    last_status = sts;
    return sts;
}

/**
 * @brief Session DeleteSet
 *
 * Delete set keys passed in as a vector
 *
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_nfound  Specifies whether or not to fail the function on the first
 *                                   not found key
 * @param[in] first_pos              Position within items with not found key that caused a failure
 * @param[out] status                Return status (see soesession.hpp)
 *                                   If keys.size() == 0 the return status is eInvalidArgument
 *                                   If fail_on_first_nfound == false and keys.size() != 0 the return status is eOk
 *                                   if fail_on_first_nfound == true and keys.size() != 0 and a key in keys is not found
 *                                   the return status is eNotFoundKey and fail_pos is set to first failed index
 */
Session::Status Session::DeleteSet(vector<Duple> &keys,
    bool fail_on_first_nfound,
    size_t *fail_pos) throw(runtime_error)
{
    Session::Status sts = Session::Status::eInvalidArgument;

    if ( fail_pos ) {
        (*fail_pos)++;
    }
    for ( vector<Duple>::iterator it = keys.begin() ; it != keys.end() ; ++it ) {
        sts = Delete((*it).data(), (*it).size());
        if ( sts != Session::Status::eOk ) {
            if ( sts == Session::Status::eNotFoundKey ) {
                if ( fail_on_first_nfound == true ) {
                    break;
                } else {
                    if ( fail_pos ) {
                        (*fail_pos)++;
                    }
                    continue;
                }
            }
            break;
        }
        if ( fail_pos ) {
            (*fail_pos)++;
        }
    }

    if ( sts == Session::Status::eNotFoundKey ) {
        if ( fail_on_first_nfound == false ) {
            sts = Session::Status::eOk;
        }
    }

    last_status = sts;
    return sts;
}

/**
 * @brief Session MergeSet
 *
 * Merge in a set of KV pairs (items) whose keys and values are passed in as a vector
 * The result, as in single Merge(), depends on the merge filter
 *
 * @param[in]  items          Vector of key/data Duple pairs
 * @param[out] status         return status (see soesession.hpp)
 *                            If keys.size() == 0 the return status is eInvalidArgument
 */
Session::Status Session::MergeSet(vector<pair<Duple, Duple>> &items) throw(runtime_error)
{
    Session::Status sts = Session::Status::eInvalidArgument;

    for ( vector<pair<Duple, Duple>>::iterator it = items.begin() ; it != items.end() ; ++it ) {
        sts = Merge((*it).first.data(), (*it).first.size(), (*it).second.data(), (*it).second.size());
        if ( sts != Session::Status::eOk ) {
            break;
        }
    }

    last_status = sts;
    return sts;
}


Session::Status Session::Put(LocalArgsDescriptor &args) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    try {
        if ( acs.put ) {
            acs.put(args);
            ret = Session::eOk;
        } else {
            ret = Session::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Put failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("Put failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Put failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Put failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" Put failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Put failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("Put failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "Put() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

Session::Status Session::Get(LocalArgsDescriptor &args) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    try {
        if ( acs.get ) {
            acs.get(args);
            ret = Session::eOk;
        } else {
            ret = Session::eNoFunction;
        }
    }  catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Get failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("Get failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Get failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" Get failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Get failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Get failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("Get failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "Get() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

Session::Status Session::Delete(LocalArgsDescriptor &args) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    try {
        if ( acs.del ) {
            acs.del(args);
            ret = Session::eOk;
        } else {
            ret = Session::eNoFunction;
        }
    }  catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Delete failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("Delete failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Delete failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" Delete failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Delete failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Delete failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("Delete failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "Delete() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

Session::Status Session::Merge(LocalArgsDescriptor &args) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    // Due to how merge works we must do it this way
    for ( int cc_cnt = 0 ; cc_cnt < 128 ; cc_cnt++ ) {
        try {
            if ( acs.merge ) {
                acs.merge(args);
                ret = Session::Status::eOk;
                break;
            } else {
                ret = Session::Status::eNoFunction;
                break;
            }
        } catch (const SoeApi::Exception &ex) {
            if ( debug == true ) {
                ostringstream str_buf;
                str_buf << __FUNCTION__ << ":" << __LINE__ << " Merge failed(SoeApi): " << ex.what() << endl << flush;
                logErrMsg(str_buf.str().c_str());
            }
            if ( no_throw != true ) {
                throw runtime_error((string("Merge failed: ") + ex.what().c_str()));
            } else {
                ret = Session::Status::eNoFunction;
            }
        } catch (const RcsdbFacade::Exception &ex) {
            if ( debug == true ) {
                ostringstream str_buf;
                str_buf << __FUNCTION__ << ":" << __LINE__ << " Merge failed(RcsdbFacade): " << ex.what() << endl << flush;
                logErrMsg(str_buf.str().c_str());
            }
            if ( no_throw != true ) {
                throw runtime_error(string(" Merge failed " + ex.what()).c_str());
            } else {
                ret = TranslateExceptionStatus(ex);
                if ( ret == Session::Status::eIncomplete ) {
                    //cout << __FILE__ << "/" << __FUNCTION__ << " retry Merge(args)" << endl;
                    continue;
                }
            }
        } catch (const runtime_error &ex) {
            if ( debug == true ) {
                ostringstream str_buf;
                str_buf << __FUNCTION__ << ":" << __LINE__ << " Merge failed(runtime_error): " << ex.what() << endl << flush;
                logErrMsg(str_buf.str().c_str());
            }
            if ( no_throw != true ) {
                throw;
            } else {
                ret = Session::Status::eOsError;
            }
        } catch ( ... ) {
            if ( debug == true ) {
                ostringstream str_buf;
                str_buf << __FUNCTION__ << ":" << __LINE__ << " Merge failed(...): " << container_name << endl << flush;
                logErrMsg(str_buf.str().c_str());
            }
            if ( no_throw != true ) {
                throw runtime_error((string("Merge failed(...)").c_str()));
            } else {
                ret = Session::Status::eOsError;
            }
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "Merge() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

Session::Status Session::GetRange(LocalArgsDescriptor &args) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    try {
        if ( acs.get_range ) {
            acs.get_range(args);
            ret = Session::eOk;
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " GetRange failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("GetRange failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " GetRange failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" GetRange failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " GetRange failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " GetRange failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("GetRange failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "GetRange() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

/**
 * @brief Session GetSetIteratively
 *
 * Get set of KV pairs (items) whose keys are passed in as a vector using an Iterator
 *
 * @param[in]  items          Vector of key/buffer Duple pairs
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::GetSetIteratively(vector<pair<Duple, Duple>> &items) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    for ( vector<pair<Duple, Duple>>::iterator it = items.begin() ; it != items.end() ; ++it ) {
        if ( (*it).first.empty() == true || (*it).second.empty() == true ) {
            ret = Session::Status::eInvalidArgument;
            break;
        }

        size_t ret_size = (*it).second.size();
        char *ret_data_ptr = const_cast<char *>((*it).second.data());
        ret = this->Get((*it).first.data(), (*it).first.size(), ret_data_ptr, &ret_size);
        if ( ret != Session::Status::eOk ) {
           break;
        }
        (*it).second = Duple(ret_data_ptr, ret_size);
    }

    last_status = ret;
    return ret;
}

Session::Status Session::GetSet(LocalArgsDescriptor &args, vector<pair<Duple, Duple>> &items) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    try {
        if ( acs.get_range ) {
            acs.get_range(args);
            ret = Session::eOk;
        } else {
            ret = GetSetIteratively(items);
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " GetSet failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("GetSet failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " GetSet failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" GetSet failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " GetSet failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " GetSet failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("GetSet failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "GetSet() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

/**
 * @brief Session PutSetIteratively
 *
 * Put set of KV pairs (items) whose keys and values are passed in as a vector using an Iterator
 *
 * @param[in]  items          Vector of key/data Duple pairs
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::PutSetIteratively(vector<pair<Duple, Duple>> &items) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    for ( vector<pair<Duple, Duple>>::iterator it = items.begin() ; it != items.end() ; ++it ) {
        if ( (*it).first.empty() == true || (*it).second.empty() == true ) {
            ret = Session::Status::eInvalidArgument;
            break;
        }

        ret = this->Put((*it).first.data(), (*it).first.size(), (*it).second.data(), (*it).second.size());
        if ( ret != Session::Status::eOk ) {
           break;
        }
    }

    return ret;
}

Session::Status Session::PutSet(LocalArgsDescriptor &args, vector<pair<Duple, Duple>> &items) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    try {
        if ( acs.put_range ) {
            acs.put_range(args);
            ret = Session::eOk;
        } else {
            ret = PutSetIteratively(items);
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " PutSet failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("PutSet failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " PutSet failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" PutSet failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " PutSet failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " PutSet failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("PutSet failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "PutSet() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

/**
 * @brief Session DeleteSetIteratively
 *
 * Delete set keys passed in as a vector using an Iterator
 *
 * @param[in]  items          Vector of key Duples
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::DeleteSetIteratively(vector<Duple> &keys) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    for ( vector<Duple>::iterator it = keys.begin() ; it != keys.end() ; ++it ) {
        if ( (*it).empty() == true ) {
            ret = Session::Status::eInvalidArgument;
            break;
        }

        ret = this->Delete((*it).data(), (*it).size());
        if ( ret != Session::Status::eOk ) {
           break;
        }
    }

    last_status = ret;
    return ret;
}

Session::Status Session::DeleteSet(LocalArgsDescriptor &args, vector<Duple> &keys) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    try {
        if ( acs.delete_range ) {
            acs.delete_range(args);
            ret = Session::eOk;
        } else {
            ret = DeleteSetIteratively(keys);
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DeleteSet failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("DeleteSet failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DeleteSet failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" DeleteSet failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DeleteSet failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DeleteSet failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("DeleteSet failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "DeleteSet() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

/**
 * @brief Session MergeSetIteratively
 *
 * Merge in set of KV pairs (items) whose keys and values are passed in as a vector using an Iterator
 * The result, as in single Merge(), depends on the merge filter
 *
 * @param[in]  items          Vector of key/data Duple pairs
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::MergeSetIteratively(vector<pair<Duple, Duple>> &items) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    for ( vector<pair<Duple, Duple>>::iterator it = items.begin() ; it != items.end() ; ++it ) {
        if ( (*it).first.empty() == true || (*it).second.empty() == true ) {
            ret = Session::Status::eInvalidArgument;
            break;
        }

        ret = this->Merge((*it).first.data(), (*it).first.size(), (*it).second.data(), (*it).second.size());
        if ( ret != Session::Status::eOk ) {
           break;
        }
    }

    last_status = ret;
    return ret;
}

Session::Status Session::MergeSet(LocalArgsDescriptor &args, vector<pair<Duple, Duple>> &items) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    try {
        if ( acs.merge_range ) {
            acs.merge_range(args);
            ret = Session::eOk;
        } else {
            ret = MergeSetIteratively(items);
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " MergeSet failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("MergeSet failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " MergeSet failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" MergeSet failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " MergeSet failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " MergeSet failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("MergeSet failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "MergeSet() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

/**
 * @brief Session CreateGroup
 *
 * Create Group object
 * NULL will be returned if no more groups can be created. Subsequently status can be checked by calling ok()
 *
 * @param[out] Group         return group (see soesession.hpp)
 */
Group *Session::CreateGroup() throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    LocalArgsDescriptor args;
    InitDesc(args);

    try {
        if ( acs.create_group ) {
            acs.create_group(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateGroup failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateGroup failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateGroup failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" CreateGroup failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateGroup failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateGroup failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateGroup failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "CreateGroup() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    if ( ret != Session::Status::eOk ) {
        last_status = ret;
        return 0;
    }

    return new Group(this_sess, args.group_idx);
}

/**
 * @brief Session DestroyGroup
 *
 * Destroy Group object
 * Destroy an existing Group object. All changes made to the group will be discarded.
 *
 * @param[in]  gr             Group to be destroyed
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::DestroyGroup(Group *gr) const throw(runtime_error)
{
    if ( !gr ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "DestroyGroup() NULL group " << " name: " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        return Session::Status::eInvalidArgument;
    }

    Session::Status ret = Session::Status::eOk;

    LocalArgsDescriptor args;
    InitDesc(args);
    args.group_idx = gr->group_no;

    try {
        if ( acs.destroy_group ) {
            acs.destroy_group(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyGroup failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("DestroyGroup failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyGroup failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" DestroyGroup failed " + ex.what()).c_str());
        } else {
            ret = TranslateConstExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyGroup failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyGroup failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("DestroyGroup failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "DestroyGroup() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    if ( ret == Session::Status::eOk ) {
        //cout << "COUNT: " << gr->sess.use_count() << endl << flush;
        delete gr;
    }

    SetCLastStatus(ret);
    return ret;
}

/**
 * @brief Session WriteGroup
 *
 * Write Group object
 *
 * Apply a sequence of edits made to the group to the underlying store.
 * Note: writing a group is not synchronized across grups. users have to provide
 * synchronization on their own.
 *
 * @param[in]  gr             Group to be written
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::Write(const Group *gr) const throw(runtime_error)
{
    if ( !gr ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "WriteGroup() NULL group " << " name: " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        return Session::Status::eInvalidArgument;
    }

    Session::Status ret = Session::Status::eOk;

    LocalArgsDescriptor args;
    InitDesc(args);
    args.group_idx = gr->group_no;

    try {
        if ( acs.write_group ) {
            acs.write_group(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " WriteGroup failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("WriteGroup failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " WriteGroup failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" WriteGroup failed " + ex.what()).c_str());
        } else {
            ret = TranslateConstExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " WriteGroup failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " WriteGroup failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("WriteGroup failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "WriteGroup() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    SetCLastStatus(ret);
    return ret;
}

/**
 * @brief Session StartTransaction
 *
 * Create and start Transaction object
 * NULL will be returned if no more transactions can be created. Subsequently status can be checked by calling ok()
 * In order to start a new transaction, an underlying session has to be created with transaction_db flag set to "true"
 *
 * @param[out] Transaction         return transaction object (see soesession.hpp)
 */
Transaction *Session::StartTransaction() throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);

    try {
        if ( acs.start_transaction ) {
            acs.start_transaction(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " StartTransaction failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("StartTransaction failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " StartTransaction failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" StartTransaction failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " StartTransaction failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " StartTransaction failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("StartTransaction failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "StartTransaction() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    if ( ret != Session::Status::eOk ) {
        last_status = ret;
        return 0;
    }

    return new Transaction(this_sess, args.transaction_idx);
}

/**
 * @brief Session CommitTransaction
 *
 * Commit Transaction's sequence of edits to an underlying store
 * Transactions are subject to serialization and checking by the underlying store engine (RocsDB being the default one).
 * In case of a conflict, an attempted commit may produce an error.
 * Edits made to constituent transaction's objects are not visible to other transactions or views using Gets, Iterators, etc.
 * before a transaction is committed.
 *
 * @param[in]  tr             Transaction to be committed
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::CommitTransaction(const Transaction *tr) const throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    if ( !tr ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "Transaction() NULL " << " name: " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        return Session::Status::eInvalidArgument;
    }
    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);
    args.transaction_idx = tr->transaction_no;

    try {
        if ( acs.commit_transaction ) {
            acs.commit_transaction(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CommitTransaction failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CommitTransaction failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CommitTransaction failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" CommitTransaction failed " + ex.what()).c_str());
        } else {
            ret = TranslateConstExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CommitTransaction failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CommitTransaction failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CommitTransaction failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "CommitTransaction() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    delete tr;
    SetCLastStatus(ret);
    return ret;
}

/**
 * @brief Session RollbackTransaction
 *
 * Roll back Transaction's sequence of edits to the underlying store
 * This is equivalent to discarding transaction's sequence of edits.
 *
 * @param[in]  tr             Transaction to be rolled back
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::RollbackTransaction(const Transaction *tr) const throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    if ( !tr ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "Transaction() NULL " << " name: " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        return Session::Status::eInvalidArgument;
    }
    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);
    args.transaction_idx = tr->transaction_no;

    try {
        if ( acs.rollback_transaction ) {
            acs.rollback_transaction(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " RollbackTransaction failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("RollbackTransaction failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " RollbackTransaction failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" Open failed " + ex.what()).c_str());
        } else {
            ret = TranslateConstExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " RollbackTransaction failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " RollbackTransaction failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("RollbackTransaction failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "RollbackTransaction() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    delete tr;
    SetCLastStatus(ret);
    return ret;
}

/**
 * @brief Session CreateSnapshotEngine
 *
 * Create Snapshot Engine
 * NULL will be returned if no more snapshot engines can be created.
 * Subsequently status can be checked by calling ok()
 *
 * @param[in]  snapshot_engine_name         Snapshot engine name
 * @param[out] SnapshotEngine               return SnapshotEngien pointer or NULL
 *
 */
SnapshotEngine *Session::CreateSnapshotEngine(const char *snapshot_engine_name) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);
    string sn(snapshot_engine_name);
    args.snapshot_name = &sn;

    try {
        if ( acs.create_snapshot_engine ) {
            acs.create_snapshot_engine(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateSnapshotEngine failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateSnapshotEngine failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateSnapshotEngine failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" CreateSnapshotEngine failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateSnapshotEngine failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateSnapshotEngine failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateSnapshotEngine failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "CreateSnapshotEngine() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    if ( ret != Session::Status::eOk ) {
        last_status = ret;
        return 0;
    }

    return new SnapshotEngine(this_sess, snapshot_engine_name, args.snapshot_idx, args.snapshot_id);
}

/**
 * @brief Session CreateSnapshot
 *
 * Create Snapshot by physically creating snapshot directory, file links and
 * updating store's config.json
 *
 * @param[in]  en               Snapshot engine
 * @param[out] status           Return status (see soesession.hpp)
 *
 */
Session::Status Session::CreateSnapshot(const SnapshotEngine *en) const throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    if ( !en ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "CreateSnapshot() NULL " << " name: " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        return Session::Status::eInvalidArgument;
    }
    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);
    string sn(en->snapshot_engine_name);
    args.snapshot_name = &sn;
    args.snapshot_idx = en->snapshot_engine_no;
    args.snapshot_id = en->snapshot_engine_id;

    try {
        if ( acs.create_snapshot ) {
            acs.create_snapshot(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateSnapshot failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateSnapshot failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateSnapshot failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" CreateSnapshot failed " + ex.what()).c_str());
        } else {
            ret = TranslateConstExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateSnapshot failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateSnapshot failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateSnapshot failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "CreateSnapshot() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    SetCLastStatus(ret);
    return ret;
}

/**
 * @brief Session DestroySnapshot
 *
 * Destroy Snapshot by physically destroying file links, deleting snapshot directory
 * and updating store's config.json
 *
 * @param[in]  en               Snapshot engine
 * @param[out] status           Return status (see soesession.hpp)
 *
 */
Session::Status Session::DestroySnapshot(const SnapshotEngine *en) const throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    if ( !en ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "DestroySnapshot() NULL " << " name: " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        return Session::Status::eInvalidArgument;
    }
    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);
    string sn(en->snapshot_engine_name);
    args.snapshot_name = &sn;
    args.snapshot_idx = en->snapshot_engine_no;
    args.snapshot_id = en->snapshot_engine_id;

    try {
        if ( acs.destroy_snapshot ) {
            acs.destroy_snapshot(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroySnapshot failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("DestroySnapshot failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroySnapshot failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" DestroySnapshot failed " + ex.what()).c_str());
        } else {
            ret = TranslateConstExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroySnapshot failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroySnapshot failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("DestroySnapshot failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "DestroySnapshot() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    SetCLastStatus(ret);
    return ret;
}

/**
 * @brief Session DestroySnapshotEngine
 *
 * Destroy Snapshot Engine object, without destroying underlying snapshots
 *
 * @param[in]  en             Snaphot Engine to be destroyed
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::DestroySnapshotEngine(const SnapshotEngine *en) const throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    if ( !en ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "DestroySnapshotEngine() NULL " << " name: " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        return Session::Status::eInvalidArgument;
    }
    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);
    string sn(en->snapshot_engine_name);
    args.snapshot_name = &sn;
    args.snapshot_idx = en->snapshot_engine_no;
    args.snapshot_id = en->snapshot_engine_id;

    try {
        if ( acs.destroy_snapshot ) {
            acs.destroy_snapshot(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroySnapshotEngine failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("DestroySnapshotEngine failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroySnapshotEngine failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" DestroySnapshotEngine failed " + ex.what()).c_str());
        } else {
            ret = TranslateConstExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroySnapshotEngine failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroySnapshotEngine failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("DestroySnapshotEngine failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "DestroySnapshotEngine() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    delete en;
    SetCLastStatus(ret);
    return ret;
}

/**
 * @brief Session ListSnapshots
 *
 * List Snapshots for a given store
 *
 * @param[in/out]  list           List of Snaphots
 * @param[out]     status         return status (see soesession.hpp)
 */
Session::Status Session::ListSnapshots(vector<pair<string, uint32_t> > &list) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);
    args.snapshots = &list;

    try {
        if ( acs.list_snapshots ) {
            acs.list_snapshots(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " ListSnapshots failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("ListSnapshots failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " ListSnapshots failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" ListSnapshots failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " ListSnapshots failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " ListSnapshots failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("ListSnapshots failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "ListSnapshots() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

/**
 * @brief Session CreateBackupEngine
 *
 * Create Backup Engine
 * NULL will be returned if no more backup engines can be created. Subsequently status can be checked by calling ok()
 *
 * @param[in]  backup_engine_name      Backup engine name
 * @param[out] BackupEngine            Return BackupEngine pointer or NULL
 * @param[in]  bckp_path               Path where the backup will be created, if NULL a default will be used,
 *                                     which is <store_path>/backups
 * @param[in]  incremental             Whether or not create an incremental backup,
 *                                     NOTE: If no backup is found in the location specified by bckp_path, a new backup will
 *                                     be created.
 *
 */
BackupEngine *Session::CreateBackupEngine(const char *backup_engine_name, const char *bckp_path, bool incremental) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);
    string bn(backup_engine_name);
    args.backup_name = &bn;
    string bp(bckp_path);
    if ( bckp_path ) {
        args.snapshot_name = &bp; // overloaded
    }
    args.backup_idx = static_cast<uint32_t>(incremental); // overloaded

    try {
        if ( acs.create_backup_engine ) {
            acs.create_backup_engine(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateBackupEngine failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateBackupEngine failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateBackupEngine failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" CreateBackupEngine failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateBackupEngine failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateBackupEngine failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateBackupEngine failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "CreateBackupEngine() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    if ( ret != Session::Status::eOk ) {
        last_status = ret;
        return 0;
    }

    return new BackupEngine(this_sess, backup_engine_name, bckp_path, args.backup_idx, args.backup_id);
}

/**
 * @brief Session CreateBackup
 *
 * Create Backup
 *
 * @param[in]  en             Backup engine
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status Session::CreateBackup(const BackupEngine *en) const throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    if ( !en ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "BackupEngine() NULL " << " name: " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        return Session::Status::eInvalidArgument;
    }
    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);
    args.backup_name = &en->backup_engine_name;;
    args.snapshot_name = &en->backup_root_dir;
    args.backup_idx = en->backup_engine_no;
    args.backup_id = en->backup_engine_id;
    args.sync_mode = cr_thread_id; // overloaded field

    try {
        if ( acs.create_backup ) {
            acs.create_backup(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateBackup failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateBackup failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateBackup failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" CreateBackup failed " + ex.what()).c_str());
        } else {
            ret = TranslateConstExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateBackup failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateBackup failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateBackup failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "CreateBackup() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    SetCLastStatus(ret);
    return ret;
}

/**
 * @brief Session RestoreBackup
 *
 * Restore Backup
 *
 * @param[in]  en             Backup engine
 * @param[in]  cl_n           Cluster name
 * @param[in]  sp_n           Space name
 * @param[in]  st_n           Store name
 * @param[in]  _overwrite     Overwrite existing contents
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status Session::RestoreBackup(const BackupEngine *en,
    const std::string &cl_n,
    const std::string &sp_n,
    const std::string &st_n,
    bool _overwrite) const throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    if ( !en ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "BackupEngine() NULL " << " name: " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        return Session::Status::eInvalidArgument;
    }
    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);
    args.backup_name = &en->backup_engine_name;;
    args.snapshot_name = &en->backup_root_dir;
    args.backup_idx = en->backup_engine_no;
    args.backup_id = en->backup_engine_id;
    args.sync_mode = cr_thread_id; // overloaded field
    args.transaction_idx = static_cast<uint32_t>(_overwrite);

    std::vector<std::pair<std::string, uint32_t> > restore_names;
    restore_names.push_back(std::pair<std::string, uint32_t>(cl_n, 0));
    restore_names.push_back(std::pair<std::string, uint32_t>(sp_n, 0));
    restore_names.push_back(std::pair<std::string, uint32_t>(st_n, 0));
    args.backups = &restore_names;

    try {
        if ( acs.restore_backup ) {
            acs.restore_backup(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " RestoreBackup failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateBackup failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " RestoreBackup failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" CreateBackup failed " + ex.what()).c_str());
        } else {
            ret = TranslateConstExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " RestoreBackup failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " RestoreBackup failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("RestoreBackup failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "RestoreBackup() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    SetCLastStatus(ret);
    return ret;
}

/**
 * @brief Session VerifyBackup
 *
 * Restore Backup
 *
 * @param[in]  en             Backup engine
 * @param[in]  cl_n           Cluster name
 * @param[in]  sp_n           Space name
 * @param[in]  st_n           Store name
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status Session::VerifyBackup(const BackupEngine *en,
    const std::string &cl_n,
    const std::string &sp_n,
    const std::string &st_n) const throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    if ( !en ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "BackupEngine() NULL " << " name: " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        return Session::Status::eInvalidArgument;
    }
    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);
    args.backup_name = &en->backup_engine_name;;
    args.snapshot_name = &en->backup_root_dir;
    args.backup_idx = en->backup_engine_no;
    args.backup_id = en->backup_engine_id;
    args.sync_mode = cr_thread_id; // overloaded field

    std::vector<std::pair<std::string, uint32_t> > restore_names;
    restore_names.push_back(std::pair<std::string, uint32_t>(cl_n, 0));
    restore_names.push_back(std::pair<std::string, uint32_t>(sp_n, 0));
    restore_names.push_back(std::pair<std::string, uint32_t>(st_n, 0));
    args.backups = &restore_names;

    try {
        if ( acs.verify_backup ) {
            acs.verify_backup(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " VerifyBackup failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateBackup failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " VerifyBackup failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" CreateBackup failed " + ex.what()).c_str());
        } else {
            ret = TranslateConstExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " VerifyBackup failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " VerifyBackup failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateBackup failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "VerifyBackup() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    SetCLastStatus(ret);
    return ret;
}

/**
 * @brief Session DestroyBackup
 *
 * Destroy Backup
 *
 * @param[in]  en             Backup engine
 * @param[out] status         Return status (see soesession.hpp)
 *
 */
Session::Status Session::DestroyBackup(const BackupEngine *en) const throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    if ( !en ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "BackupEngine() NULL " << " name: " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        return Session::Status::eInvalidArgument;
    }
    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);
    args.backup_name = &en->backup_root_dir;
    args.snapshot_name = &en->backup_root_dir;
    args.backup_idx = en->backup_engine_no;
    args.backup_id = en->backup_engine_id;
    args.sync_mode = cr_thread_id; // overloaded field

    try {
        if ( acs.destroy_backup ) {
            acs.destroy_backup(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateBackup failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateBackup failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateBackup failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" CreateBackup failed " + ex.what()).c_str());
        } else {
            ret = TranslateConstExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateBackup failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateBackup failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateBackup failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "CreateBackup() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    SetCLastStatus(ret);
    return ret;
}

/**
 * @brief Session DestroyBackupEngine
 *
 * Destroy Backup Engine, without destroying backups in the underlying store
 *
 * @param[in]  en             Backup Engine to be destroyed
 * @param[out] status         return status (see soesession.hpp)
 */
Session::Status Session::DestroyBackupEngine(const BackupEngine *en) const throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    if ( !en ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "BackupEngine() NULL " << " name: " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        return Session::Status::eInvalidArgument;
    }
    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);
    string sn(en->backup_engine_name);
    args.backup_name = &sn;
    if ( en->GetConstRootDir().empty() == false ) {
        args.snapshot_name = &en->GetConstRootDir(); // overloaded
    }
    args.backup_idx = en->backup_engine_no;
    args.backup_id = en->backup_engine_id;

    try {
        if ( acs.destroy_backup_engine ) {
            acs.destroy_backup_engine(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyBackupEngine failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("DestroyBackupEngine failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyBackupEngine failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" DestroyBackupEngine failed " + ex.what()).c_str());
        } else {
            ret = TranslateConstExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyBackupEngine failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyBackupEngine failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("DestroyBackupEngine failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "DestroyBackupEngine() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    delete en;
    SetCLastStatus(ret);
    return ret;
}

/**
 * @brief Session ListBackups
 *
 * List Backups for a given store
 *
 * @param[in/out]  list           List of Backups
 * @param[out]     status         return status (see soesession.hpp)
 */
Session::Status Session::ListBackups(vector<pair<string, uint32_t> > &list) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    LocalAdmArgsDescriptor args;
    InitAdmDesc(args);
    args.backups = &list;

    try {
        if ( acs.list_backups ) {
            acs.list_backups(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " ListBackups failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("ListBackups failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " ListBackups failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" Open failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " ListBackups failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " ListBackups failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("ListBackups failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "ListBackups() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

Session::Status Session::Iterate(LocalArgsDescriptor &args) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    try {
        if ( acs.iterate ) {
            acs.iterate(args);
            ret = Session::eOk;
        } else {
            ret = Session::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Iterate failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("Iterate failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Iterate failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Iterate failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" Iterate failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " Iterate failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("Iterate failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "Iterate() ret: " << ret <<  " name: " << container_name << " id: " << args.iterator_idx << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    last_status = ret;
    return ret;
}

/**
 * @brief Session CreateSubsetIterator
 *
 * Create Subset Iterator
 * NULL will be returned if no more iterators can be created. User can subsequently call ok() to check the status
 *
 * @param[in]   dir              Traversal direction
 * @param[in]   subs             Pointer to base class Subsetable
 * @param[in]   start_key        Start key
 * @param[in]   start_start_key  Start key size
 * @param[in]   end_key          End key
 * @param[in]   end_start_key    End key size
 * @param[out]  Iterator         Return Iterator pointer
 */
Iterator *Session::CreateSubsetIterator(Iterator::eIteratorDir dir,
    Subsetable *subs,
    const char *start_key,
    size_t start_key_size,
    const char *end_key,
    size_t end_key_size) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    LocalArgsDescriptor args;
    InitDesc(args);
    args.key = start_key;
    args.key_len = start_key_size;
    args.end_key = end_key;
    args.end_key_len = end_key_size;
    size_t pref_size;
    args.data = subs->GetKeyName(pref_size);
    args.data_len = pref_size;
    args.overwrite_dups = (dir == Iterator::eIteratorDir::eIteratorDirForward ? true : false);
    args.cluster_idx = Iterator::eType::eIteratorTypeSubset;

    try {
        if ( acs.create_iterator ) {
            acs.create_iterator(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateSubsetIterator failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateSubsetIterator failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateSubsetIterator failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" CreateSubsetIterator failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateSubsetIterator failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateSubsetIterator failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateSubsetIterator failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "CreateSubsetIterator() ret: " << ret <<  " name: " << container_name << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    if ( ret != Session::Status::eOk ) {
        return 0;
    }

    try {
        return new SubsetIterator(this_sess,
            subs->this_subs,
            args.iterator_idx,
            dir);
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateSubsetIterator failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eInternalError;
        }

        last_status = ret;
        return 0;
    }
}

/**
 * @brief Session CreateIterator
 *
 * Create Session Iterator
 * NULL will be returned if no more iterators can be created. User can subsequently call ok() to check the status
 *
 * @param[in]   dir              Traversal direction
 * @param[in]   subs             Pointer to base class Subsetable
 * @param[in]   start_key        Start key
 * @param[in]   start_start_key  Start key size
 * @param[in]   end_key          End key
 * @param[in]   end_start_key    End key size
 * @param[out]  Iterator         Return Iterator pointer
 */
Iterator *Session::CreateIterator(Iterator::eIteratorDir dir,
    const char *start_key,
    size_t start_key_size,
    const char *end_key,
    size_t end_key_size) throw(runtime_error)
{
    Session::Status ret = Session::Status::eOk;

    LocalArgsDescriptor args;
    InitDesc(args);
    args.key = start_key;
    args.key_len = start_key_size;
    args.end_key = end_key;
    args.end_key_len = end_key_size;
    args.overwrite_dups = (dir == Iterator::eIteratorDir::eIteratorDirForward ? true : false);
    args.cluster_idx = Iterator::eType::eIteratorTypeContainer;

    try {
        if ( acs.create_iterator ) {
            acs.create_iterator(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateIterator failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateIterator failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateIterator failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" CreateIterator failed " + ex.what()).c_str());
        } else {
            ret = TranslateExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateIterator failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateIterator failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("CreateIterator failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "CreateIterator() ret: " << ret <<  " name: " << container_name << " id: " << args.iterator_idx << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    if ( ret != Session::Status::eOk ) {
        return 0;
    }

    try {
        return new SessionIterator(this_sess,
            args.iterator_idx,
            dir);
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " CreateIterator failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eInternalError;
        }

        last_status = ret;
        return 0;
    }
}

/**
 * @brief Session DestroyIterator
 *
 * Destroy Iterator, either Store Iterator or SubsetIterator
 * Iterators are not writable or updatable, so upon destroying an iterator no changes are made to
 * the underlying store
 *
 * @param[in]   it             Iterator pointer
 * @param[out]  status         Return status (see soesession.hpp)
 */
Session::Status Session::DestroyIterator(Iterator *it) const throw(runtime_error)
{
    if ( !it ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << "DestroyIterator() NULL iterator " << " name: " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        return Session::Status::eInvalidArgument;
    }

    Session::Status ret = Session::Status::eOk;

    LocalArgsDescriptor args;
    InitDesc(args);
    args.iterator_idx = it->iterator_no;

    try {
        if ( acs.destroy_iterator ) {
            acs.destroy_iterator(args);
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const SoeApi::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyIterator failed(SoeApi): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("DestroyIterator failed: ") + ex.what().c_str()));
        } else {
            ret = Session::Status::eNoFunction;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyIterator failed(RcsdbFacade): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error(string(" DestroyIterator failed " + ex.what()).c_str());
        } else {
            ret = TranslateConstExceptionStatus(ex);
        }
    } catch (const runtime_error &ex) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyIterator failed(runtime_error): " << ex.what() << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw;
        } else {
            ret = Session::Status::eOsError;
        }
    } catch ( ... ) {
        if ( debug == true ) {
            ostringstream str_buf;
            str_buf << __FUNCTION__ << ":" << __LINE__ << " DestroyIterator failed(...): " << container_name << endl << flush;
            logErrMsg(str_buf.str().c_str());
        }
        if ( no_throw != true ) {
            throw runtime_error((string("DestroyIterator failed(...)").c_str()));
        } else {
            ret = Session::Status::eOsError;
        }
    }

    if ( debug == true ) {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << "DestroyIterator() ret: " << ret <<  " name: " << container_name << " id: " << it->iterator_no << endl << flush;
        logDbgMsg(str_buf.str().c_str());
    }

    if ( ret == Session::Status::eOk ) {
        //cout << "COUNT: " << it->sess.use_count() << endl << flush;
        delete it;
    }

    SetCLastStatus(ret);
    return ret;
}

/**
 * @brief Session CreateSubset
 *
 * Create Subset
 * NULL will be returned in case of a OS, Network, Internal, etc. error. User should call OK() to check the status
 *
 * @param[in]   name             Subset name
 * @param[in]   name_len         Subset name length
 * @param[in]   sb_type          Subset type (see soesubsetable.hpp)
 * @param[out]  Subsetable       Return Subset object
 */
Subsetable *Session::CreateSubset(const char *name,
    size_t name_len,
    Session::eSubsetType sb_type) throw(runtime_error)
{
    Subsetable *subs = 0;

    try {
        switch ( sb_type ) {
        case eSubsetTypeDisjoint:
            subs = new DisjointSubset(name, name_len, this_sess);
            break;
        case eSubsetTypeUnion:
            subs = new UnionSubset(name, name_len, this_sess);
            break;
        case eSubsetTypeComplement:
            subs = new ComplementSubset(name, name_len, this_sess);
            break;
        case eSubsetTypeNotComplement:
            subs = new NotComplementSubset(name, name_len, this_sess);
            break;
        case eSubsetTypeCartesian:
            subs = new CartesianSubset(name, name_len, this_sess);
            break;
        default:
            break;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        //delete subs;
        if ( no_throw != true ) {
            throw runtime_error(string(" DestroyIterator failed " + ex.what()).c_str());
        } else {
            subs = 0;
        }
    } catch (const runtime_error &ex) {
        //delete subs;
        if ( no_throw != true ) {
            throw;
        } else {
            subs = 0;
        }
    } catch ( ... ) {
        //delete subs;
        if ( no_throw != true ) {
            throw runtime_error((string("DestroyIterator failed(...)").c_str()));
        } else {
            subs = 0;
        }
    }

    if ( subs ) {
        last_status = subs->GetLastStatus();
    } else {
        last_status = Session::Status::eIOError;
    }
    return subs;
}

/**
 * @brief Session OpenSubset
 *
 * Opens Subset
 * NULL will be returned in case of an OS, Network, Internal, etc. error. User should call OK() to check the status
 *
 * @param[in]   name             Subset name
 * @param[in]   name_len         Name length
 * @param[out]  Subsetable       Return Subset object, or NULL if not found
 */
Subsetable *Session::OpenSubset(const char *name,
    size_t name_len,
    Session::eSubsetType sb_type) throw(runtime_error)
{
    Subsetable *subs = 0;

    try {
        switch ( sb_type ) {
        case eSubsetTypeDisjoint:
            subs = new DisjointSubset(name, name_len, this_sess, false);
            break;
        case eSubsetTypeUnion:
            subs = new UnionSubset(name, name_len, this_sess, false);
            break;
        case eSubsetTypeComplement:
            subs = new ComplementSubset(name, name_len, this_sess, false);
            break;
        case eSubsetTypeNotComplement:
            subs = new NotComplementSubset(name, name_len, this_sess, false);
            break;
        case eSubsetTypeCartesian:
            subs = new CartesianSubset(name, name_len, this_sess, false);
            break;
        default:
            break;
        }
    } catch (const RcsdbFacade::Exception &ex) {
        //delete subs;
        if ( no_throw != true ) {
            throw runtime_error(string(" DestroyIterator failed " + ex.what()).c_str());
        } else {
            subs = 0;
        }
    } catch (const runtime_error &ex) {
        //delete subs;
        if ( no_throw != true ) {
            throw;
        } else {
            subs = 0;
        }
    } catch ( ... ) {
        //delete subs;
        if ( no_throw != true ) {
            throw runtime_error((string("DestroyIterator failed(...)").c_str()));
        } else {
            subs = 0;
        }
    }

    if ( subs ) {
        last_status = subs->GetLastStatus();
    } else {
        last_status = Session::Status::eIOError;
    }
    return subs;
}

/**
 * @brief Session DestroySubset
 *
 * Destroy Subset, delete all of the subset's items from store, without deleting
 * the subset itself. Subset object has to deleted separately, after destroying a subset,
 * via DeleteSubset call.
 *
 * @param[in]   sb           Subset pointer
 * @param[out]  status       status (see soesesion.hpp)
 */
Session::Status Session::DestroySubset(Subsetable *sb) const throw(runtime_error)
{
    if ( !sb ) {
        SetCLastStatus(Session::Status::eInvalidArgument);
        return Session::Status::eInvalidArgument;
    }
    return sb->DeleteAll();
}

/**
 * @brief Session DeleteSubset
 *
 * Delete Subset object without deleting subset's items from the store
 *
 * @param[in]   sb           Subset pointer
 * @param[out]  status       status (see soesesion.hpp)
 */
Session::Status Session::DeleteSubset(Subsetable *sb) const throw(runtime_error)
{
    if ( !sb ) {
        SetCLastStatus(Session::Status::eInvalidArgument);
        return Session::Status::eInvalidArgument;
    }
    delete sb;
    return Session::Status::eOk;
}

void Session::logDbgMsg(const char *msg) const
{
    soe_log(SOE_DEBUG, "%s\n", msg);
}

void Session::logErrMsg(const char *msg) const
{
    soe_log(SOE_ERROR, "%s\n", msg);
}

}
