/**
 * @file    soecapi.hpp
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
 * soecapi.hpp
 *
 *  Created on: Jun 23, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOEAPI_INCLUDE_SOE_SOECAPI_HPP_
#define SOE_SOE_SOE_SOEAPI_INCLUDE_SOE_SOECAPI_HPP_

#include "soeutil.h"

SOE_EXTERNC_BEGIN

#include "soeenumdefines.hpp"

/*
 * @brief C API Types
 *
 * C API Types
 */
typedef enum _eSessionSyncMode {
    SOE_SESSION_SYNC_MODE
} eSessionSyncMode;

typedef enum _SessionStatus {
    SOE_SESSION_STATUS
} SessionStatus;

typedef enum _eSubsetableSubsetType {
    SOE_SUBSETABLE_SUBSET_TYPE
} eSubsetableSubsetType;

typedef enum _eIteratorStatus {
    SOE_ITERATOR_STATUS(CAPI)
} eIteratorStatus;

typedef enum _eIteratorDir {
    SOE_ITERATOR_DIR
} eIteratorDir;

typedef enum _eIteratorType {
    SOE_ITERATOR_TYPE
} eIteratorType;

typedef int               SoeBool;

#define SoeTrue           ((SoeBool )1)
#define SoeFalse          ((SoeBool )0)

typedef uint64_t          SessionHND;
typedef uint64_t          IteratorHND;
typedef uint64_t          TransactionHND;
typedef uint64_t          GroupHND;
typedef uint64_t          SnapshotEngineHND;
typedef uint64_t          BackupEngineHND;
typedef uint64_t          SubsetableHND;
typedef uint64_t          FutureHND;
typedef uint64_t          SetFutureHND;

#define SoeDefaultProvider 0

/*
 * @brief C API GetRange callback typedef
 */
typedef SoeBool (*CapiRangeCallback)(const char *key, size_t key_len, const char *data, size_t data_len, void *context);

/*
 * @brief CDuple typedef
 */
typedef struct _CDuple {
    const char      *data_ptr;
    size_t           data_size;
} CDuple;

/*
 * @brief CDupleVector typedef
 */
typedef struct _CDupleVector {
    size_t           size;
    CDuple          *vector;
} CDupleVector;
typedef CDupleVector                   CDVector;

/*
 * @brief CDuplePair typedef
 *
 *     pair[0] = keys
 *     pair[1] = values
 */
typedef struct _CDuplePair {
    CDuple           pair[2];
} CDuplePair;

/*
 * @brief CDuplePairVector typedef
 */
typedef struct _CDuplePairVector {
    size_t           size;
    CDuplePair      *vector;
} CDuplePairVector;
typedef CDuplePairVector               CDPairVector;


typedef struct _CStringUint32 {
    CDuple           string;
    uint32_t         num;
} CStringUint32;

typedef struct _CStringUint32Vector {
    size_t           size;
    CStringUint32   *vector;
} CStringUint32Vector;
typedef CStringUint32Vector            CDStringUin32Vector;

typedef struct _CStringUint64 {
    CDuple           string;
    uint64_t         num;
} CStringUint64;

typedef struct _CStringUint64Vector {
    size_t           size;
    CStringUint64   *vector;
} CStringUint64Vector;
typedef CStringUint64Vector            CDStringUin64Vector;

#include "soesessioncapi.hpp"
#include "soesessiongroupcapi.hpp"
#include "soesessioniteratorcapi.hpp"
#include "soesessiontransactioncapi.hpp"
#include "soesubsetiteratorcapi.hpp"
#include "soesubsetscapi.hpp"
#include "soefuturescapi.hpp"

//!
// Soe::Session::eSyncMode
//
int Translate_C_CPP_SessionSyncMode(eSessionSyncMode c_sts);
eSessionSyncMode Translate_CPP_C_SessionSyncMode(int cpp_sts);

//!
// Soe::Session::Status
//
int Translate_C_CPP_SessionStatus(SessionStatus c_sts);
SessionStatus Translate_CPP_C_SessionStatus(int cpp_sts);

//!
// Soe::Subsetable::eSubsetType
//
int Translate_C_CPP_SubsetableSubsetType(eSubsetableSubsetType c_sts);
eSubsetableSubsetType Translate_CPP_C_SubsetableSubsetType(int cpp_sts);

//!
// Soe::Session::eSubsetType
//
int Translate_C_CPP_SessionSubsetType(eSubsetableSubsetType c_sts);
eSubsetableSubsetType Translate_CPP_C_SessionSubsetType(int cpp_sts);

//!
// Soe::Iterator::eStatus
//
int Translate_C_CPP_IteratorStatus(eIteratorStatus c_sts);
eIteratorStatus Translate_CPP_C_IteratorStatus(int cpp_sts);

//!
// Soe::Iterator::eIteratorDir
//
int Translate_C_CPP_IteratorDir(eIteratorDir c_sts);
eIteratorDir Translate_CPP_C_IteratorDir(int cpp_sts);

//!
// Soe::Iterator::eType
//
int Translate_C_CPP_IteratorType(eIteratorType c_sts);
eIteratorType Translate_CPP_C_IteratorType(int cpp_sts);

SOE_EXTERNC_END

#endif /* SOE_SOE_SOE_SOEAPI_INCLUDE_SOE_SOECAPI_HPP_ */
