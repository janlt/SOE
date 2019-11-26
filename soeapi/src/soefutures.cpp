/**
 * @file    soefutures.cpp
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
 * soefutures.cpp
 *
 *  Created on: Dec 4, 2017
 *      Author: Jan Lisowiec
 */

#include <stdint.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/chrono.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <new>

#include "soe_soe.hpp"

using namespace std;
using namespace KvsFacade;
using namespace RcsdbFacade;

namespace Soe {

SimpleFuture::SimpleFuture(std::shared_ptr<Session> _sess, Session::Status _post_ret, SeqNumber _seq, Futurable *_par)
    :Futurable(_post_ret, _seq),
    sess(_sess),
    parent(_par),
    timeout_value(SimpleFuture::initial_timeout_value)
{
    futures_idxs[0]= SimpleFuture::invalid_idx;
    futures_idxs[1]= SimpleFuture::invalid_idx;
}

SimpleFuture::SimpleFuture(std::shared_ptr<Session> _sess, Futurable *_par)
    :Futurable(Session::Status::eOk, reinterpret_cast<SeqNumber>(this)),
    sess(_sess),
    parent(_par),
    timeout_value(SimpleFuture::initial_timeout_value)
{
    futures_idxs[0]= SimpleFuture::invalid_idx;
    futures_idxs[1]= SimpleFuture::invalid_idx;
}

SimpleFuture::~SimpleFuture()
{
    ResetExtraCheck();
}

/*
 * Some default implementations
 */
const char *SimpleFuture::GetKey()
{
    return 0;
}

size_t SimpleFuture::GetKeySize()
{
    return 0;
}

Duple SimpleFuture::GetDupleKey()
{
    return Duple();
}

const char *SimpleFuture::GetValue()
{
    return 0;
}

size_t SimpleFuture::GetValueSize()
{
    return 0;
}

Duple SimpleFuture::GetDupleValue()
{
    return Duple();
}

const std::string SimpleFuture::GetPath()
{
    return "";
}

void SimpleFuture::Signal(LocalArgsDescriptor *_args)
{
    ResetExtraCheck();
    GetPromise().set_value(1);
    SetReceived();
    assert(_args);
    Resolve(_args);

#ifdef __DEBUG_FUTURES__
    assert(_args->end_key_len == 2*sizeof(uint64_t));
    const uint64_t *idxs = reinterpret_cast<const uint64_t *>(_args->end_key);
    size_t pos = static_cast<size_t>(idxs[0]);
    DEBUG_FUTURES("SimpleFuture::" << __FUNCTION__ << " fut=" << hex << this << dec << " parent=0x" << hex << parent << dec <<
        " pos: " << pos << " sess=0x" << hex << idxs[1] << dec << endl);
#endif

    if ( parent ) { // this is child of a SetFuture (PutSet/GetSet/DeleteSet/MergeSet)
        if ( dynamic_cast<SetFuture *>(parent) ) {
            assert(_args->end_key_len == 2*sizeof(uint64_t));
            const uint64_t *f_idx = reinterpret_cast<const uint64_t *>(_args->end_key);
            if ( dynamic_cast<SetFuture *>(parent)->Validate(GetSeq(), f_idx, _args) == true ) {
                dynamic_cast<SetFuture *>(parent)->IncNumReceived(_args);
            }
        }
    }

    sess->DecPendingNumFutures();
}

void SimpleFuture::Reset()
{
    parent = 0;
}

void SimpleFuture::SetTimeoutValueFactor(uint32_t _fac)
{
    timeout_value += _fac; // in secs
}

/**
 * @brief SimpleFuture Get()
 *
 * Get() - wait for the future to become ready, makes future's status and result available
 *
 * @param[out] status         return status, Session::Status::eOk if ok, otherwise an error (see soesession.hpp)
 */
Session::Status SimpleFuture::Get()
{
    Session::Status sts = Session::Status::eOk;

    auto fut = GetPromise().get_future();
    boost::future_status fut_sts = fut.wait_for(boost::chrono::seconds(timeout_value));
    if ( fut_sts == boost::future_status::deferred ) {
        sts = Session::Status::eOpNotSupported;
        SetPostRet(sts);
    } else if ( fut_sts == boost::future_status::timeout ) {
        sts = Session::Status::eOpTimeout;
        SetPostRet(sts);
    } else if ( fut_sts == boost::future_status::ready ) {
        sts = Session::Status::eOk;
    }

    if ( sts == Session::Status::eOk ) {
        int promised_num_ops = fut.get();
        if ( promised_num_ops == 1 ) {
            return GetPostRet();
        }
    }
    return sts;
}

void SimpleFuture::SetParent(Futurable *par)
{
    parent = par;
}

Futurable *SimpleFuture::GetParent()
{
    return parent;
}


PutFuture::PutFuture(std::shared_ptr<Session> _sess, Session::Status _post_ret, uint64_t _seq, Futurable *_par)
    :SimpleFuture(_sess, _post_ret, _seq, _par)
{
    DEBUG_FUTURES("PutFuture::" << __FUNCTION__ << " fut=" << hex << this << dec <<
        " sess=" << hex << _sess.get() << dec << " parent=0x" << hex << _par << dec << endl);
}

PutFuture::PutFuture(std::shared_ptr<Session> _sess, Futurable *_par)
    :SimpleFuture(_sess, _par)
{
    DEBUG_FUTURES("PutFuture::" << __FUNCTION__ << " fut=" << hex << this << dec <<
        " sess=" << hex << _sess.get() << dec << " parent=0x" << hex << _par << dec << endl);
}

void PutFuture::Reset()
{
    SimpleFuture::Reset();
}

PutFuture::~PutFuture()
{
    ResetExtraCheck();
}

void PutFuture::Resolve(LocalArgsDescriptor *_args)
{
    SetPostRet(static_cast<Session::Status>(_args->status));
}

/**
 * @brief PutFuture GetKey()
 *
 *  GetKey() returns pointer to the key
 *
 * @param[out] char *         return pointer to the key
 */
const char *PutFuture::GetKey()
{
    return key.data();
}

/**
 * @brief PutFuture GetKeySize()
 *
 * GetKeySize() returns size of the key
 *
 * @param[out] size_t         return size of the key
 */
size_t PutFuture::GetKeySize()
{
    return key.size();
}

/**
 * @brief PutFuture GetDupleKey()
 *
 * GetDupleKey() returns Duple of the key
 *
 * @param[out] Duple         return Duple of the key
 */
Duple PutFuture::GetDupleKey()
{
    return Duple(key.data(), key.size());
}

const char *PutFuture::GetValue()
{
    return 0;
}

size_t PutFuture::GetValueSize()
{
    return 0;
}

Duple PutFuture::GetDupleValue()
{
    return Duple(0, 0);
}

Session::Status PutFuture::Status()
{
    return GetPostRet();
}

void PutFuture::SetKey(Duple &_key)
{
    SetTimeoutValueFactor(static_cast<uint32_t>(_key.size()/1000));
    key = _key;
}

void PutFuture::SetValue(Duple &_value)
{
    SetTimeoutValueFactor(static_cast<uint32_t>(_value.size()/10000));
    value = _value;
}




GetFuture::GetFuture(std::shared_ptr<Session> _sess, Session::Status _post_ret, uint64_t _seq, Futurable *_par, char *_buf, size_t _buf_size)
    :SimpleFuture(_sess, _post_ret, _seq, _par)
{
    DEBUG_FUTURES("GetFuture::" << __FUNCTION__ << " fut=" << hex << this << dec <<
        " sess=" << hex << _sess.get() << dec << " parent=0x" << hex << _par << dec << endl);
    value = Duple(_buf, _buf_size);
}

GetFuture::GetFuture(std::shared_ptr<Session> _sess, Futurable *_par, char *_buf, size_t _buf_size)
    :SimpleFuture(_sess, _par)
{
    DEBUG_FUTURES("GetFuture::" << __FUNCTION__ << " fut=" << hex << this << dec <<
        " sess=" << hex << _sess.get() << dec << " parent=0x" << hex << _par << dec << endl);
    value = Duple(_buf, _buf_size);
}

void GetFuture::Reset()
{
    SimpleFuture::Reset();
}

GetFuture::~GetFuture()
{
    ResetExtraCheck();
}

void GetFuture::Resolve(LocalArgsDescriptor *_args)
{
    SetPostRet(static_cast<Session::Status>(_args->status));
    if ( _args->status == Session::Status::eOk ) {
        if ( _args->buf_len > value.size() ) {
            SetPostRet(Session::Status::eTruncatedValue);
        }

        ::memcpy(value.buffer(), _args->buf, MIN(_args->buf_len, value.size()));
        value.set_size(MIN(_args->buf_len, value.size()));
    }
}

/**
 * @brief GetFuture GetValue()
 *
 * GetValue() returns pointer to the value
 *
 * @param[out] char *         return pointer to the value
 */
const char *GetFuture::GetValue()
{
    return value.data();
}

/**
 * @brief GetFuture GetValueSize()
 *
 * GetValueSize() returns size of the value
 *
 * @param[out] size_t         return size of the value
 */
size_t GetFuture::GetValueSize()
{
    return value.size();
}

/**
 * @brief GetFuture GetDupleValue()
 *
 * GetDupleValue() returns Duple of the value
 *
 * @param[out] Duple         return Duple of the value
 */
Duple GetFuture::GetDupleValue()
{
    return Duple(value.data(), value.size());
}

const char *GetFuture::GetKey()
{
    return key.data();
}

size_t GetFuture::GetKeySize()
{
    return key.size();
}

Duple GetFuture::GetDupleKey()
{
    return Duple(key.data(), key.size());
}

Session::Status GetFuture::Status()
{
    return GetPostRet();
}

void GetFuture::SetKey(Duple &_key)
{
    SetTimeoutValueFactor(static_cast<uint32_t>(_key.size()/1000));
    key = _key;
}

void GetFuture::SetValue(Duple &_value)
{
    SetTimeoutValueFactor(static_cast<uint32_t>(_value.size()/10000));
    value = _value;
}





DeleteFuture::DeleteFuture(std::shared_ptr<Session> _sess, Session::Status _post_ret, uint64_t _seq, Futurable *_par)
    :SimpleFuture(_sess, _post_ret, _seq, _par)
{
    DEBUG_FUTURES("DeleteFuture::" << __FUNCTION__ << " fut=" << hex << this << dec <<
        " sess=" << hex << _sess.get() << dec << " parent=0x" << hex << _par << dec << endl);
}

DeleteFuture::DeleteFuture(std::shared_ptr<Session> _sess, Futurable *_par)
    :SimpleFuture(_sess, _par)
{
    DEBUG_FUTURES("DeleteFuture::" << __FUNCTION__ << " fut=" << hex << this << dec <<
        " sess=" << hex << _sess.get() << dec << " parent=0x" << hex << _par << dec << endl);
}

void DeleteFuture::Reset()
{
    SimpleFuture::Reset();
}

DeleteFuture::~DeleteFuture()
{
    ResetExtraCheck();
}

void DeleteFuture::Resolve(LocalArgsDescriptor *_args)
{
    SetPostRet(static_cast<Session::Status>(_args->status));
}

/**
 * @brief DeleteFuture GetKey()
 *
 * GetKey() returns pointer to the key
 *
 * @param[out] char *         return pointer to the key
 */
const char *DeleteFuture::GetKey()
{
    return key.data();
}

/**
 * @brief DeleteFuture GetKeySize()
 *
 * GetKeySize() returns size of the key
 *
 * @param[out] size_t         return size of the key
 */
size_t DeleteFuture::GetKeySize()
{
    return key.size();
}

/**
 * @brief DeleteFuture GetDupleKey()
 *
 * GetDupleKey() returns Duple of the key
 *
 * @param[out] Duple         return Duple of the key
 */
Duple DeleteFuture::GetDupleKey()
{
    return Duple(key.data(), key.size());
}

const char *DeleteFuture::GetValue()
{
    return 0;
}

size_t DeleteFuture::GetValueSize()
{
    return 0;
}

Duple DeleteFuture::GetDupleValue()
{
    return Duple(0, 0);
}

Session::Status DeleteFuture::Status()
{
    return GetPostRet();
}

void DeleteFuture::SetKey(Duple &_key)
{
    SetTimeoutValueFactor(static_cast<uint32_t>(_key.size()/1000));
    key = _key;
}





MergeFuture::MergeFuture(std::shared_ptr<Session> _sess, Session::Status _post_ret, uint64_t _seq, Futurable *_par)
    :SimpleFuture(_sess, _post_ret, _seq, _par)
{
    DEBUG_FUTURES("MergeFuture::" << __FUNCTION__ << " fut=" << hex << this << dec <<
        " sess=" << hex << _sess.get() << dec << " parent=0x" << hex << _par << dec << endl);
}

MergeFuture::MergeFuture(std::shared_ptr<Session> _sess, Futurable *_par)
    :SimpleFuture(_sess, _par)
{
    DEBUG_FUTURES("MergeFuture::" << __FUNCTION__ << " fut=" << hex << this << dec <<
        " sess=" << hex << _sess.get() << dec << " parent=0x" << hex << _par << dec << endl);
}

void MergeFuture::Reset()
{
    SimpleFuture::Reset();
}

MergeFuture::~MergeFuture()
{
    ResetExtraCheck();
}

void MergeFuture::Resolve(LocalArgsDescriptor *_args)
{
    SetPostRet(static_cast<Session::Status>(_args->status));
}

/**
 * @brief MergeFuture GetKey()
 *
 * GetKey() returns pointer to the key
 *
 * @param[out] char *         return pointer to the key
 */
const char *MergeFuture::GetKey()
{
    return key.data();
}

/**
 * @brief MergeFuture GetKeySize()
 *
 * GetKeySize() returns size of the key
 *
 * @param[out] char *         return pointer to the key
 */
size_t MergeFuture::GetKeySize()
{
    return key.size();
}

/**
 * @brief MergeFuture GetDupleKey()
 *
 * GetDupleKey() returns Duple of the key
 *
 * @param[out] Duple         return Duple of the key
 */
Duple MergeFuture::GetDupleKey()
{
    return Duple(key.data(), key.size());
}

const char *MergeFuture::GetValue()
{
    return 0;
}

size_t MergeFuture::GetValueSize()
{
    return 0;
}

Duple MergeFuture::GetDupleValue()
{
    return Duple(0, 0);
}

Session::Status MergeFuture::Status()
{
    return GetPostRet();
}

void MergeFuture::SetKey(Duple &_key)
{
    SetTimeoutValueFactor(static_cast<uint32_t>(_key.size()/1000));
    key = _key;
}

void MergeFuture::SetValue(Duple &_value)
{
    SetTimeoutValueFactor(static_cast<uint32_t>(_value.size()/10000));
    value = _value;
}







SetFuture::SetFuture(std::shared_ptr<Session> _sess, Session::Status _post_ret, bool _fail_detect, size_t *_fail_pos)
    :Futurable(_post_ret),
     sess(_sess),
     num_requested(0),
     num_received(0),
     num_processed(0),
     fail_detect(_fail_detect),
     fail_pos(SetFuture::invalid_fail_pos),
     timeout_value(SetFuture::initial_timeout_value)
{
    if ( _fail_pos ) {
        fail_pos = *_fail_pos;
    }
}

SetFuture::~SetFuture()
{
    ResetExtraCheck();
    Reset();
    futures.clear();
}

void SetFuture::SetNumRequested(size_t _num_requested)
{
    num_requested = _num_requested;
}

void SetFuture::Reset()
{
    for ( size_t i = 0 ; i < futures.size() ; i++ ) {
        if ( futures[i] ) {
            delete futures[i];
            futures[i] = 0;
        }
    }
}

void SetFuture::Resolve(LocalArgsDescriptor *_args)
{}

uint64_t *SetFuture::InsertFuture(Futurable &_fut, size_t pos)
{
    assert(pos < futures.size());
    futures[pos] = &_fut;
    dynamic_cast<SimpleFuture &>(_fut).futures_idxs[0] = static_cast<uint64_t>(pos);
    assert(dynamic_cast<SimpleFuture *>(&_fut) && dynamic_cast<SimpleFuture &>(_fut).GetParent());
    DEBUG_FUTURES("InsertFuture::" << __FUNCTION__ << " fut=0x" << hex << &_fut << dec << " pos: " << pos <<
        " parent=0x" << hex << dynamic_cast<SimpleFuture &>(_fut).GetParent() << dec << endl);
    return &dynamic_cast<SimpleFuture &>(_fut).futures_idxs[0];
}

/**
 * @brief SetFuture GetFailDetect()
 *
 * GetFailDetect() returns fail_detect boolean flag
 *
 * @param[out] bool         return fail_detect
 */
bool SetFuture::GetFailDetect()
{
    return fail_detect;
}

/**
 * @brief SetFuture GetFailPos()
 *
 * GetFailPos() returns failed position into items (valid if fail_detect was set to true)
 *
 * @param[out] fail_pos         return failed position into items
 */
uint32_t SetFuture::GetFailPos()
{
    return fail_pos;
}

void SetFuture::Signal(LocalArgsDescriptor *_args)
{
    GetPromise().set_value(num_received);
}

void SetFuture::SetTimeoutValueFactor(uint32_t _fac)
{
    timeout_value += _fac; // in secs
}

/**
 * @brief SetFuture Get()
 *
 * Get() - wait for the future to become ready, makes future's status and result available
 *
 * @param[out] status         return status, Session::Status::eOk if ok, otherwise an error (see soesession.hpp)
 */
Session::Status SetFuture::Get()
{
    Session::Status sts = Session::Status::eOk;
    SetTimeoutValueFactor(static_cast<uint32_t>(futures.size()/500));

    auto fut = GetPromise().get_future();
    boost::future_status fut_sts = fut.wait_for(boost::chrono::seconds(timeout_value));
    if ( fut_sts == boost::future_status::deferred ) {
        sts = Session::Status::eOpNotSupported;
        SetPostRet(sts);
    } else if ( fut_sts == boost::future_status::timeout ) {
        sts = Session::Status::eOpTimeout;
        DEBUG_FUTURES("SetFuture::" << __FUNCTION__ << " fut=0x" << hex << this << dec << " num_received: " << num_received << endl);
        SetPostRet(sts);
    } else if ( fut_sts == boost::future_status::ready ) {
        sts = Session::Status::eOk;
    }

    if ( sts == Session::Status::eOk ) {
        int promised_num_ops = fut.get();
        if ( promised_num_ops == 1 ) {
            return GetPostRet();
        }
    }
    sess->DecPendingNumFutures();
    return sts;
}

void SetFuture::SetParent(Futurable *par)
{}

Futurable *SetFuture::GetParent()
{
    return 0;
}

void SetFuture::IncNumReceived(LocalArgsDescriptor *_args)
{
    num_processed++;

    if ( _args->status != Session::Status::eOk ) {
        if ( fail_detect == true ) {
            // check if fail_pos has been set
            if ( fail_pos == SetFuture::invalid_fail_pos ) {
                const uint64_t *idxs = reinterpret_cast<const uint64_t *>(_args->end_key);
                fail_pos = static_cast<size_t>(idxs[0]);
            }
        }
        SetPostRet(static_cast<Session::Status>(_args->status));
    }

    num_received++;
    if ( num_requested == num_received ) {
        DEBUG_FUTURES("SetFuture::" << __FUNCTION__ << " fut=0x" << hex << this << dec << " num_received: " << num_received << endl);
        Signal(0);
    }
}

void SetFuture::SetFuturesSize(size_t _size)
{
    futures.resize(_size);
}

size_t SetFuture::GetNumReceived()
{
    return num_received;
}

size_t SetFuture::GetNumRequested()
{
    return num_requested;
}

size_t SetFuture::GetNumProcessed()
{
    return num_requested;
}

Session::Status SetFuture::Status()
{
    return GetPostRet();
}




/**
 * @brief GetSetFuture c-tor
 *
 * C-tor protected, GetSetFuture can be created only via Session::GetSetAsync()
 *
 * _items has to stay valid until Get() returns.
 *
 */
GetSetFuture::GetSetFuture(std::shared_ptr<Session> _sess,
    std::vector<std::pair<Duple, Duple>> &_items,
    Session::Status _post_ret,
    bool _fail_detect,
    size_t *_fail_pos)
    :SetFuture(_sess, _post_ret, _fail_detect, _fail_pos),
     items(_items)
{
    SetFuturesSize(items.size());
}

GetSetFuture::~GetSetFuture()
{
    ResetExtraCheck();
}

void GetSetFuture::Resolve(LocalArgsDescriptor *_args)
{}

void GetSetFuture::Reset()
{
    SetFuture::Reset();
}

//
// Vectored Validate will use end_key/end_key_len field as an index
// into input vector.
// Future will be validated using the sequence and the key, and then
// the request's value will be saved in 'value' element of the corresponding
// slot in the input vector.
//
bool GetSetFuture::Validate(SeqNumber seq, const uint64_t *_idxs, LocalArgsDescriptor *_args)
{
    size_t pos = static_cast<size_t>(_idxs[0]);
    if ( pos >= futures.size() ) {
        DEBUG_FUTURES("GettSetFuture::" << __FUNCTION__ << " fut=0x" << hex << this << dec << " pos: " << pos <<
            " futs.size(): " << futures.size() << endl);
        return false;
    }
    //return true;
    if ( futures[pos] && futures[pos]->GetSeq() == seq ) {
        // items are ordered same as futures, i.e. items[pos] corresponds to futures[pos]
        // which also means the keys must match, i.e. _args->key === items[pos].first.data()
        Duple dupka(items[pos].first.data(), items[pos].first.size());
        if ( string(dupka.data(), dupka.size()) != string(_args->key, _args->key_len) ) {
            DEBUG_FUTURES("GetSetFuture::" << __FUNCTION__ << " fut=0x" << hex << this << dec << " pos: " << pos <<
                " key not matching" << endl);
            return false;
        }
        items[pos].second = Duple(_args->buf, _args->buf_len);
        return true;
    }

    DEBUG_FUTURES("GetSetFuture::" << __FUNCTION__ << " fut=" << hex << this << dec << " invalid pos: " << pos <<
        " seq=0x" << hex << seq << dec << " f[pos]=0x" << hex << futures[pos]->GetSeq() << dec <<
        " _args->seq=0x" << hex << _args->seq_number << dec <<
        " _idxs[1]=0x" << hex << _idxs[1] << dec << " this->sess=" << hex << this->GetSession().get() << dec << endl);
    return false;
}

/**
 * @brief GetSetFuture GetItems()
 *
 * GetItems() returns items (normally must be called after Get())
 *
 * @param[out] items         return items
 */
std::vector<std::pair<Duple, Duple>> &GetSetFuture::GetItems()
{
    return items;
}


/**
 * @brief PutSetFuture c-tor
 *
 * C-tor protected, PutSetFuture can be created only via Session::PutSetAsync()
 *
 * _items has to stay valid until Get() returns.
 *
 */
PutSetFuture::PutSetFuture(std::shared_ptr<Session> _sess,
    std::vector<std::pair<Duple, Duple>> &_items,
    Session::Status _post_ret,
    bool _fail_detect,
    size_t *_fail_pos)
    :SetFuture(_sess, _post_ret, _fail_detect, _fail_pos),
     items(_items)
{
    SetFuturesSize(items.size());
}

PutSetFuture::~PutSetFuture()
{
    ResetExtraCheck();
}

void PutSetFuture::Resolve(LocalArgsDescriptor *_args)
{}

void PutSetFuture::Reset()
{
    SetFuture::Reset();
}

bool PutSetFuture::Validate(SeqNumber seq, const uint64_t *_idxs, LocalArgsDescriptor *_args)
{
    size_t pos = static_cast<size_t>(_idxs[0]);
    if ( pos >= futures.size() ) {
        DEBUG_FUTURES("PutSetFuture::" << __FUNCTION__ << " fut=0x" << hex << this << dec << " pos: " << pos <<
            " futs.size(): " << futures.size() << endl);
        return false;
    }
    //return true;
    if ( futures[pos] && futures[pos]->GetSeq() == seq ) {
        // items are ordered the same as futures, i.e. items[pos] corresponds to futures[pos]
        // which also means the keys must match, i.e. _args->key === items[pos].first.data()
        Duple dupka(items[pos].first.data(), items[pos].first.size());
        if ( string(dupka.data(), dupka.size()) != string(_args->key, _args->key_len) ) {
            DEBUG_FUTURES("PutSetFuture::" << __FUNCTION__ << " fut=0x" << hex << this << dec << " pos: " << pos <<
                " key not matching" << endl);
            return false;
        }
        return true;
    }

    DEBUG_FUTURES("PutSetFuture::" << __FUNCTION__ << " fut=" << hex << this << dec << " invalid pos: " << pos <<
        " seq=0x" << hex << seq << dec << " f[pos]=0x" << hex << futures[pos]->GetSeq() << dec <<
        " _args->seq=0x" << hex << _args->seq_number << dec <<
        " _idxs[1]=0x" << hex << _idxs[1] << dec << " this->sess=" << hex << this->GetSession().get() << dec << endl);
    return false;
}

/**
 * @brief PutSetFuture GetItems()
 *
 * GetItems() returns items (normally must be called after Get())
 *
 * @param[out] items         return items
 */
std::vector<std::pair<Duple, Duple>> &PutSetFuture::GetItems()
{
    return items;
}



/**
 * @brief DeleteSetFuture c-tor
 *
 * C-tor protected, DeleteSetFuture can be created only via Session::DeleteSetAsync()
 *
 * _items has to stay valid until Get() returns.
 *
 */
DeleteSetFuture::DeleteSetFuture(std::shared_ptr<Session> _sess,
    std::vector<Duple> &_keys,
    Session::Status _post_ret,
    bool _fail_detect,
    size_t *_fail_pos)
    :SetFuture(_sess, _post_ret, _fail_detect, _fail_pos),
     keys(_keys)
{
    SetFuturesSize(keys.size());
}

DeleteSetFuture::~DeleteSetFuture()
{
    ResetExtraCheck();
}

void DeleteSetFuture::Resolve(LocalArgsDescriptor *_args)
{}

void DeleteSetFuture::Reset()
{
    SetFuture::Reset();
}

bool DeleteSetFuture::Validate(SeqNumber seq, const uint64_t *_idxs, LocalArgsDescriptor *_args)
{
    size_t pos = static_cast<size_t>(_idxs[0]);
    if ( pos >= futures.size() ) {
        DEBUG_FUTURES("DeleteSetFuture::" << __FUNCTION__ << " fut=0x" << hex << this << dec << " pos: " << pos <<
            " futs.size(): " << futures.size() << endl);
        return false;
    }
    //return true;
    if ( futures[pos] && futures[pos]->GetSeq() == seq ) {
        // items are ordered same as futures, i.e. items[pos] corresponds to futures[pos]
        // which also means the keys must match, i.e. _args->key === items[pos].first.data()
        Duple dupka(keys[pos].data(), keys[pos].size());
        if ( string(dupka.data(), dupka.size()) != string(_args->key, _args->key_len) ) {
            DEBUG_FUTURES("DeleteSetFuture::" << __FUNCTION__ << " fut=0x" << hex << this << dec << " pos: " << pos <<
                " key not matching" << endl);
            return false;
        }
        return true;
    }

    DEBUG_FUTURES("DeleteSetFuture::" << __FUNCTION__ << " fut=" << hex << this << dec << " invalid pos: " << pos <<
        " seq=0x" << hex << seq << dec << " f[pos]=0x" << hex << futures[pos]->GetSeq() << dec <<
        " _args->seq=0x" << hex << _args->seq_number << dec <<
        " _idxs[1]=0x" << hex << _idxs[1] << dec << " this->sess=" << hex << this->GetSession().get() << dec << endl);
    return false;
}

/**
 * @brief DeleteSetFuture GetKeys()
 *
 * GetKeys() returns keys (normally must be called after Get())
 *
 * @param[out] items         return keys
 */
std::vector<Duple> &DeleteSetFuture::GetKeys()
{
    return keys;
}



/**
 * @brief MergeSetFuture c-tor
 *
 * C-tor protected, MergeSetFuture can be created only via Session::MergeSetAsync()
 *
 * _items has to stay valid until Get() returns.
 *
 */
MergeSetFuture::MergeSetFuture(std::shared_ptr<Session> _sess,
    std::vector<std::pair<Duple, Duple>> &_items,
    Session::Status _post_ret,
    bool _fail_detect,
    size_t *_fail_pos)
    :SetFuture(_sess, _post_ret, _fail_detect, _fail_pos),
     items(_items)
{
    SetFuturesSize(items.size());
}

MergeSetFuture::~MergeSetFuture()
{
    ResetExtraCheck();
}

void MergeSetFuture::Resolve(LocalArgsDescriptor *_args)
{}

void MergeSetFuture::Reset()
{
    SetFuture::Reset();
}

bool MergeSetFuture::Validate(SeqNumber seq, const uint64_t *_idxs, LocalArgsDescriptor *_args)
{
    size_t pos = static_cast<size_t>(_idxs[0]);
    if ( pos >= futures.size() ) {
        DEBUG_FUTURES("MergeSetFuture::" << __FUNCTION__ << " fut=0x" << hex << this << dec << " pos: " << pos <<
            " futs.size(): " << futures.size() << endl);
        return false;
    }
    //return true;
    if ( futures[pos] && futures[pos]->GetSeq() == seq ) {
        // items are ordered same as futures, i.e. items[pos] corresponds to futures[pos]
        // which also means the keys must match, i.e. _args->key === items[pos].first.data()
        Duple dupka(items[pos].first.data(), items[pos].first.size());
        if ( string(dupka.data(), dupka.size()) != string(_args->key, _args->key_len) ) {
            DEBUG_FUTURES("MergeSetFuture::" << __FUNCTION__ << " fut=0x" << hex << this << dec << " pos: " << pos <<
                " key not matching" << endl);
            return false;
        }
        return true;
    }

    DEBUG_FUTURES("MergeSetFuture::" << __FUNCTION__ << " fut=" << hex << this << dec << " invalid pos: " << pos <<
        " seq=0x" << hex << seq << dec << " f[pos]=0x" << hex << futures[pos]->GetSeq() << dec <<
        " _args->seq=0x" << hex << _args->seq_number << dec <<
        " _idxs[1]=0x" << hex << _idxs[1] << dec << " this->sess=" << hex << this->GetSession().get() << dec << endl);
    return false;
}




BackupFuture::BackupFuture(std::shared_ptr<Session> _sess, Session::Status _post_ret, uint64_t _seq, const std::string &_path, Futurable *_par)
    :SimpleFuture(_sess, _post_ret, _seq, _par)
{
    DEBUG_FUTURES("GetFuture::" << __FUNCTION__ << " fut=" << hex << this << dec <<
        " sess=" << hex << _sess.get() << dec << " parent=0x" << hex << _par << dec << endl);
    backup_path = _path;
}

BackupFuture::BackupFuture(std::shared_ptr<Session> _sess, Futurable *_par)
    :SimpleFuture(_sess, _par)
{
    DEBUG_FUTURES("GetFuture::" << __FUNCTION__ << " fut=" << hex << this << dec <<
        " sess=" << hex << _sess.get() << dec << " parent=0x" << hex << _par << dec << endl);
}

void BackupFuture::Reset()
{
    SimpleFuture::Reset();
}

BackupFuture::~BackupFuture()
{
    ResetExtraCheck();
}

void BackupFuture::Resolve(LocalArgsDescriptor *_args)
{
    SetPostRet(static_cast<Session::Status>(_args->status));
}

/**
 * @brief BackupFuture GetPath()
 *
 * GetValue() returns backup path
 *
 * @param[out] std::string        return path
 */
const std::string BackupFuture::GetPath()
{
    return backup_path;
}

Session::Status BackupFuture::Status()
{
    return GetPostRet();
}

void BackupFuture::SetPath(const std::string &_path)
{
    SetTimeoutValueFactor(static_cast<uint32_t>(_path.size()/1000));
    backup_path = _path;
}

const char *BackupFuture::GetKey()
{
    return SimpleFuture::GetKey();
}

size_t BackupFuture::GetKeySize()
{
    return SimpleFuture::GetKeySize();
}

Duple BackupFuture::GetDupleKey()
{
    return SimpleFuture::GetDupleKey();
}

const char *BackupFuture::GetValue()
{
    return SimpleFuture::GetValue();
}

size_t BackupFuture::GetValueSize()
{
    return SimpleFuture::GetValueSize();
}

Duple BackupFuture::GetDupleValue()
{
    return SimpleFuture::GetDupleValue();
}


/**
 * @brief MergeSetFuture GetItems()
 *
 * GetItems() returns items (normally must be called after Get())
 *
 * @param[out] items         return items
 */
std::vector<std::pair<Duple, Duple>> &MergeSetFuture::GetItems()
{
    return items;
}

}
