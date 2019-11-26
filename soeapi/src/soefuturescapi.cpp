/**
 * @file    soefuturescapi.cpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe Futures API
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
 * soefuturescapi.cpp
 *
 *  Created on: Dec 22, 2017
 *      Author: Jan Lisowiec
 */

#include <stdint.h>
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

SOE_EXTERNC_BEGIN

/**
 * @brief C API
 *
 * Put KV pair asynchronously in store associated with session
 * If a key doesn't exist it'll be created, if it does eDuplicateKey will be returned
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[in]  data           Data
 * @param[in]  data_size      Data length
 * @param[out] future         Return future handle (see soecapi.hpp)
 */
FutureHND SessionPutAsync(SessionHND s_hnd, const char *key, size_t key_size, const char *data, size_t data_size)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return 0;
    }
    FutureHND ret_fut = 0;
    try {
        ret_fut = reinterpret_cast<FutureHND>(obj->PutAsync(key,
            key_size,
            data,
            data_size));
    } catch ( ... ) {
        ;
    }
    return ret_fut;
}

/**
 * @brief C API
 *
 * Get KV pair asynchronously from store associated with session
 * If a key doesn't exist eNotfoundKey will be returned
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[in]  buf            Buffer for data, in case of eTruncatedValue return status will
 *                            be filled in with portion of the value since some applications can work with that
 * @param[in]  buf_size       Buffer size pointer
 *                            It'll be set the the length of the value when the return status is either eOk or eTruncatedValue
 *                            The caller has to check the return status and decide if it' ok to proceed with truncated value
 * @param[out] future         Return future handle (see soecapi.hpp)
 */
FutureHND SessionGetAsync(SessionHND s_hnd, const char *key, size_t key_size, char *buf, size_t *buf_size)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return 0;
    }
    FutureHND ret_fut = 0;
    try {
        ret_fut = reinterpret_cast<FutureHND>(obj->GetAsync(key,
            key_size,
            buf,
            buf_size));
    } catch ( ... ) {
        ;
    }
    return ret_fut;
}

/**
 * @brief C API
 *
 * Delete KV pair asynchronously from a store associated with session
 * If a key doesn't exist eNotfoundKey will be returned
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[out] future         Return future handle (see soecapi.hpp)
 */
FutureHND SessionDeleteAsync(SessionHND s_hnd, const char *key, size_t key_size)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return 0;
    }
    FutureHND ret_fut = 0;
    try {
        ret_fut = reinterpret_cast<FutureHND>(obj->DeleteAsync(key,
            key_size));
    } catch ( ... ) {
        ;
    }
    return ret_fut;
}

/**
 * @brief C API
 *
 * Merge KV pair asynchronously in store associated with session
 * If a key doesn't exist it'll be created, if it does it'll be merged (updated)
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[in]  data           Data
 * @param[in]  data_size      Data length
 * @param[out] future         Return future handle (see soecapi.hpp)
 */
FutureHND SessionMergeAsync(SessionHND s_hnd, const char *key, size_t key_size, const char *data, size_t data_size)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return 0;
    }
    FutureHND ret_fut = 0;
    try {
        ret_fut = reinterpret_cast<FutureHND>(obj->MergeAsync(key,
            key_size,
            data,
            data_size));
    } catch ( ... ) {
        ;
    }
    return ret_fut;
}

/**
 * @brief C API
 *
 * Get a set of KV pairs asynchronously whose keys are passed in as a vector
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_nfound  Specifies whether or not to fail the function on the first
 *                                   not found key
 * @param[in,out]  fail_pos          Position within items with not found key that caused a failure
 * @param[out] future         Return future handle (see soecapi.hpp)
 *                                   If items.size() == 0 the future's status is eInvalidArgument
 *                                   If fail_on_first_nfound == false and items.size() != 0 the return status is eOk
 *                                   if fail_on_first_nfound == true and items.size() != 0 and a key in keys is not found
 *                                   the future's status is eNotFoundKey and fail_pos is set to first failed index
 */
SetFutureHND SessionGetSetAsync(SessionHND s_hnd, CDPairVector *items, SoeBool fail_on_first_nfound, size_t *fail_pos)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !items ) {
        return 0;
    }

    std::vector<std::pair<Soe::Duple, Soe::Duple>> *cpp_items = new std::vector<std::pair<Soe::Duple, Soe::Duple>>();
    int ret = Soe::CDPVectorToCPPDPVector(items, cpp_items);
    if ( ret ) {
        return 0;
    }

    SetFutureHND ret_fut = 0;
    try {
        ret_fut = reinterpret_cast<SetFutureHND>(obj->GetSetAsync(*cpp_items,
            static_cast<bool>(fail_on_first_nfound),
            fail_pos));

        return ret_fut;
    } catch ( ... ) {
        ;
    }
    return ret_fut;
}

/**
 * @brief C API
 *
 * Set a set of KV pairs asynchronously whose keys are passed in as a vector
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_dup     Specifies whether or not to fail the function on the first
 *                                   duplicate key
 * @param[in,out]  fail_pos          Position within items with not found key that caused a failure
 * @param[out] future         Return future handle (see soecapi.hpp)
 *                                   If items.size() == 0 the future's status is eInvalidArgument
 *                                   If fail_on_first_nfound == false and items.size() != 0 the future's status is eOk
 *                                   if fail_on_first_nfound == true and items.size() != 0 and a key in keys is not found
 *                                   the future's status is eDuplicateKey and fail_pos is set to first failed index
 */
SetFutureHND SessionPutSetAsync(SessionHND s_hnd, CDPairVector *items, SoeBool fail_on_first_dup, size_t *fail_pos)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !items ) {
        return 0;
    }

    std::vector<std::pair<Soe::Duple, Soe::Duple>> *cpp_items = new std::vector<std::pair<Soe::Duple, Soe::Duple>>();
    int ret = Soe::CDPVectorToCPPDPVector(items, cpp_items);
    if ( ret ) {
        return 0;
    }

    SetFutureHND ret_fut = 0;
    try {
        ret_fut = reinterpret_cast<SetFutureHND>(obj->PutSetAsync(*cpp_items,
            static_cast<bool>(fail_on_first_dup),
            fail_pos));

        return ret_fut;
    } catch ( ... ) {
        ;
    }
    return ret_fut;
}

/**
 * @brief C API
 *
 * Delete a set of KV pairs asynchronously whose keys are passed in as a vector
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[in]  fail_on_first_nfound  Specifies whether or not to fail the function on the first
 *                                   not found key
 * @param[in,out]  fail_pos          Position within items with not found key that caused a failure
 * @param[out] future         Return future handle (see soecapi.hpp)
 *                                   If keys.size() == 0 the future's status is eInvalidArgument
 *                                   If fail_on_first_nfound == false and keys.size() != 0 the future's status is eOk
 *                                   if fail_on_first_nfound == true and keys.size() != 0 and a key in keys is not found
 *                                   the future's status is eNotFoundKey and fail_pos is set to first failed index
 */
SetFutureHND SessionDeleteSetAsync(SessionHND s_hnd, CDVector *keys, SoeBool fail_on_first_nfound, size_t *fail_pos)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !keys ) {
        return 0;
    }

    std::vector<Soe::Duple> *cpp_keys = new std::vector<Soe::Duple>();
    int ret = Soe::CDVectorToCPPDVector(keys, cpp_keys);
    if ( ret ) {
        return 0;
    }

    SetFutureHND ret_fut = 0;
    try {
        ret_fut = reinterpret_cast<SetFutureHND>(obj->DeleteSetAsync(*cpp_keys,
            static_cast<bool>(fail_on_first_nfound),
            fail_pos));

        return ret_fut;
    } catch ( ... ) {
        ;
    }
    return ret_fut;
}

/**
 * @brief C API
 *
 * Merge a set of KV pairs asynchronously whose keys are passed in as a vector
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  items                 Vector of key/buffer Duple pairs
 * @param[out] future                Return future handle (see soecapi.hpp)
 *                                   If keys.size() == 0 the future's status is eInvalidArgument
 */
SetFutureHND SessionMergeSetAsync(SessionHND s_hnd, CDPairVector *items)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj || !items ) {
        return 0;
    }

    std::vector<std::pair<Soe::Duple, Soe::Duple>> *cpp_items = new std::vector<std::pair<Soe::Duple, Soe::Duple>>();
    int ret = Soe::CDPVectorToCPPDPVector(items, cpp_items);
    if ( ret ) {
        return 0;
    }

    SetFutureHND ret_fut = 0;
    try {
        ret_fut = reinterpret_cast<SetFutureHND>(obj->MergeSetAsync(*cpp_items));

        return ret_fut;
    } catch ( ... ) {
        ;
    }
    return ret_fut;
}

/**
 * @brief C API
 *
 * Destroy future
 *
 * @param[in]  s_hnd                 Session handle (see soecapi.hpp)
 * @param[in]  f_hnd                 Future handle (see soecapi.hpp)
 *
 */
void SessionDestroyFuture(SessionHND s_hnd, FutureHND f_hnd)
{
    Soe::Session *obj = reinterpret_cast<Soe::Session *>(s_hnd);
    if ( !obj ) {
        return;
    }

    try {
        Soe::Futurable *fut = reinterpret_cast<Soe::Futurable *>(f_hnd);
        Soe::SetFuture *set_fut = dynamic_cast<Soe::SetFuture *>(fut);
        if ( fut && set_fut ) {
            if ( dynamic_cast<Soe::PutSetFuture *>(set_fut) ) {
                delete &dynamic_cast<Soe::PutSetFuture *>(set_fut)->GetItems();
            } else if ( dynamic_cast<Soe::GetSetFuture *>(set_fut) ) {
                delete &dynamic_cast<Soe::GetSetFuture *>(set_fut)->GetItems();
            } else if ( dynamic_cast<Soe::MergeSetFuture *>(set_fut) ) {
                delete &dynamic_cast<Soe::MergeSetFuture *>(set_fut)->GetItems();
            } else if ( dynamic_cast<Soe::DeleteSetFuture *>(set_fut) ) {
                delete &dynamic_cast<Soe::DeleteSetFuture *>(set_fut)->GetKeys();
            } else {
                ;
            }
        }

        obj->DestroyFuture(reinterpret_cast<Soe::Futurable *>(f_hnd));
    } catch ( ... ) {
        ;
    }
}

/**
 * @brief C API
 *
 * Get future's status, available immediately after future creation
 *
 * @param[in]  f_hnd                 Future handle (see soecapi.hpp)
 * @param[out] status                Return future's status (see soecapi.hpp)
 *
 */
SessionStatus FutureGetPostRet(FutureHND f_hnd)
{
    Soe::Futurable *fut = reinterpret_cast<Soe::Futurable *>(f_hnd);
    if ( !fut ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(fut->GetPostRet()));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get future, available when future is signaled, potentially blocking call
 *
 * @param[in]  f_hnd                 Future handle (see soecapi.hpp)
 * @param[out] status                Return future's status (see soecapi.hpp)
 *
 */
SessionStatus FutureGet(FutureHND f_hnd)
{
    Soe::Futurable *fut = reinterpret_cast<Soe::Futurable *>(f_hnd);
    if ( !fut ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(fut->Get()));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get future's fail_detect flag, passed as an argument to vectored future call
 *
 * @param[in]  f_hnd                 Future handle (see soecapi.hpp)
 * @param[out] f_det                 Return fail detect's value
 * @param[out] status                Return future's status (see soecapi.hpp)
 *
 */
SessionStatus SetFutureGetFailDetect(SetFutureHND f_hnd, SoeBool *f_det)
{
    Soe::SetFuture *s_fut = reinterpret_cast<Soe::SetFuture *>(f_hnd);
    if ( !s_fut || !f_det ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        *f_det = s_fut->GetFailDetect();
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eOk));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get future's fail_pos, passed as an argument to vectored future call
 * Fail_pos is an index into items at which a failure occurred
 *
 * @param[in]  f_hnd                 Future handle (see soecapi.hpp)
 * @param[out] f_pos                 Return fail position value
 * @param[out] status                Return future's status (see soecapi.hpp)
 *
 */
SessionStatus SetFutureGetFailPos(SetFutureHND f_hnd, size_t *f_pos)
{
    Soe::SetFuture *s_fut = reinterpret_cast<Soe::SetFuture *>(f_hnd);
    if ( !s_fut || !f_pos ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        *f_pos = s_fut->GetFailPos();
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eOk));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get future's key, when applicable
 *
 * @param[in]  f_hnd                 Future handle (see soecapi.hpp)
 * @param[out] key                   Return key
 * @param[out] status                Return future's status (see soecapi.hpp)
 *
 */
SessionStatus FutureGetKey(FutureHND f_hnd, char **key)
{
    Soe::SimpleFuture *fut = reinterpret_cast<Soe::SimpleFuture *>(f_hnd);
    if ( !fut || !key ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        *key = const_cast<char *>(fut->GetKey());
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eOk));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get future's key size, when applicable
 *
 * @param[in]  f_hnd                 Future handle (see soecapi.hpp)
 * @param[out] key_size              Return key size
 * @param[out] status                Return future's status (see soecapi.hpp)
 *
 */
SessionStatus FutureGetKeySize(FutureHND f_hnd, size_t *key_size)
{
    Soe::SimpleFuture *fut = reinterpret_cast<Soe::SimpleFuture *>(f_hnd);
    if ( !fut || !key_size ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        *key_size = fut->GetKeySize();
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eOk));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get future's value, when applicable
 *
 * @param[in]  f_hnd                 Future handle (see soecapi.hpp)
 * @param[out] value                 Return value
 * @param[out] status                Return future's status (see soecapi.hpp)
 *
 */
SessionStatus FutureGetValue(FutureHND f_hnd, char **value)
{
    Soe::SimpleFuture *fut = reinterpret_cast<Soe::SimpleFuture *>(f_hnd);
    if ( !fut || !value ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        *value = const_cast<char *>(fut->GetValue());
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eOk));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get future's value size, when applicable
 *
 * @param[in]  f_hnd                 Future handle (see soecapi.hpp)
 * @param[out] value_size            Return value size
 * @param[out] status                Return future's status (see soecapi.hpp)
 *
 */
SessionStatus FutureGetValueSize(FutureHND f_hnd, size_t *value_size)
{
    Soe::SimpleFuture *fut = reinterpret_cast<Soe::SimpleFuture *>(f_hnd);
    if ( !fut || !value_size ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        *value_size = fut->GetValueSize();
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eOk));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get future's CDuple key, when applicable
 *
 * @param[in]  f_hnd                 Future handle (see soecapi.hpp)
 * @param[out] key_cduple            Return CDuple key
 * @param[out] status                Return future's status (see soecapi.hpp)
 *
 */
SessionStatus FutureGetCDupleKey(FutureHND f_hnd, CDuple *key_cduple)
{
    Soe::SimpleFuture *fut = reinterpret_cast<Soe::SimpleFuture *>(f_hnd);
    if ( !fut || !key_cduple ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        Soe::Duple key_duple = fut->GetDupleKey();
        key_cduple->data_ptr = key_duple.data();
        key_cduple->data_size = key_duple.size();

        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eOk));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get future's CDuple value, when applicable
 *
 * @param[in]  f_hnd                 Future handle (see soecapi.hpp)
 * @param[out] value_cduple          Return CDuple value
 * @param[out] status                Return future's status (see soecapi.hpp)
 *
 */
SessionStatus FutureGetCDupleValue(FutureHND f_hnd, CDuple *value_cduple)
{
    Soe::SimpleFuture *fut = reinterpret_cast<Soe::SimpleFuture *>(f_hnd);
    if ( !fut || !value_cduple ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        Soe::Duple value_duple = fut->GetDupleKey();
        value_cduple->data_ptr = value_duple.data();
        value_cduple->data_size = value_duple.size();

        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eOk));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get future's items vector, when applicable
 *
 * @param[in]  f_hnd                 Future handle (see soecapi.hpp)
 * @param[out] items                 Return items
 * @param[out] status                Return future's status (see soecapi.hpp)
 *
 */
SessionStatus FutureGetItems(SetFutureHND s_hnd, CDPairVector *items)
{
    Soe::GetSetFuture *fut = reinterpret_cast<Soe::GetSetFuture *>(s_hnd);
    if ( !fut || !items ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        std::vector<std::pair<Soe::Duple, Soe::Duple>> *cpp_items = 0;
        if ( dynamic_cast<Soe::GetSetFuture *>(fut) ) {
            *cpp_items = dynamic_cast<Soe::GetSetFuture *>(fut)->GetItems();
        } else if ( dynamic_cast<Soe::PutSetFuture *>(fut) ) {
            *cpp_items = dynamic_cast<Soe::PutSetFuture *>(fut)->GetItems();
        } else if ( dynamic_cast<Soe::MergeSetFuture *>(fut) ) {
            *cpp_items = dynamic_cast<Soe::MergeSetFuture *>(fut)->GetItems();
        } else {
            return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
        }

        int ret = Soe::CPPDPVectorToCDPVector(cpp_items, items, false);
        if ( ret ) {
            return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
        }

        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eOk));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get future's keys vector, when applicable
 *
 * @param[in]  f_hnd                 Future handle (see soecapi.hpp)
 * @param[out] keys                  Return keys
 * @param[out] status                Return future's status (see soecapi.hpp)
 *
 */
SessionStatus FutureGetKeys(SetFutureHND s_hnd, CDVector *keys)
{
    Soe::SetFuture *fut = reinterpret_cast<Soe::SetFuture *>(s_hnd);
    if ( !fut || !keys ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }

    try {
        if ( dynamic_cast<Soe::DeleteSetFuture *>(fut) ) {
            std::vector<Soe::Duple> &cpp_keys = dynamic_cast<Soe::DeleteSetFuture *>(fut)->GetKeys();
            int ret = Soe::CDVectorToCPPDVector(keys, &cpp_keys);
            if ( ret ) {
                return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
            }
        } else {
            return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
        }

        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eOk));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

SOE_EXTERNC_END
