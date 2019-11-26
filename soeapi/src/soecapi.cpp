/**
 * @file    soecapi.cpp
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
 * soecapi.cpp
 *
 *  Created on: Jun 23, 2017
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
#include "soecapi.hpp"

SOE_EXTERNC_BEGIN

//!
//! Soe::Session::eSyncMode
//!
int Translate_C_CPP_SessionSyncMode(eSessionSyncMode c_sts)
{
    return static_cast<int>(c_sts);
}

eSessionSyncMode Translate_CPP_C_SessionSyncMode(int cpp_sts)
{
    return static_cast<eSessionSyncMode>(cpp_sts);
}

//!
//! Soe::Session::Status
//!
int Translate_C_CPP_SessionStatus(SessionStatus c_sts)
{
    return static_cast<int>(c_sts);
}

SessionStatus Translate_CPP_C_SessionStatus(int cpp_sts)
{
    return static_cast<SessionStatus>(cpp_sts);
}

//!
//! Soe::Subsetable::eSubsetType
//!
int Translate_C_CPP_SubsetableSubsetType(eSubsetableSubsetType c_sts)
{
    return static_cast<int>(c_sts);
}

eSubsetableSubsetType Translate_CPP_C_SubsetableSubsetType(int cpp_sts)
{
    return static_cast<eSubsetableSubsetType>(cpp_sts);
}

//!
//! Soe::Session::eSubsetType
//!
int Translate_C_CPP_SessionSubsetType(eSubsetableSubsetType c_sts)
{
    return static_cast<int>(c_sts);
}

eSubsetableSubsetType Translate_CPP_C_SessionSubsetType(int cpp_sts)
{
    return static_cast<eSubsetableSubsetType>(cpp_sts);
}

//!
//! Soe::Iterator::eStatus
//!
int Translate_C_CPP_IteratorStatus(eIteratorStatus c_sts)
{
    return static_cast<int>(c_sts);
}

eIteratorStatus Translate_CPP_C_IteratorStatus(int cpp_sts)
{
    return static_cast<eIteratorStatus>(cpp_sts);
}

//!
//! Soe::Iterator::eIteratorDir
//!
int Translate_C_CPP_IteratorDir(eIteratorDir c_sts)
{
    return static_cast<int>(c_sts);
}

eIteratorDir Translate_CPP_C_IteratorDir(int cpp_sts)
{
    return static_cast<eIteratorDir>(cpp_sts);
}
//!
//! Soe::Iterator::eType
//!
int Translate_C_CPP_IteratorType(eIteratorType c_sts)
{
    return static_cast<int>(c_sts);
}

eIteratorType Translate_CPP_C_IteratorType(int cpp_sts)
{
    return static_cast<eIteratorType>(cpp_sts);
}

SOE_EXTERNC_END
