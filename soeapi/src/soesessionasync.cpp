/**
 * @file    soesessionasync.cpp
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
 * soesessionasync.cpp
 *
 *  Created on: Dec 4, 2017
 *      Author: Jan Lisowiec
 */

#include <stdint.h>
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
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <new>

#include "soe_soe.hpp"

#define gettid() syscall(SYS_gettid)

using namespace std;
using namespace KvsFacade;
using namespace RcsdbFacade;

namespace Soe {

std::list<Futurable *>     Session::global_garbage_futures;
Lock                       Session::global_garbage_futures_lock;

/**
 * @brief Session Asynchronous Put
 *
 * Put KV pair (item) to a store associated with session
 * If a key doesn't exist it'll be created, if it does it'll be updated
 *
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[in]  data           Data
 * @param[in]  data_size      Data length
 * @param[out] future         return PutFuture
 */
PutFuture *Session::PutAsync(const char *key, size_t key_size, const char *data, size_t data_size, uint64_t *pos, Futurable *_par) throw(runtime_error)
{
    LocalArgsDescriptor *args = new LocalArgsDescriptor;
    InitDesc(*args);
    args->key = key;
    args->key_len = key_size;
    args->data = data;
    args->data_len = data_size;
    args->group_idx = cr_thread_id; // overloaded field
    args->future_signaller = Soe::SignalFuture;

    PutFuture *f = new PutFuture(this_sess, _par);
    uint64_t *p_pos = 0;
    if ( _par && pos ) {
        p_pos = dynamic_cast<SetFuture *>(_par)->InsertFuture(*f, static_cast<size_t>(*pos));
    }
    if ( p_pos ) {
        p_pos[1] = reinterpret_cast<uint64_t>(this);
        args->end_key = reinterpret_cast<const char *>(p_pos);
        args->end_key_len = 2*sizeof(uint64_t);
    }
    Duple _key(key, key_size);
    Duple _value(data, data_size);
    f->SetKey(_key);
    f->SetValue(_value);
    args->seq_number = f->GetSeq();

    pending_num_futures++;
    Session::Status ret = Put(*args);

    if ( ret != Session::eOk ) {
        delete f;
        return 0;
    }

    DEBUG_FUTURES("Session::" << __FUNCTION__ << " fut=" << hex << f << dec << " sess=" << hex << this << dec <<
        " pending_num_futures=" << pending_num_futures << endl);
    f->SetPostRet(ret);
    return f;
}

/**
 * @brief Session Asynchronous Get
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
 * @param[out] future         return GetFuture
 */
GetFuture *Session::GetAsync(const char *key, size_t key_size, char *buf, size_t *buf_size, uint64_t *pos, Futurable *_par) throw(runtime_error)
{
    LocalArgsDescriptor *args = new LocalArgsDescriptor;
    InitDesc(*args);
    args->key = key;
    args->key_len = key_size;
    args->buf = buf;
    args->buf_len = *buf_size;
    args->group_idx = cr_thread_id; // overloaded field
    args->future_signaller = Soe::SignalFuture;

    GetFuture *f = new GetFuture(this_sess, _par, buf, *buf_size);
    uint64_t *p_pos = 0;
    if ( _par && pos ) {
        p_pos = dynamic_cast<SetFuture *>(_par)->InsertFuture(*f, static_cast<size_t>(*pos));
    }
    if ( p_pos ) {
        p_pos[1] = reinterpret_cast<uint64_t>(this);
        args->end_key = reinterpret_cast<const char *>(p_pos);
        args->end_key_len = 2*sizeof(uint64_t);
    }
    Duple _key(key, key_size);
    Duple _buf(buf, *buf_size);
    f->SetKey(_key);
    f->SetValue(_buf);
    args->seq_number = f->GetSeq();

    pending_num_futures++;
    Status ret = Get(*args);
    if ( ret == Session::eOk || ret == Session::eTruncatedValue ) {
        *buf_size = args->buf_len;
    } else {
        delete f;
        return 0;
    }
    last_status = ret;

    DEBUG_FUTURES("Session::" << __FUNCTION__ << " fut=" << hex << f << dec << " sess=" << hex << this << dec <<
        " pending_num_futures=" << pending_num_futures << endl);
    f->SetPostRet(ret);
    return f;
}

/**
 * @brief Session Asynchronous Delete
 *
 * Delete KV pair (item) from a store associated with session
 *
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[out] future         return DeleteFuture
 */
DeleteFuture *Session::DeleteAsync(const char *key, size_t key_size, uint64_t *pos, Futurable *_par) throw(runtime_error)
{
    LocalArgsDescriptor *args = new LocalArgsDescriptor;
    InitDesc(*args);
    args->key = key;
    args->key_len = key_size;
    args->group_idx = cr_thread_id; // overloaded field
    args->future_signaller = Soe::SignalFuture;

    DeleteFuture *f = new DeleteFuture(this_sess, _par);
    uint64_t *p_pos = 0;
    if ( _par && pos ) {
        p_pos = dynamic_cast<SetFuture *>(_par)->InsertFuture(*f, static_cast<size_t>(*pos));
    }
    if ( p_pos ) {
        p_pos[1] = reinterpret_cast<uint64_t>(this);
        args->end_key = reinterpret_cast<const char *>(p_pos);
        args->end_key_len = 2*sizeof(uint64_t);
    }
    Duple _key(key, key_size);
    f->SetKey(_key);
    args->seq_number = f->GetSeq();

    pending_num_futures++;
    Session::Status ret = Delete(*args);

    if ( ret != Session::eOk ) {
        delete f;
        return 0;
    }

    DEBUG_FUTURES("Session::" << __FUNCTION__ << " fut=" << hex << f << dec << " sess=" << hex << this << dec <<
        " pending_num_futures=" << pending_num_futures << endl);
    f->SetPostRet(ret);
    return f;
}

/**
 * @brief Session Asynchronous Merge
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
 * @param[out] future         return MergeFuture
 */
MergeFuture *Session::MergeAsync(const char *key, size_t key_size, const char *data, size_t data_size, uint64_t *pos, Futurable *_par) throw(runtime_error)
{
    LocalArgsDescriptor *args = new LocalArgsDescriptor;
    InitDesc(*args);
    args->key = key;
    args->key_len = key_size;
    args->data = data;
    args->data_len = data_size;
    args->group_idx = cr_thread_id; // overloaded field
    args->future_signaller = Soe::SignalFuture;

    MergeFuture *f = new MergeFuture(this_sess, _par);
    uint64_t *p_pos = 0;
    if ( _par && pos ) {
        p_pos = dynamic_cast<SetFuture *>(_par)->InsertFuture(*f, static_cast<size_t>(*pos));
    }
    if ( p_pos ) {
        p_pos[1] = reinterpret_cast<uint64_t>(this);
        args->end_key = reinterpret_cast<const char *>(p_pos);
        args->end_key_len = 2*sizeof(uint64_t);
    }
    Duple _key(key, key_size);
    Duple _value(data, data_size);
    f->SetKey(_key);
    f->SetValue(_value);
    args->seq_number = f->GetSeq();

    pending_num_futures++;
    Session::Status ret = Merge(*args);

    if ( ret != Session::eOk ) {
        delete f;
        return 0;
    }

    DEBUG_FUTURES("Session::" << __FUNCTION__ << " fut=" << hex << f << dec << " sess=" << hex << this << dec <<
        " pending_num_futures=" << pending_num_futures << endl);
    f->SetPostRet(ret);
    return f;
}

/**
 * @brief Session Asynchronous GetSetAsync
 *
 * Get set of KV pairs (items) whose keys are passed in as a vector
 *
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_nfound  Specifies whether or not to fail the function on the first
 *                                   not found key
 * @param[in,out]  fail_pos          Position within items with not found key that caused a failure
 * @param[out] future                Return callable Futurable that will contain status and items
 *                                   If items.size() == 0 the return status is eInvalidArgument
 *                                   If fail_on_first_nfound == false and items.size() != 0 the return status is eOk
 *                                   if fail_on_first_nfound == true and items.size() != 0 and a key in keys is not found
 *                                   the return status is eNotFoundKey and fail_pos is set to first failed index
 */
GetSetFuture *Session::GetSetAsync(vector<pair<Duple, Duple>> &items,
    bool fail_on_first_nfound,
    size_t *fail_pos) throw(runtime_error)
{
    Session::Status sts = Session::Status::eOk;

    if ( fail_pos ) {
        *fail_pos = 0;
    }

    // SetFuture has a map of SimpleFuture objects
    GetSetFuture *set_fut = new GetSetFuture(this_sess, items, Session::Status::eOk, fail_on_first_nfound, fail_pos);
    set_fut->SetNumRequested(static_cast<size_t>(items.size()));
    size_t pos = 0;
    for ( vector<pair<Duple, Duple>>::iterator it = items.begin() ; it != items.end() ; ++it ) {
        Futurable *sim_fut = GetAsync((*it).first.data(), (*it).first.size(), (*it).second.buffer(), (*it).second.buffer_size(), &pos, set_fut);
        if ( !sim_fut ) {
            DEBUG_FUTURES("Session::" << __FUNCTION__ << " NULL future" << endl);
            continue;
        } else {
            sts = sim_fut->GetPostRet();
            if ( sts != Session::Status::eOk ) {
                DEBUG_FUTURES("Session::" << __FUNCTION__ << " sts" << sts << endl);
            }
        }

        pos++;
    }

    if ( set_fut ) {
        last_status = sts;
        set_fut->SetPostRet(last_status);
    }

    pending_num_futures++;
    DEBUG_FUTURES("Session::" << __FUNCTION__ << " sf=0x" << hex << set_fut << dec << " sess=0x" << hex << this <<
        dec << " pen_n_f=" << pending_num_futures <<
        " sf->n_req: " << set_fut->GetNumRequested() << " sf->n_rcv: " << set_fut->GetNumReceived() <<
        " sf->n_prc: " << set_fut->GetNumProcessed() << " pos: " << pos << endl);
    return set_fut;
}

/**
 * @brief Session Asynchronous PutSetAsync
 *
 * Put set of KV pairs (items) whose keys and values are passed in as a vector
 *
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_dup     Specifies whether or not to fail the function on the first duplicate key
 * @param[in,out] first_pos          Position within items with not found key that caused a failure
 * @param[out] future                Return callable Futurable that will contain status and items
 *                                   If items.size() == 0 the return status is eInvalidArgument
 *                                   If fail_on_first_nfound == false and items.size() != 0 the return status is eOk
 *                                   if fail_on_first_nfound == true and items.size() != 0 and a key in keys is not found
 *                                   the return status is eDuplicateKey and fail_pos is set to first failed index
 */
PutSetFuture *Session::PutSetAsync(vector<pair<Duple, Duple>> &items,
    bool fail_on_first_dup,
    size_t *fail_pos) throw(runtime_error)
{
    Session::Status sts = Session::Status::eOk;

    if ( fail_pos ) {
        *fail_pos= 0;
    }

    // SetFuture has a map of SimpleFuture objects
    PutSetFuture *set_fut = new PutSetFuture(this_sess, items, Session::Status::eOk, fail_on_first_dup, fail_pos);
    set_fut->SetNumRequested(static_cast<size_t>(items.size()));
    size_t pos = 0;
    for ( vector<pair<Duple, Duple>>::iterator it = items.begin() ; it != items.end() ; ++it ) {
        Futurable *sim_fut = PutAsync((*it).first.data(), (*it).first.size(), (*it).second.data(), (*it).second.size(), &pos, set_fut);
        if ( !sim_fut ) {
            DEBUG_FUTURES("Session::" << __FUNCTION__ << " NULL future" << endl);
            continue;
        } else {
            sts = sim_fut->GetPostRet();
            if ( sts != Session::Status::eOk ) {
                DEBUG_FUTURES("Session::" << __FUNCTION__ << " sts" << sts << endl);
            }
        }

        pos++;
    }

    if ( set_fut ) {
        last_status = sts;
        set_fut->SetPostRet(last_status);
    }

    pending_num_futures++;
    DEBUG_FUTURES("Session::" << __FUNCTION__ << " sf=0x" << hex << set_fut << dec << " sess=0x" << hex << this <<
        dec << " pen_n_f=" << pending_num_futures <<
        " sf->n_req: " << set_fut->GetNumRequested() << " sf->n_rcv: " << set_fut->GetNumReceived() <<
        " sf->n_prc: " << set_fut->GetNumProcessed() << " pos: " << pos << endl);
    return set_fut;
}

/**
 * @brief Session Asynchronous DeleteSetAsync
 *
 * Delete set keys passed in as a vector
 *
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_nfound  Specifies whether or not to fail the function on the first
 *                                   not found key
 * @param[in] first_pos              Position within items with not found key that caused a failure
 * @param[out] future                Return callable Futurable that will contain status and keys
 *                                   If keys.size() == 0 the return status is eInvalidArgument
 *                                   If fail_on_first_nfound == false and keys.size() != 0 the return status is eOk
 *                                   if fail_on_first_nfound == true and keys.size() != 0 and a key in keys is not found
 *                                   the return status is eNotFoundKey and fail_pos is set to first failed index
 */
DeleteSetFuture *Session::DeleteSetAsync(vector<Duple> &keys,
    bool fail_on_first_nfound,
    size_t *fail_pos) throw(runtime_error)
{
    Session::Status sts = Session::Status::eOk;

    if ( fail_pos ) {
        (*fail_pos)++;
    }

    // SetFuture has a map of SimpleFuture objects
    DeleteSetFuture *set_fut = new DeleteSetFuture(this_sess, keys, Session::Status::eOk, fail_on_first_nfound, fail_pos);
    set_fut->SetNumRequested(static_cast<size_t>(keys.size()));
    size_t pos = 0;
    for ( vector<Duple>::iterator it = keys.begin() ; it != keys.end() ; ++it ) {
        Futurable *sim_fut = DeleteAsync((*it).data(), (*it).size(), &pos, set_fut);
        if ( !sim_fut ) {
            DEBUG_FUTURES("Session::" << __FUNCTION__ << " NULL future" << endl);
            continue;
        } else {
            sts = sim_fut->GetPostRet();
            if ( sts != Session::Status::eOk ) {
                DEBUG_FUTURES("Session::" << __FUNCTION__ << " sts" << sts << endl);
            }
        }

        pos++;
    }

    if ( set_fut ) {
        last_status = sts;
        set_fut->SetPostRet(last_status);
    }

    pending_num_futures++;
    DEBUG_FUTURES("Session::" << __FUNCTION__ << " sf=0x" << hex << set_fut << dec << " sess=0x" << hex << this <<
        dec << " pen_n_f=" << pending_num_futures <<
        " sf->n_req: " << set_fut->GetNumRequested() << " sf->n_rcv: " << set_fut->GetNumReceived() <<
        " sf->n_prc: " << set_fut->GetNumProcessed() << " pos: " << pos << endl);
    return set_fut;
}

/**
 * @brief Session Asynchronous MergeSetAsync
 *
 * Merge in a set of KV pairs (items) whose keys and values are passed in as a vector
 * The result, as in single Merge(), depends on the merge filter
 *
 * @param[in]  items          Vector of key/data Duple pairs
 * @param[out] future         Return callable Futurable that will contain status and items
 *                            If keys.size() == 0 the return status is eInvalidArgument
 */
MergeSetFuture *Session::MergeSetAsync(vector<pair<Duple, Duple>> &items) throw(runtime_error)
{
    Session::Status sts = Session::Status::eOk;

    // SetFuture has a map of SimpleFuture objects
    MergeSetFuture *set_fut = new MergeSetFuture(this_sess, items, Session::Status::eOk);
    set_fut->SetNumRequested(static_cast<size_t>(items.size()));
    size_t pos = 0;
    for ( vector<pair<Duple, Duple>>::iterator it = items.begin() ; it != items.end() ; ++it ) {
        Futurable *sim_fut = MergeAsync((*it).first.data(), (*it).first.size(), (*it).second.data(), (*it).second.size(), &pos, set_fut);
        if ( !sim_fut ) {
            DEBUG_FUTURES("Session::" << __FUNCTION__ << " NULL future" << endl);
            continue;
        } else {
            sts = sim_fut->GetPostRet();
            if ( sts != Session::Status::eOk ) {
                DEBUG_FUTURES("Session::" << __FUNCTION__ << " sts" << sts << endl);
            }
        }

        pos++;
    }

    if ( set_fut ) {
        last_status = sts;
        set_fut->SetPostRet(last_status);
    }

    pending_num_futures++;
    DEBUG_FUTURES("Session::" << __FUNCTION__ << " sf=0x" << hex << set_fut << dec << " sess=0x" << hex << this <<
        dec << " pen_n_f=" << pending_num_futures <<
        " sf->n_req: " << set_fut->GetNumRequested() << " sf->n_rcv: " << set_fut->GetNumReceived() <<
        " sf->n_prc: " << set_fut->GetNumProcessed() << " pos: " << pos << endl);
    return set_fut;
}

size_t Session::DestroySetFuture(Futurable *fut, bool no_lock)
{
    size_t collected = 0;
    SetFuture *set_f = dynamic_cast<SetFuture *>(fut);
    size_t futures_processed = set_f->GetNumProcessed();
    for ( size_t idx = 0 ; idx < set_f->futures.size() ; idx++ ) {
        if ( set_f->futures[idx] ) {
            if ( idx < futures_processed && set_f->futures[idx]->GetReceived() == true ) {
                delete set_f->futures[idx];
            } else {
                if ( no_lock == false ) {
                    WriteLock w_lock(Session::garbage_futures_lock);
                    garbage_futures.push_back(set_f->futures[idx]);
                } else {
                    garbage_futures.push_back(set_f->futures[idx]);
                }
                collected++;
            }
            set_f->futures[idx] = 0;
        }
    }

    return collected;
}

/**
 * @brief Session DeleteFuture
 *
 * Futures are managed objects and can be deleted only through their sessions
 *
 * @param[in]  fut          Futurable
 *
 */
void Session::DestroyFuture(Futurable *fut)
{
    if ( dynamic_cast<SimpleFuture *>(fut) ) {
        DEBUG_FUTURES("Session::" << __FUNCTION__ << " SimpleFuture=0x" << hex << fut << dec << endl);
        delete fut;
    } else if ( dynamic_cast<SetFuture *>(fut) ) {
        DEBUG_FUTURES("Session::" << __FUNCTION__ << " SetFuture=0x" << hex << fut << dec << endl);
        SetFuture *set_f = dynamic_cast<SetFuture *>(fut);
        (void )DestroySetFuture(set_f);

        // if all children have been received it's safe to delete fut, otherwise garbage collect it
        if ( set_f->GetNumProcessed() >= set_f->GetNumRequested() ) {
            delete fut;
        } else {
            WriteLock w_lock(Session::garbage_futures_lock);
            garbage_futures.push_back(fut);
        }
    } else {
        ostringstream str_buf;
        str_buf << __FUNCTION__ << ":" << __LINE__ << " Unknown Soe future " << typeid(fut).name() << endl << flush;
        logErrMsg(str_buf.str().c_str());
    }

    // check again the garbage_futures
    {
        WriteLock w_lock(Session::garbage_futures_lock);

        size_t coll;
        for ( ;; ) {
            coll = 0;
            for ( auto it = garbage_futures.begin() ; it != garbage_futures.end() ; it++ ) {
                if ( dynamic_cast<SimpleFuture *>(*it) ) {
                    if ( (*it)->GetReceived() == true ) {
                        delete (*it);
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
                    str_buf << __FUNCTION__ << ":" << __LINE__ << " Unknown Soe future " << typeid(fut).name() << endl << flush;
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

void Session::DecPendingNumFutures()
{
    pending_num_futures--;
}

void Session::IncPendingNumFutures()
{
    pending_num_futures++;
}

}
