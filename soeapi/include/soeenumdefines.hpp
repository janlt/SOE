/**
 * @file    soeenumdefines.hpp
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
 * soeenumdefines.hpp
 *
 *  Created on: Jun 23, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOEAPI_INCLUDE_SOEENUMDEFINES_HPP_
#define SOE_SOE_SOE_SOEAPI_INCLUDE_SOEENUMDEFINES_HPP_

#include "soeutil.h"

SOE_EXTERNC_BEGIN

#define SOE_SESSION_STATUS \
        eOk                        = 0, \
        eDuplicateKey              = 1, \
        eNotFoundKey               = 2, \
        eTruncatedValue            = 3, \
        eStoreCorruption           = 4, \
        eOpNotSupported            = 5, \
        eInvalidArgument           = 6, \
        eIOError                   = 7, \
        eMergeInProgress           = 8, \
        eIncomplete                = 9, \
        eShutdownInProgress        = 10, \
        eOpTimeout                 = 11, \
        eOpAborted                 = 12, \
        eStoreBusy                 = 13, \
        eItemExpired               = 14, \
        eTryAgain                  = 15, \
        eInternalError             = 16, \
        eOsError                   = 17, \
        eOsResourceExhausted       = 18, \
        eProviderResourceExhausted = 19, \
        eFoobar                    = 20, \
        eInvalidStoreState         = 21, \
        eProviderUnreachable       = 22, \
        eChannelError              = 23, \
        eNetworkError              = 24, \
        eNoFunction                = 25, \
        eDuplicateIdDetected       = 26, \
        eInvalidProvider           = 27, \
        eBackupInProgress          = 28, \
        eRestoreInProgress         = 29


typedef struct SoeessionStatusText_ {
    int         sts;
    const char *sts_text;
} SoeSessionStatusText;

#define SOE_SESSION_STATUS_TEXT(s, t) {(s), (t)}

#define SOE_SESSION_STATUS_TEXT_A \
        SOE_SESSION_STATUS_TEXT(eOk, "Ok"), \
        SOE_SESSION_STATUS_TEXT(eDuplicateKey, "Duplicate key"), \
        SOE_SESSION_STATUS_TEXT(eNotFoundKey, "Key not found"), \
        SOE_SESSION_STATUS_TEXT(eTruncatedValue, "Truncated value"), \
        SOE_SESSION_STATUS_TEXT(eStoreCorruption, "Store corrupted"), \
        SOE_SESSION_STATUS_TEXT(eOpNotSupported, "Operation not supported"), \
        SOE_SESSION_STATUS_TEXT(eInvalidArgument, "Invalid argument"), \
        SOE_SESSION_STATUS_TEXT(eIOError, "IO error"), \
        SOE_SESSION_STATUS_TEXT(eMergeInProgress, "Store merge in progress"), \
        SOE_SESSION_STATUS_TEXT(eIncomplete, "Operation incomplete"), \
        SOE_SESSION_STATUS_TEXT(eShutdownInProgress, "Store shutdown in progress"), \
        SOE_SESSION_STATUS_TEXT(eOpTimeout, "Operation timed out"), \
        SOE_SESSION_STATUS_TEXT(eOpAborted, "Operation aborted"), \
        SOE_SESSION_STATUS_TEXT(eStoreBusy, "Store busy"), \
        SOE_SESSION_STATUS_TEXT(eItemExpired, "Item expired"), \
        SOE_SESSION_STATUS_TEXT(eTryAgain, "Try operation again"), \
        SOE_SESSION_STATUS_TEXT(eInternalError, "Internal error, contact support"), \
        SOE_SESSION_STATUS_TEXT(eOsError, "OS error"), \
        SOE_SESSION_STATUS_TEXT(eOsResourceExhausted, "OS resource exhausted"), \
        SOE_SESSION_STATUS_TEXT(eProviderResourceExhausted, "Provider's resource exhausted"), \
        SOE_SESSION_STATUS_TEXT(eFoobar, "Foobar, contact support"), \
        SOE_SESSION_STATUS_TEXT(eInvalidStoreState, "Invalid store state for the operation attempted"), \
        SOE_SESSION_STATUS_TEXT(eProviderUnreachable, "Provider unreachable"), \
        SOE_SESSION_STATUS_TEXT(eChannelError, "Channel error"), \
        SOE_SESSION_STATUS_TEXT(eNetworkError, "Network error"), \
        SOE_SESSION_STATUS_TEXT(eNoFunction, "No function, no such store"), \
        SOE_SESSION_STATUS_TEXT(eDuplicateIdDetected, "Duplicate store, space or cluster detected"), \
        SOE_SESSION_STATUS_TEXT(eInvalidProvider, "Invalid provider"), \
        SOE_SESSION_STATUS_TEXT(eBackupInProgress, "Backup in progress"), \
        SOE_SESSION_STATUS_TEXT(eRestoreInProgress, "Restore in progress")


#define SOE_SESSION_SUBSET_TYPE \
        eSubsetTypeDisjoint      = 0, \
        eSubsetTypeUnion         = 1, \
        eSubsetTypeComplement    = 2, \
        eSubsetTypeNotComplement = 3, \
        eSubsetTypeCartesian     = 4


#define SOE_SESSION_SYNC_MODE \
        eSyncDefault               = 0, \
        eSyncFsync                 = 1, \
        eSyncAsync                 = 2


#define SOE_ITERATOR_STATUS(pfix) \
        eOk ## pfix             = 0, \
        eNotFound ## pfix       = 1, \
        eInvalidArg ## pfix     = 2, \
        eInvalidIter ## pfix    = 3, \
        eEndKey ## pfix         = 4, \
        eTruncatedValue ## pfix = 5, \
        eAborted ## pfix        = 6


#define SOE_ITERATOR_DIR \
        eIteratorDirForward  = 0, \
        eIteratorDirBackward = 1


#define SOE_ITERATOR_TYPE \
        eIteratorTypeContainer = 0, \
        eIteratorTypeSubset    = 1


#define SOE_SUBSETABLE_SUBSET_TYPE \
        eSubsetTypeDisjoint      = 0, \
        eSubsetTypeUnion         = 1, \
        eSubsetTypeComplement    = 2, \
        eSubsetTypeNotComplement = 3, \
        eSubsetTypeCartesian     = 4

SOE_EXTERNC_END

#endif /* SOE_SOE_SOE_SOEAPI_INCLUDE_SOEENUMDEFINES_HPP_ */
