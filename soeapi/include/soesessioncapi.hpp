/**
 * @file    soesessioncapi.hpp
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
 * soesessioncapi.hpp
 *
 *  Created on: Jun 23, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOEAPI_INCLUDE_SOESESSIONCAPI_HPP_
#define SOE_SOE_SOE_SOEAPI_INCLUDE_SOESESSIONCAPI_HPP_



SOE_EXTERNC_BEGIN

/*
 * @brief Session C API
 *
 * Session C API
 */
SessionHND SessionNew(const char *c_n,
        const char *s_n,
        const char *cn_n,
        const char *pr,
        SoeBool _transation_db,
        SoeBool _debug,
        eSessionSyncMode s_m);

SessionStatus SessionOpen(SessionHND s_hnd,
    const char *c_n,
    const char *s_n,
    const char *cn_n,
    const char *pr,
    SoeBool _transation_db,
    SoeBool _debug,
    eSessionSyncMode s_m);

SessionStatus SessionLastStatus(SessionHND s_hnd);
SoeBool SessionOk(SessionHND s_hnd, SessionStatus *sts);
SessionStatus SessionCreate(SessionHND s_hnd);
SessionStatus SessionDestroyStore(SessionHND s_hnd);

SessionStatus SessionListClusters(SessionHND s_hnd, CDStringUin64Vector *clusters);
SessionStatus SessionListSpaces(SessionHND s_hnd, const char *c_n, uint64_t c_id, CDStringUin64Vector *spaces);
SessionStatus SessionListStores(SessionHND s_hnd, const char *c_n, uint64_t c_id, const char *s_n, uint64_t s_id, CDStringUin64Vector *stores);
SessionStatus SessionListSubsets(SessionHND s_hnd, const char *c_n, uint64_t c_id, const char *s_n, uint64_t s_id, const char *t_n, uint64_t t_id, CDStringUin64Vector *subsets);

SessionStatus SessionPut(SessionHND s_hnd, const char *key, size_t key_size, const char *data, size_t data_size);
SessionStatus SessionGet(SessionHND s_hnd, const char *key, size_t key_size, char *buf, size_t *buf_size);
SessionStatus SessionDelete(SessionHND s_hnd, const char *key, size_t key_size);
SessionStatus SessionMerge(SessionHND s_hnd, const char *key, size_t key_size, const char *data, size_t data_size);

SessionStatus SessionGetRange(SessionHND s_hnd,
        const char *start_key,
        size_t start_key_size,
        const char *end_key,
        size_t end_key_size,
        char *buf,
        size_t *buf_size,
        CapiRangeCallback r_cb,
        void *ctx,
        eIteratorDir dir);

SessionStatus SessionGetSet(SessionHND s_hnd, CDPairVector *items, SoeBool fail_on_first_nfound, size_t *fail_pos);
SessionStatus SessionPutSet(SessionHND s_hnd, CDPairVector *items, SoeBool fail_on_first_dup, size_t *fail_pos);
SessionStatus SessionDeleteSet(SessionHND s_hnd, CDVector *keys, SoeBool fail_on_first_nfound, size_t *fail_pos);
SessionStatus SessionMergeSet(SessionHND s_hnd, CDPairVector *items);

TransactionHND SessionStartTransaction(SessionHND s_hnd);
SessionStatus SessionCommitTransaction(SessionHND s_hnd, const TransactionHND tr);
SessionStatus SessionRollbackTransaction(SessionHND s_hnd, const TransactionHND tr);

SnapshotEngineHND SessionCreateSnapshotEngine(SessionHND s_hnd, const char *name);
SessionStatus SessionDestroySnapshotEngine(SessionHND s_hnd, const SnapshotEngineHND en);
SessionStatus SessionListSnapshots(SessionHND s_hnd, CDStringUin32Vector *list);

BackupEngineHND SessionCreateBackupEngine(SessionHND s_hnd, const char *name);
SessionStatus SessionDestroyBackupEngine(SessionHND s_hnd, const BackupEngineHND en);
SessionStatus SessionListBackups(SessionHND s_hnd, CDStringUin32Vector *list);

GroupHND SessionCreateGroup(SessionHND s_hnd);
SessionStatus SessionDestroyGroup(SessionHND s_hnd, GroupHND gr);
SessionStatus SessionWrite(SessionHND s_hnd, const GroupHND gr);

IteratorHND SessionCreateIterator(SessionHND s_hnd,
    eIteratorDir dir,
    const char *start_key,
    size_t start_key_size,
    const char *end_key,
    size_t end_key_size);

SessionStatus SessionDestroyIterator(SessionHND s_hnd, IteratorHND it);

SubsetableHND SessionCreateSubset(SessionHND s_hnd,
    const char *name,
    size_t name_len,
    eSubsetableSubsetType sb_type);

SubsetableHND SessionOpenSubset(SessionHND s_hnd,
    const char *name,
    size_t name_len,
    eSubsetableSubsetType sb_type);

SessionStatus SessionDestroySubset(SessionHND s_hnd, SubsetableHND sb);
SessionStatus SessionDeleteSubset(SessionHND s_hnd, SubsetableHND sb);

void SessionClose(SessionHND s_hnd);
SessionStatus SessionRepair(SessionHND s_hnd);

SOE_EXTERNC_END

#endif /* SOE_SOE_SOE_SOEAPI_INCLUDE_SOESESSIONCAPI_HPP_ */
