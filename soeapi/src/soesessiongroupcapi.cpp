/**
 * @file    soesessiongroupcapi.cpp
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
 * soesessiongroupcapi.cpp
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
 * Put KV pair (item) in Group associated with session
 * If a key doesn't exist it'll be created, if it does it'll be updated
 *
 * @param[in]  s_hnd          Group handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[in]  data           Data
 * @param[in]  data_size      Data length
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus GroupPut(GroupHND g_hnd, const char *key, size_t key_size, const char *data, size_t data_size)
{
    Soe::Group *obj = reinterpret_cast<Soe::Group *>(g_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->Put(key,
            key_size,
            data,
            data_size)));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Merge KV pair (item) in group associated with session
 * If a key doesn't exist it'll be created, if it does it'll be merged
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[in]  data           Data
 * @param[in]  data_size      Data length
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus GroupMerge(GroupHND g_hnd, const char *key, size_t key_size, const char *data, size_t data_size)
{
    Soe::Group *obj = reinterpret_cast<Soe::Group *>(g_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->Merge(key,
            key_size,
            data,
            data_size)));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

/**
 * @brief C API
 *
 * Delete KV pair (item) in group associated with session
 * If a key doesn't exist it'll be created, if it does it'll be merged
 *
 * @param[in]  s_hnd          Session handle (see soecapi.hpp)
 * @param[in]  key            Key
 * @param[in]  key_size       Key length
 * @param[out] status         Return status (see soecapi.hpp)
 */
SessionStatus GroupDelete(GroupHND g_hnd, const char *key, size_t key_size)
{
    Soe::Group *obj = reinterpret_cast<Soe::Group *>(g_hnd);
    if ( !obj ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eInvalidArgument));
    }
    try {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(obj->Delete(key,
            key_size)));
    } catch ( ... ) {
        return static_cast<SessionStatus>(Translate_CPP_C_SessionStatus(Soe::Session::Status::eFoobar));
    }
}

SOE_EXTERNC_END
