/**
 * @file    soesessioniteratorcapi.cpp
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
 * soesessioniteratorcapi.cpp
 *
 *  Created on: Jun 25, 2017
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
 * Set the iterator at first position from [start_key - end_key] range, or [key - end_key] range,
 * See Session::CreateIteartor for return status
 * Iterator may get invalidated if a value is written or updated that falls within itearator's range
 * User sall always check the validity of iterator via Valid() function
 *
 * @param[in]  i_hnd          Iterator handle (see soecapi.hpp)
 * @param[in]  key            Key, if "0" go to start_key of the iterator
 * @param[in]  key_size       Key length, if "0" go to start_key of the iterator
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus SessionIteratorFirst(IteratorHND i_hnd, const char *key, size_t key_size)
{
    Soe::SessionIterator *obj = reinterpret_cast<Soe::SessionIterator *>(i_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->First(key,
            key_size)));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Advance to next position
 * See Session::CreateIteartor for return status
 * Iterator may get invalidated if a value is written or updated that falls within itearator's range
 * User sall always check the validity of iterator via Valid() function
 *
 * @param[in]  i_hnd          Iterator handle (see soecapi.hpp)
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus SessionIteratorNext(IteratorHND i_hnd)
{
    Soe::SessionIterator *obj = reinterpret_cast<Soe::SessionIterator *>(i_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->Next()));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Set the iterator at last position
 * Iterator may get invalidated if a value is written or updated that falls within itearator's range
 * User sall always check the validity of iterator via Valid() function
 *
 * @param[in]  i_hnd          Iterator handle (see soecapi.hpp)
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus SessionIteratorLast(IteratorHND i_hnd)
{
    Soe::SessionIterator *obj = reinterpret_cast<Soe::SessionIterator *>(i_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->Last()));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Get key duple at current position
 * Iterator may get invalidated if a value is written or updated that falls within itearator's range
 * User sall always check the validity of iterator via Valid() function
 *
 * @param[in]  i_hnd          Iterator handle (see soecapi.hpp)
 * @param[out] CDuple         Return key (see soecapi.hpp)
 */
CDuple SessionIteratorKey(IteratorHND i_hnd)
{
    Soe::SessionIterator *obj = reinterpret_cast<Soe::SessionIterator *>(i_hnd);
    if ( !obj ) {
        CDuple cdp = {0, 0};
        return cdp;
    }
    try {
        Soe::Duple key = obj->Key();
        CDuple cdp;
        cdp.data_ptr = key.data();
        cdp.data_size = key.size();
        return cdp;
    } catch ( ... ) {
        CDuple cdp = {0, 0};
        return cdp;
    }
}

/**
 * @brief C API
 *
 * Get value duple at current position
 * Iterator may get invalidated if a value is written or updated that falls within itearator's range
 * User sall always check the validity of iterator via Valid() function
 *
 * @param[in]  i_hnd          Iterator handle (see soecapi.hpp)
 * @param[out] CDuple         Return value (see soecapi.hpp)
 */
CDuple SessionIteratorValue(IteratorHND i_hnd)
{
    Soe::SessionIterator *obj = reinterpret_cast<Soe::SessionIterator *>(i_hnd);
    if ( !obj ) {
        CDuple cdp = {0, 0};
        return cdp;
    }
    try {
        Soe::Duple value = obj->Value();
        CDuple cdp;
        cdp.data_ptr = value.data();
        cdp.data_size = value.size();
        return cdp;
    } catch ( ... ) {
        CDuple cdp = {0, 0};
        return cdp;
    }
}

/**
 * @brief C API
 *
 * Get iterator's validity
 * Iterator may get invalidated if a value is written or updated that falls within itearator's range
 * User sall always check the validity of iterator via Valid() function
 *
 * @param[in]  i_hnd          Iterator handle (see soecapi.hpp)
 * @param[out] SoeBool        Return iterator validity
 */
SoeBool SessionIteratorValid(IteratorHND i_hnd)
{
    Soe::SessionIterator *obj = reinterpret_cast<Soe::SessionIterator *>(i_hnd);
    if ( !obj ) {
        return SoeFalse;
    }
    try {
        return static_cast<SoeBool>(obj->Valid());
    } catch ( ... ) {
        return SoeFalse;
    }
}

SOE_EXTERNC_END
