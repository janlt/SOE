/**
 * @file    main.c
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
 * main.c
 *
 *  Created on: Jun 23, 2017
 *      Author: Jan Lisowiec
 */

#define _GNU_SOURCE

#include <pwd.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>

#include <string.h>
#include <dlfcn.h>

#include "soeutil.h"
#include "soecapi.hpp"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

typedef enum _TestType {
    eCreateStore          = 0,
    eCreateSubset         = 1,
    eWriteStore           = 2,
    eWriteSubset          = 3,
    eMergeStore           = 4,
    eMergeSubset          = 5,
    eReadStore            = 6,
    eRepairStore          = 7,
    eReadSubset           = 8,
    eReadKVItemStore      = 9,
    eReadKVItemSubset     = 10,
    eDeleteKVItemStore    = 11,
    eDeleteKVItemSubset   = 12,
    eDestroyStore         = 13,
    eDestroySubset        = 14,
    eTraverseNameSpace    = 15,
    eRegression1          = 16,
    eRegression2          = 17,
    eRegression3          = 18,
    eRegression4          = 19,
    eRegression5          = 20,
    eRegression6          = 21,
    eRegression7          = 22,
    eRegression8          = 23,
    eRegression9          = 24,
    eRegression10         = 25,
    eRegression11         = 26,
    eRegression12         = 27,
    eRegression13         = 28,
    eRegression14         = 29,
    eRegression15         = 30,
    eRegression16         = 31,
    eRegression17         = 32,
    eWriteStoreAsync      = 33,
    eReadStoreAsync       = 34,
    eMergeStoreAsync      = 35,
    eDeleteStoreAsync     = 36
} TestType;

#define MAX_NUM_OPS        200000
#define MAX_THREAD_COUNT   512
#define STORE_12_COUNT     16

typedef struct _RegressionSubsetTest12 {
    SubsetableHND                   sub_hndl;
    char                            sub_name[256];
} RegressionSubsetTest12;

typedef struct _RegressionSubsetParamsTest12 {
    SessionHND                      sess_hndl;
    char                            store_name[256];
    SessionHND                      sub_sess_hndl;
    char                            sub_store_name[256];
    RegressionSubsetTest12          subsets[STORE_12_COUNT];
    uint32_t                        subset_count;
} RegressionSubsetParamsTest12;

typedef struct _RegressionSessionParamsTest12 {
    RegressionSubsetParamsTest12    sessions[STORE_12_COUNT];
    uint32_t                        session_count;
} RegressionSessionParamsTest12;

typedef struct _RegressionParamsTest12 {
    RegressionSessionParamsTest12   threads[MAX_THREAD_COUNT];
    uint32_t                        thread_count;
} RegressionParamsTest12;

typedef union _RegressionParams {
    RegressionParamsTest12 params12;
} RegressionParams;

typedef struct _RegressionArgs {
    SessionHND        sess_hndl;
    SubsetableHND     sub_hndl;
    eSessionSyncMode  sess_sync_mode;
    const char       *cluster_name;
    const char       *space_name;
    const char       *store_name ;
    int               provider;
    const char       *provider_name;
    const char       *key;
    size_t            key_size;
    const char       *end_key;
    size_t            end_key_size;
    int               it_dir;
    int               default_first;
    char             *value;
    size_t            value_buffer_size;
    size_t            value_size;
    uint32_t          num_ops;
    uint32_t          num_regressions;
    const char       *snap_back_subset_name;
    uint32_t          snap_back_id;
    int               transactional;
    int               sync_type;
    int               debug;
    int               wait_for_key;
    int               hex_dump;
    useconds_t        sleep_usecs;
    uint32_t          sleep_multiplier;
    int               no_print;
    uint64_t          io_counter;
    int               group_write;
    char            **key_ptrs;
    size_t           *key_sizes;
    char            **value_ptrs;
    size_t           *value_sizes;
    int               sync_async_destroy;
    TestType          test_type;
    int               th_number;
    RegressionParams *params;
} RegressionArgs;

static void debugPrintHexBuf(FILE *fp,
    const char buf[],
    unsigned int len)
{
    unsigned int i;
    fprintf(fp, "------- buf(%p) len(%u)", buf, len);
    for ( i = 0 ; i < len ; i++ ) {
        if ( !(i%32) ) {
            fprintf(fp, "\n%p:    ", &buf[i]);
        }
        if ( i%32 && !(i%8) ) {
            fprintf(fp, ":");
        }
        fprintf(fp, "%02x", (unsigned char )buf[i]);
    }
    fprintf(fp, "\n-------\n");
    fflush(stderr);
}

void debugPrintAsciiHexBuf(FILE *fp,
    const char buf[],
    unsigned int len)
{
    unsigned int i;
    fprintf(fp, "------- buf(%p) len(%u)", buf, len);
    for ( i = 0 ; i < len ; i++ ) {
        if ( !(i%32) ) {
            fprintf(fp, "\n%p:    ", &buf[i]);
        }
        if ( i%32 && !(i%8) ) {
            fprintf(fp, ":");
        }
        if ( isascii(buf[i]) && buf[i] ) {
            fprintf(fp, "%c ", buf[i]);
        } else {
            fprintf(fp, "  ");
        }
    }
    for ( i = 0 ; i < len ; i++ ) {
        if ( !(i%32) ) {
            fprintf(fp, "\n%p:    ", &buf[i]);
        }
        if ( i%32 && !(i%8) ) {
            fprintf(fp, ":");
        }
        fprintf(fp, "%02x", (unsigned char )buf[i]);
    }
    fprintf(fp, "\n-------\n");
    fflush(stderr);
}

/*
 * @brief Create store
 */
static int CreateStore(SessionHND *sess_hndl,
    const char *cluster_name,
    const char *space_name,
    const char *store_name,
    SoeBool transactional,
    eSessionSyncMode sync_mode,
    const char *provider,
    SoeBool debug)
{
    /*
     * @brief Obtain Session handle
     */
    *sess_hndl = SessionNew(cluster_name,
        space_name,
        store_name,
        provider,
        transactional,
        debug,
        sync_mode);
    if ( !*sess_hndl ) {
        fprintf(stderr, "SessionNew(%s) failed\n", store_name);
        return -1;
    }

    /*
     * @brief Create session
     */
    SessionStatus sts = SessionCreate(*sess_hndl);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionCreate(%s) failed(%u)\n", store_name, sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Open store
 */
static int OpenStore(SessionHND *sess_hndl,
    const char *cluster_name,
    const char *space_name,
    const char *store_name,
    SoeBool transactional,
    eSessionSyncMode sync_mode,
    const char *provider,
    SoeBool debug)
{
    /*
     * @brief Obtain Session handle
     */
    *sess_hndl = SessionNew(cluster_name,
        space_name,
        store_name,
        provider,
        transactional,
        debug,
        sync_mode);
    if ( !*sess_hndl ) {
        fprintf(stderr, "SessionNew(%s) failed\n", store_name);
        return -1;
    }

    /*
     * @brief Create session
     */
    SessionStatus sts = SessionOpen(*sess_hndl,
        cluster_name,
        space_name,
        store_name,
        provider,
        transactional,
        debug,
        sync_mode);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionOpen(%s) failed(%u)\n", store_name, sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Repair store
 */
static int RepairStore(const SessionHND sess_hndl)
{
    /*
     * @brief Write record in store
     */
    SessionStatus sts = SessionRepair(sess_hndl);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionRepair() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Write store
 */
static int WriteStore(const SessionHND sess_hndl,
    const char *key,
    size_t key_size,
    const char *value,
    size_t value_size)
{
    /*
     * @brief Write record in store
     */
    SessionStatus sts = SessionPut(sess_hndl, key, key_size, value, value_size);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionPut() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Write store asynchronously
 */
static int WriteStoreAsync(const SessionHND sess_hndl,
    const char *key,
    size_t key_size,
    const char *value,
    size_t value_size)
{
    int ret = 0;

    /*
     * @brief Write record in store by getting a future
     */
    FutureHND fut = SessionPutAsync(sess_hndl, key, key_size, value, value_size);
    if ( !fut ) {
        fprintf(stderr, "SessionPutAsync() returned NULL future\n");
        return -1;
    }

    /*
     * Get the return code from the future
     */
    SessionStatus sts = FutureGet(fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGet() failed(%u)\n", sts);
        SessionDestroyFuture(sess_hndl, fut);
        return -1;
    }

    /*
     * Get the status of the operation
     */
    sts = FutureGetPostRet(fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGetPostRet() status(%u)\n", sts);
        ret = -1;
    }

    SessionDestroyFuture(sess_hndl, fut);

    return ret;
}

/*
 * @brief Merge store
 */
static int MergeStore(const SessionHND sess_hndl,
    const char *key,
    size_t key_size,
    const char *value,
    size_t value_size)
{
    /*
     * @brief Write record in store
     */
    SessionStatus sts = SessionMerge(sess_hndl, key, key_size, value, value_size);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionMerge() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Merge store asynchronously
 */
static int MergeStoreAsync(const SessionHND sess_hndl,
    const char *key,
    size_t key_size,
    const char *value,
    size_t value_size)
{
    int ret = 0;

    /*
     * @brief Write record in store by getting a future
     */
    FutureHND fut = SessionMergeAsync(sess_hndl, key, key_size, value, value_size);
    if ( !fut ) {
        fprintf(stderr, "SessionMergeAsync() returned NULL future\n");
        return -1;
    }

    /*
     * Get the return code from the future
     */
    SessionStatus sts = FutureGet(fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGet() failed(%u)\n", sts);
        SessionDestroyFuture(sess_hndl, fut);
        return -1;
    }

    /*
     * Get the status of the operation
     */
    sts = FutureGetPostRet(fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGetPostRet() status(%u)\n", sts);
        ret = -1;
    }

    SessionDestroyFuture(sess_hndl, fut);

    return ret;
}

/*
 * @brief Write store in a separate thread
 */
static void *WriteStoreThread(void *_args)
{
    /*
     * @brief Write record in store
     */

    RegressionArgs *args = (RegressionArgs *)_args;

    int o_ret = OpenStore(&args->sess_hndl,
        args->cluster_name,
        args->space_name,
        args->store_name,
        args->transactional,
        args->sess_sync_mode,
        args->provider_name,
        (SoeBool )args->debug);
    if ( o_ret || !args->sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return 0;
    }

    SessionStatus sts = SessionPut(args->sess_hndl, args->key, args->key_size, args->value, args->value_size);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionPut() failed(%u)\n", sts);
    }
    return 0;
}

/*
 * @brief Write store asynchronouosly in a separate thread
 */
static void *WriteStoreThreadAsync(void *_args)
{
    /*
     * @brief Write record in store
     */
    RegressionArgs *args = (RegressionArgs *)_args;

    int o_ret = OpenStore(&args->sess_hndl,
        args->cluster_name,
        args->space_name,
        args->store_name,
        args->transactional,
        args->sess_sync_mode,
        args->provider_name,
        (SoeBool )args->debug);
    if ( o_ret || !args->sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return 0;
    }

    /*
     * @brief Write record in store by getting a future
     */
    FutureHND fut = SessionPutAsync(args->sess_hndl, args->key, args->key_size, args->value, args->value_size);
    if ( !fut ) {
        fprintf(stderr, "SessionPutAsync() returned NULL future\n");
        return 0;
    }

    /*
     * Get the return code from the future
     */
    SessionStatus sts = FutureGet(fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGet() failed(%u)\n", sts);
        SessionDestroyFuture(args->sess_hndl, fut);
        return 0;
    }

    /*
     * Get the status of the operation
     */
    sts = FutureGetPostRet(fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGetPostRet() status(%u)\n", sts);
    }

    SessionDestroyFuture(args->sess_hndl, fut);

    return 0;
}

/*
 * @brief Merge store in a separate thread
 */
static void *MergeStoreThread(void *_args)
{
    /*
     * @brief Write record in store
     */

    RegressionArgs *args = (RegressionArgs *)_args;

    int o_ret = OpenStore(&args->sess_hndl,
        args->cluster_name,
        args->space_name,
        args->store_name,
        args->transactional,
        args->sess_sync_mode,
        args->provider_name,
        (SoeBool )args->debug);
    if ( o_ret || !args->sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return 0;
    }

    SessionStatus sts = SessionMerge(args->sess_hndl, args->key, args->key_size, args->value, args->value_size);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionMerge() failed(%u)\n", sts);
    }
    return 0;
}

/*
 * @brief Merge store in a separate thread asynchrnously
 */
static void *MergeStoreThreadAsync(void *_args)
{
    /*
     * @brief Write record in store
     */

    RegressionArgs *args = (RegressionArgs *)_args;

    int o_ret = OpenStore(&args->sess_hndl,
        args->cluster_name,
        args->space_name,
        args->store_name,
        args->transactional,
        args->sess_sync_mode,
        args->provider_name,
        (SoeBool )args->debug);
    if ( o_ret || !args->sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return 0;
    }

    SessionStatus sts = SessionMerge(args->sess_hndl, args->key, args->key_size, args->value, args->value_size);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionMerge() failed(%u)\n", sts);
    }
    return 0;
}

/*
 * @brief Write num_ops KV items asynchronously in store
 */
static int WriteStoreNAsync(const SessionHND sess_hndl,
    size_t num_ops,
    char *keys[],
    size_t *key_sizes,
    char *values[],
    size_t *value_sizes)
{
    int ret = 0;

    uint32_t i;
    CDPairVector kv_vector;
    size_t fail_pos;

    kv_vector.size = num_ops;
    kv_vector.vector = malloc(sizeof(CDuplePair)*kv_vector.size);

    /*
     * @brief Write num_ops records in store
     */
    for ( i = 0 ; i < num_ops ; i++ ) {
        CDuple *cd_key = &kv_vector.vector[i].pair[0];
        CDuple *cd_value = &kv_vector.vector[i].pair[1];

        cd_key->data_ptr = keys[i];
        cd_key->data_size = key_sizes[i];

        cd_value->data_ptr = values[i];
        cd_value->data_size = value_sizes[i];
    }

    SetFutureHND fut = SessionPutSetAsync(sess_hndl, &kv_vector, SoeTrue, &fail_pos);
    if ( !fut ) {
        fprintf(stderr, "SessionPutSetAsync() returned NULL future\n");
        free(kv_vector.vector);
        return -1;
    }

    /*
     * Get the return code from the future
     */
    SessionStatus sts = FutureGet((FutureHND )fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGet() failed(%u)\n", sts);
        SessionDestroyFuture(sess_hndl, (FutureHND )fut);
        free(kv_vector.vector);
        return -1;
    }

    /*
     * Get the status of the operation
     */
    sts = FutureGetPostRet((FutureHND )fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGetPostRet() status(%u)\n", sts);
        ret = -1;
    }

    SessionDestroyFuture(sess_hndl, (FutureHND )fut);
    free(kv_vector.vector);

    return ret;
}

/*
 * @brief Write num_ops KV items in store
 */
static int WriteStoreN(const SessionHND sess_hndl,
    size_t num_ops,
    char *keys[],
    size_t *key_sizes,
    char *values[],
    size_t *value_sizes)
{
    int ret = 0;

    uint32_t i;
    CDPairVector kv_vector;
    size_t fail_pos;

    kv_vector.size = num_ops;
    kv_vector.vector = malloc(sizeof(CDuplePair)*kv_vector.size);

    /*
     * @brief Write num_ops records in store
     */
    for ( i = 0 ; i < num_ops ; i++ ) {
        CDuple *cd_key = &kv_vector.vector[i].pair[0];
        CDuple *cd_value = &kv_vector.vector[i].pair[1];

        cd_key->data_ptr = keys[i];
        cd_key->data_size = key_sizes[i];

        cd_value->data_ptr = values[i];
        cd_value->data_size = value_sizes[i];
    }

    SessionStatus sts = SessionPutSet(sess_hndl, &kv_vector, SoeTrue, &fail_pos);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionPutSet() failed(%u) fail_pos(%lu)\n", sts, fail_pos);
        free(kv_vector.vector);
        return -1;
    }

    free(kv_vector.vector);
    return ret;
}

/*
 * @brief Write num_ops KV items in store in a separate thread
 */
static void *WriteStoreNThread(void *_args)
{
    uint32_t i;
    CDPairVector kv_vector;
    size_t fail_pos;

    RegressionArgs *args = (RegressionArgs *)_args;

    int o_ret = OpenStore(&args->sess_hndl,
        args->cluster_name,
        args->space_name,
        args->store_name,
        args->transactional,
        args->sess_sync_mode,
        args->provider_name,
        (SoeBool )args->debug);
    if ( o_ret || !args->sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return 0;
    }

    kv_vector.size = args->num_ops;
    kv_vector.vector = malloc(sizeof(CDuplePair)*kv_vector.size);

    /*
     * @brief Write num_ops records in store
     */
    for ( i = 0 ; i < args->num_ops ; i++ ) {
        CDuple *cd_key = &kv_vector.vector[i].pair[0];
        CDuple *cd_value = &kv_vector.vector[i].pair[1];

        cd_key->data_ptr = args->key_ptrs[i];
        cd_key->data_size = args->key_sizes[i];

        cd_value->data_ptr = args->value_ptrs[i];
        cd_value->data_size = args->value_sizes[i];
    }

    SessionStatus sts = SessionPutSet(args->sess_hndl, &kv_vector, SoeTrue, &fail_pos);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionPutSet() failed(%u) fail_pos(%lu)\n", sts, fail_pos);
    }

    free(kv_vector.vector);
    return 0;
}

/*
 * @brief Write num_ops KV items asynchronously in store in a separate thread
 */
static void *WriteStoreNThreadAsync(void *_args)
{
    uint32_t i;
    CDPairVector kv_vector;
    size_t fail_pos;

    RegressionArgs *args = (RegressionArgs *)_args;

    int o_ret = OpenStore(&args->sess_hndl,
        args->cluster_name,
        args->space_name,
        args->store_name,
        args->transactional,
        args->sess_sync_mode,
        args->provider_name,
        (SoeBool )args->debug);
    if ( o_ret || !args->sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return 0;
    }

    kv_vector.size = args->num_ops;
    kv_vector.vector = malloc(sizeof(CDuplePair)*kv_vector.size);

    /*
     * @brief Write num_ops records in store
     */
    for ( i = 0 ; i < args->num_ops ; i++ ) {
        CDuple *cd_key = &kv_vector.vector[i].pair[0];
        CDuple *cd_value = &kv_vector.vector[i].pair[1];

        cd_key->data_ptr = args->key_ptrs[i];
        cd_key->data_size = args->key_sizes[i];

        cd_value->data_ptr = args->value_ptrs[i];
        cd_value->data_size = args->value_sizes[i];
    }

    SetFutureHND fut = SessionPutSetAsync(args->sess_hndl, &kv_vector, SoeTrue, &fail_pos);
    if ( !fut ) {
        fprintf(stderr, "SessionPutSetAsync() returned NULL future\n");
        free(kv_vector.vector);
        return 0;
    }

    /*
     * Get the return code from the future
     */
    SessionStatus sts = FutureGet((FutureHND )fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGet() failed(%u)\n", sts);
        SessionDestroyFuture(args->sess_hndl, (FutureHND )fut);
        free(kv_vector.vector);
        return 0;
    }

    /*
     * Get the status of the operation
     */
    sts = FutureGetPostRet((FutureHND )fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGetPostRet() status(%u)\n", sts);
    }

    SessionDestroyFuture(args->sess_hndl, (FutureHND )fut);
    free(kv_vector.vector);

    return 0;
}

/*
 * @brief Merge num_ops KV items in store
 */
static int MergeStoreN(const SessionHND sess_hndl,
    size_t num_ops,
    char *keys[],
    size_t *key_sizes,
    char *values[],
    size_t *value_sizes)
{
    int ret = 0;

    uint32_t i;
    CDPairVector kv_vector;

    kv_vector.size = num_ops;
    kv_vector.vector = malloc(sizeof(CDuplePair)*kv_vector.size);

    /*
     * @brief Write num_ops records in store
     */
    for ( i = 0 ; i < num_ops ; i++ ) {
        CDuple *cd_key = &kv_vector.vector[i].pair[0];
        CDuple *cd_value = &kv_vector.vector[i].pair[1];

        cd_key->data_ptr = keys[i];
        cd_key->data_size = key_sizes[i];

        cd_value->data_ptr = values[i];
        cd_value->data_size = value_sizes[i];
    }

    SessionStatus sts = SessionMergeSet(sess_hndl, &kv_vector);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionMergeSet() failed(%u)\n", sts);
        free(kv_vector.vector);
        return -1;
    }

    free(kv_vector.vector);
    return ret;
}

/*
 * @brief Merge num_ops KV items in store asynchronously
 */
static int MergeStoreNAsync(const SessionHND sess_hndl,
    size_t num_ops,
    char *keys[],
    size_t *key_sizes,
    char *values[],
    size_t *value_sizes)
{
    int ret = 0;

    uint32_t i;
    CDPairVector kv_vector;

    kv_vector.size = num_ops;
    kv_vector.vector = malloc(sizeof(CDuplePair)*kv_vector.size);

    /*
     * @brief Write num_ops records in store
     */
    for ( i = 0 ; i < num_ops ; i++ ) {
        CDuple *cd_key = &kv_vector.vector[i].pair[0];
        CDuple *cd_value = &kv_vector.vector[i].pair[1];

        cd_key->data_ptr = keys[i];
        cd_key->data_size = key_sizes[i];

        cd_value->data_ptr = values[i];
        cd_value->data_size = value_sizes[i];
    }

    SetFutureHND fut = SessionMergeSetAsync(sess_hndl, &kv_vector);
    if ( !fut ) {
        fprintf(stderr, "SessionMergeSetAsync() returned NULL future\n");
        free(kv_vector.vector);
        return -1;
    }

    /*
     * Get the return code from the future
     */
    SessionStatus sts = FutureGet((FutureHND )fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGet() failed(%u)\n", sts);
        SessionDestroyFuture(sess_hndl, (FutureHND )fut);
        free(kv_vector.vector);
        return -1;
    }

    /*
     * Get the status of the operation
     */
    sts = FutureGetPostRet((FutureHND )fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGetPostRet() status(%u)\n", sts);
        ret = -1;
    }

    SessionDestroyFuture(sess_hndl, (FutureHND )fut);
    free(kv_vector.vector);

    return ret;
}

/*
 * @brief Merge num_ops KV items in store in a separate thread
 */
static void *MergeStoreNThread(void *_args)
{
    uint32_t i;
    CDPairVector kv_vector;

    RegressionArgs *args = (RegressionArgs *)_args;

    int o_ret = OpenStore(&args->sess_hndl,
        args->cluster_name,
        args->space_name,
        args->store_name,
        args->transactional,
        args->sess_sync_mode,
        args->provider_name,
        (SoeBool )args->debug);
    if ( o_ret || !args->sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return 0;
    }

    kv_vector.size = args->num_ops;
    kv_vector.vector = malloc(sizeof(CDuplePair)*kv_vector.size);

    /*
     * @brief Write num_ops records in store
     */
    for ( i = 0 ; i < args->num_ops ; i++ ) {
        CDuple *cd_key = &kv_vector.vector[i].pair[0];
        CDuple *cd_value = &kv_vector.vector[i].pair[1];

        cd_key->data_ptr = args->key_ptrs[i];
        cd_key->data_size = args->key_sizes[i];

        cd_value->data_ptr = args->value_ptrs[i];
        cd_value->data_size = args->value_sizes[i];
    }

    SessionStatus sts = SessionMergeSet(args->sess_hndl, &kv_vector);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionMergeSet() failed(%u)\n", sts);
    }

    free(kv_vector.vector);
    return 0;
}

/*
 * @brief Merge num_ops KV items asynchronously in store in a separate thread
 */
static void *MergeStoreNThreadAsync(void *_args)
{
    uint32_t i;
    CDPairVector kv_vector;

    RegressionArgs *args = (RegressionArgs *)_args;

    int o_ret = OpenStore(&args->sess_hndl,
        args->cluster_name,
        args->space_name,
        args->store_name,
        args->transactional,
        args->sess_sync_mode,
        args->provider_name,
        (SoeBool )args->debug);
    if ( o_ret || !args->sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return 0;
    }

    kv_vector.size = args->num_ops;
    kv_vector.vector = malloc(sizeof(CDuplePair)*kv_vector.size);

    /*
     * @brief Write num_ops records in store
     */
    for ( i = 0 ; i < args->num_ops ; i++ ) {
        CDuple *cd_key = &kv_vector.vector[i].pair[0];
        CDuple *cd_value = &kv_vector.vector[i].pair[1];

        cd_key->data_ptr = args->key_ptrs[i];
        cd_key->data_size = args->key_sizes[i];

        cd_value->data_ptr = args->value_ptrs[i];
        cd_value->data_size = args->value_sizes[i];
    }

    SetFutureHND fut = SessionMergeSetAsync(args->sess_hndl, &kv_vector);
    if ( !fut ) {
        fprintf(stderr, "SessionPutMergeAsync() returned NULL future\n");
        free(kv_vector.vector);
        return 0;
    }

    /*
     * Get the return code from the future
     */
    SessionStatus sts = FutureGet((FutureHND )fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGet() failed(%u)\n", sts);
        SessionDestroyFuture(args->sess_hndl, (FutureHND )fut);
        free(kv_vector.vector);
        return 0;
    }

    /*
     * Get the status of the operation
     */
    sts = FutureGetPostRet((FutureHND )fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGetPostRet() status(%u)\n", sts);
    }

    SessionDestroyFuture(args->sess_hndl, (FutureHND )fut);
    free(kv_vector.vector);

    return 0;
}

/*
 * @brief Read all KV items from store
 */
static int ReadStore(const SessionHND sess_hndl,
    const char *key,
    size_t key_size,
    const char *end_key,
    size_t end_key_size,
    int iter_dir,
    int default_first,
    int debug,
    int hex_dump,
    int no_print,
    uint64_t *counter)
{
    const char *key_arg = key;
    size_t key_arg_size = key_size;
    const char *end_key_arg = end_key;
    size_t end_key_arg_size = end_key_size;
    eIteratorDir dir = !iter_dir ? eIteratorDirForward : eIteratorDirBackward;
    const char *s_key = (dir == eIteratorDirForward ? key_arg : end_key_arg);
    size_t s_key_size = (dir == eIteratorDirForward ? key_arg_size : end_key_arg_size);

    if ( !default_first ) {
        s_key = 0;
        s_key_size = 0;
    }

    /*
     * @brief Create iterator
     */
    IteratorHND i_hndl = SessionCreateIterator(sess_hndl,
        dir,
        key_arg,
        key_arg_size,
        end_key_arg,
        end_key_arg_size);
    if ( !i_hndl ) {
        fprintf(stderr, "SessionCreateIterator() failed(%u)\n", SessionLastStatus(sess_hndl));
        return -1;
    }

    if ( counter ) {
        (*counter) += 2;
    }

    SessionStatus sts = eOk;
    for ( sts = SessionIteratorFirst(i_hndl, s_key, s_key_size) ; SessionIteratorValid(i_hndl) == SoeTrue ; sts = SessionIteratorNext(i_hndl) ) {
        char key[100000];
        char data[200000];

        //printf("###### sts(%u) valid(%d)\n", sts, DisjointSubsetIteratorValid(i_hndl));
        if ( !SessionIteratorKey(i_hndl).data_ptr || !SessionIteratorKey(i_hndl).data_size ) {
            continue;
        }

        if ( counter ) {
            (*counter) += 3;
        }

        memcpy(key, SessionIteratorKey(i_hndl).data_ptr, MIN(sizeof(key), SessionIteratorKey(i_hndl).data_size));
        key[MIN(sizeof(key) - 1, SessionIteratorKey(i_hndl).data_size)] = '\0';
        memcpy(data, SessionIteratorValue(i_hndl).data_ptr, MIN(sizeof(data), SessionIteratorValue(i_hndl).data_size));
        data[MIN(sizeof(data) - 1, SessionIteratorValue(i_hndl).data_size)] = '\0';

        if ( !no_print ) {
            if ( hex_dump == 1 ) {
                fprintf(stdout, "key ---- >\n");
                debugPrintHexBuf(stdout, key, SessionIteratorKey(i_hndl).data_size);

                fprintf(stdout, "data ---- >\n");
                debugPrintHexBuf(stdout, data, SessionIteratorValue(i_hndl).data_size);
            } else if ( hex_dump == 2 ) {
                fprintf(stderr, key, SessionIteratorKey(i_hndl).data_size);
                fprintf(stdout, data, SessionIteratorValue(i_hndl).data_size);
            }

            if ( debug != -1 && hex_dump != 2 ) {
                size_t data_size = SessionIteratorValue(i_hndl).data_size;
                if ( data_size > 128 ) {
                    data_size = 128;
                    data[128] = '\0';
                }
                fprintf(stdout, "key(%lu,%s) value(%lu,%s)\n",
                    SessionIteratorKey(i_hndl).data_size, key, SessionIteratorValue(i_hndl).data_size, data);
            }
        }
    }

    if ( sts != eOk ) {
        fprintf(stderr, "Store iterator's results invalid sts(%u)\n", sts);
    }

    sts = SessionDestroyIterator(sess_hndl, i_hndl);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionDestroyIterator() failed(%u)\n", sts);
        return -1;
    }

    if ( counter ) {
        (*counter)++;
    }

    return 0;
}

static int ReadKVItemStoreAsync(const SessionHND sess_hndl,
    const char *key,
    size_t key_size,
    char *buf,
    size_t buf_size,
    int hex_dump,
    int no_print);

/*
 * @brief Read all KV items asynchronously from store
 */
static int ReadStoreAsync(const SessionHND sess_hndl,
    const char *key,
    size_t key_size,
    const char *end_key,
    size_t end_key_size,
    int iter_dir,
    int default_first,
    int debug,
    int hex_dump,
    int no_print,
    uint64_t *counter)
{
    const char *key_arg = key;
    size_t key_arg_size = key_size;
    const char *end_key_arg = end_key;
    size_t end_key_arg_size = end_key_size;
    eIteratorDir dir = !iter_dir ? eIteratorDirForward : eIteratorDirBackward;
    const char *s_key = (dir == eIteratorDirForward ? key_arg : end_key_arg);
    size_t s_key_size = (dir == eIteratorDirForward ? key_arg_size : end_key_arg_size);

    if ( !default_first ) {
        s_key = 0;
        s_key_size = 0;
    }

    /*
     * @brief Create iterator
     */
    IteratorHND i_hndl = SessionCreateIterator(sess_hndl,
        dir,
        key_arg,
        key_arg_size,
        end_key_arg,
        end_key_arg_size);
    if ( !i_hndl ) {
        fprintf(stderr, "SessionCreateIterator() failed(%u)\n", SessionLastStatus(sess_hndl));
        return -1;
    }

    if ( counter ) {
        (*counter) += 2;
    }

    SessionStatus sts = eOk;
    for ( sts = SessionIteratorFirst(i_hndl, s_key, s_key_size) ; SessionIteratorValid(i_hndl) == SoeTrue ; sts = SessionIteratorNext(i_hndl) ) {
        char data[200000];

        //printf("###### sts(%u) valid(%d)\n", sts, DisjointSubsetIteratorValid(i_hndl));
        if ( !SessionIteratorKey(i_hndl).data_ptr || !SessionIteratorKey(i_hndl).data_size ) {
            continue;
        }

        if ( counter ) {
            (*counter) += 3;
        }

        /*
         * Read KV item asynchrpnously
         */
        (void )ReadKVItemStoreAsync(sess_hndl,
            SessionIteratorKey(i_hndl).data_ptr,
            SessionIteratorKey(i_hndl).data_size,
            data,
            sizeof(data),
            hex_dump,
            no_print);
    }

    if ( sts != eOk ) {
        fprintf(stderr, "Store iterator's results invalid sts(%u)\n", sts);
    }

    sts = SessionDestroyIterator(sess_hndl, i_hndl);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionDestroyIterator() failed(%u)\n", sts);
        return -1;
    }

    if ( counter ) {
        (*counter)++;
    }

    return 0;
}

/*
 * @brief Read one KV item from store
 */
static int ReadKVItemStore(const SessionHND sess_hndl,
    const char *key,
    size_t key_size,
    char *buf,
    size_t buf_size,
    int hex_dump,
    int no_print)
{
    /*
     * @brief Get item
     */
    size_t orig_buf_size = buf_size;
    SessionStatus sts = SessionGet(sess_hndl, key, key_size, buf, &buf_size);
    if ( sts != eOk && sts != eTruncatedValue ) {
        fprintf(stderr, "SessionGet() failed(%u) buf_size(%lu)\n", sts, buf_size);
        return -1;
    }
    if ( sts == eTruncatedValue ) {
        fprintf(stderr, "SessionGet() truncation occured(%u) buf_size(%lu) actual value size(%lu)\n",
            sts, orig_buf_size, buf_size);
        buf_size = orig_buf_size;
    }

    if ( !no_print ) {
        if ( hex_dump == 1 ) {
            fprintf(stdout, "key ---- >\n");
            debugPrintHexBuf(stdout, key, key_size);

            fprintf(stdout, "data ---- >\n");
            debugPrintHexBuf(stdout, buf, buf_size);
        } else if ( hex_dump == 2 ) {
            fprintf(stderr, key, key_size);
            fprintf(stdout, buf, buf_size);
        }

        if ( hex_dump != 2 ) {
            char print_buf[129];
            char *print_buf_ptr = buf;
            size_t print_buf_size = buf_size;
            if ( print_buf_size > sizeof(print_buf) - 1 ) {
                print_buf_size = sizeof(print_buf) - 1;
                memcpy(print_buf, buf, print_buf_size);
                print_buf_ptr = print_buf;
                print_buf[print_buf_size] = '\0';
            }
            fprintf(stdout, "key(%lu,%s) value(%lu,%s)\n", key_size, key, buf_size, print_buf_ptr);
        }
    }

    return 0;
}

/*
 * @brief Read one KV item from store asynchronously
 */
static int ReadKVItemStoreAsync(const SessionHND sess_hndl,
    const char *key,
    size_t key_size,
    char *buf,
    size_t buf_size,
    int hex_dump,
    int no_print)
{
    char *fut_key = 0;
    size_t fut_key_size;
    char *fut_value = 0;
    size_t fut_value_size = buf_size;
    CDuple fut_key_cduple;
    CDuple fut_value_cduple;

    /*
     * @brief Get item
     */
    FutureHND fut = SessionGetAsync(sess_hndl, key, key_size, buf, &buf_size);
    if ( !fut ) {
        fprintf(stderr, "SessionGetAsync() returned NULL future\n");
        return -1;
    }

    /*
     * Get the return code from the future
     */
    SessionStatus sts = FutureGet(fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGet() failed(%u)\n", sts);
        SessionDestroyFuture(sess_hndl, fut);
        return -1;
    }

    /*
     * Get the status of the operation
     */
    sts = FutureGetPostRet(fut);
    if ( sts != eOk ) {
        if ( sts == eTruncatedValue ) {
            fprintf(stderr, "FutureGetPostRet() truncation occured(%u)\n", sts);
        } else {
            fprintf(stderr, "FutureGetPostRet() status(%u)\n", sts);
            SessionDestroyFuture(sess_hndl, fut);
            return -1;
        }
    }

    /*
     * Test all Get fuctions
     */
    sts = FutureGetKey(fut, &fut_key);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGetKey() failed(%u)\n", sts);
        SessionDestroyFuture(sess_hndl, fut);
        return -1;
    }
    sts = FutureGetKeySize(fut, &fut_key_size);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGetKeySize() failed(%u)\n", sts);
        SessionDestroyFuture(sess_hndl, fut);
        return -1;
    }
    sts = FutureGetValue(fut, &fut_value);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGetValue() failed(%u)\n", sts);
        SessionDestroyFuture(sess_hndl, fut);
        return -1;
    }
    sts = FutureGetValueSize(fut, &fut_value_size);
    if ( sts != eOk ) {
        if ( sts == eTruncatedValue ) {
            fprintf(stderr, "FutureGetValueSize() truncation occured(%u) buf_size(%lu) actual value size(%lu)\n",
                sts, buf_size, fut_value_size);
        } else {
            fprintf(stderr, "FutureGetValueSize() failed(%u)\n", sts);
            SessionDestroyFuture(sess_hndl, fut);
            return -1;
        }
    }
    sts = FutureGetCDupleKey(fut, &fut_key_cduple);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGetCDupleKey() failed(%u)\n", sts);
        SessionDestroyFuture(sess_hndl, fut);
        return -1;
    }
    sts = FutureGetCDupleValue(fut, &fut_value_cduple);
    if ( sts != eOk ) {
        if ( sts == eTruncatedValue ) {
            fprintf(stderr, "FutureGetCDupleValue() truncation occured(%u) buf_size(%lu) actual value size(%lu)\n",
                sts, buf_size, fut_value_cduple.data_size);
            fut_value_size = fut_value_cduple.data_size;
        } else {
            fprintf(stderr, "FutureGetCDupleValue() failed(%u)\n", sts);
            SessionDestroyFuture(sess_hndl, fut);
            return -1;
        }
    }

    /*
     * Validate key and key size
     */
    if ( key_size != fut_key_size ) {
        fprintf(stderr, "Key size mismatch key_size(%lu) fut_key_size(%lu)\n", key_size, fut_key_size);
        SessionDestroyFuture(sess_hndl, fut);
        return -1;
    }
    if ( memcmp(key, fut_key, fut_key_size) ) {
        fprintf(stderr, "Key mismatch\n");
        SessionDestroyFuture(sess_hndl, fut);
        return -1;
    }

    /*
     * Validate value
     */
    if ( !fut_value || !fut_value_size ) {
        fprintf(stderr, "Value NULL or fut_value(%lu) value_size(%lu)\n", (size_t )fut_value, fut_value_size);
        SessionDestroyFuture(sess_hndl, fut);
        return -1;
    }

    if ( !no_print ) {
        if ( hex_dump == 1 ) {
            fprintf(stdout, "key ---- >\n");
            debugPrintHexBuf(stdout, fut_key, fut_key_size);

            fprintf(stdout, "data ---- >\n");
            debugPrintHexBuf(stdout, buf, fut_value_size);
        } else if ( hex_dump == 2 ) {
            char tmp_buf[128];
            fprintf(stderr, key, key_size);
            memcpy(tmp_buf, fut_value, MIN(sizeof(tmp_buf), fut_value_size));
            tmp_buf[MIN(sizeof(tmp_buf), fut_value_size) - 1] = '\0';
            fprintf(stdout, tmp_buf, MIN(sizeof(tmp_buf), fut_value_size));
        }

        if ( hex_dump != 2 ) {
            char print_buf[129];
            char *print_buf_ptr = buf;
            size_t print_buf_size = buf_size;
            if ( print_buf_size > sizeof(print_buf) - 1 ) {
                print_buf_size = sizeof(print_buf) - 1;
                memcpy(print_buf, buf, print_buf_size);
                print_buf_ptr = print_buf;
                print_buf[print_buf_size] = '\0';
            }
            fprintf(stdout, "key(%lu,%s) value(%lu,%s)\n", key_size, key, buf_size, print_buf_ptr);
        }
    }

    return 0;
}

/*
 * @brief Delete one KV item from store
 */
static int DeleteKVItemStore(const SessionHND sess_hndl,
    const char *key,
    size_t key_size)
{
    /*
     * @brief Delete item
     */
    SessionStatus sts = SessionDelete(sess_hndl, key, key_size);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionDelete() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Delete one KV item from store Asynchronously
 */
static int DeleteKVItemStoreAsync(const SessionHND sess_hndl,
    const char *key,
    size_t key_size)
{
    /*
     * @brief Delete item
     */
    FutureHND fut = SessionDeleteAsync(sess_hndl, key, key_size);
    if ( !fut ) {
        fprintf(stderr, "SessionDeleteAsync() returned NULL future\n");
        return -1;
    }

    /*
     * Get the return code from the future
     */
    SessionStatus sts = FutureGet(fut);
    if ( sts != eOk ) {
        fprintf(stderr, "FutureGet() failed(%u)\n", sts);
        SessionDestroyFuture(sess_hndl, fut);
        return -1;
    }

    /*
     * Get the status of the operation
     */
    sts = FutureGetPostRet(fut);
    if ( sts != eOk ) {
        if ( sts == eNotFoundKey ) {
            fprintf(stderr, "FutureGetPostRet() key not found(%u)\n", sts);
        } else {
            fprintf(stderr, "FutureGetPostRet() status(%u)\n", sts);
            SessionDestroyFuture(sess_hndl, fut);
            return -1;
        }
    }

    SessionDestroyFuture(sess_hndl, fut);

    return 0;
}

/*
 * @brief Destroy store
 */
static int DestroyStore(SessionHND sess_hndl)
{
    SessionStatus sts = SessionDestroyStore(sess_hndl);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionDestroyStore() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Close store
 */
static void CloseStore(SessionHND sess_hndl, int wait_for_key)
{
    if ( wait_for_key ) {
        (void )fgetc(stdin);
    }
    SessionClose(sess_hndl);
}

/*
 * @brief Create group in store
 */
static int CreateGroup(const SessionHND sess_hndl,
    GroupHND *grp_hndl)
{
    /*
     * @brief Obtain Group handle
     */
    *grp_hndl = SessionCreateGroup(sess_hndl);
    if ( !*grp_hndl ) {
        fprintf(stderr, "SessionCreateGroup() failed(%u)\n", SessionLastStatus(sess_hndl));
        return -1;
    }

    return 0;
}

/*
 * @brief Create group in store
 */
static int DeleteGroup(const SessionHND sess_hndl,
    GroupHND grp_hndl)
{
    /*
     * @brief Delete Group handle
     */
    SessionStatus sts = SessionDestroyGroup(sess_hndl, grp_hndl);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionDeleteGroup() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Put elem in group in store
 */
static int GroupPutTo(GroupHND grp_hndl,
    const char *key,
    size_t key_size,
    const char *data,
    size_t data_size)
{
    /*
     * @brief Put item in group
     */
    SessionStatus sts = GroupPut(grp_hndl, key, key_size, data, data_size);
    if ( sts != eOk ) {
        fprintf(stderr, "PutGroup() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Merge elem from group in store
 */
static int GroupMergeTo(GroupHND grp_hndl,
    const char *key,
    size_t key_size,
    const char *data,
    size_t data_size)
{
    /*
     * @brief Merge item in group
     */
    SessionStatus sts = GroupMerge(grp_hndl, key, key_size, data, data_size);
    if ( sts != eOk ) {
        fprintf(stderr, "GroupMerge() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Delete elem from group in store
 */
static int GroupDeleteFrom(GroupHND grp_hndl,
    const char *key,
    size_t key_size)
{
    /*
     * @brief Delete item in group
     */
    SessionStatus sts = GroupDelete(grp_hndl, key, key_size);
    if ( sts != eOk ) {
        fprintf(stderr, "GroupDelete() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Write group in store
 */
static int WriteGroup(const SessionHND sess_hndl,
    const GroupHND grp_hndl)
{
    /*
     * @brief Write Group handle
     */
    SessionStatus sts = SessionWrite(sess_hndl, grp_hndl);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionWrite() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Start transaction in store
 */
static int StartTransaction(const SessionHND sess_hndl,
    TransactionHND *trans_hndl)
{
    /*
     * @brief Obtain Transaction handle
     */
    *trans_hndl = SessionStartTransaction(sess_hndl);
    if ( !*trans_hndl ) {
        fprintf(stderr, "SessionCreateTransaction() failed(%u)\n", SessionLastStatus(sess_hndl));
        return -1;
    }

    return 0;
}

/*
 * @brief Commit transaction in store
 */
static int CommitTransaction(const SessionHND sess_hndl,
    TransactionHND trans_hndl)
{
    /*
     * @brief Commit to transaction handle
     */
    SessionStatus sts = SessionCommitTransaction(sess_hndl, trans_hndl);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionCommitTransaction() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Rollback transaction in store
 */
static int RollbackTransaction(const SessionHND sess_hndl,
    TransactionHND trans_hndl)
{
    /*
     * @brief Rollback to transaction handle
     */
    SessionStatus sts = SessionRollbackTransaction(sess_hndl, trans_hndl);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionRollbackTransaction() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Put elem in transaction in store
 */
static int TransactionPutTo(TransactionHND trans_hndl,
    const char *key,
    size_t key_size,
    const char *data,
    size_t data_size)
{
    /*
     * @brief Put item in transaction
     */
    SessionStatus sts = TransactionPut(trans_hndl, key, key_size, data, data_size);
    if ( sts != eOk ) {
        fprintf(stderr, "TransactionPut() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Get elem from transaction in store
 */
static int TransactionGetFrom(TransactionHND trans_hndl,
    const char *key,
    size_t key_size,
    char *buf,
    size_t buf_size)
{
    /*
     * @brief Get item from transaction
     */
    SessionStatus sts = TransactionGet(trans_hndl, key, key_size);
    if ( sts != eOk ) {
        fprintf(stderr, "TransactionPut() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Merge elem from transaction in store
 */
static int TransactionMergeTo(TransactionHND trans_hndl,
    const char *key,
    size_t key_size,
    const char *data,
    size_t data_size)
{
    /*
     * @brief Merge item in transaction
     */
    SessionStatus sts = TransactionMerge(trans_hndl, key, key_size, data, data_size);
    if ( sts != eOk ) {
        fprintf(stderr, "TransactionMerge() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Delete elem from transaction in store
 */
static int TransactionDeleteFrom(TransactionHND trans_hndl,
    const char *key,
    size_t key_size)
{
    /*
     * @brief Delete item in transaction
     */
    SessionStatus sts = TransactionDelete(trans_hndl, key, key_size);
    if ( sts != eOk ) {
        fprintf(stderr, "TransactionDelete() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Create subset
 */
static int CreateSubset(const SessionHND sess_hndl,
    SubsetableHND *sub_hndl,
    const char *subset_name,
    size_t subset_name_size)
{
    /*
     * @brief Create subset
     */
    SubsetableHND new_sub_hndl = SessionCreateSubset(sess_hndl, subset_name, subset_name_size, eSubsetTypeDisjoint);
    if ( !new_sub_hndl ) {
        fprintf(stderr, "SessionCreateSubset() failed(%u)\n", SessionLastStatus(sess_hndl));
        return -1;
    }
    *sub_hndl = new_sub_hndl;

    return 0;
}

/*
 * @brief Open subset
 */
static int OpenSubset(const SessionHND sess_hndl,
    SubsetableHND *sub_hndl,
    const char *subset_name,
    size_t subset_name_size)
{
    /*
     * @brief Open subset
     */
    SubsetableHND new_sub_hndl = SessionOpenSubset(sess_hndl, subset_name, subset_name_size, eSubsetTypeDisjoint);
    if ( !new_sub_hndl ) {
        fprintf(stderr, "SessionOpenSubset() failed(%u)\n", SessionLastStatus(sess_hndl));
        return -1;
    }
    *sub_hndl = new_sub_hndl;

    return 0;
}

/*
 * @brief Delete (Close) subset object
 */
static int CloseSubset(const SessionHND sess_hndl, SubsetableHND sub_hndl)
{
    SessionStatus sts = SessionDeleteSubset(sess_hndl, sub_hndl);
    if ( sts != eOk ) {
        fprintf(stderr, "CloseSubset() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Write subset
 */
static int WriteSubset(SubsetableHND sub_hndl,
    const char *key,
    size_t key_size,
    const char *value,
    size_t value_size)
{
    /*
     * @brief Insert record
     */
    SessionStatus sts = DisjointSubsetPut(sub_hndl, key, key_size, value, value_size);
    if ( sts != eOk ) {
        fprintf(stderr, "DisjointSubsetPut() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Merge subset
 */
static int MergeSubset(SubsetableHND sub_hndl,
    const char *key,
    size_t key_size,
    const char *value,
    size_t value_size)
{
    /*
     * @brief Insert record
     */
    SessionStatus sts = DisjointSubsetMerge(sub_hndl, key, key_size, value, value_size);
    if ( sts != eOk ) {
        fprintf(stderr, "DisjointSubsetMerge() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Write num_ops KV items in subset
 */
static int WriteSubsetN(SubsetableHND sub_hndl,
    size_t num_ops,
    char *keys[],
    size_t *key_sizes,
    char *values[],
    size_t *value_sizes)
{
    int ret = 0;
    uint32_t i;
    CDPairVector kv_vector;
    size_t fail_pos;

    kv_vector.size = num_ops;
    kv_vector.vector = malloc(sizeof(CDuplePair)*kv_vector.size);

    /*
     * @brief Write num_ops records in store
     */
    for ( i = 0 ; i < num_ops ; i++ ) {
        CDuple *cd_key = &kv_vector.vector[i].pair[0];
        CDuple *cd_value = &kv_vector.vector[i].pair[1];

        //printf("key (%s/%lu) ", keys[i], key_sizes[i]);
        cd_key->data_ptr = keys[i];
        cd_key->data_size = key_sizes[i];

        //printf("value (%s/%lu)\n", values[i], value_sizes[i]);
        cd_value->data_ptr = values[i];
        cd_value->data_size = value_sizes[i];
    }

    SessionStatus sts = DisjointSubsetPutSet(sub_hndl, &kv_vector, SoeTrue, &fail_pos);
    if ( sts != eOk ) {
        fprintf(stderr, "DisjointSubsetPutSet() failed(%u) fail_pos(%lu)\n", sts, fail_pos);
        free(kv_vector.vector);
        return -1;
    }

    free(kv_vector.vector);
    return ret;
}

/*
 * @brief Merge num_ops KV items in subset
 */
static int MergeSubsetN(SubsetableHND sub_hndl,
    size_t num_ops,
    char *keys[],
    size_t *key_sizes,
    char *values[],
    size_t *value_sizes)
{
    int ret = 0;
    uint32_t i;
    CDPairVector kv_vector;

    kv_vector.size = num_ops;
    kv_vector.vector = malloc(sizeof(CDuplePair)*kv_vector.size);

    /*
     * @brief Write num_ops records in store
     */
    for ( i = 0 ; i < num_ops ; i++ ) {
        CDuple *cd_key = &kv_vector.vector[i].pair[0];
        CDuple *cd_value = &kv_vector.vector[i].pair[1];

        //printf("key (%s/%lu) ", keys[i], key_sizes[i]);
        cd_key->data_ptr = keys[i];
        cd_key->data_size = key_sizes[i];

        //printf("value (%s/%lu)\n", values[i], value_sizes[i]);
        cd_value->data_ptr = values[i];
        cd_value->data_size = value_sizes[i];
    }

    SessionStatus sts = DisjointSubsetMergeSet(sub_hndl, &kv_vector);
    if ( sts != eOk ) {
        fprintf(stderr, "DisjointSubsetPutSet() failed(%u)\n", sts);
        free(kv_vector.vector);
        return -1;
    }

    free(kv_vector.vector);
    return ret;
}

/*
 * @brief Read one KV item from subset
 */
static int ReadKVItemSubset(const SessionHND sess_hndl,
    const SubsetableHND sub_hndl,
    const char *key,
    size_t key_size,
    int hex_dump,
    int no_print)
{
    /*
     * @brief Get item
     */
    char buf[200000];
    size_t buf_size = sizeof(buf);
    SessionStatus sts = DisjointSubsetGet(sub_hndl, key, key_size, buf, &buf_size);
    if ( sts != eOk ) {
        fprintf(stderr, "DisjointSubsetGet() failed(%u)\n", sts);
        return -1;
    }

    if ( !no_print ) {
        if ( hex_dump != 2 ) {
            size_t print_buf_size = buf_size;
            if ( print_buf_size > 128 ) {
                print_buf_size = 128;
                buf[128] = '\0';
            }
            fprintf(stdout, "key(%lu,%s) value(%lu,%s)\n", key_size, key, buf_size, buf);
        }

        if ( hex_dump == 1 ) {
            fprintf(stdout, "key ---- >\n");
            debugPrintHexBuf(stdout, key, key_size);

            fprintf(stdout, "data ---- >\n");
            debugPrintHexBuf(stdout, buf, buf_size);
        } else if ( hex_dump == 2 ) {
            fprintf(stderr, key, key_size);
            fprintf(stdout, buf, buf_size);
        }
    }

    return 0;
}

/*
 * @brief Delete one KV item from subset
 */
static int DeleteKVItemSubset(const SessionHND sess_hndl,
    const SubsetableHND sub_hndl,
    const char *key,
    size_t key_size)
{
    /*
     * @brief Delete item
     */
    SessionStatus sts = DisjointSubsetDelete(sub_hndl, key, key_size);
    if ( sts != eOk ) {
        fprintf(stderr, "DisjointSubsetDelete() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

/*
 * @brief Create subset
 */
static int ReadSubset(const SubsetableHND sub_hndl,
    const char *key,
    size_t key_size,
    const char *end_key,
    size_t end_key_size,
    int iter_dir,
    int default_first,
    int debug,
    int hex_dump,
    int no_print,
    uint64_t *counter)
{
    const char *key_arg = key;
    size_t key_arg_size = key_size;
    const char *end_key_arg = end_key;
    size_t end_key_arg_size = end_key_size;
    eIteratorDir dir = !iter_dir ? eIteratorDirForward : eIteratorDirBackward;
    const char *s_key = (dir == eIteratorDirForward ? key_arg : end_key_arg);
    size_t s_key_size = (dir == eIteratorDirForward ? key_arg_size : end_key_arg_size);

    if ( !default_first ) {
        s_key = 0;
        s_key_size = 0;
    }

    /*
     * @brief Create iterator
     */
    IteratorHND i_hndl = DisjointSubsetCreateIterator(sub_hndl,
        dir,
        key_arg,
        key_arg_size,
        end_key_arg,
        end_key_arg_size);
    if ( !i_hndl ) {
        fprintf(stderr, "SubsetCreateIterator() failed\n");
        return -1;
    }

    if ( counter ) {
        (*counter) += 2;
    }

    SessionStatus sts = eOk;
    for ( sts = DisjointSubsetIteratorFirst(i_hndl, s_key, s_key_size) ; DisjointSubsetIteratorValid(i_hndl) == SoeTrue ; sts = DisjointSubsetIteratorNext(i_hndl) ) {
        char key[100000];
        char data[200000];

        //printf("@@@@@@@ sts(%u) valid(%d)\n", sts, DisjointSubsetIteratorValid(i_hndl));
        if ( !SessionIteratorKey(i_hndl).data_ptr || !SessionIteratorKey(i_hndl).data_size ) {
            continue;
        }

        if ( counter ) {
            (*counter) += 3;
        }

        memcpy(key, DisjointSubsetIteratorKey(i_hndl).data_ptr, MIN(sizeof(key), DisjointSubsetIteratorKey(i_hndl).data_size));
        key[MIN(sizeof(key) - 1, DisjointSubsetIteratorKey(i_hndl).data_size)] = '\0';
        memcpy(data, DisjointSubsetIteratorValue(i_hndl).data_ptr, MIN(sizeof(data), DisjointSubsetIteratorValue(i_hndl).data_size));
        data[MIN(sizeof(data) - 1, DisjointSubsetIteratorValue(i_hndl).data_size)] = '\0';

        if ( !no_print ) {
            if ( hex_dump == 1 ) {
                fprintf(stdout, "key ---- >\n");
                debugPrintHexBuf(stdout, key, SessionIteratorKey(i_hndl).data_size);

                fprintf(stdout, "data ---- >\n");
                debugPrintHexBuf(stdout, data, SessionIteratorValue(i_hndl).data_size);
            } else if ( hex_dump == 2 ) {
                fprintf(stderr, key, SessionIteratorKey(i_hndl).data_size);
                fprintf(stdout, data, SessionIteratorValue(i_hndl).data_size);
            }

            if ( debug != -1 && hex_dump != 2 ) {
                size_t data_size = SessionIteratorValue(i_hndl).data_size;
                if ( data_size > 128 ) {
                    data_size = 128;
                    data[128] = '\0';
                }
                fprintf(stdout, "key(%lu,%s) value(%lu,%s)\n", DisjointSubsetIteratorKey(i_hndl).data_size, key, SessionIteratorValue(i_hndl).data_size, data);
            }
        }
    }
    if ( sts != eOk ) {
        fprintf(stderr, "Subset iterator's results invalid sts(%u)\n", sts);
    }

    sts = DisjointSubsetDestroyIterator(sub_hndl, i_hndl);
    if ( sts != eOk ) {
        fprintf(stderr, "SubsetDestroyIterator() failed(%u)\n", sts);
        return -1;
    }

    if ( counter ) {
        (*counter)++;
    }

    return 0;
}

/*
 * @brief DestroySubset subset
 */
static int DestroySubset(const SessionHND sess_hndl, const SubsetableHND sub_hndl)
{
    SessionStatus sts = SessionDestroySubset(sess_hndl, sub_hndl);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionDestroySubset() failed(%u)\n", sts);
        return -1;
    }

    return 0;
}

#define gettid() syscall(SYS_gettid)

const char *rcsdb_p = "RCSDB";
const char *metadbcli_p = "METADBCLI";
const char *not_p_p = "NOTPROVIDED";
static const char *GetProviderName(int prov)
{
    switch ( prov ) {
    case 0:
        return rcsdb_p;
        break;
    case 1:
        fprintf(stderr, "provider currently not supported\n");
        return not_p_p;
        break;
    case 2:
        return metadbcli_p;
        break;
    case 3:
        fprintf(stderr, "provider currently not supported\n");
        return not_p_p;
        break;
    case 4:
        return 0;
        break;
    default:
        fprintf(stderr, "provider currently not supported\n");
        return not_p_p;
        break;
    }

    return not_p_p;
}

static eSessionSyncMode GetSyncMode(int sync_m)
{
    eSessionSyncMode sync_mode = eSyncDefault;
    switch ( sync_m ) {
    case 0:
        sync_mode = eSyncDefault;
        break;
    case 1:
        sync_mode = eSyncFsync;
        break;
    case 2:
        sync_mode = eSyncAsync;
        break;
    default:
        sync_mode = eSyncDefault;
        break;
    }

    return sync_mode;
}

static void RegressionTest1(RegressionArgs *args)
{
    const char *cluster_name = args->cluster_name;
    const char *space_name = args->space_name;
    const char *store_name = args->store_name;
    int provider = args->provider;
    const char *provider_name = 0;
    const char *key = args->key;
    size_t key_size = args->key_size;
    const char *end_key = args->end_key;
    size_t end_key_size = args->end_key_size;
    int it_dir = args->it_dir;
    int default_first = args->default_first;
    char *value = args->value;
    size_t value_size = args->value_size;
    uint32_t num_ops = args->num_ops;
    uint32_t num_regressions = args->num_regressions;
    const char *snap_back_subset_name = args->snap_back_subset_name;
    //uint32_t snap_back_id = args->snap_back_id;
    int transactional = args->transactional;
    int sync_type = args->sync_type;
    int debug = args->debug;
    int wait_for_key = args->wait_for_key;
    int hex_dump = args->hex_dump;
    useconds_t sleep_usecs = args->sleep_usecs;
    int no_print = args->no_print;
    //int group_write = args->group_write;
    char local_store_name[1024];
    char local_store_subset_name[1024];
    char store_subset_name[1024];

    SessionHND        sess_hndl = 0;
    SubsetableHND     sub_hndl = 0;
    eSessionSyncMode  sess_sync_mode = 0;

    char search_key[512];
    strcpy(search_key, key);
    size_t search_key_size = key_size;
    char search_end_key[512];
    strcpy(search_end_key, end_key);
    size_t search_end_key_size = end_key_size;

    char write_key[512];
    strcpy(write_key, key);

    uint32_t i = 0;
    int ret = 0;
    int s_ret = 0;
    memset(local_store_name, '\0', sizeof(local_store_name));
    memset(local_store_subset_name, '\0', sizeof(local_store_subset_name));
    memset(store_subset_name, '\0', sizeof(store_subset_name));
    sprintf(store_subset_name, "%s_subset", store_name);
    SessionHND sub_sess_hndl = 0;
    pid_t tid = gettid();
    printf("cl_n=%s sp_n=%s st_n=%s "
        "prov=%d prov_n=%s key=%s "
        "key_len=%lu end_key=%s end_key_len=%lu "
        "it_dir=%d defau_fst=%d "
        "value=%s value_len=%lu num_ops=%d "
        "num_regres=%d subset_n=%s trans=%d "
        "sync_typ=%d debug=%d no_prnt=%d "
        "hex_dmp=%d "
        "sess_sync_mod=%d tid=%u\n",
        cluster_name, space_name, store_name,
        provider, provider_name, key,
        key_size, end_key, end_key_size,
        it_dir, default_first,
        value, value_size, num_ops,
        num_regressions, snap_back_subset_name, transactional,
        sync_type, debug, no_print,
        hex_dump,
        sess_sync_mode, tid);
    /*
     * @brief
     * Sync mode
     */
    sess_sync_mode = GetSyncMode(sync_type);

    /*
     * @brief
     * Provider
     */
    provider_name = GetProviderName(provider);

    if ( num_ops > MAX_NUM_OPS ) {
        fprintf(stderr, "number of ops too big\n");
        return;
    }

    int r_ret = 0;
    for ( i = 0 ; i < num_regressions ; i++ ) {
        memset(local_store_name, '\0', sizeof(local_store_name));
        sprintf(local_store_name, "%s_%d_%u", store_name, tid, 2);
        memset(local_store_subset_name, '\0', sizeof(local_store_subset_name));
        sprintf(local_store_subset_name, "%s_%d_%u", store_subset_name, tid, 2);
        fprintf(stdout, "Iteration(%u) store(%s) subset(%s)\n", i, local_store_name, local_store_subset_name);

        /*
         * Store
         */
        ret = OpenStore(&sess_hndl,
            cluster_name,
            space_name,
            local_store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
        }

        if ( ret < 0 ) {
            if ( sess_hndl ) {
                SessionClose(sess_hndl);
                sess_hndl = 0;
            }
            ret = CreateStore(&sess_hndl,
                cluster_name,
                space_name,
                local_store_name,
                transactional,
                sess_sync_mode,
                provider_name,
                (SoeBool )debug);
            if ( ret || !sess_hndl ) {
                fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                break;
            }
            fprintf(stderr, "CreateStore(%s) ok\n", local_store_name);
        }

        /*
         * Another store for subset
         */
        ret = OpenStore(&sub_sess_hndl,
            cluster_name,
            space_name,
            local_store_subset_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sub_sess_hndl ) {
            fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_subset_name, SessionLastStatus(sub_sess_hndl));
        }

        if ( ret < 0 ) {
            if ( sub_sess_hndl ) {
                SessionClose(sub_sess_hndl);
                sub_sess_hndl = 0;
            }
            ret = CreateStore(&sub_sess_hndl,
                cluster_name,
                space_name,
                local_store_subset_name,
                transactional,
                sess_sync_mode,
                provider_name,
                (SoeBool )debug);
            if ( ret || !sub_sess_hndl ) {
                fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_subset_name, SessionLastStatus(sub_sess_hndl));
                break;
            }
            fprintf(stderr, "CreateStore(%s) ok\n", local_store_subset_name);
        }

        args->io_counter += 4;

        if ( num_ops == 1 ) {
            if ( !strlen(write_key) ) {
                sprintf(write_key, "%s_%d_%u_%u", "regression_4_key", tid, 0, i);
                key_size = strlen(write_key);
            }
            if ( !strlen(value) ) {
                sprintf(value, "%s_%d_%u_%u", "regression_4_value", tid, 0, i);
                value_size = strlen(value);
            }
            if ( value_size > strlen(value) ) {
                memset(value + strlen(value), 'V', value_size - strlen(value));
            }
            ret = WriteStore(sess_hndl,
                write_key,
                key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "WriteStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            args->io_counter++;
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t j;

            for ( j = 0 ; j < num_ops ; j++ ) {
                key_ptrs[j] = malloc(key_size + 128);
                sprintf(key_ptrs[j], "%d_%s_%u_%u", tid, key, j, i);
                key_sizes[j] = (size_t )strlen(key_ptrs[j]);

                value_ptrs[j] = malloc(value_size + 128);
                sprintf(value_ptrs[j], "%d_%s_%u_%u", tid, value, j, i);
                if ( value_size > strlen(value_ptrs[j]) ) {
                    memset(value_ptrs[j] + strlen(value_ptrs[j]), 'V', value_size - strlen(value_ptrs[j]));
                    *(value_ptrs[j] + value_size) = '\0';
                }
                value_sizes[j] = strlen(value_ptrs[j]);
            }

            ret = WriteStoreN(sess_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "WriteStoreN(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            for ( j = 0 ; j < num_ops ; j++ ) {
                if ( key_ptrs[j] ) {
                    free(key_ptrs[j]);
                }
                if ( value_ptrs[j] ) {
                    free(value_ptrs[j]);
                }
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            args->io_counter += num_ops;

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }
        }

        ret = ReadStore(sess_hndl, search_key, search_key_size, search_end_key, search_end_key_size, it_dir, default_first, debug, hex_dump, no_print, &args->io_counter);
        if ( ret ) {
            fprintf(stderr, "ReadStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            r_ret = 1;
        }

        /*
         * Subset
         */
        s_ret = OpenSubset(sub_sess_hndl,
            &sub_hndl,
            snap_back_subset_name,
            (size_t )strlen(snap_back_subset_name));
        if ( s_ret ) {
            fprintf(stderr, "OpenSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
        }
        if ( s_ret < 0 ) {
            s_ret = CreateSubset(sub_sess_hndl,
                &sub_hndl,
                snap_back_subset_name,
                (size_t )strlen(snap_back_subset_name));
            if ( s_ret ) {
                fprintf(stderr, "CreateSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
                r_ret = 1;
            }
            fprintf(stderr, "CreateSubset(%s) ok\n", snap_back_subset_name);
            r_ret = 0;
        }
        if ( r_ret ) {
            break;
        }

        args->io_counter += 2;

        if ( num_ops == 1 ) {
            if ( !strlen(write_key) ) {
                sprintf(write_key, "%s_%d_%u_%u", "regression_4_key", tid, 0, i);
                key_size = strlen(write_key);
            }
            if ( !strlen(value) ) {
                sprintf(value, "%s_%d_%u_%u", "regression_4_value", tid, 0, i);
                value_size = strlen(value);
            }
            if ( value_size > strlen(value) ) {
                memset(value + strlen(value), 'V', value_size - strlen(value));
            }
            s_ret = WriteSubset(sub_hndl,
                write_key,
                key_size,
                value,
                value_size);
            if ( s_ret ) {
                fprintf(stderr, "WriteSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
                r_ret = 1;
            }

            args->io_counter++;
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t k;

            for ( k = 0 ; k < num_ops ; k++ ) {
                key_ptrs[k] = calloc(1, key_size + 128);
                sprintf(key_ptrs[k], "%d_%s_%u_%u", tid, key, k, i);
                key_sizes[k] = (size_t )strlen(key_ptrs[k]);

                value_ptrs[k] = calloc(1, value_size + 128);
                sprintf(value_ptrs[k], "%d_%s_%u_%u", tid, value, k, i);
                if ( value_size > strlen(value_ptrs[k]) ) {
                    memset(value_ptrs[k] + strlen(value_ptrs[k]), 'V', value_size - strlen(value_ptrs[k]));
                    *(value_ptrs[k] + value_size) = '\0';
                }
                value_sizes[k] = strlen(value_ptrs[k]);
            }

            s_ret = WriteSubsetN(sub_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( s_ret ) {
                fprintf(stderr, "WriteSubsetN(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
                r_ret = 1;
            }
            for ( k = 0 ; k < num_ops ; k++ ) {
                free(key_ptrs[k]);
                free(value_ptrs[k]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            args->io_counter += num_ops;
        }

        ret = ReadSubset(sub_hndl, search_key, search_key_size, search_end_key, search_end_key_size, it_dir, default_first, debug, hex_dump, no_print, &args->io_counter);
        if ( ret ) {
            fprintf(stderr, "ReadSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
            r_ret = 1;
        }

        /*
         * Close subset
         */
        ret = CloseSubset(sub_sess_hndl, sub_hndl);
        if ( ret ) {
            fprintf(stderr, "CloseSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
        }
        sub_hndl = 0;

        /*
         * Close stores
         */
        CloseStore(sess_hndl, wait_for_key);
        sess_hndl = 0;
        CloseStore(sub_sess_hndl, wait_for_key);
        sub_sess_hndl = 0;

        args->io_counter += 2;

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }

        if ( r_ret ) {
            r_ret = 0;
        }
    }

    /*
     *
     */
    if ( sess_hndl ) {
        CloseStore(sess_hndl, wait_for_key);
    }
    if ( sub_sess_hndl ) {
        CloseStore(sub_sess_hndl, wait_for_key);
    }

    printf("RegressionTest1(%u) io_counter=%lu DONE\n", i, args->io_counter);
}

static void RegressionTest2(RegressionArgs *args)
{
    const char *cluster_name = args->cluster_name;
    const char *space_name = args->space_name;
    const char *store_name = args->store_name;
    int provider = args->provider;
    const char *provider_name = 0;
    uint32_t num_regressions = args->num_regressions;
    int transactional = args->transactional;
    int sync_type = args->sync_type;
    //int group_write = args->group_write;
    int debug = args->debug;
    useconds_t sleep_usecs = args->sleep_usecs;
    char local_store_name[1024];

    SessionHND        sess_hndl = 0;
    eSessionSyncMode  sess_sync_mode;

    uint32_t i;
    int ret;
    char store_subset_name[1024];
    sprintf(store_subset_name, "%s_subset", store_name);
    pid_t tid = gettid();

    /*
     * @brief
     * Sync mode
     */
    sess_sync_mode = GetSyncMode(sync_type);

    /*
     * @brief
     * Provider
     */
    provider_name = GetProviderName(provider);

    for ( i = 0 ; i < num_regressions ; i++ ) {
        sprintf(local_store_name, "%s_%d_%u", store_name, tid, 2);
        fprintf(stdout, "Iteration(%u) store(%s)\n", i, local_store_name);

        /*
         * Store
         */
        ret = CreateStore(&sess_hndl,
            cluster_name,
            space_name,
            local_store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            break;
        }
        fprintf(stderr, "CreateStore(%s, %lu) ok\n", local_store_name, sess_hndl);

        /*
         * DestroyStore
         */
        ret = DestroyStore(sess_hndl);
        if ( ret ) {
            fprintf(stderr, "DestroyStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            CloseStore(sess_hndl, 0);
            break;
        }

        /*
         * Close store
         */
        CloseStore(sess_hndl, 0);
        sess_hndl = 0;

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }
    }

    printf("RegressionTest2(%u) DONE\n", i);
}

static void RegressionTest3(RegressionArgs *args)
{
    const char *cluster_name = args->cluster_name;
    const char *space_name = args->space_name;
    const char *store_name = args->store_name;
    int provider = args->provider;
    const char *provider_name = 0;
    uint32_t num_regressions = args->num_regressions;
    int transactional = args->transactional;
    int sync_type = args->sync_type;
    //int group_write = args->group_write;
    int debug = args->debug;
    useconds_t sleep_usecs = args->sleep_usecs;
    char local_store_name[1024];
    char local_subset_name[1024];
    int no_print = args->no_print;
    int hex_dump = args->hex_dump;

    SessionHND        sess_hndl = 0;
    SubsetableHND     sub_hndl = 0;
    eSessionSyncMode  sess_sync_mode;

    uint32_t i;
    int ret, s_ret;
    char store_subset_name[1024];
    sprintf(store_subset_name, "%s_subset", store_name);
    pid_t tid = gettid();

    /*
     * @brief
     * Sync mode
     */
    sess_sync_mode = GetSyncMode(sync_type);

    /*
     * @brief
     * Provider
     */
    provider_name = GetProviderName(provider);


    sprintf(local_store_name, "%s_%d", store_name, tid);

    /*
     * Store
     */
    ret = CreateStore(&sess_hndl,
        cluster_name,
        space_name,
        local_store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
        return;
    }
    fprintf(stderr, "CreateStore(%s,%lu) ok\n", local_store_name, sess_hndl);

    for ( i = 0 ; i < num_regressions ; i++ ) {
        sprintf(local_subset_name, "%s_%u_%d", store_subset_name, tid, 10);
        if ( !no_print ) {
            fprintf(stdout, "Iteration(%u) subset(%s)\n", i, store_subset_name);
        }

        s_ret = CreateSubset(sess_hndl,
            &sub_hndl,
            local_subset_name,
            (size_t )strlen(local_subset_name));
        if ( s_ret ) {
            fprintf(stderr, "CreateSubset(%s) failed(%u)\n", local_subset_name, SessionLastStatus(sess_hndl));
            break;
        }
        if ( !no_print ) {
            fprintf(stderr, "CreateSubset(%s,%lu) ok\n", local_subset_name, sub_hndl);
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }

        int r_ret = ReadSubset(sub_hndl, "", 0, "", 0, 0, 0, debug, hex_dump, no_print, 0);
        if ( r_ret ) {
            fprintf(stderr, "ReadSubset(%s) failed(%u)\n", local_subset_name, SessionLastStatus(sess_hndl));
        }

        s_ret = DestroySubset(sess_hndl, sub_hndl);
        if ( s_ret ) {
            fprintf(stderr, "DestroySubset(%s) failed(%u)\n", local_subset_name, SessionLastStatus(sess_hndl));
            (void )SessionDeleteSubset(sess_hndl, sub_hndl);
            break;
        }

        s_ret = SessionDeleteSubset(sess_hndl, sub_hndl);
        if ( s_ret ) {
            fprintf(stderr, "SessionDeleteSubset(%s) failed(%u)\n", local_subset_name, SessionLastStatus(sess_hndl));
            break;
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }
    }

    /*
     * DestroyStore
     */
    ret = DestroyStore(sess_hndl);
    if ( ret ) {
        fprintf(stderr, "DestroyStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
    }

    /*
     * Close store
     */
    CloseStore(sess_hndl, 0);
    sess_hndl = 0;

    printf("RegressionTest3(%u) DONE\n", i);
}

static void RegressionTest4(RegressionArgs *args)
{
    const char *cluster_name = args->cluster_name;
    const char *space_name = args->space_name;
    const char *store_name = args->store_name;
    int provider = args->provider;
    const char *provider_name = 0;
    const char *key = args->key;
    size_t key_size = args->key_size;
    const char *end_key = args->end_key;
    size_t end_key_size = args->end_key_size;
    int it_dir = args->it_dir;
    int default_first = args->default_first;
    char *value = args->value;
    size_t value_size = args->value_size;
    uint32_t num_ops = args->num_ops;
    uint32_t num_regressions = args->num_regressions;
    const char *snap_back_subset_name = args->snap_back_subset_name;
    //uint32_t snap_back_id = args->snap_back_id;
    int transactional = args->transactional;
    int sync_type = args->sync_type;
    int debug = args->debug;
    int wait_for_key = args->wait_for_key;
    int hex_dump = args->hex_dump;
    useconds_t sleep_usecs = args->sleep_usecs;
    int no_print = args->no_print;
    //int group_write = args->group_write;
    char local_store_name[1024];
    char local_store_subset_name[1024];
    char store_subset_name[1024];

    SessionHND        sess_hndl = 0;
    SubsetableHND     sub_hndl = 0;
    eSessionSyncMode  sess_sync_mode = 0;
    pid_t tid = gettid();

    char search_key[512];
    strcpy(search_key, key);
    size_t search_key_size = key_size;
    char search_end_key[512];
    strcpy(search_end_key, end_key);
    size_t search_end_key_size = end_key_size;

    char write_key[512];
    strcpy(write_key, key);

    uint32_t i = 0;
    int ret = 0;
    int s_ret = 0;
    memset(local_store_name, '\0', sizeof(local_store_name));
    memset(local_store_subset_name, '\0', sizeof(local_store_subset_name));
    memset(store_subset_name, '\0', sizeof(store_subset_name));
    sprintf(store_subset_name, "%s_subset", store_name);
    SessionHND sub_sess_hndl = 0;
    printf("cl_n=%s sp_n=%s st_n=%s "
        "prov=%d prov_nam=%s key=%s "
        "key_len=%lu end_key=%s end_key_len=%lu "
        "it_dir=%d defau_frst=%d "
        "value=%s value_len=%lu num_ops=%d "
        "num_regs=%d subset_n=%s trans=%d "
        "sync_typ=%d debug=%d no_print=%d "
        "hex_dmp=%d "
        "sess_sync_mod=%d\n",
        cluster_name, space_name, store_name,
        provider, provider_name, key,
        key_size, end_key, end_key_size,
        it_dir, default_first,
        value, value_size, num_ops,
        num_regressions, snap_back_subset_name, transactional,
        sync_type, debug, no_print,
        hex_dump,
        sess_sync_mode);
    /*
     * @brief
     * Sync mode
     */
    sess_sync_mode = GetSyncMode(sync_type);

    /*
     * @brief
     * Provider
     */
    provider_name = GetProviderName(provider);

    if ( num_ops > MAX_NUM_OPS ) {
        fprintf(stderr, "number of ops too big\n");
        return;
    }

    memset(local_store_name, '\0', sizeof(local_store_name));
    sprintf(local_store_name, "%s_%d", store_name, 3);
    memset(local_store_subset_name, '\0', sizeof(local_store_subset_name));
    sprintf(local_store_subset_name, "%s_%d", store_subset_name, 3);

    /*
     * Store
     */
    ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        local_store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
    }

    if ( ret < 0 ) {
        if ( sess_hndl ) {
            SessionClose(sess_hndl);
            sess_hndl = 0;
        }
        ret = CreateStore(&sess_hndl,
            cluster_name,
            space_name,
            local_store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            return;
        }
        fprintf(stderr, "CreateStore(%s) ok\n", local_store_name);
    }

    /*
     * Another store for subset
     */
    ret = OpenStore(&sub_sess_hndl,
        cluster_name,
        space_name,
        local_store_subset_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sub_sess_hndl ) {
        fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_subset_name, SessionLastStatus(sub_sess_hndl));
    }

    if ( ret < 0 ) {
        if ( sub_sess_hndl ) {
            SessionClose(sub_sess_hndl);
            sub_sess_hndl = 0;
        }
        ret = CreateStore(&sub_sess_hndl,
            cluster_name,
            space_name,
            local_store_subset_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sub_sess_hndl ) {
            fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_subset_name, SessionLastStatus(sub_sess_hndl));
            return;
        }
        fprintf(stderr, "CreateStore(%s) ok\n", local_store_subset_name);
    }

    /*
     * Subset
     */
    s_ret = OpenSubset(sub_sess_hndl,
        &sub_hndl,
        snap_back_subset_name,
        (size_t )strlen(snap_back_subset_name));
    if ( s_ret ) {
        fprintf(stderr, "OpenSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
    }

    if ( s_ret < 0 ) {
        s_ret = CreateSubset(sub_sess_hndl,
            &sub_hndl,
            snap_back_subset_name,
            (size_t )strlen(snap_back_subset_name));
        if ( s_ret ) {
            fprintf(stderr, "CreateSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
        }
        fprintf(stderr, "CreateSubset(%s) ok\n", snap_back_subset_name);
    }

    int r_ret = 0;
    for ( i = 0 ; i < num_regressions ; i++ ) {
        fprintf(stdout, "Iteration(%u) store(%s) subset_store(%s) subset(%s)\n", i, local_store_name, local_store_subset_name, snap_back_subset_name);

        if ( num_ops == 1 ) {
            if ( !strlen(write_key) ) {
                sprintf(write_key, "%s_%d_%u_%u", "regression_4_key", tid, 0, i);
                key_size = strlen(write_key);
            }
            if ( !strlen(value) ) {
                sprintf(value, "%s_%d_%u_%u", "regression_4_value", tid, 0, i);
                value_size = strlen(value);
            }
            if ( value_size > strlen(value) ) {
                memset(value + strlen(value), 'V', value_size - strlen(value));
            }
            ret = WriteStore(sess_hndl,
                write_key,
                key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "WriteStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t j;

            for ( j = 0 ; j < num_ops ; j++ ) {
                key_ptrs[j] = malloc(128);
                memset(key_ptrs[j], '\0', 128);
                sprintf(key_ptrs[j], "%d_%s_%u_%u", tid, "regression_4_key", j, i);
                key_sizes[j] = (size_t )strlen(key_ptrs[j]);

                value_ptrs[j] = malloc(value_size + 128);
                memset(value_ptrs[j], '\0', value_size + 128);
                sprintf(value_ptrs[j], "%d_%s_%u_%u", tid, "regression_4_value", j, i);
                if ( value_size > strlen(value_ptrs[j]) ) {
                    memset(value_ptrs[j] + strlen(value_ptrs[j]), 'V', value_size - strlen(value_ptrs[j]));
                    *(value_ptrs[j] + value_size) = '\0';
                }
                value_sizes[j] = strlen(value_ptrs[j]);
            }

            ret = WriteStoreN(sess_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "WriteStoreN(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            for ( j = 0 ; j < num_ops ; j++ ) {
                free(key_ptrs[j]);
                free(value_ptrs[j]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }
        }

        ret = ReadStore(sess_hndl, search_key, search_key_size, search_end_key, search_end_key_size, it_dir, default_first, debug, hex_dump, no_print, 0);
        if ( ret ) {
            fprintf(stderr, "ReadStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            r_ret = 1;
        }

        if ( num_ops == 1 ) {
            if ( !strlen(write_key) ) {
                sprintf(write_key, "%s_%d_%u_%u", "subset_regression_4_key", tid, 0, i);
                key_size = strlen(write_key);
            }
            if ( !strlen(value) ) {
                sprintf(value, "%s_%d_%u_%u", "subset_regression_4_value", tid, 0, i);
                value_size = strlen(value);
            }
            if ( value_size > strlen(value) ) {
                memset(value + strlen(value), 'S', value_size - strlen(value));
            }
            s_ret = WriteSubset(sub_hndl,
                write_key,
                key_size,
                value,
                value_size);
            if ( s_ret ) {
                fprintf(stderr, "WriteSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t k;

            for ( k = 0 ; k < num_ops ; k++ ) {
                key_ptrs[k] = malloc(512);
                memset(key_ptrs[k], '\0', 512);
                sprintf(key_ptrs[k], "%s_%d_%u_%u", "subset_regression_4_key", tid, k, i);
                key_sizes[k] = (size_t )strlen(key_ptrs[k]);

                value_ptrs[k] = malloc(value_size + 128);
                memset(value_ptrs[k], '\0', value_size + 128);
                sprintf(value_ptrs[k], "%s_%d_%u_%u", "subset_regression_4_value", tid, k, i);
                if ( value_size > strlen(value_ptrs[k]) ) {
                    memset(value_ptrs[k] + strlen(value_ptrs[k]), 'S', value_size - strlen(value_ptrs[k]));
                    *(value_ptrs[k] + value_size) = '\0';
                }
                value_sizes[k] = strlen(value_ptrs[k]);
            }

            s_ret = WriteSubsetN(sub_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( s_ret ) {
                fprintf(stderr, "WriteSubsetN(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
                r_ret = 1;
            }
            for ( k = 0 ; k < num_ops ; k++ ) {
                free(key_ptrs[k]);
                free(value_ptrs[k]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);
        }

        ret = ReadSubset(sub_hndl, search_key, search_key_size, search_end_key, search_end_key_size, it_dir, default_first, debug, hex_dump, no_print, 0);
        if ( ret ) {
            fprintf(stderr, "ReadSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
            r_ret = 1;
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }

        if ( r_ret ) {
            r_ret = 0;
        }
    }

    /*
     * Close subset
     */
    ret = CloseSubset(sub_sess_hndl, sub_hndl);
    if ( ret ) {
        fprintf(stderr, "CloseSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sess_hndl));
    }
    sub_hndl = 0;

    /*
     * Close stores
     */
    CloseStore(sess_hndl, wait_for_key);
    sess_hndl = 0;

    CloseStore(sub_sess_hndl, wait_for_key);
    sub_sess_hndl = 0;

    printf("RegressionTest4(%u) DONE\n", i);
}

static void RegressionTest5(RegressionArgs *args)
{
    const char *cluster_name = args->cluster_name;
    const char *space_name = args->space_name;
    const char *store_name = args->store_name;
    int provider = args->provider;
    const char *provider_name = 0;
    const char *key = args->key;
    size_t key_size = args->key_size;
    const char *end_key = args->end_key;
    size_t end_key_size = args->end_key_size;
    int it_dir = args->it_dir;
    int default_first = args->default_first;
    char *value = args->value;
    size_t value_size = args->value_size;
    uint32_t num_ops = args->num_ops;
    uint32_t num_regressions = args->num_regressions;
    const char *snap_back_subset_name = args->snap_back_subset_name;
    //uint32_t snap_back_id = args->snap_back_id;
    int transactional = args->transactional;
    int sync_type = args->sync_type;
    int debug = args->debug;
    int wait_for_key = args->wait_for_key;
    int hex_dump = args->hex_dump;
    useconds_t sleep_usecs = args->sleep_usecs;
    int no_print = args->no_print;
    //int group_write = args->group_write;
    char local_store_name[1024];

    SessionHND        sess_hndl = 0;
    eSessionSyncMode  sess_sync_mode = 0;
    pid_t tid = gettid();

    uint32_t i = 0;
    int ret = 0;
    memset(local_store_name, '\0', sizeof(local_store_name));
    printf("cl_n=%s sp_n=%s st_n=%s "
        "prov=%d prov_name=%s key=%s "
        "key_len=%lu end_key=%s end_key_len=%lu "
        "it_dir=%d "
        "value=%s value_len=%lu num_ops=%d "
        "num_regres=%d subset_name=%s trans=%d "
        "sync_type=%d debug=%d wait_for_key=%d "
        "hex_dump=%d "
        "sess_sync_mode=%d\n",
        cluster_name, space_name, store_name,
        provider, provider_name, key,
        key_size, end_key, end_key_size,
        it_dir,
        value, value_size, num_ops,
        num_regressions, snap_back_subset_name, transactional,
        sync_type, debug, wait_for_key,
        hex_dump,
        sess_sync_mode);

    /*
     * @brief
     * Sync mode
     */
    sess_sync_mode = GetSyncMode(sync_type);

    /*
     * @brief
     * Provider
     */
    provider_name = GetProviderName(provider);

    if ( num_ops > MAX_NUM_OPS ) {
        fprintf(stderr, "number of ops too big\n");
        return;
    }

    memset(local_store_name, '\0', sizeof(local_store_name));
    sprintf(local_store_name, "%s", store_name);

    /*
     * Store
     */
    ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        local_store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
    }

    if ( ret < 0 ) {
        if ( sess_hndl ) {
            SessionClose(sess_hndl);
            sess_hndl = 0;
        }
        ret = CreateStore(&sess_hndl,
            cluster_name,
            space_name,
            local_store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            return;
        }
        fprintf(stderr, "CreateStore(%s) ok\n", local_store_name);
    }

    int r_ret = 0;
    for ( i = 0 ; i < num_regressions ; i++ ) {
        fprintf(stdout, "Iteration(%u) store(%s)\n", i, local_store_name);

        if ( num_ops == 1 ) {
            char new_key[10000];
            sprintf(new_key, "%s_%s_%u", "regression10_key", key, i);
            size_t new_key_size = (size_t )strlen(new_key);

            ret = WriteStore(sess_hndl,
                new_key,
                new_key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "WriteStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t j;

            for ( j = 0 ; j < num_ops ; j++ ) {
                key_ptrs[j] = malloc(key_size + 128);
                sprintf(key_ptrs[j], "%s_%d_%u_%u", "regression5_key", tid, j, i);
                key_sizes[j] = (size_t )strlen(key_ptrs[j]);

                value_ptrs[j] = malloc(value_size + 128);
                sprintf(value_ptrs[j], "%s_%d_%u_%u", "regression5_value", tid, j, i);
                if ( value_size > strlen(value_ptrs[j]) ) {
                    memset(value_ptrs[j] + strlen(value_ptrs[j]), 'V', value_size - strlen(value_ptrs[j]));
                }
                value_sizes[j] = value_size;
            }

            ret = WriteStoreN(sess_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "WriteStoreN(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            for ( j = 0 ; j < num_ops ; j++ ) {
                free(key_ptrs[j]);
                free(value_ptrs[j]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }
        }

        ret = ReadStore(sess_hndl, key, key_size, end_key, end_key_size, it_dir, default_first, debug, hex_dump, no_print, 0);
        if ( ret ) {
            fprintf(stderr, "ReadStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            r_ret = 1;
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }

        if ( r_ret ) {
            r_ret = 0;
        }
    }

    /*
     * Close stores
     */
    CloseStore(sess_hndl, wait_for_key);
    sess_hndl = 0;

    printf("RegressionTest5(%u) DONE\n", i);
}

static void RegressionTest6(RegressionArgs *args)
{
    const char *cluster_name = args->cluster_name;
    const char *space_name = args->space_name;
    const char *store_name = args->store_name;
    int provider = args->provider;
    const char *provider_name = 0;
    const char *key = args->key;
    size_t key_size = args->key_size;
    const char *end_key = args->end_key;
    size_t end_key_size = args->end_key_size;
    int it_dir = args->it_dir;
    int default_first = args->default_first;
    char *value = args->value;
    size_t value_size = args->value_size;
    uint32_t num_ops = args->num_ops;
    uint32_t num_regressions = args->num_regressions;
    const char *snap_back_subset_name = args->snap_back_subset_name;
    //uint32_t snap_back_id = args->snap_back_id;
    int transactional = args->transactional;
    int sync_type = args->sync_type;
    int debug = args->debug;
    int wait_for_key = args->wait_for_key;
    int hex_dump = args->hex_dump;
    useconds_t sleep_usecs = args->sleep_usecs;
    int no_print = args->no_print;
    //int group_write = args->group_write;
    char local_store_name[1024];
    char local_store_subset_name[1024];
    char store_subset_name[1024];

    SessionHND        sess_hndl = 0;
    SubsetableHND     sub_hndl = 0;
    eSessionSyncMode  sess_sync_mode = 0;

    /* Intializes random number generator */
    time_t t;
    srand((unsigned int)time(&t));

    uint32_t i = 0;
    int ret = 0;
    int s_ret = 0;
    memset(local_store_name, '\0', sizeof(local_store_name));
    memset(local_store_subset_name, '\0', sizeof(local_store_subset_name));
    memset(store_subset_name, '\0', sizeof(store_subset_name));
    sprintf(store_subset_name, "%s_subset", store_name);
    SessionHND sub_sess_hndl = 0;
    pid_t tid = gettid();
    printf("cl_n=%s sp_n=%s st_n=%s "
        "prov=%d prov_n=%s key=%s "
        "key_len=%lu end_key=%s end_key_len=%lu "
        "it_dir=%d defau_fst=%d "
        "value=%s value_len=%lu num_ops=%d "
        "num_regres=%d subset_n=%s trans=%d "
        "sync_typ=%d debug=%d no_prnt=%d "
        "hex_dmp=%d "
        "sess_sync_mod=%d tid=%u\n",
        cluster_name, space_name, store_name,
        provider, provider_name, key,
        key_size, end_key, end_key_size,
        it_dir, default_first,
        value, value_size, num_ops,
        num_regressions, snap_back_subset_name, transactional,
        sync_type, debug, no_print,
        hex_dump,
        sess_sync_mode, tid);
    /*
     * @brief
     * Sync mode
     */
    sess_sync_mode = GetSyncMode(sync_type);

    /*
     * @brief
     * Provider
     */
    provider_name = GetProviderName(provider);

    if ( num_ops > MAX_NUM_OPS ) {
        fprintf(stderr, "number of ops too big\n");
        return;
    }


    memset(local_store_name, '\0', sizeof(local_store_name));
    sprintf(local_store_name, "%s_%d_%u", store_name, tid, 2);
    memset(local_store_subset_name, '\0', sizeof(local_store_subset_name));
    sprintf(local_store_subset_name, "%s_%d_%u", store_subset_name, tid, 2);
    fprintf(stdout, "Iteration(%u) store(%s) subset(%s)\n", i, local_store_name, local_store_subset_name);

    /*
     * Store
     */
    ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        local_store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
    }

    if ( ret < 0 ) {
        if ( sess_hndl ) {
            SessionClose(sess_hndl);
            sess_hndl = 0;
        }
        ret = CreateStore(&sess_hndl,
            cluster_name,
            space_name,
            local_store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            return;
        }
        fprintf(stderr, "CreateStore(%s) ok\n", local_store_name);
    }

    /*
     * Another store for subset
     */
    ret = OpenStore(&sub_sess_hndl,
        cluster_name,
        space_name,
        local_store_subset_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sub_sess_hndl ) {
        fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_subset_name, SessionLastStatus(sub_sess_hndl));
    }

    if ( ret < 0 ) {
        if ( sub_sess_hndl ) {
            SessionClose(sub_sess_hndl);
            sub_sess_hndl = 0;
        }
        ret = CreateStore(&sub_sess_hndl,
            cluster_name,
            space_name,
            local_store_subset_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sub_sess_hndl ) {
            fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_subset_name, SessionLastStatus(sub_sess_hndl));
            CloseStore(sess_hndl, wait_for_key);
            return;
        }
        fprintf(stderr, "CreateStore(%s) ok\n", local_store_subset_name);
    }

    args->io_counter += 4;

    /*
     * Subset
     */
    s_ret = OpenSubset(sub_sess_hndl,
        &sub_hndl,
        snap_back_subset_name,
        (size_t )strlen(snap_back_subset_name));
    if ( s_ret ) {
        fprintf(stderr, "OpenSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
    }
    if ( s_ret < 0 ) {
        s_ret = CreateSubset(sub_sess_hndl,
            &sub_hndl,
            snap_back_subset_name,
            (size_t )strlen(snap_back_subset_name));
        if ( s_ret ) {
            fprintf(stderr, "CreateSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
            CloseStore(sess_hndl, wait_for_key);
            CloseStore(sub_sess_hndl, wait_for_key);
            return;
        }
        fprintf(stderr, "CreateSubset(%s) ok\n", snap_back_subset_name);
    }

    args->io_counter += 2;

    int r_ret = 0;
    for ( i = 0 ; i < num_regressions ; i++ ) {
        if ( num_ops == 1 ) {
            if ( value_size > strlen(value) ) {
                memset(value + strlen(value), 'V', value_size - strlen(value));
            }
            ret = WriteStore(sess_hndl,
                key,
                key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "WriteStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            args->io_counter++;
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t j;

            for ( j = 0 ; j < num_ops ; j++ ) {
                key_ptrs[j] = malloc(key_size + 128);
                sprintf(key_ptrs[j], "%d_%d_%s_%u_%u", rand(), tid, key, j, i);
                key_sizes[j] = (size_t )strlen(key_ptrs[j]);

                value_ptrs[j] = malloc(value_size + 128);
                sprintf(value_ptrs[j], "%d_%s_%u_%u", tid, value, j, i);
                if ( value_size > strlen(value_ptrs[j]) ) {
                    memset(value_ptrs[j] + strlen(value_ptrs[j]), 'V', value_size - strlen(value_ptrs[j]));
                }
                value_sizes[j] = value_size;
            }

            ret = WriteStoreN(sess_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "WriteStoreN(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            for ( j = 0 ; j < num_ops ; j++ ) {
                free(key_ptrs[j]);
                free(value_ptrs[j]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            args->io_counter += num_ops;

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }
        }

        ret = ReadStore(sess_hndl, 0, 0, 0, 0, it_dir, default_first, debug, hex_dump, no_print, &args->io_counter);
        if ( ret ) {
            fprintf(stderr, "ReadStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            r_ret = 1;
        }

        if ( num_ops == 1 ) {
            if ( value_size > strlen(value) ) {
                memset(value + strlen(value), 'S', value_size - strlen(value));
            }
            s_ret = WriteSubset(sub_hndl,
                key,
                key_size,
                value,
                value_size);
            if ( s_ret ) {
                fprintf(stderr, "WriteSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
                r_ret = 1;
            }

            args->io_counter++;
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t k;

            for ( k = 0 ; k < num_ops ; k++ ) {
                key_ptrs[k] = malloc(key_size + 128);
                sprintf(key_ptrs[k], "%d_%d_%s_%u_%u", rand(), tid, key, k, i);
                key_sizes[k] = (size_t )strlen(key_ptrs[k]);

                value_ptrs[k] = malloc(value_size + 128);
                sprintf(value_ptrs[k], "%d_%s_%u_%u", tid, value, k, i);
                if ( value_size > strlen(value_ptrs[k]) ) {
                    memset(value_ptrs[k] + strlen(value_ptrs[k]), 'S', value_size - strlen(value_ptrs[k]));
                }
                value_sizes[k] = value_size;
            }

            s_ret = WriteSubsetN(sub_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( s_ret ) {
                fprintf(stderr, "WriteSubsetN(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
                r_ret = 1;
            }
            for ( k = 0 ; k < num_ops ; k++ ) {
                free(key_ptrs[k]);
                free(value_ptrs[k]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            args->io_counter += num_ops;
        }

        ret = ReadSubset(sub_hndl, 0, 0, 0, 0, it_dir, default_first, debug, hex_dump, no_print, &args->io_counter);
        if ( ret ) {
            fprintf(stderr, "ReadSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
            r_ret = 1;
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }
    }

    /*
     *
     */
    /*
     * Close subset
     */
    ret = CloseSubset(sub_sess_hndl, sub_hndl);
    if ( ret ) {
        fprintf(stderr, "CloseSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
    }
    sub_hndl = 0;

    /*
     * Close stores
     */
    CloseStore(sess_hndl, wait_for_key);
    sess_hndl = 0;
    CloseStore(sub_sess_hndl, wait_for_key);
    sub_sess_hndl = 0;

    args->io_counter += 2;

    printf("RegressionTest6(%u) io_counter=%lu DONE\n", i, args->io_counter);
}

static void RegressionTest7(RegressionArgs *args)
{
    const char *key = args->key;
    size_t key_size = args->key_size;
    const char *end_key = args->end_key;
    size_t end_key_size = args->end_key_size;
    int it_dir = args->it_dir;
    int default_first = args->default_first;
    char *value = args->value;
    size_t value_size = args->value_size;
    uint32_t num_ops = args->num_ops;
    uint32_t num_regressions = args->num_regressions;
    const char *snap_back_subset_name = args->snap_back_subset_name;
    //uint32_t snap_back_id = args->snap_back_id;
    int transactional = args->transactional;
    int sync_type = args->sync_type;
    int debug = args->debug;
    int hex_dump = args->hex_dump;
    useconds_t sleep_usecs = args->sleep_usecs;
    int no_print = args->no_print;

    SessionHND        sess_hndl = args->sess_hndl;
    SubsetableHND     sub_hndl = args->sub_hndl;

    /* Intializes random number generator */
    time_t t;
    srand((unsigned int)time(&t));

    uint32_t i = 0;
    int ret = 0;
    int s_ret = 0;
    pid_t tid = gettid();
    printf("key=%s "
        "key_len=%lu end_key=%s end_key_len=%lu "
        "it_dir=%d defau_fst=%d "
        "value=%s value_len=%lu num_ops=%d "
        "num_regres=%d subset_n=%s trans=%d "
        "sync_typ=%d debug=%d no_prnt=%d "
        "hex_dmp=%d "
        "tid=%u\n",
        key, key_size, end_key, end_key_size,
        it_dir, default_first,
        value, value_size, num_ops,
        num_regressions, snap_back_subset_name, transactional,
        sync_type, debug, no_print,
        hex_dump, tid);

    if ( num_ops > MAX_NUM_OPS ) {
        fprintf(stderr, "number of ops too big\n");
        return;
    }

    args->io_counter += 2;

    int r_ret = 0;
    for ( i = 0 ; i < num_regressions ; i++ ) {
        if ( num_ops == 1 ) {
            if ( value_size > strlen(value) ) {
                memset(value + strlen(value), 'V', value_size - strlen(value));
            }
            ret = WriteStore(sess_hndl,
                key,
                key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "WriteStore(%s) failed(%u)\n", args->store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            args->io_counter++;
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t j;

            for ( j = 0 ; j < num_ops ; j++ ) {
                key_ptrs[j] = malloc(key_size + 128);
                sprintf(key_ptrs[j], "%d_%d_%s_%u_%u", rand(), tid, key, j, i);
                key_sizes[j] = (size_t )strlen(key_ptrs[j]);

                value_ptrs[j] = malloc(value_size + 128);
                sprintf(value_ptrs[j], "%d_%s_%u_%u", tid, value, j, i);
                if ( value_size > strlen(value_ptrs[j]) ) {
                    memset(value_ptrs[j] + strlen(value_ptrs[j]), 'V', value_size - strlen(value_ptrs[j]));
                }
                value_sizes[j] = value_size;
            }

            ret = WriteStoreN(sess_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "WriteStoreN(%s) failed(%u)\n", args->store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            for ( j = 0 ; j < num_ops ; j++ ) {
                free(key_ptrs[j]);
                free(value_ptrs[j]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            args->io_counter += num_ops;

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }
        }

        ret = ReadStore(sess_hndl, 0, 0, 0, 0, it_dir, default_first, debug, hex_dump, no_print, &args->io_counter);
        if ( ret ) {
            fprintf(stderr, "ReadStore(%s) failed(%u)\n", args->store_name, SessionLastStatus(sess_hndl));
            r_ret = 1;
        }

        if ( num_ops == 1 ) {
            if ( value_size > strlen(value) ) {
                memset(value + strlen(value), 'S', value_size - strlen(value));
            }
            s_ret = WriteSubset(sub_hndl,
                key,
                key_size,
                value,
                value_size);
            if ( s_ret ) {
                fprintf(stderr, "WriteSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            args->io_counter++;
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t k;

            for ( k = 0 ; k < num_ops ; k++ ) {
                key_ptrs[k] = malloc(key_size + 128);
                sprintf(key_ptrs[k], "%d_%d_%s_%u_%u", rand(), tid, key, k, i);
                key_sizes[k] = (size_t )strlen(key_ptrs[k]);

                value_ptrs[k] = malloc(value_size + 128);
                sprintf(value_ptrs[k], "%d_%s_%u_%u", tid, value, k, i);
                if ( value_size > strlen(value_ptrs[k]) ) {
                    memset(value_ptrs[k] + strlen(value_ptrs[k]), 'S', value_size - strlen(value_ptrs[k]));
                }
                value_sizes[k] = value_size;
            }

            s_ret = WriteSubsetN(sub_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( s_ret ) {
                fprintf(stderr, "WriteSubsetN(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }
            for ( k = 0 ; k < num_ops ; k++ ) {
                free(key_ptrs[k]);
                free(value_ptrs[k]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            args->io_counter += num_ops;
        }

        ret = ReadSubset(sub_hndl, 0, 0, 0, 0, it_dir, default_first, debug, hex_dump, no_print, &args->io_counter);
        if ( ret ) {
            fprintf(stderr, "ReadSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sess_hndl));
            r_ret = 1;
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }

        SessionStatus s_ret = SessionDeleteSubset(sess_hndl, sub_hndl);
        if ( s_ret ) {
            fprintf(stderr, "SessionDeleteSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sess_hndl));
        }
        sub_hndl = 0;

        for ( ;; ) {
            s_ret = OpenSubset(sess_hndl,
                &sub_hndl,
                snap_back_subset_name,
                (size_t )strlen(snap_back_subset_name));
            if ( s_ret ) {
                fprintf(stderr, "OpenSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sess_hndl));
                sleep(1);
            } else {
                fprintf(stderr, "OpenSubset(%s) reopen ok sub_hndl(%lu)\n", snap_back_subset_name, sess_hndl);
                break;
            }
        }
    }

    args->io_counter += 2;

    printf("RegressionTest7(%u) io_counter=%lu DONE\n", i, args->io_counter);
}

static void RegressionTest9(RegressionArgs *args)
{
    const char *key = args->key;
    size_t key_size = args->key_size;
    const char *end_key = args->end_key;
    size_t end_key_size = args->end_key_size;
    const char *cluster_name = args->cluster_name;
    const char *space_name = args->space_name;
    const char *store_name = args->store_name;
    int provider = args->provider;
    const char *provider_name = 0;
    char *value = args->value;
    size_t value_size = args->value_size;
    uint32_t num_regressions = args->num_regressions;
    uint32_t num_ops = args->num_ops;
    const char *snap_back_subset_name = args->snap_back_subset_name;
    int transactional = args->transactional;
    int sync_type = args->sync_type;
    int it_dir = args->it_dir;
    int default_first = args->default_first;
    //int group_write = args->group_write;
    int hex_dump = args->hex_dump;
    useconds_t sleep_usecs = args->sleep_usecs;
    int no_print = args->no_print;
    int debug = args->debug;
    char local_store_name[1024];
    char local_subset_name[1024];

    SessionHND        sess_hndl = 0;
    SubsetableHND     sub_hndl = 0;
    eSessionSyncMode  sess_sync_mode;

    uint32_t i;
    int ret, s_ret;
    char store_subset_name[1024];
    sprintf(store_subset_name, "%s_subset", store_name);
    pid_t tid = gettid();

    /*
     * @brief
     * Sync mode
     */
    sess_sync_mode = GetSyncMode(sync_type);

    /*
     * @brief
     * Provider
     */
    provider_name = GetProviderName(provider);


    sprintf(local_store_name, "%s", store_name);

    /*
     * Store
     */
    ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        local_store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
    }

    if ( ret < 0 ) {
        if ( sess_hndl ) {
            SessionClose(sess_hndl);
            sess_hndl = 0;
        }
        ret = CreateStore(&sess_hndl,
            cluster_name,
            space_name,
            local_store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            return;
        }
        fprintf(stderr, "CreateStore(%s) ok\n", local_store_name);
    }
    fprintf(stderr, "CreateStore(%s,%lu) ok\n", local_store_name, sess_hndl);

    int r_ret = 0;
    for ( i = 0 ; i < num_regressions ; i++ ) {
        sprintf(local_subset_name, "%s_%u_%d", snap_back_subset_name, tid, i);
        fprintf(stdout, "Iteration(%u/%u) subset(%s)\n", i, tid, local_subset_name);

        s_ret = CreateSubset(sess_hndl,
            &sub_hndl,
            local_subset_name,
            (size_t )strlen(local_subset_name));
        if ( s_ret ) {
            fprintf(stderr, "CreateSubset(%s) failed(%u)\n", store_subset_name, SessionLastStatus(sess_hndl));
            break;
        }
        fprintf(stderr, "CreateSubset(%s,%lu) ok\n", local_subset_name, sub_hndl);

        if ( num_ops == 1 ) {
            if ( value_size > strlen(value) ) {
                memset(value + strlen(value), 'S', value_size - strlen(value));
            }
            s_ret = WriteSubset(sub_hndl,
                key,
                key_size,
                value,
                value_size);
            if ( s_ret ) {
                fprintf(stderr, "WriteSubset(%s) failed(%u)\n", local_subset_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            args->io_counter++;
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t k;

            for ( k = 0 ; k < num_ops ; k++ ) {
                key_ptrs[k] = malloc(key_size + 128);
                sprintf(key_ptrs[k], "%d_%d_%s_%u_%u", rand(), tid, key, k, i);
                key_sizes[k] = (size_t )strlen(key_ptrs[k]);

                value_ptrs[k] = malloc(value_size + 128);
                sprintf(value_ptrs[k], "%d_%s_%u_%u", tid, value, k, i);
                if ( value_size > strlen(value_ptrs[k]) ) {
                    memset(value_ptrs[k] + strlen(value_ptrs[k]), 'S', value_size - strlen(value_ptrs[k]));
                }
                value_sizes[k] = value_size;
            }

            s_ret = WriteSubsetN(sub_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( s_ret ) {
                fprintf(stderr, "WriteSubsetN(%s) failed(%u)\n", local_subset_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }
            for ( k = 0 ; k < num_ops ; k++ ) {
                free(key_ptrs[k]);
                free(value_ptrs[k]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            args->io_counter += num_ops;
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }

        ret = ReadSubset(sub_hndl, key, key_size, end_key, end_key_size, it_dir, default_first, debug, hex_dump, no_print, &args->io_counter);
        if ( ret ) {
            fprintf(stderr, "ReadSubset(%s) failed(%u)\n", local_subset_name, SessionLastStatus(sess_hndl));
            r_ret = 1;
        }

        s_ret = DestroySubset(sess_hndl, sub_hndl);
        if ( s_ret ) {
            fprintf(stderr, "DestroySubset(%s) failed(%u)\n", local_subset_name, SessionLastStatus(sess_hndl));
            (void )SessionDeleteSubset(sess_hndl, sub_hndl);
            break;
        }

        s_ret = SessionDeleteSubset(sess_hndl, sub_hndl);
        if ( s_ret ) {
            fprintf(stderr, "SessionDeleteSubset(%s) failed(%u)\n", local_subset_name, SessionLastStatus(sess_hndl));
            break;
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }
    }

    /*
     * Close store
     */
    CloseStore(sess_hndl, 0);
    sess_hndl = 0;

    printf("RegressionTest9(%u) DONE\n", i);
}

static void RegressionTest10(RegressionArgs *args)
{
    const char *cluster_name = args->cluster_name;
    const char *space_name = args->space_name;
    const char *store_name = args->store_name;
    int provider = args->provider;
    const char *provider_name = 0;
    const char *key = args->key;
    size_t key_size = args->key_size;
    const char *end_key = args->end_key;
    size_t end_key_size = args->end_key_size;
    int it_dir = args->it_dir;
    int default_first = args->default_first;
    char *value = args->value;
    size_t value_size = args->value_size;
    uint32_t num_ops = args->num_ops;
    uint32_t num_regressions = args->num_regressions;
    const char *snap_back_subset_name = args->snap_back_subset_name;
    //uint32_t snap_back_id = args->snap_back_id;
    int transactional = args->transactional;
    int sync_type = args->sync_type;
    int debug = args->debug;
    int wait_for_key = args->wait_for_key;
    int hex_dump = args->hex_dump;
    useconds_t sleep_usecs = args->sleep_usecs;
    int no_print = args->no_print;
    //int group_write = args->group_write;
    char local_store_name[1024];

    SessionHND        sess_hndl = 0;
    eSessionSyncMode  sess_sync_mode = 0;
    pid_t tid = gettid();

    uint32_t i = 0;
    int ret = 0;
    memset(local_store_name, '\0', sizeof(local_store_name));
    printf("cl_n=%s sp_n=%s st_n=%s "
        "prov=%d prov_name=%s key=%s "
        "key_len=%lu end_key=%s end_key_len=%lu "
        "it_dir=%d "
        "value=%s value_len=%lu num_ops=%d "
        "num_regres=%d subset_name=%s trans=%d "
        "sync_type=%d debug=%d wait_for_key=%d "
        "hex_dump=%d "
        "sess_sync_mode=%d\n",
        cluster_name, space_name, store_name,
        provider, provider_name, key,
        key_size, end_key, end_key_size,
        it_dir,
        value, value_size, num_ops,
        num_regressions, snap_back_subset_name, transactional,
        sync_type, debug, wait_for_key,
        hex_dump,
        sess_sync_mode);

    /*
     * @brief
     * Sync mode
     */
    sess_sync_mode = GetSyncMode(sync_type);

    /*
     * @brief
     * Provider
     */
    provider_name = GetProviderName(provider);

    if ( num_ops > MAX_NUM_OPS ) {
        fprintf(stderr, "number of ops too big\n");
        return;
    }

    memset(local_store_name, '\0', sizeof(local_store_name));
    sprintf(local_store_name, "%s", store_name);

    /*
     * Store
     */
    ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        local_store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
    }

    if ( ret < 0 ) {
        if ( sess_hndl ) {
            SessionClose(sess_hndl);
            sess_hndl = 0;
        }
        ret = CreateStore(&sess_hndl,
            cluster_name,
            space_name,
            local_store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            return;
        }
        fprintf(stderr, "CreateStore(%s) ok\n", local_store_name);
    }

    int r_ret = 0;
    for ( i = 0 ; i < num_regressions ; i++ ) {
        fprintf(stdout, "Iteration(%u) store(%s)\n", i, local_store_name);

        if ( num_ops == 1 ) {
            char new_key[10000];
            sprintf(new_key, "%s_%s_%u", "regression10_key", key, i);
            size_t new_key_size = (size_t )strlen(new_key);

            ret = WriteStore(sess_hndl,
                new_key,
                new_key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "WriteStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t j;

            for ( j = 0 ; j < num_ops ; j++ ) {
                key_ptrs[j] = malloc(key_size + 128);
                sprintf(key_ptrs[j], "%s_%d_%u_%u", "regression10_key", tid, j, i);
                key_sizes[j] = (size_t )strlen(key_ptrs[j]);

                value_ptrs[j] = malloc(value_size + 128);
                sprintf(value_ptrs[j], "%s_%d_%u_%u", "regression10_value", tid, j, i);
                if ( value_size > strlen(value_ptrs[j]) ) {
                    memset(value_ptrs[j] + strlen(value_ptrs[j]), 'V', value_size - strlen(value_ptrs[j]));
                }
                value_sizes[j] = value_size;
            }

            ret = WriteStoreN(sess_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "WriteStoreN(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            for ( j = 0 ; j < num_ops ; j++ ) {
                free(key_ptrs[j]);
                free(value_ptrs[j]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }
        }

        ret = ReadStore(sess_hndl, "", 0, "", 0, it_dir, default_first, debug, hex_dump, no_print, 0);
        if ( ret ) {
            fprintf(stderr, "ReadStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            r_ret = 1;
        }

        CloseStore(sess_hndl, wait_for_key);
        sess_hndl = 0;

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }

        if ( r_ret ) {
            r_ret = 0;
        }

        ret = OpenStore(&sess_hndl,
            cluster_name,
            space_name,
            local_store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            break;
        }
    }

    /*
     * Close stores
     */
    CloseStore(sess_hndl, wait_for_key);
    sess_hndl = 0;

    printf("RegressionTest10(%u) DONE\n", i);
}

static void RegressionTest11(RegressionArgs *args)
{
    const char *cluster_name = args->cluster_name;
    const char *space_name = args->space_name;
    const char *store_name = args->store_name;
    int provider = args->provider;
    const char *provider_name = 0;
    const char *key = args->key;
    size_t key_size = args->key_size;
    const char *end_key = args->end_key;
    size_t end_key_size = args->end_key_size;
    int it_dir = args->it_dir;
    int default_first = args->default_first;
    char *value = args->value;
    size_t value_size = args->value_size;
    uint32_t num_ops = args->num_ops;
    uint32_t num_regressions = args->num_regressions;
    const char *snap_back_subset_name = args->snap_back_subset_name;
    //uint32_t snap_back_id = args->snap_back_id;
    int transactional = args->transactional;
    int sync_type = args->sync_type;
    int debug = args->debug;
    int wait_for_key = args->wait_for_key;
    int hex_dump = args->hex_dump;
    useconds_t sleep_usecs = args->sleep_usecs;
    int no_print = args->no_print;
    //int group_write = args->group_write;
    char local_store_name[1024];
    char local_store_subset_name[1024];
    char store_subset_name[1024];
    char local_snap_back_subset_name[1024];

    SessionHND        sess_hndl = 0;
    SubsetableHND     sub_hndl = 0;
    eSessionSyncMode  sess_sync_mode = 0;

    /* Intializes random number generator */
    time_t t;
    srand((unsigned int)time(&t));

    uint32_t i = 0;
    int ret = 0;
    int s_ret = 0;
    memset(local_store_name, '\0', sizeof(local_store_name));
    memset(local_store_subset_name, '\0', sizeof(local_store_subset_name));
    memset(store_subset_name, '\0', sizeof(store_subset_name));
    sprintf(store_subset_name, "%s_subset", store_name);
    SessionHND sub_sess_hndl = 0;
    pid_t tid = gettid();
    printf("cl_n=%s sp_n=%s st_n=%s "
        "prov=%d prov_n=%s key=%s "
        "key_len=%lu end_key=%s end_key_len=%lu "
        "it_dir=%d defau_fst=%d "
        "value=%s value_len=%lu num_ops=%d "
        "num_regres=%d subset_n=%s trans=%d "
        "sync_typ=%d debug=%d no_prnt=%d "
        "hex_dmp=%d "
        "sess_sync_mod=%d tid=%u\n",
        cluster_name, space_name, store_name,
        provider, provider_name, key,
        key_size, end_key, end_key_size,
        it_dir, default_first,
        value, value_size, num_ops,
        num_regressions, snap_back_subset_name, transactional,
        sync_type, debug, no_print,
        hex_dump,
        sess_sync_mode, tid);
    /*
     * @brief
     * Sync mode
     */
    sess_sync_mode = GetSyncMode(sync_type);

    /*
     * @brief
     * Provider
     */
    provider_name = GetProviderName(provider);

    if ( num_ops > MAX_NUM_OPS ) {
        fprintf(stderr, "number of ops too big\n");
        return;
    }

    int r_ret = 0;
    for ( i = 0 ; i < num_regressions ; i++ ) {
        memset(local_store_name, '\0', sizeof(local_store_name));
        sprintf(local_store_name, "%s_%d_%u", store_name, tid, i);
        memset(local_store_subset_name, '\0', sizeof(local_store_subset_name));
        sprintf(local_store_subset_name, "%s_%d_%u", store_subset_name, tid, i);
        memset(local_snap_back_subset_name, '\0', sizeof(local_snap_back_subset_name));
        sprintf(local_snap_back_subset_name, "%s_%d_%u", "subset", tid, i);
        fprintf(stdout, "Iteration(%u) store(%s) subset_store(%s) subset(%s)\n",
            i, local_store_name, local_store_subset_name, local_snap_back_subset_name);

        /*
         * Store
         */
        ret = OpenStore(&sess_hndl,
            cluster_name,
            space_name,
            local_store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
        }

        if ( ret < 0 ) {
            if ( sess_hndl ) {
                SessionClose(sess_hndl);
                sess_hndl = 0;
            }
            ret = CreateStore(&sess_hndl,
                cluster_name,
                space_name,
                local_store_name,
                transactional,
                sess_sync_mode,
                provider_name,
                (SoeBool )debug);
            if ( ret || !sess_hndl ) {
                fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                return;
            }
            fprintf(stderr, "CreateStore(%s) ok\n", local_store_name);
        }

        /*
         * Another store for subset
         */
        ret = OpenStore(&sub_sess_hndl,
            cluster_name,
            space_name,
            local_store_subset_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sub_sess_hndl ) {
            fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_subset_name, SessionLastStatus(sub_sess_hndl));
        }

        if ( ret < 0 ) {
            if ( sub_sess_hndl ) {
                SessionClose(sub_sess_hndl);
                sub_sess_hndl = 0;
            }
            ret = CreateStore(&sub_sess_hndl,
                cluster_name,
                space_name,
                local_store_subset_name,
                transactional,
                sess_sync_mode,
                provider_name,
                (SoeBool )debug);
            if ( ret || !sub_sess_hndl ) {
                fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_subset_name, SessionLastStatus(sub_sess_hndl));
                CloseStore(sess_hndl, wait_for_key);
                return;
            }
            fprintf(stderr, "CreateStore(%s) ok\n", local_store_subset_name);
        }

        args->io_counter += 4;

        /*
         * Subset
         */
        s_ret = OpenSubset(sub_sess_hndl,
            &sub_hndl,
            local_snap_back_subset_name,
            (size_t )strlen(local_snap_back_subset_name));
        if ( s_ret ) {
            fprintf(stderr, "OpenSubset(%s) failed(%u)\n", local_snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
        }
        if ( s_ret < 0 ) {
            s_ret = CreateSubset(sub_sess_hndl,
                &sub_hndl,
                local_snap_back_subset_name,
                (size_t )strlen(local_snap_back_subset_name));
            if ( s_ret ) {
                fprintf(stderr, "CreateSubset(%s) failed(%u)\n", local_snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
                CloseStore(sess_hndl, wait_for_key);
                CloseStore(sub_sess_hndl, wait_for_key);
                return;
            }
            fprintf(stderr, "CreateSubset(%s) ok\n", local_snap_back_subset_name);
        }

        args->io_counter += 2;

        /*
         * Start i/os
         */
        if ( num_ops == 1 ) {
            if ( value_size > strlen(value) ) {
                memset(value + strlen(value), 'V', value_size - strlen(value));
            }
            ret = WriteStore(sess_hndl,
                key,
                key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "WriteStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            args->io_counter++;
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t j;

            for ( j = 0 ; j < num_ops ; j++ ) {
                key_ptrs[j] = malloc(key_size + 128);
                sprintf(key_ptrs[j], "%d_%d_%s_%u_%u", rand(), tid, key, j, i);
                key_sizes[j] = (size_t )strlen(key_ptrs[j]);

                value_ptrs[j] = malloc(value_size + 128);
                sprintf(value_ptrs[j], "%d_%s_%u_%u", tid, value, j, i);
                if ( value_size > strlen(value_ptrs[j]) ) {
                    memset(value_ptrs[j] + strlen(value_ptrs[j]), 'V', value_size - strlen(value_ptrs[j]));
                }
                value_sizes[j] = value_size;
            }

            ret = WriteStoreN(sess_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "WriteStoreN(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            for ( j = 0 ; j < num_ops ; j++ ) {
                free(key_ptrs[j]);
                free(value_ptrs[j]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            args->io_counter += num_ops;

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }
        }

        ret = ReadStore(sess_hndl, 0, 0, 0, 0, it_dir, default_first, debug, hex_dump, no_print, &args->io_counter);
        if ( ret ) {
            fprintf(stderr, "ReadStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            r_ret = 1;
        }

        if ( num_ops == 1 ) {
            if ( value_size > strlen(value) ) {
                memset(value + strlen(value), 'S', value_size - strlen(value));
            }
            s_ret = WriteSubset(sub_hndl,
                key,
                key_size,
                value,
                value_size);
            if ( s_ret ) {
                fprintf(stderr, "WriteSubset(%s) failed(%u)\n", local_snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
                r_ret = 1;
            }

            args->io_counter++;
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t k;

            for ( k = 0 ; k < num_ops ; k++ ) {
                key_ptrs[k] = malloc(key_size + 128);
                sprintf(key_ptrs[k], "%d_%d_%s_%u_%u", rand(), tid, key, k, i);
                key_sizes[k] = (size_t )strlen(key_ptrs[k]);

                value_ptrs[k] = malloc(value_size + 128);
                sprintf(value_ptrs[k], "%d_%s_%u_%u", tid, value, k, i);
                if ( value_size > strlen(value_ptrs[k]) ) {
                    memset(value_ptrs[k] + strlen(value_ptrs[k]), 'S', value_size - strlen(value_ptrs[k]));
                }
                value_sizes[k] = value_size;
            }

            s_ret = WriteSubsetN(sub_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( s_ret ) {
                fprintf(stderr, "WriteSubsetN(%s) failed(%u)\n", local_snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
                r_ret = 1;
            }
            for ( k = 0 ; k < num_ops ; k++ ) {
                free(key_ptrs[k]);
                free(value_ptrs[k]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            args->io_counter += num_ops;
        }

        ret = ReadSubset(sub_hndl, 0, 0, 0, 0, it_dir, default_first, debug, hex_dump, no_print, &args->io_counter);
        if ( ret ) {
            fprintf(stderr, "ReadSubset(%s) failed(%u)\n", local_snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
            r_ret = 1;
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }

        /*
         * Destroy subset
         */
        ret = DestroySubset(sub_sess_hndl, sub_hndl);
        if ( ret ) {
            fprintf(stderr, "DestroySubset(%s) failed(%u)\n", local_snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
        }

        ret = SessionDeleteSubset(sub_sess_hndl, sub_hndl);
        if ( ret ) {
            fprintf(stderr, "SessionDeleteSubset(%s) failed(%u)\n", local_snap_back_subset_name, SessionLastStatus(sub_sess_hndl));
        }

        /*
         * Destroy stores
         */
        ret = DestroyStore(sess_hndl);
        if ( ret ) {
            fprintf(stderr, "DestroyStore(%s) failed\n", local_store_name);
        }

        /*
         * Close store
         */
        CloseStore(sess_hndl, 0);
        sess_hndl = 0;

        ret = DestroyStore(sub_sess_hndl);
        if ( ret ) {
            fprintf(stderr, "DestroyStore(%s) failed\n", local_store_subset_name);
        }

        /*
         * Close store
         */
        CloseStore(sub_sess_hndl, 0);
        sub_sess_hndl = 0;

        args->io_counter += 2;
    }

    printf("RegressionTest11(%u) io_counter=%lu DONE\n", i, args->io_counter);
}

static void RegressionTest12(RegressionArgs *args)
{
    const char *cluster_name = args->cluster_name;
    const char *space_name = args->space_name;
    const char *store_name = args->store_name;
    int provider = args->provider;
    const char *provider_name = 0;
    const char *key = args->key;
    size_t key_size = args->key_size;
    const char *end_key = args->end_key;
    size_t end_key_size = args->end_key_size;
    int it_dir = args->it_dir;
    int default_first = args->default_first;
    char *value = args->value;
    size_t value_size = args->value_size;
    uint32_t num_ops = args->num_ops;
    uint32_t num_regressions = args->num_regressions;
    const char *snap_back_subset_name = args->snap_back_subset_name;
    //uint32_t snap_back_id = args->snap_back_id;
    int transactional = args->transactional;
    int sync_type = args->sync_type;
    int debug = args->debug;
    int wait_for_key = args->wait_for_key;
    int hex_dump = args->hex_dump;
    useconds_t sleep_usecs = args->sleep_usecs;
    int no_print = args->no_print;
    //int group_write = args->group_write;
    uint32_t sleep_multiplier = args->sleep_multiplier;
    char local_store_name[1024];
    char local_store_subset_name[1024];
    char store_subset_name[1024];
    char local_snap_back_subset_name[1024];
    int sync_async_destroy = args->sync_async_destroy;
    int th_number = args->th_number;

    eSessionSyncMode  sess_sync_mode = 0;

    /* Intializes random number generator */
    time_t t;
    srand((unsigned int)time(&t));

    uint32_t i = 0;
    int ret = 0;
    int s_ret = 0;
    memset(local_store_name, '\0', sizeof(local_store_name));
    memset(local_store_subset_name, '\0', sizeof(local_store_subset_name));
    memset(store_subset_name, '\0', sizeof(store_subset_name));
    sprintf(store_subset_name, "%s_subset", store_name);

    SessionHND m_sess_hndl[STORE_12_COUNT];
    SessionHND m_sub_sess_hndl[STORE_12_COUNT];
    SubsetableHND m_sub_hndl[STORE_12_COUNT];
    memset(m_sess_hndl, '\0', sizeof(m_sess_hndl));
    memset(m_sub_sess_hndl, '\0', sizeof(m_sub_sess_hndl));
    memset(m_sub_hndl, '\0', sizeof(m_sub_hndl));

    pid_t tid = gettid();
    printf("cl_n=%s sp_n=%s st_n=%s "
        "prov=%d prov_n=%s key=%s "
        "key_len=%lu end_key=%s end_key_len=%lu "
        "it_dir=%d defau_fst=%d "
        "value=%s value_len=%lu num_ops=%d "
        "num_regres=%d subset_n=%s trans=%d "
        "sync_typ=%d debug=%d no_prnt=%d "
        "hex_dmp=%d "
        "sleep_u=%u sleep_m=%u "
        "sess_sync_mod=%d sync_async_destroy=%d th_num=%d tid=%u\n",
        cluster_name, space_name, store_name,
        provider, provider_name, key,
        key_size, end_key, end_key_size,
        it_dir, default_first,
        value, value_size, num_ops,
        num_regressions, snap_back_subset_name, transactional,
        sync_type, debug, no_print,
        hex_dump,
        sleep_usecs, sleep_multiplier,
        sess_sync_mode, sync_async_destroy, th_number, tid);
    /*
     * @brief
     * Sync mode
     */
    sess_sync_mode = GetSyncMode(sync_type);

    /*
     * @brief
     * Provider
     */
    provider_name = GetProviderName(provider);

    if ( num_ops > MAX_NUM_OPS ) {
        fprintf(stderr, "number of ops too big\n");
        return;
    }

    memset(local_snap_back_subset_name, '\0', sizeof(local_snap_back_subset_name));
    sprintf(local_snap_back_subset_name, "%s_%d", "subset", tid);

    uint32_t s_cnt;
    uint32_t max_s_cnt;
    RegressionSessionParamsTest12 *params = 0;
    if ( sync_async_destroy ) {
        params = &args->params->params12.threads[args->th_number];
    }

    if ( !sync_async_destroy ) {
        /*
         * Open/Create STORE_12_COUNT Stores
         */
        for ( s_cnt = 0 ; s_cnt < STORE_12_COUNT ; s_cnt++ ) {
            memset(local_store_name, '\0', sizeof(local_store_name));
            sprintf(local_store_name, "%s_%d_%u", store_name, tid, s_cnt);
            memset(local_store_subset_name, '\0', sizeof(local_store_subset_name));
            sprintf(local_store_subset_name, "%s_%d_%u", store_subset_name, tid, s_cnt);

            ret = OpenStore(&m_sess_hndl[s_cnt],
                cluster_name,
                space_name,
                local_store_name,
                transactional,
                sess_sync_mode,
                provider_name,
                (SoeBool )debug);
            if ( ret || !m_sess_hndl[s_cnt] ) {
                fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(m_sess_hndl[s_cnt]));
            }

            if ( ret < 0 ) {
                if ( m_sess_hndl[s_cnt] ) {
                    SessionClose(m_sess_hndl[s_cnt]);
                    m_sess_hndl[s_cnt] = 0;
                }
                ret = CreateStore(&m_sess_hndl[s_cnt],
                    cluster_name,
                    space_name,
                    local_store_name,
                    transactional,
                    sess_sync_mode,
                    provider_name,
                    (SoeBool )debug);
                if ( ret || !m_sess_hndl[s_cnt] ) {
                    fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(m_sess_hndl[s_cnt]));
                    return;
                }
                fprintf(stderr, "CreateStore(%s) ok\n", local_store_name);
            }

            /*
             * Open/Create another store for subset
             */
            ret = OpenStore(&m_sub_sess_hndl[s_cnt],
                cluster_name,
                space_name,
                local_store_subset_name,
                transactional,
                sess_sync_mode,
                provider_name,
                (SoeBool )debug);
            if ( ret || !m_sub_sess_hndl[s_cnt] ) {
                fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_subset_name, SessionLastStatus(m_sub_sess_hndl[s_cnt]));
            }

            if ( ret < 0 ) {
                if ( m_sub_sess_hndl[s_cnt] ) {
                    SessionClose(m_sub_sess_hndl[s_cnt]);
                    m_sub_sess_hndl[s_cnt] = 0;
                }
                ret = CreateStore(&m_sub_sess_hndl[s_cnt],
                    cluster_name,
                    space_name,
                    local_store_subset_name,
                    transactional,
                    sess_sync_mode,
                    provider_name,
                    (SoeBool )debug);
                if ( ret || !m_sub_sess_hndl[s_cnt] ) {
                    fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_subset_name, SessionLastStatus(m_sub_sess_hndl[s_cnt]));
                    CloseStore(m_sess_hndl[s_cnt], wait_for_key);
                    return;
                }
                fprintf(stderr, "CreateStore(%s) ok\n", local_store_subset_name);
            }
        }

        max_s_cnt = STORE_12_COUNT;
    } else {
        /*
         * Open/Create STORE_12_COUNT Stores
         */
        params->session_count = STORE_12_COUNT;
        for ( s_cnt = 0 ; s_cnt < params->session_count ; s_cnt++ ) {
            memset(local_store_name, '\0', sizeof(local_store_name));
            sprintf(local_store_name, "%s_%d_%u", store_name, tid, s_cnt);
            memset(local_store_subset_name, '\0', sizeof(local_store_subset_name));
            sprintf(local_store_subset_name, "%s_%d_%u", store_subset_name, tid, s_cnt);
            strcpy(params->sessions[s_cnt].store_name, local_store_name);
            strcpy(params->sessions[s_cnt].sub_store_name, local_store_subset_name);

            ret = OpenStore(&params->sessions[s_cnt].sess_hndl,
                cluster_name,
                space_name,
                local_store_name,
                transactional,
                sess_sync_mode,
                provider_name,
                (SoeBool )debug);
            if ( ret || !params->sessions[s_cnt].sess_hndl ) {
                fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(params->sessions[s_cnt].sess_hndl));
            }

            if ( ret < 0 ) {
                if ( params->sessions[s_cnt].sess_hndl ) {
                    SessionClose(params->sessions[s_cnt].sess_hndl);
                    params->sessions[s_cnt].sess_hndl = 0;
                }
                ret = CreateStore(&params->sessions[s_cnt].sess_hndl,
                    cluster_name,
                    space_name,
                    local_store_name,
                    transactional,
                    sess_sync_mode,
                    provider_name,
                    (SoeBool )debug);
                if ( ret || !params->sessions[s_cnt].sess_hndl ) {
                    fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(params->sessions[s_cnt].sess_hndl));
                    return;
                }
                fprintf(stderr, "CreateStore(%s) ok\n", local_store_name);
            }

            /*
             * Open/Create another store for subset
             */
            ret = OpenStore(&params->sessions[s_cnt].sub_sess_hndl,
                cluster_name,
                space_name,
                local_store_subset_name,
                transactional,
                sess_sync_mode,
                provider_name,
                (SoeBool )debug);
            if ( ret || !params->sessions[s_cnt].sub_sess_hndl ) {
                fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_subset_name, SessionLastStatus(params->sessions[s_cnt].sub_sess_hndl));
            }

            if ( ret < 0 ) {
                if ( params->sessions[s_cnt].sub_sess_hndl ) {
                    SessionClose(params->sessions[s_cnt].sub_sess_hndl);
                    params->sessions[s_cnt].sub_sess_hndl = 0;
                }
                ret = CreateStore(&params->sessions[s_cnt].sub_sess_hndl,
                    cluster_name,
                    space_name,
                    local_store_subset_name,
                    transactional,
                    sess_sync_mode,
                    provider_name,
                    (SoeBool )debug);
                if ( ret || !params->sessions[s_cnt].sub_sess_hndl ) {
                    fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_subset_name, SessionLastStatus(params->sessions[s_cnt].sub_sess_hndl));
                    CloseStore(params->sessions[s_cnt].sess_hndl, wait_for_key);
                    return;
                }
                fprintf(stderr, "CreateStore(%s) ok\n", local_store_subset_name);
            }

            params->sessions[s_cnt].subset_count = 1;
            uint32_t l;
            for ( l = 0 ; l < params->sessions[s_cnt].subset_count ; l++ ) {
                strcpy(params->sessions[s_cnt].subsets[l].sub_name, local_snap_back_subset_name);
            }
        }

        max_s_cnt = params->session_count;
    }

    args->io_counter += 4;

#define TEST12_SESS_HNDL(i)       (!sync_async_destroy ? m_sess_hndl[(i)] : params->sessions[(i)].sess_hndl)
#define TEST12_SUB_SESS_HNDL(i)   (!sync_async_destroy ? m_sub_sess_hndl[(i)] : params->sessions[(i)].sub_sess_hndl)
#define TEST12_SUB_HNDL(i)        (!sync_async_destroy ? m_sub_hndl[(i)] : params->sessions[(i)].subsets[0].sub_hndl)
#define TEST12_SUB_PHNDL(i)       (!sync_async_destroy ? &m_sub_hndl[(i)] : &params->sessions[(i)].subsets[0].sub_hndl)
#define TEST12_SUB_HNDL_SET(i, v) (!sync_async_destroy ? (m_sub_hndl[(i)] = (v)): (params->sessions[(i)].subsets[0].sub_hndl = (v)))
#define TEST12_SUB_NAME(i)        (!sync_async_destroy ? local_snap_back_subset_name : params->sessions[(i)].subsets[0].sub_name)

    int r_ret = 0;
    for ( i = 0 ; i < num_regressions ; i++ ) {
        fprintf(stdout, "Iteration(%u) store(%s/%u) subset_store(%s/%u) subset(%s)\n",
            i, store_name, i, store_subset_name, i, TEST12_SUB_NAME(0));

        for ( s_cnt = 0 ; s_cnt < max_s_cnt ; s_cnt++ ) {
            /*
             * Subset
             */
            s_ret = OpenSubset(TEST12_SUB_SESS_HNDL(s_cnt),
                TEST12_SUB_PHNDL(s_cnt),
                TEST12_SUB_NAME(s_cnt),
                (size_t )strlen(TEST12_SUB_NAME(s_cnt)));
            if ( s_ret ) {
                fprintf(stderr, "OpenSubset(%s) failed(%u)\n", TEST12_SUB_NAME(s_cnt), SessionLastStatus(TEST12_SUB_SESS_HNDL(s_cnt)));
            }
            if ( s_ret < 0 ) {
                s_ret = CreateSubset(TEST12_SUB_SESS_HNDL(s_cnt),
                    TEST12_SUB_PHNDL(s_cnt),
                    TEST12_SUB_NAME(s_cnt),
                    (size_t )strlen(TEST12_SUB_NAME(s_cnt)));
                if ( s_ret ) {
                    fprintf(stderr, "CreateSubset(%s) failed(%u)\n", TEST12_SUB_NAME(s_cnt), SessionLastStatus(TEST12_SUB_SESS_HNDL(s_cnt)));
                    CloseStore(TEST12_SESS_HNDL(s_cnt), wait_for_key);
                    CloseStore(TEST12_SUB_SESS_HNDL(s_cnt), wait_for_key);
                    return;
                }
                fprintf(stderr, "CreateSubset(%s) ok\n", TEST12_SUB_NAME(s_cnt));
            }

            args->io_counter += 2;

            /*
             * Start i/os
             */
            if ( num_ops == 1 ) {
                if ( value_size > strlen(value) ) {
                    memset(value + strlen(value), 'V', value_size - strlen(value));
                }
                ret = WriteStore(TEST12_SESS_HNDL(s_cnt),
                    key,
                    key_size,
                    value,
                    value_size);
                if ( ret ) {
                    fprintf(stderr, "WriteStore(%s/%u) failed(%u)\n", local_store_name, s_cnt, SessionLastStatus(m_sess_hndl[s_cnt]));
                    r_ret = 1;
                }

                args->io_counter++;
            } else {
                char **key_ptrs = malloc(num_ops*sizeof(char *));
                memset(key_ptrs, '\0', num_ops*sizeof(char *));
                size_t *key_sizes = malloc(num_ops*sizeof(size_t));
                memset(key_sizes, '\0', num_ops*sizeof(size_t));
                char **value_ptrs = malloc(num_ops*sizeof(char *));
                memset(value_ptrs, '\0', num_ops*sizeof(char *));
                size_t *value_sizes = malloc(num_ops*sizeof(size_t));
                memset(value_sizes, '\0', num_ops*sizeof(size_t));
                uint32_t j;

                for ( j = 0 ; j < num_ops ; j++ ) {
                    key_ptrs[j] = malloc(key_size + 128);
                    sprintf(key_ptrs[j], "%d_%d_%s_%u_%u", rand(), tid, key, j, i);
                    key_sizes[j] = (size_t )strlen(key_ptrs[j]);

                    value_ptrs[j] = malloc(value_size + 128);
                    sprintf(value_ptrs[j], "%d_%s_%u_%u", tid, value, j, i);
                    if ( value_size > strlen(value_ptrs[j]) ) {
                        memset(value_ptrs[j] + strlen(value_ptrs[j]), 'V', value_size - strlen(value_ptrs[j]));
                    }
                    value_sizes[j] = value_size;
                }

                ret = WriteStoreN(TEST12_SESS_HNDL(s_cnt),
                    num_ops,
                    key_ptrs,
                    key_sizes,
                    value_ptrs,
                    value_sizes);
                if ( ret ) {
                    fprintf(stderr, "WriteStoreN(%s/%u) failed(%u)\n", local_store_name, s_cnt, SessionLastStatus(m_sess_hndl[s_cnt]));
                    r_ret = 1;
                }

                for ( j = 0 ; j < num_ops ; j++ ) {
                    free(key_ptrs[j]);
                    free(value_ptrs[j]);
                }

                free(key_ptrs);
                free(key_sizes);
                free(value_ptrs);
                free(value_sizes);

                args->io_counter += num_ops;

                if ( sleep_usecs ) {
                    usleep(sleep_usecs);
                }
            }

            ret = ReadStore(TEST12_SESS_HNDL(s_cnt), 0, 0, 0, 0, it_dir, default_first, debug, hex_dump, no_print, &args->io_counter);
            if ( ret ) {
                fprintf(stderr, "ReadStore(%s/%u) failed(%u)\n", local_store_name, s_cnt, SessionLastStatus(m_sess_hndl[s_cnt]));
                r_ret = 1;
            }

            if ( num_ops == 1 ) {
                if ( value_size > strlen(value) ) {
                    memset(value + strlen(value), 'S', value_size - strlen(value));
                }
                s_ret = WriteSubset(m_sub_hndl[s_cnt],
                    key,
                    key_size,
                    value,
                    value_size);
                if ( s_ret ) {
                    fprintf(stderr, "WriteSubset(%s/%u) failed(%u)\n", local_snap_back_subset_name, s_cnt, SessionLastStatus(m_sub_sess_hndl[s_cnt]));
                    r_ret = 1;
                }

                args->io_counter++;
            } else {
                char **key_ptrs = malloc(num_ops*sizeof(char *));
                memset(key_ptrs, '\0', num_ops*sizeof(char *));
                size_t *key_sizes = malloc(num_ops*sizeof(size_t));
                memset(key_sizes, '\0', num_ops*sizeof(size_t));
                char **value_ptrs = malloc(num_ops*sizeof(char *));
                memset(value_ptrs, '\0', num_ops*sizeof(char *));
                size_t *value_sizes = malloc(num_ops*sizeof(size_t));
                memset(value_sizes, '\0', num_ops*sizeof(size_t));
                uint32_t k;

                for ( k = 0 ; k < num_ops ; k++ ) {
                    key_ptrs[k] = malloc(key_size + 128);
                    sprintf(key_ptrs[k], "%d_%d_%s_%u_%u", rand(), tid, key, k, i);
                    key_sizes[k] = (size_t )strlen(key_ptrs[k]);

                    value_ptrs[k] = malloc(value_size + 128);
                    sprintf(value_ptrs[k], "%d_%s_%u_%u", tid, value, k, i);
                    if ( value_size > strlen(value_ptrs[k]) ) {
                        memset(value_ptrs[k] + strlen(value_ptrs[k]), 'S', value_size - strlen(value_ptrs[k]));
                    }
                    value_sizes[k] = value_size;
                }

                s_ret = WriteSubsetN(TEST12_SUB_HNDL(s_cnt),
                    num_ops,
                    key_ptrs,
                    key_sizes,
                    value_ptrs,
                    value_sizes);
                if ( s_ret ) {
                    fprintf(stderr, "WriteSubsetN(%s/%u) failed(%u)\n", local_snap_back_subset_name, s_cnt, SessionLastStatus(m_sub_sess_hndl[s_cnt]));
                    r_ret = 1;
                }
                for ( k = 0 ; k < num_ops ; k++ ) {
                    free(key_ptrs[k]);
                    free(value_ptrs[k]);
                }

                free(key_ptrs);
                free(key_sizes);
                free(value_ptrs);
                free(value_sizes);

                args->io_counter += num_ops;
            }

            ret = ReadSubset(TEST12_SUB_HNDL(s_cnt), 0, 0, 0, 0, it_dir, default_first, debug, hex_dump, no_print, &args->io_counter);
            if ( ret ) {
                fprintf(stderr, "ReadSubset(%s/%u) failed(%u)\n", local_snap_back_subset_name, s_cnt, SessionLastStatus(m_sub_sess_hndl[s_cnt]));
                r_ret = 1;
            }

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }

            ret = CloseSubset(TEST12_SUB_SESS_HNDL(s_cnt), TEST12_SUB_HNDL(s_cnt));
            if ( ret ) {
                fprintf(stderr, "CloseSubset(%s/%u) failed(%u)\n", local_snap_back_subset_name, s_cnt, SessionLastStatus(m_sub_sess_hndl[s_cnt]));
                r_ret = 1;
            }
            TEST12_SUB_HNDL_SET(s_cnt, 0);
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }
    }

    if ( sleep_usecs ) {
        sleep(sleep_multiplier);
    }

    /*
     * Destroy stores/subsets
     */
    if ( !sync_async_destroy ) {
        for ( s_cnt = 0 ; s_cnt < STORE_12_COUNT ; s_cnt++ ) {
            s_ret = OpenSubset(m_sub_sess_hndl[s_cnt],
                &m_sub_hndl[s_cnt],
                local_snap_back_subset_name,
                (size_t )strlen(local_snap_back_subset_name));
            if ( s_ret ) {
                fprintf(stderr, "OpenSubset(%s/%u) for destroy failed(%u)\n", local_snap_back_subset_name, s_cnt, SessionLastStatus(m_sub_sess_hndl[s_cnt]));
            }

            ret = DestroySubset(m_sub_sess_hndl[s_cnt], m_sub_hndl[s_cnt]);
            if ( ret ) {
                fprintf(stderr, "DestroySubset(%s/%u) failed(%u)\n", local_snap_back_subset_name, s_cnt, SessionLastStatus(m_sub_sess_hndl[s_cnt]));
            }

            ret = SessionDeleteSubset(m_sub_sess_hndl[s_cnt], m_sub_hndl[s_cnt]);
            if ( ret ) {
                fprintf(stderr, "SessionDeleteSubset(%s/%u) failed(%u)\n", local_snap_back_subset_name, s_cnt, SessionLastStatus(m_sub_sess_hndl[s_cnt]));
            }

            /*
             * Destroy stores
             */
            ret = DestroyStore(m_sess_hndl[s_cnt]);
            if ( ret ) {
                fprintf(stderr, "DestroyStore(%s/%u) failed\n", local_store_name, s_cnt);
            }

            /*
             * Close store
             */
            CloseStore(m_sess_hndl[s_cnt], 0);
            m_sess_hndl[s_cnt] = 0;

            ret = DestroyStore(m_sub_sess_hndl[s_cnt]);
            if ( ret ) {
                fprintf(stderr, "DestroyStore(%s/%u) failed\n", local_store_subset_name, s_cnt);
            }

            /*
             * Close store
             */
            CloseStore(m_sub_sess_hndl[s_cnt], 0);
            m_sub_sess_hndl[s_cnt] = 0;
        }

        args->io_counter += (6*STORE_12_COUNT);
    }

    printf("RegressionTest12(%u) io_counter=%lu DONE\n", i, args->io_counter);
}

static void RegressionTest15(RegressionArgs *args)
{
    const char *cluster_name = args->cluster_name;
    const char *space_name = args->space_name;
    const char *store_name = args->store_name;
    int provider = args->provider;
    const char *provider_name = 0;
    const char *key = args->key;
    size_t key_size = args->key_size;
    const char *end_key = args->end_key;
    size_t end_key_size = args->end_key_size;
    int it_dir = args->it_dir;
    int default_first = args->default_first;
    char *value = args->value;
    size_t value_size = args->value_size;
    uint32_t num_ops = args->num_ops;
    uint32_t num_regressions = args->num_regressions;
    const char *snap_back_subset_name = args->snap_back_subset_name;
    //uint32_t snap_back_id = args->snap_back_id;
    int transactional = args->transactional;
    int sync_type = args->sync_type;
    int debug = args->debug;
    int wait_for_key = args->wait_for_key;
    int hex_dump = args->hex_dump;
    useconds_t sleep_usecs = args->sleep_usecs;
    int no_print = args->no_print;
    //int group_write = args->group_write;
    char local_store_name[1024];

    SessionHND        sess_hndl = 0;
    eSessionSyncMode  sess_sync_mode = 0;
    pid_t tid = gettid();

    uint32_t i = 0;
    int ret = 0;
    memset(local_store_name, '\0', sizeof(local_store_name));
    printf("cl_n=%s sp_n=%s st_n=%s "
        "prov=%d prov_name=%s key=%s "
        "key_len=%lu end_key=%s end_key_len=%lu "
        "it_dir=%d "
        "value=%s value_len=%lu num_ops=%d "
        "num_regres=%d subset_name=%s trans=%d "
        "sync_type=%d debug=%d wait_for_key=%d "
        "hex_dump=%d "
        "sess_sync_mode=%d\n",
        cluster_name, space_name, store_name,
        provider, provider_name, key,
        key_size, end_key, end_key_size,
        it_dir,
        value, value_size, num_ops,
        num_regressions, snap_back_subset_name, transactional,
        sync_type, debug, wait_for_key,
        hex_dump,
        sess_sync_mode);

    /*
     * @brief
     * Sync mode
     */
    sess_sync_mode = GetSyncMode(sync_type);

    /*
     * @brief
     * Provider
     */
    provider_name = GetProviderName(provider);

    if ( num_ops > MAX_NUM_OPS ) {
        fprintf(stderr, "number of ops too big\n");
        return;
    }

    memset(local_store_name, '\0', sizeof(local_store_name));

    int r_ret = 0;
    for ( i = 0 ; i < num_regressions ; i++ ) {
        sprintf(local_store_name, "%s_%u_%d", store_name, tid, i);
        /*
         * Store
         */
        ret = OpenStore(&sess_hndl,
            cluster_name,
            space_name,
            local_store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
        }

        if ( ret < 0 ) {
            if ( sess_hndl ) {
                SessionClose(sess_hndl);
                sess_hndl = 0;
            }
            ret = CreateStore(&sess_hndl,
                cluster_name,
                space_name,
                local_store_name,
                transactional,
                sess_sync_mode,
                provider_name,
                (SoeBool )debug);
            if ( ret || !sess_hndl ) {
                fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                return;
            }
            fprintf(stderr, "CreateStore(%s) ok\n", local_store_name);
        }

        fprintf(stdout, "Iteration(%u) thread(%u) store(%s)\n", i, tid, local_store_name);

        if ( num_ops == 1 ) {
            char new_key[10000];
            sprintf(new_key, "%s_%s_%u", "regression15_key", key, i);
            size_t new_key_size = (size_t )strlen(new_key);

            ret = WriteStore(sess_hndl,
                new_key,
                new_key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "WriteStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t j;

            for ( j = 0 ; j < num_ops ; j++ ) {
                key_ptrs[j] = malloc(key_size + 128);
                sprintf(key_ptrs[j], "%s_%d_%u_%u", "regression15_key", tid, j, i);
                key_sizes[j] = (size_t )strlen(key_ptrs[j]);

                value_ptrs[j] = malloc(value_size + 128);
                sprintf(value_ptrs[j], "%s_%d_%u_%u", "regression15_value", tid, j, i);
                if ( value_size > strlen(value_ptrs[j]) ) {
                    memset(value_ptrs[j] + strlen(value_ptrs[j]), 'V', value_size - strlen(value_ptrs[j]));
                }
                value_sizes[j] = value_size;
            }

            ret = WriteStoreN(sess_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "WriteStoreN(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            for ( j = 0 ; j < num_ops ; j++ ) {
                free(key_ptrs[j]);
                free(value_ptrs[j]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }
        }

        ret = ReadStore(sess_hndl, "", 0, "", 0, it_dir, default_first, debug, hex_dump, no_print, 0);
        if ( ret ) {
            fprintf(stderr, "ReadStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            r_ret = 1;
        }

        CloseStore(sess_hndl, wait_for_key);
        sess_hndl = 0;

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }

        if ( r_ret ) {
            r_ret = 0;
        }
    }

    printf("RegressionTest15(%u) DONE\n", i);
}

static void RegressionTest16(RegressionArgs *args)
{
    const char *cluster_name = args->cluster_name;
    const char *space_name = args->space_name;
    const char *store_name = args->store_name;
    int provider = args->provider;
    const char *provider_name = 0;
    const char *key = args->key;
    size_t key_size = args->key_size;
    const char *end_key = args->end_key;
    size_t end_key_size = args->end_key_size;
    int it_dir = args->it_dir;
    int default_first = args->default_first;
    char *value = args->value;
    size_t value_size = args->value_size;
    uint32_t num_ops = args->num_ops;
    uint32_t num_regressions = args->num_regressions;
    const char *snap_back_subset_name = args->snap_back_subset_name;
    //uint32_t snap_back_id = args->snap_back_id;
    int transactional = args->transactional;
    int sync_type = args->sync_type;
    int debug = args->debug;
    int wait_for_key = args->wait_for_key;
    int hex_dump = args->hex_dump;
    useconds_t sleep_usecs = args->sleep_usecs;
    int no_print = args->no_print;
    //int group_write = args->group_write;
    char local_store_name[1024];

    SessionHND        sess_hndl = 0;
    eSessionSyncMode  sess_sync_mode = 0;
    pid_t tid = gettid();

    uint32_t i = 0;
    int ret = 0;
    memset(local_store_name, '\0', sizeof(local_store_name));
    printf("cl_n=%s sp_n=%s st_n=%s "
        "prov=%d prov_name=%s key=%s "
        "key_len=%lu end_key=%s end_key_len=%lu "
        "it_dir=%d "
        "value=%s value_len=%lu num_ops=%d "
        "num_regres=%d subset_name=%s trans=%d "
        "sync_type=%d debug=%d wait_for_key=%d "
        "hex_dump=%d "
        "sess_sync_mode=%d\n",
        cluster_name, space_name, store_name,
        provider, provider_name, key,
        key_size, end_key, end_key_size,
        it_dir,
        value, value_size, num_ops,
        num_regressions, snap_back_subset_name, transactional,
        sync_type, debug, wait_for_key,
        hex_dump,
        sess_sync_mode);

    /*
     * @brief
     * Sync mode
     */
    sess_sync_mode = GetSyncMode(sync_type);

    /*
     * @brief
     * Provider
     */
    provider_name = GetProviderName(provider);

    if ( num_ops > MAX_NUM_OPS ) {
        fprintf(stderr, "number of ops too big\n");
        return;
    }

    memset(local_store_name, '\0', sizeof(local_store_name));

    sprintf(local_store_name, "%s_%u", store_name, tid);
    /*
     * Store
     */
    ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        local_store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
    }

    if ( ret < 0 ) {
        if ( sess_hndl ) {
            SessionClose(sess_hndl);
            sess_hndl = 0;
        }
        ret = CreateStore(&sess_hndl,
            cluster_name,
            space_name,
            local_store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            return;
        }
        fprintf(stderr, "CreateStore(%s) ok\n", local_store_name);
    }

    sleep(1);

    int r_ret = 0;
    for ( i = 0 ; i < num_regressions ; i++ ) {
        fprintf(stdout, "Iteration(%u) thread(%u) store(%s)\n", i, tid, local_store_name);

        if ( num_ops == 1 ) {
            char new_key[10000];
            sprintf(new_key, "%s_%s_%u", "regression16_key", key, i);
            size_t new_key_size = (size_t )strlen(new_key);

            ret = WriteStore(sess_hndl,
                new_key,
                new_key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "WriteStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t j;

            for ( j = 0 ; j < num_ops ; j++ ) {
                key_ptrs[j] = malloc(key_size + 128);
                sprintf(key_ptrs[j], "%s_%d_%u_%u", "regression16_key", tid, j, i);
                key_sizes[j] = (size_t )strlen(key_ptrs[j]);

                value_ptrs[j] = malloc(value_size + 128);
                sprintf(value_ptrs[j], "%s_%d_%u_%u", "regression16_value", tid, j, i);
                if ( value_size > strlen(value_ptrs[j]) ) {
                    memset(value_ptrs[j] + strlen(value_ptrs[j]), 'V', value_size - strlen(value_ptrs[j]));
                }
                value_sizes[j] = value_size;
            }

            ret = WriteStoreN(sess_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "WriteStoreN(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            for ( j = 0 ; j < num_ops ; j++ ) {
                free(key_ptrs[j]);
                free(value_ptrs[j]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }
        }

        ret = ReadStore(sess_hndl, "", 0, "", 0, it_dir, default_first, debug, hex_dump, no_print, 0);
        if ( ret ) {
            fprintf(stderr, "ReadStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl));
            r_ret = 1;
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }

        if ( r_ret ) {
            r_ret = 0;
        }
    }

    CloseStore(sess_hndl, wait_for_key);
    sess_hndl = 0;

    printf("RegressionTest16(%u) DONE\n", i);
}

static void RegressionTest17(RegressionArgs *args)
{
    const char *cluster_name = args->cluster_name;
    const char *space_name = args->space_name;
    const char *store_name = args->store_name;
    int provider = args->provider;
    const char *provider_name = 0;
    const char *key = args->key;
    size_t key_size = args->key_size;
    const char *end_key = args->end_key;
    size_t end_key_size = args->end_key_size;
    int it_dir = args->it_dir;
    int default_first = args->default_first;
    char *value = args->value;
    size_t value_size = args->value_size;
    uint32_t num_ops = args->num_ops;
    uint32_t num_regressions = args->num_regressions;
    const char *snap_back_subset_name = args->snap_back_subset_name;
    //uint32_t snap_back_id = args->snap_back_id;
    int transactional = args->transactional;
    int sync_type = args->sync_type;
    int debug = args->debug;
    int wait_for_key = args->wait_for_key;
    int hex_dump = args->hex_dump;
    useconds_t sleep_usecs = args->sleep_usecs;
    int no_print = args->no_print;
    //int group_write = args->group_write;
    char local_store_name[1024];

    SessionHND        sess_hndl1 = 0;
    SessionHND        sess_hndl2 = 0;
    eSessionSyncMode  sess_sync_mode = 0;
    pid_t tid = gettid();

    uint32_t i = 0;
    int ret = 0;
    memset(local_store_name, '\0', sizeof(local_store_name));
    printf("cl_n=%s sp_n=%s st_n=%s "
        "prov=%d prov_name=%s key=%s "
        "key_len=%lu end_key=%s end_key_len=%lu "
        "it_dir=%d "
        "value=%s value_len=%lu num_ops=%d "
        "num_regres=%d subset_name=%s trans=%d "
        "sync_type=%d debug=%d wait_for_key=%d "
        "hex_dump=%d "
        "sess_sync_mode=%d\n",
        cluster_name, space_name, store_name,
        provider, provider_name, key,
        key_size, end_key, end_key_size,
        it_dir,
        value, value_size, num_ops,
        num_regressions, snap_back_subset_name, transactional,
        sync_type, debug, wait_for_key,
        hex_dump,
        sess_sync_mode);

    /*
     * @brief
     * Sync mode
     */
    sess_sync_mode = GetSyncMode(sync_type);

    /*
     * @brief
     * Provider
     */
    provider_name = GetProviderName(provider);

    if ( num_ops > MAX_NUM_OPS ) {
        fprintf(stderr, "number of ops too big\n");
        return;
    }

    memset(local_store_name, '\0', sizeof(local_store_name));

    sprintf(local_store_name, "%s_%u", store_name, tid);
    /*
     * Store
     */
    ret = OpenStore(&sess_hndl1,
        cluster_name,
        space_name,
        local_store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl1 ) {
        fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl1));
    }

    if ( ret < 0 ) {
        if ( sess_hndl1 ) {
            SessionClose(sess_hndl1);
            sess_hndl1 = 0;
        }
        ret = CreateStore(&sess_hndl1,
            cluster_name,
            space_name,
            local_store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl1 ) {
            fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl1));
            return;
        }
        fprintf(stderr, "CreateStore(%s) ok\n", local_store_name);
    }

    ret = OpenStore(&sess_hndl2,
        cluster_name,
        space_name,
        local_store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl2 ) {
        fprintf(stderr, "OpenStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl2));
    } else {
        fprintf(stderr, "OpenStore(%s) ok\n", local_store_name);
    }

    if ( ret < 0 ) {
        if ( sess_hndl2 ) {
            SessionClose(sess_hndl2);
            sess_hndl2 = 0;
        }
        ret = CreateStore(&sess_hndl2,
            cluster_name,
            space_name,
            local_store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl2 ) {
            fprintf(stderr, "CreateStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl2));
            return;
        }
        fprintf(stderr, "CreateStore(%s) ok\n", local_store_name);
    }

    int r_ret = 0;
    for ( i = 0 ; i < num_regressions ; i++ ) {
        fprintf(stdout, "Iteration(%u) thread(%u) store(%s)\n", i, tid, local_store_name);

        if ( num_ops == 1 ) {
            char new_key[10000];
            sprintf(new_key, "%s_%s_%u", "regression17_key_session1", key, i);
            size_t new_key_size = (size_t )strlen(new_key);

            ret = WriteStore(sess_hndl1,
                new_key,
                new_key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "WriteStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl1));
                r_ret = 1;
            }
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t j;

            for ( j = 0 ; j < num_ops ; j++ ) {
                key_ptrs[j] = malloc(key_size + 128);
                sprintf(key_ptrs[j], "%s_%d_%u_%u", "regression17_key_session1", tid, j, i);
                key_sizes[j] = (size_t )strlen(key_ptrs[j]);

                value_ptrs[j] = malloc(value_size + 128);
                sprintf(value_ptrs[j], "%s_%d_%u_%u", "regression17_value_session1", tid, j, i);
                if ( value_size > strlen(value_ptrs[j]) ) {
                    memset(value_ptrs[j] + strlen(value_ptrs[j]), 'V', value_size - strlen(value_ptrs[j]));
                }
                value_sizes[j] = value_size;
            }

            ret = WriteStoreN(sess_hndl1,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "WriteStoreN(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl1));
                r_ret = 1;
            }

            for ( j = 0 ; j < num_ops ; j++ ) {
                free(key_ptrs[j]);
                free(value_ptrs[j]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }
        }

        ret = ReadStore(sess_hndl1, "", 0, "", 0, it_dir, default_first, debug, hex_dump, no_print, 0);
        if ( ret ) {
            fprintf(stderr, "ReadStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl1));
            r_ret = 1;
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }

        if ( r_ret ) {
            r_ret = 0;
        }
    }

    r_ret = 0;
    for ( i = 0 ; i < num_regressions ; i++ ) {
        fprintf(stdout, "Iteration(%u) thread(%u) store(%s)\n", i, tid, local_store_name);

        if ( num_ops == 1 ) {
            char new_key[10000];
            sprintf(new_key, "%s_%s_%u", "regression17_key_session2", key, i);
            size_t new_key_size = (size_t )strlen(new_key);

            ret = WriteStore(sess_hndl2,
                new_key,
                new_key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "WriteStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl2));
                r_ret = 1;
            }
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t j;

            for ( j = 0 ; j < num_ops ; j++ ) {
                key_ptrs[j] = malloc(key_size + 128);
                sprintf(key_ptrs[j], "%s_%d_%u_%u", "regression17_key_session2", tid, j, i);
                key_sizes[j] = (size_t )strlen(key_ptrs[j]);

                value_ptrs[j] = malloc(value_size + 128);
                sprintf(value_ptrs[j], "%s_%d_%u_%u", "regression17_value_session2", tid, j, i);
                if ( value_size > strlen(value_ptrs[j]) ) {
                    memset(value_ptrs[j] + strlen(value_ptrs[j]), 'V', value_size - strlen(value_ptrs[j]));
                }
                value_sizes[j] = value_size;
            }

            ret = WriteStoreN(sess_hndl2,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "WriteStoreN(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl2));
                r_ret = 1;
            }

            for ( j = 0 ; j < num_ops ; j++ ) {
                free(key_ptrs[j]);
                free(value_ptrs[j]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }
        }

        ret = ReadStore(sess_hndl2, "", 0, "", 0, it_dir, default_first, debug, hex_dump, no_print, 0);
        if ( ret ) {
            fprintf(stderr, "ReadStore(%s) failed(%u)\n", local_store_name, SessionLastStatus(sess_hndl2));
            r_ret = 1;
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }

        if ( r_ret ) {
            r_ret = 0;
        }
    }

    CloseStore(sess_hndl1, wait_for_key);
    sess_hndl1 = 0;

    CloseStore(sess_hndl2, wait_for_key);
    sess_hndl2 = 0;

    printf("RegressionTest17(%u) DONE\n", i);
}


/*
 * Start thread functions
 */
static void *RegressionStartTest1(void *arg)
{
    RegressionArgs *args = (RegressionArgs *)arg;
    RegressionTest1(args);
    return 0;
}

static void *RegressionStartTest2(void *arg)
{
    RegressionArgs *args = (RegressionArgs *)arg;
    RegressionTest2(args);
    return 0;
}

static void *RegressionStartTest3(void *arg)
{
    RegressionArgs *args = (RegressionArgs *)arg;
    RegressionTest3(args);
    return 0;
}

static void *RegressionStartTest4(void *arg)
{
    RegressionArgs *args = (RegressionArgs *)arg;
    RegressionTest4(args);
    return 0;
}

static void *RegressionStartTest5(void *arg)
{
    RegressionArgs *args = (RegressionArgs *)arg;
    RegressionTest5(args);
    return 0;
}

static void *RegressionStartTest6(void *arg)
{
    RegressionArgs *args = (RegressionArgs *)arg;
    RegressionTest6(args);
    return 0;
}

static void *RegressionStartTest7(void *arg)
{
    RegressionArgs *args = (RegressionArgs *)arg;
    RegressionTest7(args);
    return 0;
}

static void *RegressionStartTest9(void *arg)
{
    RegressionArgs *args = (RegressionArgs *)arg;
    RegressionTest9(args);
    return 0;
}

static void *RegressionStartTest10(void *arg)
{
    RegressionArgs *args = (RegressionArgs *)arg;
    RegressionTest10(args);
    return 0;
}

static void *RegressionStartTest11(void *arg)
{
    RegressionArgs *args = (RegressionArgs *)arg;
    RegressionTest11(args);
    return 0;
}

static void *RegressionStartTest12(void *arg)
{
    RegressionArgs *args = (RegressionArgs *)arg;
    RegressionTest12(args);
    return 0;
}

static void *RegressionStartTest15(void *arg)
{
    RegressionArgs *args = (RegressionArgs *)arg;
    RegressionTest15(args);
    return 0;
}

static void *RegressionStartTest16(void *arg)
{
    RegressionArgs *args = (RegressionArgs *)arg;
    RegressionTest16(args);
    return 0;
}

static void *RegressionStartTest17(void *arg)
{
    RegressionArgs *args = (RegressionArgs *)arg;
    RegressionTest17(args);
    return 0;
}


/*
 * @brief usage()
 */
static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage %s [options]\n"
        "  -o, --cluster-name             Soe cluster name\n"
        "  -z, --space-name               Soe space name\n"
        "  -c, --store-name               Soe store name\n"
        "  -l, --transactional            Soe transactional store (default 0 - non-transactional)\n"
        "  -n, --num-ops                  Number of ops (default = 1)\n"
        "  -k, --key                      Key (default key_1)\n"
        "  -e, --end-key                  End key (default key_1)\n"
        "  -a, --hex-key                  Key specified in hex format, e.g. 0F45E8CD08C5C5C5\n"
        "  -t, --iterator-dir             Iterator dir (0 - forward(default), 1 - reverse)\n"
        "  -u, --default-first-iter-arg   Use iterator's First() args defaults\n"
        "  -v, --value                    Value (default value_1)\n"
        "  -d  --snap-backup-subset-name  Snapshot, backup or subset name (default snapshot_1, backup_1, subset_1)\n"
        "  -f  --snap-back-id             Snapshot or backup id (default 0)\n"
        "  -i, --sync-type                Sync type (0 - default, 1 - fdatasync, 2 - async WAL)\n"
        "  -m, --provider                 Store provider (0 - RCSDBEM, 1 - KVS, 2 - METADBCLI, 3 - POSTGRES, 4 - DEFAULT provider)\n"
        "  -j, --debug                    Debug (default 0)\n"
        "  -g, --write-group              Do writes as group write (batching) instead of individual writes (1 - Put, 2 - Merge)\n"
        "  -w, --write-transaction        Do writes as transaction (1,2,4,5 - commit, 3 - rollback, 1,3,4,5 - Put, 2 - Merge, 4 - Get, 5 - Delete)\n"
        "  -s, --delete-from-group        Do delete from group in the middle of batching\n"
        "  -A, --create-store             Create cluster/space/store\n"
        "  -B, --create-subset            Create subset\n"
        "  -C, --write-store              Write store num_ops (default 1) KV pairs\n"
        "  -D, --write-subset             Write subset num_ops (default 1) KV pairs\n"
        "  -O, --merge-store              Merge in store num_ops (default 1) KV pairs\n"
        "  -P, --merge-subset             Merge in subset num_ops (default 1) KV pairs\n"
        "  -E, --read-store               Read all KV items from store\n"
        "  -Q  --repair-store             Repair store\n"
        "  -S  --write-store-async        Write store async num_ops (default 1) KV pairs\n"
        "  -W  --read-store-async         Read KV items from store async num_ops (defualt 1)\n"
        "  -Y  --merge-store-async        Merge in store num_ops (default 1)\n"
        "  -Z  --delete-store-async       Delete KV items async (defualt 1)\n"
        "  -R  --traverse-name-space      Traverse name space (print all clusters/spaces/stores/subsets that are there)\n"
        "  -F, --read-subset              Read all KV items from subset\n"
        "  -G, --read-kv-item-store       Read one KV item from store\n"
        "  -H, --read-kv-item-subset      Read one KV item from subset\n"
        "  -I, --delete-kv-item-store     Delete one KV item from store\n"
        "  -J, --delete-kv-item-subset    Delete one KV item from subset\n"
        "  -K, --destroy-store            Destroy store\n"
        "  -L, --destroy-subset           Destroy subset\n"
        "  -M, --hex-dump                 Do hex dump of read records (1 - hex print, 2 - pipe to stdout)\n"
        "  -X, --regression               Regression test\n"
        "                                 1 - create/destroy stores multi-threaded (default)\n"
        "                                 2 - create/destroy stores/subsets multi-threaded\n"
        "                                 3 - create/destroy and write/read different name stores/subsets multi-threaded\n"
        "                                 4 - create/destroy and write/read same name single store/subset multi-threaded\n"
        "                                 5 - create/destroy and write/read same name single store multi-threaded\n"
        "                                 6 - performance test\n"
        "                                 7 - single session multiple threads performance test\n"
        "                                 8 - single session loop(open/create/write/read/close)\n"
        "                                 9 - create/write/destroy different subset same store multi-threaded\n"
        "                                10 - create/open/write/read/close same name\n"
        "                                11 - create/open/write/read/close/destroy same name\n"
        "                                12 - create/open/write/read and wait/close/destroy same name\n"
        "                                13 - destroy/create name and write async # ops\n"
        "                                14 - memory leak test\n"
        "                                15 - create/open/write/read/close new name in each iteration\n"
        "                                16 - open/create then write/read in each iteration then close\n"
        "                                17 - multiple sessions, same thread opne/open/close/close\n"
        "  -N, --data-size                Set data size to an arbitrary value (will override -v)\n"
        "  -T, --sleep-usecs              Sleep usecs between tests\n"
        "  -U, --sleep_multiplier         Sleep multiplier (Regression12)/No close store (set to 0, Regresion13)\n"
        "  -b, --sync_async_destroy       Sync (default) or async destroy store(s) and delete session(s), (sync - 0, async - 1)\n"
        "  -y, --regression-loop-cnt      Regression loop count\n"
        "  -r, --regression-thread-cnt    Regression thread count\n"
        "  -p, --no-print                 No printing key/value (default print)\n"
        "  -x, --user-name                Run unser user-name\n"
        "  -h, --help                     Print this help\n", prog);
}

static int spo_getopt(int argc, char *argv[],
    const char **cluster_name,
    const char **space_name,
    const char **store_name,
    int *provider,
    const char **key,
    size_t *key_size,
    const char **end_key,
    size_t *end_key_size,
    int *it_dir,
    int *default_first,
    char *value,
    size_t value_buf_size,
    size_t *value_size,
    uint32_t *num_ops,
    uint32_t *num_regressions,
    uint32_t *regression_type,
    const char **snap_back_subset_name,
    uint32_t *snap_back_id,
    int *transactional,
    int *sync_type,
    TestType *test_type,
    int *wait_for_key,
    int *thread_count,
    int *hex_dump,
    char user_name[],
    useconds_t *sleep_usecs,
    int *no_print,
    int *group_write,
    int *delete_from_group,
    int *transaction_write,
    int *key_in_hex,
    uint32_t *sleep_multiplier,
    int *sync_async_destroy,
    int *debug)
{
    int c = 0;
    int ret = 3;

    while (1) {
        static char short_options[] = "o:z:c:t:m:n:k:e:v:d:f:y:r:i:j:x:g:w:b:M:X:N:T:U:asuphlABCOPDEFGHIJKLQRSWYZ";
        static struct option long_options[] = {
            {"cluster-name",            1, 0, 'o'},
            {"space-name",              1, 0, 'z'},
            {"store-name",              1, 0, 'c'},
            {"transactional",           0, 0, 'l'},
            {"num-ops",                 1, 0, 'n'},
            {"key",                     1, 0, 'k'},
            {"end-key",                 1, 0, 'e'},
            {"hex-key",                 1, 0, 'a'},
            {"iterator-dir",            1, 0, 't'},
            {"default-first-iter-arg",  0, 0, 'u'},
            {"value",                   1, 0, 'v'},
            {"snap-backup-subset-name", 1, 0, 'd'},
            {"snap-back-id",            0, 0, 'f'},
            {"sync-type",               1, 0, 'i'},
            {"provider",                1, 0, 'm'},
            {"debug",                   1, 0, 'j'},
            {"write-group",             1, 0, 'g'},
            {"write-transaction",       1, 0, 'w'},
            {"delete-from-group",       1, 0, 's'},
            {"create-store",            0, 0, 'A'},
            {"create-subset",           0, 0, 'B'},
            {"write-store",             0, 0, 'C'},
            {"write-subset",            0, 0, 'D'},
            {"merge-store",             0, 0, 'O'},
            {"merge-subset",            0, 0, 'P'},
            {"read-store",              0, 0, 'E'},
            {"repair-store",            0, 0, 'Q'},
            {"write-store-async",       0, 0, 'S'},
            {"read-store-async",        0, 0, 'W'},
            {"merge-store-async",       0, 0, 'Y'},
            {"delete-store-async",      0, 0, 'Z'},
            {"traverse-name-space",     0, 0, 'R'},
            {"read-subset",             0, 0, 'F'},
            {"read-kv-item-store",      0, 0, 'G'},
            {"read-kv-item-subset",     0, 0, 'H'},
            {"delete-kv-item-store",    0, 0, 'I'},
            {"delete-kv-item-subset",   0, 0, 'J'},
            {"destroy-store",           0, 0, 'K'},
            {"destroy-subset",          0, 0, 'L'},
            {"hex-dump",                1, 0, 'M'},
            {"regression",              1, 0, 'X'},
            {"data-size",               1, 0, 'N'},
            {"sleep-usecs",             1, 0, 'T'},
            {"sleep_multiplier",        1, 0, 'U'},
            {"sync_async_destroy",      1, 0, 'b'},
            {"regression-loop-cnt",     1, 0, 'y'},
            {"regression-thread-cnt",   1, 0, 'r'},
            {"no-print",                0, 0, 'p'},
            {"user-name",               1, 0, 'x'},
            {"help",                    0, 0, 'h'},
            {0, 0, 0, 0}
        };

        if ( (c = getopt_long(argc, argv, short_options, long_options, NULL)) == -1 ) {
            break;
        }

        ret = 0;

        switch ( c ) {
        case 'a':
            *key_in_hex = 1;
            break;

        case 'o':
            *cluster_name = optarg;
            break;

        case 'z':
            *space_name = optarg;
            break;

        case 'c':
            *store_name = optarg;
            break;

        case 'l':
            *transactional = 1;
            break;

        case 'n':
            *num_ops = (uint32_t )atoi(optarg);
            break;

        case 'k':
            *key = optarg;
            *key_size = strlen(*key);
            break;

        case 'e':
            *end_key = optarg;
            *end_key_size = strlen(*end_key);
            break;

        case 't':
            *it_dir = atoi(optarg);
            break;

        case 'u':
            *default_first = 0;
            break;

        case 'v':
            *value_size = (size_t )strlen(optarg);
            strcpy(value, optarg);
            break;

        case 'd':
            *snap_back_subset_name = optarg;
            break;

        case 'f':
           *snap_back_id = (uint32_t )atoi(optarg);
            break;

        case 'g':
           *group_write = atoi(optarg);
            break;

        case 'w':
           *transaction_write = atoi(optarg);
            break;

        case 's':
           *delete_from_group = 1;
            break;

        case 'i':
           *sync_type = atoi(optarg);
            break;

        case 'j':
           *debug = atoi(optarg);
            break;

        case 'm':
           *provider = atoi(optarg);
            break;

        case 'r':
           *thread_count = atoi(optarg);
            break;

        case 'p':
           *no_print = 1;
            break;

        case 'A':
            *test_type = eCreateStore;
            break;

        case 'B':
            *test_type = eCreateSubset;
            break;

        case 'C':
            *test_type = eWriteStore;
            break;

        case 'D':
            *test_type = eWriteSubset;
            break;

        case 'O':
            *test_type = eMergeStore;
            break;

        case 'P':
            *test_type = eMergeSubset;
            break;

        case 'E':
            *test_type = eReadStore;
            break;

        case 'F':
            *test_type = eReadSubset;
            break;

        case 'G':
            *test_type = eReadKVItemStore;
            break;

        case 'H':
            *test_type = eReadKVItemSubset;
            break;

        case 'I':
            *test_type = eDeleteKVItemStore;
            break;

        case 'J':
            *test_type = eDeleteKVItemSubset;
            break;

        case 'K':
            *test_type = eDestroyStore;
            break;

        case 'L':
            *test_type = eDestroySubset;
            break;

        case 'Q':
            *test_type = eRepairStore;
            break;

        case 'R':
            *test_type = eTraverseNameSpace;
            break;

        case 'S':
            *test_type = eWriteStoreAsync;
            break;

        case 'Y':
            *test_type = eMergeStoreAsync;
            break;

        case 'W':
            *test_type = eReadStoreAsync;
            break;

        case 'Z':
            *test_type = eDeleteStoreAsync;
            break;

        case 'X':
            *regression_type = (uint32_t )atoi(optarg);
            if ( *regression_type == 1 ) {
                *test_type = eRegression1;
            } else if ( *regression_type == 2 ) {
                *test_type = eRegression2;
            } else if ( *regression_type == 3 ) {
                *test_type = eRegression3;
            } else if ( *regression_type == 4 ) {
                *test_type = eRegression4;
            } else if ( *regression_type == 5 ) {
                *test_type = eRegression5;
            } else if ( *regression_type == 6 ) {
                *test_type = eRegression6;
            } else if ( *regression_type == 7 ) {
                *test_type = eRegression7;
            } else if ( *regression_type == 8 ) {
                *test_type = eRegression8;
            } else if ( *regression_type == 9 ) {
                *test_type = eRegression9;
            } else if ( *regression_type == 10 ) {
                *test_type = eRegression10;
            } else if ( *regression_type == 11 ) {
                *test_type = eRegression11;
            } else if ( *regression_type == 12 ) {
                *test_type = eRegression12;
            } else if ( *regression_type == 13 ) {
                *test_type = eRegression13;
            } else if ( *regression_type == 14 ) {
                *test_type = eRegression14;
            } else if ( *regression_type == 15 ) {
                *test_type = eRegression15;
            } else if ( *regression_type == 16 ) {
                *test_type = eRegression16;
            } else if ( *regression_type == 17 ) {
                *test_type = eRegression17;
            } else { // default
                *test_type = eRegression1;
            }
            break;

        case 'M':
            *hex_dump = atoi(optarg);
            break;

        case 'N':
            *value_size = (size_t )atoi(optarg);
            break;

        case 'T':
            *sleep_usecs = (useconds_t )atoi(optarg);
            break;

        case 'U':
            *sleep_multiplier = (uint32_t )atoi(optarg);
            break;

        case 'b':
            *sync_async_destroy = atoi(optarg);
            break;

        case 'y':
            *num_regressions = (uint32_t )atoi(optarg);
            break;

        case 'x':
            strcpy(user_name, optarg);
            break;

        case 'h':
        case '?':
            usage(argv[0]);
            ret = 8;
            break;
        default:
            ret = -1;
            break;
        }
    }

    return ret;
}


static int SwitchUid(const char *username)
{
    int ret;
    size_t buffer_len;
    char pwd_buffer[1024];

    struct passwd pwd;
    struct passwd *pwd_res = 0;
    memset(&pwd, '\0', sizeof(struct passwd));

    buffer_len = sysconf(_SC_GETPW_R_SIZE_MAX) * sizeof(char);

    ret = getpwnam_r(username, &pwd, pwd_buffer, buffer_len, &pwd_res);
    if ( !pwd_res || ret ) {
        fprintf(stderr, "getpwnam_r failed to find requested entry.\n");
        return -3;
    }
    //printf("uid: %d\n", pwd.pw_uid);
    //printf("gid: %d\n", pwd.pw_gid);

    if ( getuid() == pwd_res->pw_uid ) {
        return 0;
    }

    ret = setegid(pwd_res->pw_gid);
    if ( ret ) {
        fprintf(stderr, "can't setegid() errno(%d)\n", errno);
        return -4;
    }
    ret = seteuid(pwd_res->pw_uid);
    if ( ret ) {
        fprintf(stderr, "can't seteuid() errno(%d)\n", errno);
        return -4;
    }

    return 0;
}


#define SOE_BIG_BUFFER 100000000


//
// Test routines
//
static void RunTestCreateStore(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        const char *provider_name,
        int wait_for_key,
        int debug)
{
    int ret = CreateStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "CreateStore() failed\n");
        //break;
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestCreateSubset(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        const char *snap_back_subset_name,
        const char *provider_name,
        int wait_for_key,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    ret = CreateSubset(sess_hndl,
        &sub_hndl,
        snap_back_subset_name,
        (size_t )strlen(snap_back_subset_name));
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "CreateSubset() failed\n");
        return;
    }

    ret = CloseSubset(sess_hndl, sub_hndl);
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "CloseSubset() failed\n");
        return;
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestWriteStore(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    if ( num_ops == 1 ) {
        if ( value_size > strlen(value) ) {
            memset(value + strlen(value), 'S', value_size - strlen(value));
        }

        if ( !group_write && !transaction_write ) {
            if ( thread_count == 1 ) {
                ret = WriteStore(sess_hndl,
                    key,
                    key_size,
                    value,
                    value_size);
                if ( ret ) {
                    fprintf(stderr, "WriteStore() failed\n");
                }

                if ( sleep_usecs ) {
                    usleep(sleep_usecs);
                }
            } else {
                RegressionArgs args;
                args.cluster_name = cluster_name;
                args.space_name = space_name;
                args.store_name = store_name;
                args.provider = provider;
                args.provider_name = provider_name;
                args.key = key;
                args.key_size = key_size;
                args.end_key = end_key;
                args.end_key_size = end_key_size;
                args.it_dir = it_dir;
                args.value = value;
                args.value_buffer_size = value_buffer_size;
                args.value_size = value_size;
                args.num_ops = num_ops;
                args.num_regressions = num_regressions;
                args.snap_back_subset_name = snap_back_subset_name;
                args.snap_back_id = snap_back_id;
                args.transactional = transactional;
                args.sync_type = sync_type;
                args.debug = debug;
                args.wait_for_key = wait_for_key;
                args.sess_hndl = 0;
                args.sub_hndl = sub_hndl;
                args.sess_sync_mode = sess_sync_mode;
                args.hex_dump = hex_dump;
                args.sleep_usecs = sleep_usecs;
                args.no_print = no_print;
                args.default_first = default_first;
                args.io_counter = 0;

                RegressionArgs thread_args[MAX_THREAD_COUNT];
                memset(thread_args, '\0', sizeof(thread_args));

                int i;
                pthread_t thread_ids[32];
                int rc;

                if ( thread_count > 32 ) {
                    thread_count = 32;
                }

                for ( i = 0 ; i < thread_count ; i++ ) {
                    thread_args[i] = args;

                    char *th_key = malloc(strlen(key + 32));
                    sprintf(th_key, "%s_%u_%u", key, i, 0);
                    thread_args[i].key = th_key;
                    thread_args[i].key_size = strlen(thread_args[i].key);
                    rc = pthread_create(&thread_ids[i], 0, WriteStoreThread, &thread_args[i]);
                    if ( rc ) {
                        fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                        break;
                    }
                }

                for ( i = 0 ; i < thread_count ; i++ ) {
                    int *ret;
                    (void )pthread_join(thread_ids[i], (void **)&ret);
                    CloseStore(thread_args[i].sess_hndl, wait_for_key);

                    free((void *)thread_args[i].key);
                }
            }
        } else {
            if ( group_write && !transaction_write ) {
                GroupHND grp_hndl;
                int ret = CreateGroup(sess_hndl, &grp_hndl);
                if ( ret ) {
                    fprintf(stderr, "CreateGroup() failed\n");
                } else {
                    if ( group_write == 1 ) {
                        ret = GroupPutTo(grp_hndl, key, key_size, value, value_size);
                    } else if ( group_write == 2 ) {
                        ret = GroupMergeTo(grp_hndl, key, key_size, value, value_size);
                    } else {
                        ;
                    }
                    if ( ret ) {
                        if ( group_write == 1 ) {
                            fprintf(stderr, "GroupPutTo() failed\n");
                        } else if ( group_write == 2 ) {
                            fprintf(stderr, "GroupMergeTo() failed\n");
                        } else {
                            ;
                        }
                    }

                    if ( !ret ) {
                        ret = WriteGroup(sess_hndl, grp_hndl);
                        if ( ret ) {
                            fprintf(stderr, "WriteGroup() failed\n");
                        }
                    }

                    if ( sleep_usecs ) {
                        usleep(sleep_usecs);
                    }

                    ret = DeleteGroup(sess_hndl, grp_hndl);
                    if ( ret ) {
                        fprintf(stderr, "DeleteGroup() failed\n");
                    }
                }
            } else if ( transaction_write && !group_write ) {
                TransactionHND trans_hndl;
                char buf[10000];
                size_t buf_size = sizeof(buf);
                int ret = StartTransaction(sess_hndl, &trans_hndl);
                if ( ret ) {
                    fprintf(stderr, "CreateTransaction() failed\n");
                } else {
                    if ( transaction_write == 1 || transaction_write == 3 || transaction_write == 4 || transaction_write == 5 ) {
                        ret = TransactionPutTo(trans_hndl, key, key_size, value, value_size);
                    } else if ( transaction_write == 2 ) {
                        ret = TransactionMergeTo(trans_hndl, key, key_size, value, value_size);
                    } else {
                        fprintf(stderr, "Invalid transaction_write option(%d)\n", transaction_write);
                    }
                    if ( ret ) {
                        if ( transaction_write == 1 || transaction_write == 3 || transaction_write == 4 || transaction_write == 5 ) {
                            fprintf(stderr, "TransactionPutTo() failed\n");
                        } else if ( transaction_write == 2 ) {
                            fprintf(stderr, "TransactionMergeTo() failed\n");
                        } else {
                            fprintf(stderr, "Invalid transaction_write option(%d)\n", transaction_write);
                        }
                    }

                    if ( !ret ) {
                        if ( transaction_write == 4 ) {
                            ret = TransactionGetFrom(trans_hndl, key, key_size, buf, buf_size);
                        } else if ( transaction_write == 5 ) {
                            ret = TransactionDeleteFrom(trans_hndl, key, key_size);
                        } else {
                            ;
                        }
                    }
                    if ( !ret ) {
                        if ( transaction_write == 1 || transaction_write == 2 || transaction_write == 4 || transaction_write == 5 ) {
                            ret = CommitTransaction(sess_hndl, trans_hndl);
                        } else if ( transaction_write == 3 ) {
                            ret = RollbackTransaction(sess_hndl, trans_hndl);
                        } else {
                            fprintf(stderr, "Invalid transaction_write option(%d)\n", transaction_write);
                        }
                        if ( ret ) {
                            fprintf(stderr, "Transaction() failed\n");
                        }
                    }

                    if ( sleep_usecs ) {
                        usleep(sleep_usecs);
                    }
                }
            } else {
                fprintf(stderr, "Invalid configuration\n");
            }
        }
    } else {
        char **key_ptrs = malloc(num_ops*sizeof(char *));
        memset(key_ptrs, '\0', num_ops*sizeof(char *));
        size_t *key_sizes = malloc(num_ops*sizeof(size_t));
        memset(key_sizes, '\0', num_ops*sizeof(size_t));
        char **value_ptrs = malloc(num_ops*sizeof(char *));
        memset(value_ptrs, '\0', num_ops*sizeof(char *));
        size_t *value_sizes = malloc(num_ops*sizeof(size_t));
        memset(value_sizes, '\0', num_ops*sizeof(size_t));
        uint32_t i;

        for ( i = 0 ; i < num_ops ; i++ ) {
            key_ptrs[i] = malloc(key_size + 16);
            sprintf(key_ptrs[i], "%s_%u", key, i);
            key_sizes[i] = (size_t )strlen(key_ptrs[i]);

            value_ptrs[i] = malloc(value_size + 16);
            sprintf(value_ptrs[i], "%s_%u", value, i);
            if ( value_size > strlen(value_ptrs[i]) ) {
                memset(value_ptrs[i] + strlen(value_ptrs[i]), 'V', value_size - strlen(value_ptrs[i]));
            }
            value_sizes[i] = value_size;
        }

        if ( !group_write ) {
            if ( thread_count == 1 ) {
                ret = WriteStoreN(sess_hndl,
                    num_ops,
                    key_ptrs,
                    key_sizes,
                    value_ptrs,
                    value_sizes);
                if ( ret ) {
                    fprintf(stderr, "WriteStoreN() failed\n");
                    return;
                }

                if ( sleep_usecs ) {
                    usleep(sleep_usecs);
                }
            } else {
                RegressionArgs args;
                args.cluster_name = cluster_name;
                args.space_name = space_name;
                args.store_name = store_name;
                args.provider = provider;
                args.provider_name = provider_name;
                args.key = key;
                args.key_size = key_size;
                args.end_key = end_key;
                args.end_key_size = end_key_size;
                args.it_dir = it_dir;
                args.value = value;
                args.value_buffer_size = value_buffer_size;
                args.value_size = value_size;
                args.num_ops = num_ops;
                args.num_regressions = num_regressions;
                args.snap_back_subset_name = snap_back_subset_name;
                args.snap_back_id = snap_back_id;
                args.transactional = transactional;
                args.sync_type = sync_type;
                args.debug = debug;
                args.wait_for_key = wait_for_key;
                args.sess_hndl = 0;
                args.sub_hndl = sub_hndl;
                args.sess_sync_mode = sess_sync_mode;
                args.hex_dump = hex_dump;
                args.sleep_usecs = sleep_usecs;
                args.no_print = no_print;
                args.default_first = default_first;
                args.io_counter = 0;

                RegressionArgs thread_args[MAX_THREAD_COUNT];
                memset(thread_args, '\0', sizeof(thread_args));

                int i;
                uint32_t j;
                pthread_t thread_ids[32];
                int rc;

                if ( thread_count > 32 ) {
                    thread_count = 32;
                }

                for ( i = 0 ; i < thread_count ; i++ ) {
                    thread_args[i] = args;

                    thread_args[i].key_ptrs = malloc(thread_args[i].num_ops*sizeof(char *));
                    memset(thread_args[i].key_ptrs, '\0', thread_args[i].num_ops*sizeof(char *));
                    thread_args[i].key_sizes = malloc(thread_args[i].num_ops*sizeof(size_t));
                    memset(thread_args[i].key_sizes, '\0', thread_args[i].num_ops*sizeof(size_t));
                    thread_args[i].value_ptrs = malloc(thread_args[i].num_ops*sizeof(char *));
                    memset(thread_args[i].value_ptrs, '\0', thread_args[i].num_ops*sizeof(char *));
                    thread_args[i].value_sizes = malloc(thread_args[i].num_ops*sizeof(size_t));
                    memset(thread_args[i].value_sizes, '\0', thread_args[i].num_ops*sizeof(size_t));

                    for ( j = 0 ; j < thread_args[i].num_ops ; j++ ) {
                        thread_args[i].key_ptrs[j] = malloc(thread_args[i].key_size + 32);
                        sprintf(thread_args[i].key_ptrs[j], "%s_%u_%u", thread_args[i].key, i, j);
                        thread_args[i].key_sizes[j] = (size_t )strlen(thread_args[i].key_ptrs[j]);

                        thread_args[i].value_ptrs[j] = malloc(thread_args[i].value_size + 32);
                        sprintf(thread_args[i].value_ptrs[j], "%s_%u_%u", thread_args[i].value, i, j);
                        if ( thread_args[i].value_size > strlen(thread_args[i].value_ptrs[j]) ) {
                            memset(thread_args[i].value_ptrs[j] + strlen(thread_args[i].value_ptrs[j]), 'V',
                                thread_args[i].value_size - strlen(thread_args[i].value_ptrs[j]));
                        }
                        thread_args[i].value_sizes[j] = value_size;
                    }

                    rc = pthread_create(&thread_ids[i], 0, WriteStoreNThread, &thread_args[i]);
                    if ( rc ) {
                        fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                        break;
                    }
                }

                if ( sleep_usecs ) {
                    usleep(sleep_usecs);
                }

                for ( i = 0 ; i < thread_count ; i++ ) {
                    int *ret;
                    (void )pthread_join(thread_ids[i], (void **)&ret);
                    CloseStore(thread_args[i].sess_hndl, wait_for_key);

                    for ( j = 0 ; j < thread_args[i].num_ops ; j++ ) {
                        free(thread_args[i].key_ptrs[j]);
                        free(thread_args[i].value_ptrs[j]);
                    }

                    free(thread_args[i].key_ptrs);
                    free(thread_args[i].key_sizes);
                    free(thread_args[i].value_ptrs);
                    free(thread_args[i].value_sizes);
                }
            }
        } else {
            GroupHND grp_hndl;
            int ret = CreateGroup(sess_hndl, &grp_hndl);
            if ( ret ) {
                fprintf(stderr, "CreateGroup() failed\n");
            } else {
                for ( i = 0 ; i < num_ops ; i++ ) {
                    if ( group_write == 1 ) {
                        ret = GroupPutTo(grp_hndl, key_ptrs[i], key_sizes[i], value_ptrs[i], value_sizes[i]);
                    } else if ( group_write == 2 ) {
                        ret = GroupMergeTo(grp_hndl, key_ptrs[i], key_sizes[i], value_ptrs[i], value_sizes[i]);
                    } else {
                        ;
                    }
                    if ( ret ) {
                        if ( group_write == 1 ) {
                            fprintf(stderr, "GroupPutTo() failed\n");
                        } else if ( group_write == 2 ) {
                            fprintf(stderr, "GroupMergeTo() failed\n");
                        } else {
                            ;
                        }
                        break;
                    }
                }

                // delete from group, we delete half of the elements
                if ( delete_from_group ) {
                    for ( i = 0 ; i < num_ops/2 ; i++ ) {
                        ret = GroupDeleteFrom(grp_hndl, key_ptrs[i], key_sizes[i]);
                        if ( ret ) {
                            fprintf(stderr, "GroupDeleteFrom() failed\n");
                            break;
                        }
                    }
                }

                if ( !ret ) {
                    ret = WriteGroup(sess_hndl, grp_hndl);
                    if ( ret ) {
                        fprintf(stderr, "WriteGroup() failed\n");
                    }
                }

                if ( sleep_usecs ) {
                    usleep(sleep_usecs);
                }

                ret = DeleteGroup(sess_hndl, grp_hndl);
                if ( ret ) {
                    fprintf(stderr, "DeleteGroup() failed\n");
                }
            }
        }

        for ( i = 0 ; i < num_ops ; i++ ) {
            free(key_ptrs[i]);
            free(value_ptrs[i]);
        }

        free(key_ptrs);
        free(key_sizes);
        free(value_ptrs);
        free(value_sizes);
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestWriteSubset(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    ret = OpenSubset(sess_hndl,
        &sub_hndl,
        snap_back_subset_name,
        (size_t )strlen(snap_back_subset_name));
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "OpenSubset() failed\n");
        return;
    }

    if ( num_ops == 1 ) {
        if ( value_size > strlen(value) ) {
            memset(value + strlen(value), 'S', value_size - strlen(value));
        }
        ret = WriteSubset(sub_hndl,
            key,
            key_size,
            value,
            value_size);
        if ( ret ) {
            (void )CloseSubset(sess_hndl, sub_hndl);
            CloseStore(sess_hndl, wait_for_key);
            fprintf(stderr, "WriteSubset() failed\n");
            return;
        }
    } else {
        char *key_ptrs[MAX_NUM_OPS];
        size_t key_sizes[MAX_NUM_OPS];
        char *value_ptrs[MAX_NUM_OPS];
        size_t value_sizes[MAX_NUM_OPS];
        uint32_t i;

        for ( i = 0 ; i < num_ops ; i++ ) {
            key_ptrs[i] = calloc(1, key_size + 128);
            sprintf(key_ptrs[i], "%s_%u", key, i);
            key_sizes[i] = (size_t )strlen(key_ptrs[i]);

            value_ptrs[i] = calloc(1, value_size + 128);
            sprintf(value_ptrs[i], "%s_%u", value, i);
            if ( value_size > strlen(value_ptrs[i]) ) {
                memset(value_ptrs[i] + strlen(value_ptrs[i]), 'S', value_size - strlen(value_ptrs[i]));
            }
            value_sizes[i] = value_size;
        }

        ret = WriteSubsetN(sub_hndl,
            num_ops,
            key_ptrs,
            key_sizes,
            value_ptrs,
            value_sizes);
        if ( ret ) {
            (void )CloseSubset(sess_hndl, sub_hndl);
            CloseStore(sess_hndl, wait_for_key);
            fprintf(stderr, "WriteSubsetN() failed\n");
            return;
        }
        for ( i = 0 ; i < num_ops ; i++ ) {
            free(key_ptrs[i]);
            free(value_ptrs[i]);
        }
    }

    ret = CloseSubset(sess_hndl, sub_hndl);
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "CloseSubset() failed\n");
        return;
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestMergeStore(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    if ( num_ops == 1 ) {
        if ( value_size > strlen(value) ) {
            memset(value + strlen(value), 'S', value_size - strlen(value));
        }

        if ( thread_count == 1 ) {
            ret = MergeStore(sess_hndl,
                key,
                key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "MergeStore() failed\n");
            }
        } else {
            RegressionArgs args;
            args.cluster_name = cluster_name;
            args.space_name = space_name;
            args.store_name = store_name;
            args.provider = provider;
            args.provider_name = provider_name;
            args.key = key;
            args.key_size = key_size;
            args.end_key = end_key;
            args.end_key_size = end_key_size;
            args.it_dir = it_dir;
            args.value = value;
            args.value_buffer_size = value_buffer_size;
            args.value_size = value_size;
            args.num_ops = num_ops;
            args.num_regressions = num_regressions;
            args.snap_back_subset_name = snap_back_subset_name;
            args.snap_back_id = snap_back_id;
            args.transactional = transactional;
            args.sync_type = sync_type;
            args.debug = debug;
            args.wait_for_key = wait_for_key;
            args.sess_hndl = 0;
            args.sub_hndl = sub_hndl;
            args.sess_sync_mode = sess_sync_mode;
            args.hex_dump = hex_dump;
            args.sleep_usecs = sleep_usecs;
            args.no_print = no_print;
            args.default_first = default_first;
            args.io_counter = 0;

            RegressionArgs thread_args[MAX_THREAD_COUNT];
            memset(thread_args, '\0', sizeof(thread_args));

            int i;
            pthread_t thread_ids[32];
            int rc = -1;

            if ( thread_count > 32 ) {
                thread_count = 32;
            }

            for ( i = 0 ; i < thread_count ; i++ ) {
                thread_args[i] = args;

                char *th_key = malloc(strlen(key + 32));
                sprintf(th_key, "%s_%u_%u", key, i, 0);
                thread_args[i].key = th_key;
                thread_args[i].key_size = strlen(thread_args[i].key);
                rc = pthread_create(&thread_ids[i], 0, MergeStoreThread, &thread_args[i]);
                if ( rc ) {
                    fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                    return;
                }
            }

            for ( i = 0 ; i < thread_count ; i++ ) {
                int *ret;
                (void )pthread_join(thread_ids[i], (void **)&ret);
                CloseStore(thread_args[i].sess_hndl, wait_for_key);

                free((void *)thread_args[i].key);
            }
        }
    } else { // multiple ops
        char **key_ptrs = malloc(num_ops*sizeof(char *));
        memset(key_ptrs, '\0', num_ops*sizeof(char *));
        size_t *key_sizes = malloc(num_ops*sizeof(size_t));
        memset(key_sizes, '\0', num_ops*sizeof(size_t));
        char **value_ptrs = malloc(num_ops*sizeof(char *));
        memset(value_ptrs, '\0', num_ops*sizeof(char *));
        size_t *value_sizes = malloc(num_ops*sizeof(size_t));
        memset(value_sizes, '\0', num_ops*sizeof(size_t));
        uint32_t i;

        for ( i = 0 ; i < num_ops ; i++ ) {
            key_ptrs[i] = malloc(key_size + 16);
            sprintf(key_ptrs[i], "%s_%u", key, i);
            key_sizes[i] = (size_t )strlen(key_ptrs[i]);

            value_ptrs[i] = malloc(value_size + 16);
            sprintf(value_ptrs[i], "%s_%u", value, i);
            if ( value_size > strlen(value_ptrs[i]) ) {
                memset(value_ptrs[i] + strlen(value_ptrs[i]), 'V', value_size - strlen(value_ptrs[i]));
            }
            value_sizes[i] = value_size;
        }

        if ( thread_count == 1 ) {
            ret = MergeStoreN(sess_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "MergeStoreN() failed\n");
                return;
            }
        } else {
            RegressionArgs args;
            args.cluster_name = cluster_name;
            args.space_name = space_name;
            args.store_name = store_name;
            args.provider = provider;
            args.provider_name = provider_name;
            args.key = key;
            args.key_size = key_size;
            args.end_key = end_key;
            args.end_key_size = end_key_size;
            args.it_dir = it_dir;
            args.value = value;
            args.value_buffer_size = value_buffer_size;
            args.value_size = value_size;
            args.num_ops = num_ops;
            args.num_regressions = num_regressions;
            args.snap_back_subset_name = snap_back_subset_name;
            args.snap_back_id = snap_back_id;
            args.transactional = transactional;
            args.sync_type = sync_type;
            args.debug = debug;
            args.wait_for_key = wait_for_key;
            args.sess_hndl = 0;
            args.sub_hndl = sub_hndl;
            args.sess_sync_mode = sess_sync_mode;
            args.hex_dump = hex_dump;
            args.sleep_usecs = sleep_usecs;
            args.no_print = no_print;
            args.default_first = default_first;
            args.io_counter = 0;

            RegressionArgs thread_args[MAX_THREAD_COUNT];
            memset(thread_args, '\0', sizeof(thread_args));

            int i;
            uint32_t j;
            pthread_t thread_ids[32];
            int rc = -1;

            if ( thread_count > 32 ) {
                thread_count = 32;
            }

            for ( i = 0 ; i < thread_count ; i++ ) {
                thread_args[i] = args;

                thread_args[i].key_ptrs = malloc(thread_args[i].num_ops*sizeof(char *));
                memset(thread_args[i].key_ptrs, '\0', thread_args[i].num_ops*sizeof(char *));
                thread_args[i].key_sizes = malloc(thread_args[i].num_ops*sizeof(size_t));
                memset(thread_args[i].key_sizes, '\0', thread_args[i].num_ops*sizeof(size_t));
                thread_args[i].value_ptrs = malloc(thread_args[i].num_ops*sizeof(char *));
                memset(thread_args[i].value_ptrs, '\0', thread_args[i].num_ops*sizeof(char *));
                thread_args[i].value_sizes = malloc(thread_args[i].num_ops*sizeof(size_t));
                memset(thread_args[i].value_sizes, '\0', thread_args[i].num_ops*sizeof(size_t));

                for ( j = 0 ; j < thread_args[i].num_ops ; j++ ) {
                    thread_args[i].key_ptrs[j] = malloc(thread_args[i].key_size + 32);
                    sprintf(thread_args[i].key_ptrs[j], "%s_%u_%u", thread_args[i].key, i, j);
                    thread_args[i].key_sizes[j] = (size_t )strlen(thread_args[i].key_ptrs[j]);

                    thread_args[i].value_ptrs[j] = malloc(thread_args[i].value_size + 32);
                    sprintf(thread_args[i].value_ptrs[j], "%s_%u_%u", thread_args[i].value, i, j);
                    if ( thread_args[i].value_size > strlen(thread_args[i].value_ptrs[j]) ) {
                        memset(thread_args[i].value_ptrs[j] + strlen(thread_args[i].value_ptrs[j]), 'V',
                            thread_args[i].value_size - strlen(thread_args[i].value_ptrs[j]));
                    }
                    thread_args[i].value_sizes[j] = value_size;
                }

                rc = pthread_create(&thread_ids[i], 0, MergeStoreNThread, &thread_args[i]);
                if ( rc ) {
                    fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                    return;
                }
            }

            for ( i = 0 ; i < thread_count ; i++ ) {
                int *ret;
                (void )pthread_join(thread_ids[i], (void **)&ret);
                CloseStore(thread_args[i].sess_hndl, wait_for_key);

                for ( j = 0 ; j < thread_args[i].num_ops ; j++ ) {
                    free(thread_args[i].key_ptrs[j]);
                    free(thread_args[i].value_ptrs[j]);
                }

                free(thread_args[i].key_ptrs);
                free(thread_args[i].key_sizes);
                free(thread_args[i].value_ptrs);
                free(thread_args[i].value_sizes);
            }
        }


        for ( i = 0 ; i < num_ops ; i++ ) {
            free(key_ptrs[i]);
            free(value_ptrs[i]);
        }

        free(key_ptrs);
        free(key_sizes);
        free(value_ptrs);
        free(value_sizes);
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestMergeSubset(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    ret = OpenSubset(sess_hndl,
        &sub_hndl,
        snap_back_subset_name,
        (size_t )strlen(snap_back_subset_name));
    if ( ret ) {
        fprintf(stderr, "OpenSubset() failed\n");
        return;
    }

    if ( num_ops == 1 ) {
        if ( value_size > strlen(value) ) {
            memset(value + strlen(value), 'S', value_size - strlen(value));
        }
        ret = MergeSubset(sub_hndl,
            key,
            key_size,
            value,
            value_size);
        if ( ret ) {
            (void )CloseSubset(sess_hndl, sub_hndl);
            CloseStore(sess_hndl, wait_for_key);
            fprintf(stderr, "MergeSubset() failed\n");
            return;
        }
    } else {
        char *key_ptrs[MAX_NUM_OPS];
        size_t key_sizes[MAX_NUM_OPS];
        char *value_ptrs[MAX_NUM_OPS];
        size_t value_sizes[MAX_NUM_OPS];
        uint32_t i;

        for ( i = 0 ; i < num_ops ; i++ ) {
            key_ptrs[i] = calloc(1, key_size + 128);
            sprintf(key_ptrs[i], "%s_%u", key, i);
            key_sizes[i] = (size_t )strlen(key_ptrs[i]);

            value_ptrs[i] = calloc(1, value_size + 128);
            sprintf(value_ptrs[i], "%s_%u", value, i);
            if ( value_size > strlen(value_ptrs[i]) ) {
                memset(value_ptrs[i] + strlen(value_ptrs[i]), 'S', value_size - strlen(value_ptrs[i]));
            }
            value_sizes[i] = value_size;
        }

        ret = MergeSubsetN(sub_hndl,
            num_ops,
            key_ptrs,
            key_sizes,
            value_ptrs,
            value_sizes);
        if ( ret ) {
            (void )CloseSubset(sess_hndl, sub_hndl);
            CloseStore(sess_hndl, wait_for_key);
            fprintf(stderr, "MergeSubsetN() failed\n");
            return;
        }
        for ( i = 0 ; i < num_ops ; i++ ) {
            free(key_ptrs[i]);
            free(value_ptrs[i]);
        }
    }

    ret = CloseSubset(sess_hndl, sub_hndl);
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "CloseSubset() failed\n");
        return;
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestReadStore(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        const char *provider_name,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        int wait_for_key,
        int hex_dump,
        int no_print,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    ret = ReadStore(sess_hndl, key, key_size, end_key, end_key_size, it_dir, default_first, debug, hex_dump, no_print, 0);
    if ( ret ) {
        fprintf(stderr, "ReadStore() failed\n");
        return;
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestWriteStoreAsync(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    if ( num_ops == 1 ) {
        if ( value_size > strlen(value) ) {
            memset(value + strlen(value), 'S', value_size - strlen(value));
        }

        if ( thread_count == 1 ) {
            ret = WriteStoreAsync(sess_hndl,
                key,
                key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "WriteStoreAsync() failed\n");
            }

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }
        } else {
            RegressionArgs args;
            args.cluster_name = cluster_name;
            args.space_name = space_name;
            args.store_name = store_name;
            args.provider = provider;
            args.provider_name = provider_name;
            args.key = key;
            args.key_size = key_size;
            args.end_key = end_key;
            args.end_key_size = end_key_size;
            args.it_dir = it_dir;
            args.value = value;
            args.value_buffer_size = value_buffer_size;
            args.value_size = value_size;
            args.num_ops = num_ops;
            args.num_regressions = num_regressions;
            args.snap_back_subset_name = snap_back_subset_name;
            args.snap_back_id = snap_back_id;
            args.transactional = transactional;
            args.sync_type = sync_type;
            args.debug = debug;
            args.wait_for_key = wait_for_key;
            args.sess_hndl = 0;
            args.sub_hndl = sub_hndl;
            args.sess_sync_mode = sess_sync_mode;
            args.hex_dump = hex_dump;
            args.sleep_usecs = sleep_usecs;
            args.no_print = no_print;
            args.default_first = default_first;
            args.io_counter = 0;

            RegressionArgs thread_args[MAX_THREAD_COUNT];
            memset(thread_args, '\0', sizeof(thread_args));

            int i;
            pthread_t thread_ids[MAX_THREAD_COUNT];
            int rc;

            if ( thread_count > MAX_THREAD_COUNT ) {
                thread_count = MAX_THREAD_COUNT;
            }

            for ( i = 0 ; i < thread_count ; i++ ) {
                thread_args[i] = args;

                char *th_key = malloc(strlen(key + 32));
                sprintf(th_key, "%s_%u_%u", key, i, 0);
                thread_args[i].key = th_key;
                thread_args[i].key_size = strlen(thread_args[i].key);
                rc = pthread_create(&thread_ids[i], 0, WriteStoreThreadAsync, &thread_args[i]);
                if ( rc ) {
                    fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                    break;
                }
            }

            for ( i = 0 ; i < thread_count ; i++ ) {
                int *ret;
                (void )pthread_join(thread_ids[i], (void **)&ret);
                CloseStore(thread_args[i].sess_hndl, wait_for_key);

                free((void *)thread_args[i].key);
            }
        }
    } else {
        char **key_ptrs = malloc(num_ops*sizeof(char *));
        memset(key_ptrs, '\0', num_ops*sizeof(char *));
        size_t *key_sizes = malloc(num_ops*sizeof(size_t));
        memset(key_sizes, '\0', num_ops*sizeof(size_t));
        char **value_ptrs = malloc(num_ops*sizeof(char *));
        memset(value_ptrs, '\0', num_ops*sizeof(char *));
        size_t *value_sizes = malloc(num_ops*sizeof(size_t));
        memset(value_sizes, '\0', num_ops*sizeof(size_t));
        uint32_t i;

        for ( i = 0 ; i < num_ops ; i++ ) {
            key_ptrs[i] = malloc(key_size + 16);
            sprintf(key_ptrs[i], "%s_%u", key, i);
            key_sizes[i] = (size_t )strlen(key_ptrs[i]);

            value_ptrs[i] = malloc(value_size + 16);
            sprintf(value_ptrs[i], "%s_%u", value, i);
            if ( value_size > strlen(value_ptrs[i]) ) {
                memset(value_ptrs[i] + strlen(value_ptrs[i]), 'V', value_size - strlen(value_ptrs[i]));
            }
            value_sizes[i] = value_size;
        }

        if ( thread_count == 1 ) {
            ret = WriteStoreNAsync(sess_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "WriteStoreNAsync() failed\n");
                return;
            }

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }
        } else {
            RegressionArgs args;
            args.cluster_name = cluster_name;
            args.space_name = space_name;
            args.store_name = store_name;
            args.provider = provider;
            args.provider_name = provider_name;
            args.key = key;
            args.key_size = key_size;
            args.end_key = end_key;
            args.end_key_size = end_key_size;
            args.it_dir = it_dir;
            args.value = value;
            args.value_buffer_size = value_buffer_size;
            args.value_size = value_size;
            args.num_ops = num_ops;
            args.num_regressions = num_regressions;
            args.snap_back_subset_name = snap_back_subset_name;
            args.snap_back_id = snap_back_id;
            args.transactional = transactional;
            args.sync_type = sync_type;
            args.debug = debug;
            args.wait_for_key = wait_for_key;
            args.sess_hndl = 0;
            args.sub_hndl = sub_hndl;
            args.sess_sync_mode = sess_sync_mode;
            args.hex_dump = hex_dump;
            args.sleep_usecs = sleep_usecs;
            args.no_print = no_print;
            args.default_first = default_first;
            args.io_counter = 0;

            RegressionArgs thread_args[MAX_THREAD_COUNT];
            memset(thread_args, '\0', sizeof(thread_args));

            int i;
            uint32_t j;
            pthread_t thread_ids[MAX_THREAD_COUNT];
            int rc;

            if ( thread_count > MAX_THREAD_COUNT ) {
                thread_count = MAX_THREAD_COUNT;
            }

            for ( i = 0 ; i < thread_count ; i++ ) {
                thread_args[i] = args;

                thread_args[i].key_ptrs = malloc(thread_args[i].num_ops*sizeof(char *));
                memset(thread_args[i].key_ptrs, '\0', thread_args[i].num_ops*sizeof(char *));
                thread_args[i].key_sizes = malloc(thread_args[i].num_ops*sizeof(size_t));
                memset(thread_args[i].key_sizes, '\0', thread_args[i].num_ops*sizeof(size_t));
                thread_args[i].value_ptrs = malloc(thread_args[i].num_ops*sizeof(char *));
                memset(thread_args[i].value_ptrs, '\0', thread_args[i].num_ops*sizeof(char *));
                thread_args[i].value_sizes = malloc(thread_args[i].num_ops*sizeof(size_t));
                memset(thread_args[i].value_sizes, '\0', thread_args[i].num_ops*sizeof(size_t));

                for ( j = 0 ; j < thread_args[i].num_ops ; j++ ) {
                    thread_args[i].key_ptrs[j] = malloc(thread_args[i].key_size + 32);
                    sprintf(thread_args[i].key_ptrs[j], "%s_%u_%u", thread_args[i].key, i, j);
                    thread_args[i].key_sizes[j] = (size_t )strlen(thread_args[i].key_ptrs[j]);

                    thread_args[i].value_ptrs[j] = malloc(thread_args[i].value_size + 32);
                    sprintf(thread_args[i].value_ptrs[j], "%s_%u_%u", thread_args[i].value, i, j);
                    if ( thread_args[i].value_size > strlen(thread_args[i].value_ptrs[j]) ) {
                        memset(thread_args[i].value_ptrs[j] + strlen(thread_args[i].value_ptrs[j]), 'V',
                            thread_args[i].value_size - strlen(thread_args[i].value_ptrs[j]));
                    }
                    thread_args[i].value_sizes[j] = value_size;
                }

                rc = pthread_create(&thread_ids[i], 0, WriteStoreNThreadAsync, &thread_args[i]);
                if ( rc ) {
                    fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                    break;
                }
            }

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }

            for ( i = 0 ; i < thread_count ; i++ ) {
                int *ret;
                (void )pthread_join(thread_ids[i], (void **)&ret);
                CloseStore(thread_args[i].sess_hndl, wait_for_key);

                for ( j = 0 ; j < thread_args[i].num_ops ; j++ ) {
                    free(thread_args[i].key_ptrs[j]);
                    free(thread_args[i].value_ptrs[j]);
                }

                free(thread_args[i].key_ptrs);
                free(thread_args[i].key_sizes);
                free(thread_args[i].value_ptrs);
                free(thread_args[i].value_sizes);
            }
        }


        for ( i = 0 ; i < num_ops ; i++ ) {
            free(key_ptrs[i]);
            free(value_ptrs[i]);
        }

        free(key_ptrs);
        free(key_sizes);
        free(value_ptrs);
        free(value_sizes);
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestReadStoreAsync(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        const char *provider_name,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        int wait_for_key,
        int hex_dump,
        int no_print,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    ret = ReadStoreAsync(sess_hndl, key, key_size, end_key, end_key_size, it_dir, default_first, debug, hex_dump, no_print, 0);
    if ( ret ) {
        fprintf(stderr, "ReadStore() failed\n");
        return;
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestMergeStoreAsync(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    if ( num_ops == 1 ) {
        if ( value_size > strlen(value) ) {
            memset(value + strlen(value), 'S', value_size - strlen(value));
        }

        if ( thread_count == 1 ) {
            ret = MergeStoreAsync(sess_hndl,
                key,
                key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "MergeStore() failed\n");
            }
        } else {
            RegressionArgs args;
            args.cluster_name = cluster_name;
            args.space_name = space_name;
            args.store_name = store_name;
            args.provider = provider;
            args.provider_name = provider_name;
            args.key = key;
            args.key_size = key_size;
            args.end_key = end_key;
            args.end_key_size = end_key_size;
            args.it_dir = it_dir;
            args.value = value;
            args.value_buffer_size = value_buffer_size;
            args.value_size = value_size;
            args.num_ops = num_ops;
            args.num_regressions = num_regressions;
            args.snap_back_subset_name = snap_back_subset_name;
            args.snap_back_id = snap_back_id;
            args.transactional = transactional;
            args.sync_type = sync_type;
            args.debug = debug;
            args.wait_for_key = wait_for_key;
            args.sess_hndl = 0;
            args.sub_hndl = sub_hndl;
            args.sess_sync_mode = sess_sync_mode;
            args.hex_dump = hex_dump;
            args.sleep_usecs = sleep_usecs;
            args.no_print = no_print;
            args.default_first = default_first;
            args.io_counter = 0;

            RegressionArgs thread_args[MAX_THREAD_COUNT];
            memset(thread_args, '\0', sizeof(thread_args));

            int i;
            pthread_t thread_ids[MAX_THREAD_COUNT];
            int rc = -1;

            if ( thread_count > MAX_THREAD_COUNT ) {
                thread_count = MAX_THREAD_COUNT;
            }

            for ( i = 0 ; i < thread_count ; i++ ) {
                thread_args[i] = args;

                char *th_key = malloc(strlen(key + 32));
                sprintf(th_key, "%s_%u_%u", key, i, 0);
                thread_args[i].key = th_key;
                thread_args[i].key_size = strlen(thread_args[i].key);
                rc = pthread_create(&thread_ids[i], 0, MergeStoreThreadAsync, &thread_args[i]);
                if ( rc ) {
                    fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                    return;
                }
            }

            for ( i = 0 ; i < thread_count ; i++ ) {
                int *ret;
                (void )pthread_join(thread_ids[i], (void **)&ret);
                CloseStore(thread_args[i].sess_hndl, wait_for_key);

                free((void *)thread_args[i].key);
            }
        }
    } else { // multiple ops
        char **key_ptrs = malloc(num_ops*sizeof(char *));
        memset(key_ptrs, '\0', num_ops*sizeof(char *));
        size_t *key_sizes = malloc(num_ops*sizeof(size_t));
        memset(key_sizes, '\0', num_ops*sizeof(size_t));
        char **value_ptrs = malloc(num_ops*sizeof(char *));
        memset(value_ptrs, '\0', num_ops*sizeof(char *));
        size_t *value_sizes = malloc(num_ops*sizeof(size_t));
        memset(value_sizes, '\0', num_ops*sizeof(size_t));
        uint32_t i;

        for ( i = 0 ; i < num_ops ; i++ ) {
            key_ptrs[i] = malloc(key_size + 16);
            sprintf(key_ptrs[i], "%s_%u", key, i);
            key_sizes[i] = (size_t )strlen(key_ptrs[i]);

            value_ptrs[i] = malloc(value_size + 16);
            sprintf(value_ptrs[i], "%s_%u", value, i);
            if ( value_size > strlen(value_ptrs[i]) ) {
                memset(value_ptrs[i] + strlen(value_ptrs[i]), 'V', value_size - strlen(value_ptrs[i]));
            }
            value_sizes[i] = value_size;
        }

        if ( thread_count == 1 ) {
            ret = MergeStoreNAsync(sess_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "MergeStoreN() failed\n");
                return;
            }
        } else {
            RegressionArgs args;
            args.cluster_name = cluster_name;
            args.space_name = space_name;
            args.store_name = store_name;
            args.provider = provider;
            args.provider_name = provider_name;
            args.key = key;
            args.key_size = key_size;
            args.end_key = end_key;
            args.end_key_size = end_key_size;
            args.it_dir = it_dir;
            args.value = value;
            args.value_buffer_size = value_buffer_size;
            args.value_size = value_size;
            args.num_ops = num_ops;
            args.num_regressions = num_regressions;
            args.snap_back_subset_name = snap_back_subset_name;
            args.snap_back_id = snap_back_id;
            args.transactional = transactional;
            args.sync_type = sync_type;
            args.debug = debug;
            args.wait_for_key = wait_for_key;
            args.sess_hndl = 0;
            args.sub_hndl = sub_hndl;
            args.sess_sync_mode = sess_sync_mode;
            args.hex_dump = hex_dump;
            args.sleep_usecs = sleep_usecs;
            args.no_print = no_print;
            args.default_first = default_first;
            args.io_counter = 0;

            RegressionArgs thread_args[MAX_THREAD_COUNT];
            memset(thread_args, '\0', sizeof(thread_args));

            int i;
            uint32_t j;
            pthread_t thread_ids[MAX_THREAD_COUNT];
            int rc = -1;

            if ( thread_count > MAX_THREAD_COUNT ) {
                thread_count = MAX_THREAD_COUNT;
            }

            for ( i = 0 ; i < thread_count ; i++ ) {
                thread_args[i] = args;

                thread_args[i].key_ptrs = malloc(thread_args[i].num_ops*sizeof(char *));
                memset(thread_args[i].key_ptrs, '\0', thread_args[i].num_ops*sizeof(char *));
                thread_args[i].key_sizes = malloc(thread_args[i].num_ops*sizeof(size_t));
                memset(thread_args[i].key_sizes, '\0', thread_args[i].num_ops*sizeof(size_t));
                thread_args[i].value_ptrs = malloc(thread_args[i].num_ops*sizeof(char *));
                memset(thread_args[i].value_ptrs, '\0', thread_args[i].num_ops*sizeof(char *));
                thread_args[i].value_sizes = malloc(thread_args[i].num_ops*sizeof(size_t));
                memset(thread_args[i].value_sizes, '\0', thread_args[i].num_ops*sizeof(size_t));

                for ( j = 0 ; j < thread_args[i].num_ops ; j++ ) {
                    thread_args[i].key_ptrs[j] = malloc(thread_args[i].key_size + 32);
                    sprintf(thread_args[i].key_ptrs[j], "%s_%u_%u", thread_args[i].key, i, j);
                    thread_args[i].key_sizes[j] = (size_t )strlen(thread_args[i].key_ptrs[j]);

                    thread_args[i].value_ptrs[j] = malloc(thread_args[i].value_size + 32);
                    sprintf(thread_args[i].value_ptrs[j], "%s_%u_%u", thread_args[i].value, i, j);
                    if ( thread_args[i].value_size > strlen(thread_args[i].value_ptrs[j]) ) {
                        memset(thread_args[i].value_ptrs[j] + strlen(thread_args[i].value_ptrs[j]), 'V',
                            thread_args[i].value_size - strlen(thread_args[i].value_ptrs[j]));
                    }
                    thread_args[i].value_sizes[j] = value_size;
                }

                rc = pthread_create(&thread_ids[i], 0, MergeStoreNThreadAsync, &thread_args[i]);
                if ( rc ) {
                    fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                    return;
                }
            }

            for ( i = 0 ; i < thread_count ; i++ ) {
                int *ret;
                (void )pthread_join(thread_ids[i], (void **)&ret);
                CloseStore(thread_args[i].sess_hndl, wait_for_key);

                for ( j = 0 ; j < thread_args[i].num_ops ; j++ ) {
                    free(thread_args[i].key_ptrs[j]);
                    free(thread_args[i].value_ptrs[j]);
                }

                free(thread_args[i].key_ptrs);
                free(thread_args[i].key_sizes);
                free(thread_args[i].value_ptrs);
                free(thread_args[i].value_sizes);
            }
        }


        for ( i = 0 ; i < num_ops ; i++ ) {
            free(key_ptrs[i]);
            free(value_ptrs[i]);
        }

        free(key_ptrs);
        free(key_sizes);
        free(value_ptrs);
        free(value_sizes);
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestDeleteStoreAsync(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        const char *provider_name,
        const char *key,
        size_t key_size,
        int wait_for_key,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    ret = DeleteKVItemStoreAsync(sess_hndl, key, key_size);
    if ( ret ) {
        fprintf(stderr, "DeleteKVItemStore() failed\n");
        return;
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestRepairStore(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        const char *provider_name,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        int wait_for_key,
        int hex_dump,
        int no_print,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
    }

    ret = RepairStore(sess_hndl);
    if ( ret ) {
        fprintf(stderr, "RepairStore() failed\n");
        return;
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestReadSubset(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        const char *provider_name,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        const char *snap_back_subset_name,
        int wait_for_key,
        int hex_dump,
        int no_print,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    ret = OpenSubset(sess_hndl,
        &sub_hndl,
        snap_back_subset_name,
        (size_t )strlen(snap_back_subset_name));
    if ( ret ) {
        fprintf(stderr, "OpenSubset() failed\n");
        return;
    }

    ret = ReadSubset(sub_hndl, key, key_size, end_key, end_key_size, it_dir, default_first, debug, hex_dump, no_print, 0);
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "ReadSubset() failed\n");
        return;
    }

    ret = CloseSubset(sess_hndl, sub_hndl);
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "SessionDeleteSubset() failed\n");
        return;
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestReadKVItemStore(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        const char *provider_name,
        const char *key,
        size_t key_size,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        int wait_for_key,
        int hex_dump,
        int no_print,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    memset(value, '\0', value_buffer_size);
    ret = ReadKVItemStore(sess_hndl, key, key_size, value, value_size, hex_dump, no_print);
    if ( ret ) {
        fprintf(stderr, "ReadKVItemStore() failed\n");
        return;
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestReadKVItemSubset(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        const char *provider_name,
        const char *key,
        size_t key_size,
        const char *snap_back_subset_name,
        int wait_for_key,
        int hex_dump,
        int no_print,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    ret = OpenSubset(sess_hndl,
        &sub_hndl,
        snap_back_subset_name,
        (size_t )strlen(snap_back_subset_name));
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "OpenSubset() failed\n");
        return;
    }

    ret = ReadKVItemSubset(sub_hndl, sub_hndl, key, key_size, hex_dump, no_print);
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "ReadKVItemSubset() failed\n");
        return;
    }

    ret = CloseSubset(sess_hndl, sub_hndl);
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "SessionDeleteSubset() failed\n");
        return;
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestDeleteKVItemStore(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        const char *provider_name,
        const char *key,
        size_t key_size,
        int wait_for_key,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    ret = DeleteKVItemStore(sess_hndl, key, key_size);
    if ( ret ) {
        fprintf(stderr, "DeleteKVItemStore() failed\n");
        return;
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestDeleteKVItemSubset(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        const char *provider_name,
        const char *key,
        size_t key_size,
        const char *snap_back_subset_name,
        int wait_for_key,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    ret = OpenSubset(sess_hndl,
        &sub_hndl,
        snap_back_subset_name,
        (size_t )strlen(snap_back_subset_name));
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "OpenSubset() failed\n");
        return;
    }

    ret = DeleteKVItemSubset(sub_hndl, sub_hndl, key, key_size);
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "DeleteKVItemSubset() failed\n");
        return;
    }

    ret = CloseSubset(sess_hndl, sub_hndl);
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "SessionDeleteSubset() failed\n");
        return;
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestDestroyStore(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        const char *provider_name,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
    }

    ret = DestroyStore(sess_hndl);
    if ( ret ) {
        fprintf(stderr, "DestroyStore() failed\n");
        return;
    }
}

static void RunTestDestroySubset(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        const char *provider_name,
        const char *snap_back_subset_name,
        int wait_for_key,
        int debug)
{
    int ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        fprintf(stderr, "OpenStore() failed\n");
        return;
    }

    ret = OpenSubset(sess_hndl,
        &sub_hndl,
        snap_back_subset_name,
        (size_t )strlen(snap_back_subset_name));
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "OpenSubset() failed\n");
        return;
    }

    ret = DestroySubset(sess_hndl, sub_hndl);
    if ( ret ) {
        CloseStore(sess_hndl, wait_for_key);
        fprintf(stderr, "DestroyStore() failed\n");
        return;
    }

    CloseStore(sess_hndl, wait_for_key);
}

static void RunTestTraverseSubsets(CDStringUin64Vector *subsets, const char *indent)
{
    size_t i;
    for ( i = 0 ; i < subsets->size ; i++ ) {
        fprintf(stdout, "%s  subset(%s/%lu)\n", indent, subsets->vector[i].string.data_ptr, subsets->vector[i].num);
    }
}

static void RunTestTraverseStores(SessionHND sess_hndl, const char *c_n, uint64_t c_id, const char *s_n, uint64_t s_id, CDStringUin64Vector *stores, const char *indent)
{
    size_t i;
    for ( i = 0 ; i < stores->size ; i++ ) {
        fprintf(stdout, "%s  store(%s/%lu)\n", indent, stores->vector[i].string.data_ptr, stores->vector[i].num);

        CDStringUin64Vector subsets;
        subsets.size = 0;
        subsets.vector = 0;
        SessionStatus sts = SessionListSubsets(sess_hndl, c_n, c_id, s_n, s_id, stores->vector[i].string.data_ptr, stores->vector[i].num, &subsets);
        if ( sts != eOk ) {
            fprintf(stderr, "SessionListSubsets() failed sts(%d)\n", sts);
            return;
        }

        RunTestTraverseSubsets(&subsets, "            ");
    }
}

static void RunTestTraverseSpaces(SessionHND sess_hndl, const char *c_n, uint64_t c_id, CDStringUin64Vector *spaces, const char *indent)
{
    size_t i;
    for ( i = 0 ; i < spaces->size ; i++ ) {
        fprintf(stdout, "%s  space(%s/%lu)\n", indent, spaces->vector[i].string.data_ptr, spaces->vector[i].num);

        CDStringUin64Vector stores;
        stores.size = 0;
        stores.vector = 0;
        SessionStatus sts = SessionListStores(sess_hndl, c_n, c_id, spaces->vector[i].string.data_ptr, spaces->vector[i].num, &stores);
        if ( sts != eOk ) {
            fprintf(stderr, "SessionListStores() failed sts(%d)\n", sts);
            return;
        }

        RunTestTraverseStores(sess_hndl, c_n, c_id, spaces->vector[i].string.data_ptr, spaces->vector[i].num, &stores, "        ");
    }
}

static void RunTestTraverseClusters(SessionHND sess_hndl, CDStringUin64Vector *clusters, const char *indent)
{
    size_t i;
    for ( i = 0 ; i < clusters->size ; i++ ) {
        fprintf(stdout, "%s  cluster(%s/%lu)\n", indent, clusters->vector[i].string.data_ptr, clusters->vector[i].num);

        CDStringUin64Vector spaces;
        spaces.size = 0;
        spaces.vector = 0;
        SessionStatus sts = SessionListSpaces(sess_hndl, clusters->vector[i].string.data_ptr, clusters->vector[i].num, &spaces);
        if ( sts != eOk ) {
            fprintf(stderr, "SessionListSpaces() failed sts(%d)\n", sts);
            return;
        }

        RunTestTraverseSpaces(sess_hndl, clusters->vector[i].string.data_ptr, clusters->vector[i].num, &spaces, "    ");
    }
}

static void RunTestTraverseNameSpace(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        const char *provider_name,
        int wait_for_key,
        int debug)
{
    SessionHND sess_hndl = SessionNew(cluster_name,
        space_name,
        store_name,
        provider_name,
        transactional,
        debug,
        sess_sync_mode);
    if ( !sess_hndl ) {
        fprintf(stderr, "SessionNew() failed\n");
        return;
    }

    CDStringUin64Vector clusters;
    clusters.size = 0;
    clusters.vector = 0;
    SessionStatus sts = SessionListClusters(sess_hndl, &clusters);
    if ( sts != eOk ) {
        fprintf(stderr, "SessionListClusters() failed sts(%d)\n", sts);
        SessionClose(sess_hndl);
        return;
    }

    RunTestTraverseClusters(sess_hndl, &clusters, "");

    SessionClose(sess_hndl);
}

/*
 * Regression test functions
 */
static void InitRegressionParams(RegressionArgs *args, int thread_count)
{
    assert(!args->params);

    args->params = malloc(sizeof(RegressionParams));
    memset(args->params, '\0', sizeof(RegressionParams));

    switch ( args->test_type ) {
    case eRegression1:
    case eRegression2:
    case eRegression3:
    case eRegression4:
    case eRegression5:
    case eRegression6:
    case eRegression7:
    case eRegression8:
    case eRegression9:
    case eRegression10:
    case eRegression11:
    case eRegression12:
        args->params->params12.thread_count = thread_count;
        break;
    case eRegression13:
        break;
    default:
        fprintf(stderr, "Unsupported regression test type(%d)\n", args->test_type);
        break;
    }
}

static void TermRegressionParams(RegressionArgs *args, int thread_count)
{
    assert(args->params);

    switch ( args->test_type ) {
    case eRegression1:
    case eRegression2:
    case eRegression3:
    case eRegression4:
    case eRegression5:
    case eRegression6:
    case eRegression7:
    case eRegression8:
    case eRegression9:
    case eRegression10:
    case eRegression11:
    case eRegression12:
        {
        uint32_t i, j, k;
        int s_ret, ret;

        /*
         * Open/Destroy/Delete subsets
         */
        for ( i = 0 ; i < args->params->params12.thread_count ; i++ ) {
            for ( j = 0 ; j < args->params->params12.threads[i].session_count ; j++ ) {
                for ( k = 0 ; k < args->params->params12.threads[i].sessions[j].subset_count ; k++ ) {
                    if ( strlen(args->params->params12.threads[i].sessions[j].subsets[k].sub_name) ) {
                        SessionHND s_hndl = args->params->params12.threads[i].sessions[j].sub_sess_hndl;
                        SubsetableHND *sub_hndl = &args->params->params12.threads[i].sessions[j].subsets[k].sub_hndl;
                        char *s_name = args->params->params12.threads[i].sessions[j].subsets[k].sub_name;

                        s_ret = OpenSubset(s_hndl, sub_hndl, s_name, (size_t )strlen(s_name));
                        if ( s_ret ) {
                            fprintf(stderr, "OpenSubset(%s/%u) for destroy failed(%u)\n", s_name, k, SessionLastStatus(s_hndl));
                        }

                        ret = DestroySubset(s_hndl, *sub_hndl);
                        if ( ret ) {
                            fprintf(stderr, "DestroySubset(%s/%u) failed(%u)\n", s_name, k, SessionLastStatus(s_hndl));
                        }

                        ret = SessionDeleteSubset(s_hndl, *sub_hndl);
                        if ( ret ) {
                            fprintf(stderr, "SessionDeleteSubset(%s/%u) failed(%u)\n", s_name, k, SessionLastStatus(s_hndl));
                        }
                    }
                }
            }
        }

        /*
         * Destroy/Delete stores
         */
        for ( i = 0 ; i < args->params->params12.thread_count ; i++ ) {
            for ( j = 0 ; j < args->params->params12.threads[i].session_count ; j++ ) {
                ret = DestroyStore(args->params->params12.threads[i].sessions[j].sess_hndl);
                if ( ret ) {
                    fprintf(stderr, "DestroyStore(%s/%u) failed\n", args->params->params12.threads[i].sessions[j].store_name, j);
                }

                /*
                 * Close main store
                 */
                CloseStore(args->params->params12.threads[i].sessions[j].sess_hndl, 0);
                args->params->params12.threads[i].sessions[j].sess_hndl = 0;

                ret = DestroyStore(args->params->params12.threads[i].sessions[j].sub_sess_hndl);
                if ( ret ) {
                    fprintf(stderr, "DestroyStore(%s/%u) failed\n", args->params->params12.threads[i].sessions[j].sub_store_name, j);
                }

                /*
                 * Close subset store
                 */
                CloseStore(args->params->params12.threads[i].sessions[j].sub_sess_hndl, 0);
                args->params->params12.threads[i].sessions[j].sub_sess_hndl = 0;
            }
        }
        }
        break;
    case eRegression13:
        break;
    default:
        fprintf(stderr, "Unsupported regression test type(%d)\n", args->test_type);
        break;
    }

    free(args->params);
    args->params = 0;
}

static void RunTestRegression1(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    RegressionArgs args;
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess_hndl = sess_hndl;
    args.sub_hndl = sub_hndl;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.io_counter = 0;
    args.default_first = default_first;
    args.test_type = eRegression1;

    if ( thread_count == 1 ) {
        RegressionTest1(&args);
    } else {
        int i;
        pthread_t thread_ids[MAX_THREAD_COUNT];
        int rc;

        if ( thread_count > MAX_THREAD_COUNT ) {
            thread_count = MAX_THREAD_COUNT;
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            rc = pthread_create(&thread_ids[i], 0, RegressionStartTest1, &args);
            if ( rc ) {
                fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                break;
            }
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            int *ret;
            (void )pthread_join(thread_ids[i], (void **)&ret);
        }
    }
}

static void RunTestRegression2(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    RegressionArgs args;
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess_hndl = sess_hndl;
    args.sub_hndl = sub_hndl;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.io_counter = 0;
    args.default_first = default_first;
    args.test_type = eRegression2;

    if ( thread_count == 1 ) {
        RegressionTest2(&args);
    } else {
        int i;
        pthread_t thread_ids[MAX_THREAD_COUNT];
        int rc;

        if ( thread_count > MAX_THREAD_COUNT ) {
            thread_count = MAX_THREAD_COUNT;
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            rc = pthread_create(&thread_ids[i], 0, RegressionStartTest2, &args);
            if ( rc ) {
                fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                break;
            }
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            int *ret;
            (void )pthread_join(thread_ids[i], (void **)&ret);
        }
    }
}

static void RunTestRegression3(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    RegressionArgs args;
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess_hndl = sess_hndl;
    args.sub_hndl = sub_hndl;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.io_counter = 0;
    args.default_first = default_first;
    args.test_type = eRegression3;

    if ( thread_count == 1 ) {
        RegressionTest3(&args);
    } else {
        int i;
        pthread_t thread_ids[MAX_THREAD_COUNT];
        int rc;

        if ( thread_count > MAX_THREAD_COUNT ) {
            thread_count = MAX_THREAD_COUNT;
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            rc = pthread_create(&thread_ids[i], 0, RegressionStartTest3, &args);
            if ( rc ) {
                fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                break;
            }
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            int *ret;
            (void )pthread_join(thread_ids[i], (void **)&ret);
        }
    }
}

static void RunTestRegression4(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    RegressionArgs args;
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess_hndl = sess_hndl;
    args.sub_hndl = sub_hndl;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.io_counter = 0;
    args.default_first = default_first;
    args.test_type = eRegression4;

    if ( thread_count == 1 ) {
        RegressionTest4(&args);
    } else {
        int i;
        pthread_t thread_ids[MAX_THREAD_COUNT];
        int rc;

        if ( thread_count > MAX_THREAD_COUNT ) {
            thread_count = MAX_THREAD_COUNT;
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            rc = pthread_create(&thread_ids[i], 0, RegressionStartTest4, &args);
            if ( rc ) {
                fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                break;
            }
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            int *ret;
            (void )pthread_join(thread_ids[i], (void **)&ret);
        }
    }
}

static void RunTestRegression5(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    RegressionArgs args;
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess_hndl = sess_hndl;
    args.sub_hndl = sub_hndl;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.io_counter = 0;
    args.default_first = default_first;
    args.test_type = eRegression5;

    if ( thread_count == 1 ) {
        RegressionTest5(&args);
    } else {
        int i;
        pthread_t thread_ids[MAX_THREAD_COUNT];
        int rc;

        if ( thread_count > MAX_THREAD_COUNT ) {
            thread_count = MAX_THREAD_COUNT;
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            rc = pthread_create(&thread_ids[i], 0, RegressionStartTest5, &args);
            if ( rc ) {
                fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                break;
            }
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            int *ret;
            (void )pthread_join(thread_ids[i], (void **)&ret);
        }
    }
}

static void RunTestRegression6(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    RegressionArgs args;
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess_hndl = sess_hndl;
    args.sub_hndl = sub_hndl;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.default_first = default_first;
    args.io_counter = 0;
    args.test_type = eRegression6;

    RegressionArgs thread_args[MAX_THREAD_COUNT];
    memset(thread_args, '\0', sizeof(thread_args));

    if ( thread_count == 1 ) {
        RegressionTest6(&args);
    } else {
        int i;
        pthread_t thread_ids[MAX_THREAD_COUNT];
        int rc;

        if ( thread_count > MAX_THREAD_COUNT ) {
            thread_count = MAX_THREAD_COUNT;
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            thread_args[i] = args;
            rc = pthread_create(&thread_ids[i], 0, RegressionStartTest6, &thread_args[i]);
            if ( rc ) {
                fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                break;
            }
        }

        uint64_t io_total_counter = 0;
        for ( i = 0 ; i < thread_count ; i++ ) {
            int *ret;
            (void )pthread_join(thread_ids[i], (void **)&ret);
            io_total_counter += thread_args[i].io_counter;
        }

        printf("io_total_counter=%lu\n", io_total_counter);
    }
}

static void RunTestRegression7(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    RegressionArgs args;
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess_hndl = sess_hndl;
    args.sub_hndl = sub_hndl;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.default_first = default_first;
    args.io_counter = 0;
    args.test_type = eRegression7;

    int ret = 0;
    int s_ret = 0;

    RegressionArgs thread_args[MAX_THREAD_COUNT];
    memset(thread_args, '\0', sizeof(thread_args));

    /*
     * Store
     */
    ret = OpenStore(&args.sess_hndl,
        args.cluster_name,
        args.space_name,
        args.store_name,
        args.transactional,
        args.sess_sync_mode,
        args.provider_name,
        (SoeBool )args.debug);
    if ( ret || !args.sess_hndl ) {
        fprintf(stderr, "OpenStore(%s) failed(%u)\n", args.store_name, SessionLastStatus(args.sess_hndl));
    }

    if ( ret < 0 ) {
        if ( args.sess_hndl ) {
            SessionClose(args.sess_hndl);
            args.sess_hndl = 0;
        }
        ret = CreateStore(&args.sess_hndl,
            args.cluster_name,
            args.space_name,
            args.store_name,
            args.transactional,
            args.sess_sync_mode,
            args.provider_name,
            (SoeBool )args.debug);
        if ( ret || !args.sess_hndl ) {
            fprintf(stderr, "CreateStore(%s) failed(%u)\n", args.store_name, SessionLastStatus(args.sess_hndl));
            return;
        }
        fprintf(stderr, "CreateStore(%s) ok\n", args.store_name);
    }

    args.io_counter += 2;

    /*
     * Subset
     */
    s_ret = OpenSubset(args.sess_hndl,
        &args.sub_hndl,
        snap_back_subset_name,
        (size_t )strlen(snap_back_subset_name));
    if ( s_ret ) {
        fprintf(stderr, "OpenSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(args.sess_hndl));
    } else {
        fprintf(stderr, "OpenSubset(%s) ok sub_hndl(%lu)\n", snap_back_subset_name, args.sub_hndl);
    }
    if ( s_ret < 0 ) {
        s_ret = CreateSubset(args.sess_hndl,
            &args.sub_hndl,
            snap_back_subset_name,
            (size_t )strlen(snap_back_subset_name));
        if ( s_ret ) {
            fprintf(stderr, "CreateSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(args.sess_hndl));
            CloseStore(args.sess_hndl, wait_for_key);
            return;
        }
        fprintf(stderr, "CreateSubset(%s) ok sub_hndl(%lu)\n", snap_back_subset_name, args.sess_hndl);
    }

    if ( thread_count == 1 ) {
        RegressionTest7(&args);
    } else {
        int i;
        pthread_t thread_ids[MAX_THREAD_COUNT];
        int rc;

        if ( thread_count > MAX_THREAD_COUNT ) {
            thread_count = MAX_THREAD_COUNT;
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            thread_args[i] = args;
            if ( i ) {
                thread_args[i].sub_hndl = 0;
                s_ret = OpenSubset(thread_args[i].sess_hndl,
                    &thread_args[i].sub_hndl,
                    snap_back_subset_name,
                    (size_t )strlen(snap_back_subset_name));
                if ( s_ret ) {
                    fprintf(stderr, "OpenSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(thread_args[i].sess_hndl));
                } else {
                    fprintf(stderr, "OpenSubset(%s) ok sub_hndl(%lu)\n", snap_back_subset_name, thread_args[i].sub_hndl);
                }
                if ( s_ret < 0 ) {
                    s_ret = CreateSubset(thread_args[i].sess_hndl,
                        &thread_args[i].sub_hndl,
                        snap_back_subset_name,
                        (size_t )strlen(snap_back_subset_name));
                    if ( s_ret ) {
                        fprintf(stderr, "CreateSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(thread_args[i].sess_hndl));
                        CloseStore(thread_args[i].sess_hndl, wait_for_key);
                        return;
                    }
                    fprintf(stderr, "CreateSubset(%s) ok sub_hndl(%lu)\n", snap_back_subset_name, thread_args[i].sub_hndl);
                }
            }

            rc = pthread_create(&thread_ids[i], 0, RegressionStartTest7, &thread_args[i]);
            if ( rc ) {
                fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                break;
            }
        }

        uint64_t io_total_counter = 0;
        for ( i = 0 ; i < thread_count ; i++ ) {
            int *ret;
            (void )pthread_join(thread_ids[i], (void **)&ret);
            io_total_counter += thread_args[i].io_counter;
        }

        printf("io_total_counter=%lu\n", io_total_counter);

        /*
         * Close stores
         */
        CloseStore(args.sess_hndl, wait_for_key);
        args.sess_hndl = 0;
    }
}

static void RunTestRegression8(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    int ret = 0;
    int toggle_merge = 0;

    for ( ;; ) {
        /*
         * Store
         */
        ret = OpenStore(&sess_hndl,
            cluster_name,
            space_name,
            store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            fprintf(stderr, "OpenStore(%s) failed(%u)\n", store_name, SessionLastStatus(sess_hndl));
        }

        if ( ret < 0 ) {
            if ( sess_hndl ) {
                SessionClose(sess_hndl);
                sess_hndl = 0;
            }
            ret = CreateStore(&sess_hndl,
                cluster_name,
                space_name,
                store_name,
                transactional,
                sess_sync_mode,
                provider_name,
                (SoeBool )debug);
            if ( ret || !sess_hndl ) {
                fprintf(stderr, "CreateStore(%s) failed(%u)\n", store_name, SessionLastStatus(sess_hndl));
                break;
            }
            fprintf(stderr, "CreateStore(%s) ok\n", store_name);
        }

        if ( !toggle_merge ) {
            ret = WriteStore(sess_hndl,
                key,
                key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "WriteStore(%s) failed(%u)\n", store_name, SessionLastStatus(sess_hndl));
                break;
            }
            toggle_merge = 1;
        } else {
            ret = MergeStore(sess_hndl,
                key,
                key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "MergeStore(%s) failed(%u)\n", store_name, SessionLastStatus(sess_hndl));
                break;
            }
            toggle_merge = 0;
        }

        ret = ReadKVItemStore(sess_hndl, key, key_size, value, value_size, hex_dump, no_print);
        if ( ret ) {
            fprintf(stderr, "ReadKVItemStore() failed\n");
            break;
        }

        ret = DeleteKVItemStore(sess_hndl, key, key_size);
        if ( ret ) {
            fprintf(stderr, "DeleteKVItemStore() failed\n");
            break;
        }

        /*
         * Close stores
         */
        CloseStore(sess_hndl, wait_for_key);
    }
}

static void RunTestRegression9(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    RegressionArgs args;
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess_hndl = sess_hndl;
    args.sub_hndl = sub_hndl;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.io_counter = 0;
    args.default_first = default_first;
    args.test_type = eRegression9;

    if ( thread_count == 1 ) {
        RegressionTest9(&args);
    } else {
        int i;
        pthread_t thread_ids[MAX_THREAD_COUNT];
        int rc;

        if ( thread_count > MAX_THREAD_COUNT ) {
            thread_count = MAX_THREAD_COUNT;
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            rc = pthread_create(&thread_ids[i], 0, RegressionStartTest9, &args);
            if ( rc ) {
                fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                break;
            }
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            int *ret;
            (void )pthread_join(thread_ids[i], (void **)&ret);
        }
    }
}

static void RunTestRegression10(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    RegressionArgs args;
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess_hndl = sess_hndl;
    args.sub_hndl = sub_hndl;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.io_counter = 0;
    args.default_first = default_first;
    args.test_type = eRegression10;

    if ( thread_count == 1 ) {
        RegressionTest10(&args);
    } else {
        int i;
        pthread_t thread_ids[MAX_THREAD_COUNT];
        int rc;

        if ( thread_count > MAX_THREAD_COUNT ) {
            thread_count = MAX_THREAD_COUNT;
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            rc = pthread_create(&thread_ids[i], 0, RegressionStartTest10, &args);
            if ( rc ) {
                fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                break;
            }
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            int *ret;
            (void )pthread_join(thread_ids[i], (void **)&ret);
        }
    }
}

static void RunTestRegression11(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    RegressionArgs args;
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess_hndl = sess_hndl;
    args.sub_hndl = sub_hndl;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.io_counter = 0;
    args.default_first = default_first;
    args.test_type = eRegression11;

    if ( thread_count == 1 ) {
        RegressionTest11(&args);
    } else {
        int i;
        pthread_t thread_ids[MAX_THREAD_COUNT];
        int rc;

        if ( thread_count > MAX_THREAD_COUNT ) {
            thread_count = MAX_THREAD_COUNT;
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            rc = pthread_create(&thread_ids[i], 0, RegressionStartTest11, &args);
            if ( rc ) {
                fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                break;
            }
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            int *ret;
            (void )pthread_join(thread_ids[i], (void **)&ret);
        }
    }
}

static void RunTestRegression12(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        uint32_t sleep_multiplier,
        int sync_async_destroy,
        int debug)
{
    RegressionArgs args;
    memset(&args, '\0', sizeof(args));
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess_hndl = sess_hndl;
    args.sub_hndl = sub_hndl;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.io_counter = 0;
    args.default_first = default_first;
    args.sleep_multiplier = sleep_multiplier;
    args.sync_async_destroy = sync_async_destroy;
    args.test_type = eRegression12;

    /*
     * Alloc session/subset handle block for later
     */
    if ( sync_async_destroy ) {
        InitRegressionParams(&args, thread_count);
    }

    if ( thread_count == 1 ) {
        args.th_number = 0;
        RegressionTest12(&args);
    } else {
        int i;
        pthread_t thread_ids[MAX_THREAD_COUNT];
        int rc;

        if ( thread_count > MAX_THREAD_COUNT ) {
            thread_count = MAX_THREAD_COUNT;
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            RegressionArgs *t_args = malloc(sizeof(RegressionArgs));
            *t_args = args;
            t_args->th_number = i;
            rc = pthread_create(&thread_ids[i], 0, RegressionStartTest12, t_args);
            if ( rc ) {
                fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                break;
            }
        }

        /*
         * Destroy stores and delete sessions
         */
        if ( sync_async_destroy ) {
            TermRegressionParams(&args, thread_count);
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            int *ret;
            (void )pthread_join(thread_ids[i], (void **)&ret);
        }
    }
}

static void RunTestRegression13(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        uint32_t sleep_multiplier,
        int sync_async_destroy,
        int debug)
{
    SessionHND sess1_hndl;

    /*
     * Open store, get spare handle
     * If store not found create it
     */
    int ret = OpenStore(&sess1_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess1_hndl ) {
        if ( sess1_hndl ) {
            CloseStore(sess1_hndl, 0);
            sess1_hndl = 0;
        }

        ret = CreateStore(&sess1_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
        if ( ret || !sess1_hndl ) {
            return;
        }
    }

    /*
     * Open and destroy store
     * If store not found create it
     */
    ret = OpenStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
    if ( ret || !sess_hndl ) {
        if ( sess_hndl ) {
            CloseStore(sess_hndl, 0);
            sess_hndl = 0;
        }

        ret = CreateStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            return;
        }
    } else {
        ret = DestroyStore(sess_hndl);
        if ( ret ) {
            fprintf(stderr, "DestroyStore(%s) failed\n", store_name);
            return;
        }

        CloseStore(sess_hndl, 0);
        sess_hndl = 0;

        /*
         * recreate it
         */
        ret = CreateStore(&sess_hndl,
        cluster_name,
        space_name,
        store_name,
        transactional,
        sess_sync_mode,
        provider_name,
        (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            return;
        }
    }

    assert(sess_hndl);
    assert(sess1_hndl);

    /*
     * Write num_ops k/v items
     */
    for ( uint32_t k = 0 ; k < num_regressions ; k++ ) {
        char **key_ptrs = malloc(num_ops*sizeof(char *));
        memset(key_ptrs, '\0', num_ops*sizeof(char *));
        size_t *key_sizes = malloc(num_ops*sizeof(size_t));
        memset(key_sizes, '\0', num_ops*sizeof(size_t));
        char **value_ptrs = malloc(num_ops*sizeof(char *));
        memset(value_ptrs, '\0', num_ops*sizeof(char *));
        size_t *value_sizes = malloc(num_ops*sizeof(size_t));
        memset(value_sizes, '\0', num_ops*sizeof(size_t));
        uint32_t i;

        for ( i = 0 ; i < num_ops ; i++ ) {
            key_ptrs[i] = malloc(key_size + 32);
            sprintf(key_ptrs[i], "%s_%u_%u", key, k, i);
            key_sizes[i] = (size_t )strlen(key_ptrs[i]);

            value_ptrs[i] = malloc(value_size + 32);
            sprintf(value_ptrs[i], "%s_%u_%u", value, k, i);
            if ( value_size > strlen(value_ptrs[i]) ) {
                memset(value_ptrs[i] + strlen(value_ptrs[i]), 'V', value_size - strlen(value_ptrs[i]));
            }
            value_sizes[i] = value_size;
        }

        ret = WriteStoreNAsync(sess_hndl,
            num_ops,
            key_ptrs,
            key_sizes,
            value_ptrs,
            value_sizes);
        if ( ret ) {
            fprintf(stderr, "WriteStoreNAsync(%s) failed\n", store_name);
        }

        for ( i = 0 ; i < num_ops ; i++ ) {
            free(key_ptrs[i]);
            free(value_ptrs[i]);
        }

        free(key_ptrs);
        free(key_sizes);
        free(value_ptrs);
        free(value_sizes);

        if ( ret ) {
            break;
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }
    }

    if ( sleep_multiplier == 1 ) {
        CloseStore(sess_hndl, 0);
    }

    if ( sleep_multiplier == 1 ) {
        CloseStore(sess1_hndl, 0);
    }

    return;
}

static void RunTestRegression14(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        uint32_t sleep_multiplier,
        int sync_async_destroy,
        int debug)
{
    /* Intializes random number generator */
    time_t t;
    srand((unsigned int)time(&t));

    uint32_t i = 0;
    int ret = 0;
    int s_ret = 0;
    printf("key=%s "
        "key_len=%lu end_key=%s end_key_len=%lu "
        "it_dir=%d defau_fst=%d "
        "value=%s value_len=%lu num_ops=%d "
        "num_regres=%d subset_n=%s trans=%d "
        "sync_typ=%d debug=%d no_prnt=%d "
        "hex_dmp=%d\n",
        key, key_size, end_key, end_key_size,
        it_dir, default_first,
        value, value_size, num_ops,
        num_regressions, snap_back_subset_name, transactional,
        sync_type, debug, no_print,
        hex_dump);

    if ( num_ops > MAX_NUM_OPS ) {
        fprintf(stderr, "number of ops too big\n");
        return;
    }

    int r_ret = 0;
    for ( i = 0 ; i < num_regressions ; i++ ) {
        /*
         * Open and destroy store
         * If store not found create it
         */
        ret = OpenStore(&sess_hndl,
            cluster_name,
            space_name,
            store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
        if ( ret || !sess_hndl ) {
            if ( sess_hndl ) {
                CloseStore(sess_hndl, 0);
                sess_hndl = 0;
            }

            ret = CreateStore(&sess_hndl,
            cluster_name,
            space_name,
            store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            (SoeBool )debug);
            if ( ret || !sess_hndl ) {
                fprintf(stderr, "CreateStore(%s) failed(%u)\n", store_name, SessionLastStatus(sess_hndl));
                return;
            }
        }

        /*
         * Open/Create Subset
         */
        s_ret = OpenSubset(sess_hndl,
            &sub_hndl,
            snap_back_subset_name,
            (size_t )strlen(snap_back_subset_name));
        if ( s_ret ) {
            fprintf(stderr, "OpenSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sess_hndl));
        }
        if ( s_ret < 0 ) {
            s_ret = CreateSubset(sess_hndl,
                &sub_hndl,
                snap_back_subset_name,
                (size_t )strlen(snap_back_subset_name));
            if ( s_ret ) {
                fprintf(stderr, "CreateSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sess_hndl));
                CloseStore(sess_hndl, wait_for_key);
                return;
            }
            fprintf(stderr, "CreateSubset(%s) ok\n", snap_back_subset_name);
        }

        /*
         * Handle store ops
         */
        if ( num_ops == 1 ) {
            if ( value_size > strlen(value) ) {
                memset(value + strlen(value), 'V', value_size - strlen(value));
            }
            ret = WriteStore(sess_hndl,
                key,
                key_size,
                value,
                value_size);
            if ( ret ) {
                fprintf(stderr, "WriteStore(%s) failed(%u)\n", store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t j;

            for ( j = 0 ; j < num_ops ; j++ ) {
                key_ptrs[j] = malloc(key_size + 128);
                sprintf(key_ptrs[j], "%d_%s_%u_%u", rand(), key, j, i);
                key_sizes[j] = (size_t )strlen(key_ptrs[j]);

                value_ptrs[j] = malloc(value_size + 128);
                sprintf(value_ptrs[j], "%s_%u_%u", value, j, i);
                if ( value_size > strlen(value_ptrs[j]) ) {
                    memset(value_ptrs[j] + strlen(value_ptrs[j]), 'V', value_size - strlen(value_ptrs[j]));
                }
                value_sizes[j] = value_size;
            }

            ret = WriteStoreN(sess_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( ret ) {
                fprintf(stderr, "WriteStoreN(%s) failed(%u)\n", store_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }

            for ( j = 0 ; j < num_ops ; j++ ) {
                free(key_ptrs[j]);
                free(value_ptrs[j]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);

            if ( sleep_usecs ) {
                usleep(sleep_usecs);
            }
        }

        ret = ReadStore(sess_hndl, 0, 0, 0, 0, it_dir, default_first, debug, hex_dump, no_print, 0);
        if ( ret ) {
            fprintf(stderr, "ReadStore(%s) failed(%u)\n", store_name, SessionLastStatus(sess_hndl));
            r_ret = 1;
        }

        if ( num_ops == 1 ) {
            if ( value_size > strlen(value) ) {
                memset(value + strlen(value), 'S', value_size - strlen(value));
            }
            s_ret = WriteSubset(sub_hndl,
                key,
                key_size,
                value,
                value_size);
            if ( s_ret ) {
                fprintf(stderr, "WriteSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }
        } else {
            char **key_ptrs = malloc(num_ops*sizeof(char *));
            memset(key_ptrs, '\0', num_ops*sizeof(char *));
            size_t *key_sizes = malloc(num_ops*sizeof(size_t));
            memset(key_sizes, '\0', num_ops*sizeof(size_t));
            char **value_ptrs = malloc(num_ops*sizeof(char *));
            memset(value_ptrs, '\0', num_ops*sizeof(char *));
            size_t *value_sizes = malloc(num_ops*sizeof(size_t));
            memset(value_sizes, '\0', num_ops*sizeof(size_t));
            uint32_t k;

            for ( k = 0 ; k < num_ops ; k++ ) {
                key_ptrs[k] = malloc(key_size + 128);
                sprintf(key_ptrs[k], "%d_%s_%u_%u", rand(), key, k, i);
                key_sizes[k] = (size_t )strlen(key_ptrs[k]);

                value_ptrs[k] = malloc(value_size + 128);
                sprintf(value_ptrs[k], "%s_%u_%u", value, k, i);
                if ( value_size > strlen(value_ptrs[k]) ) {
                    memset(value_ptrs[k] + strlen(value_ptrs[k]), 'S', value_size - strlen(value_ptrs[k]));
                }
                value_sizes[k] = value_size;
            }

            s_ret = WriteSubsetN(sub_hndl,
                num_ops,
                key_ptrs,
                key_sizes,
                value_ptrs,
                value_sizes);
            if ( s_ret ) {
                fprintf(stderr, "WriteSubsetN(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sess_hndl));
                r_ret = 1;
            }
            for ( k = 0 ; k < num_ops ; k++ ) {
                free(key_ptrs[k]);
                free(value_ptrs[k]);
            }

            free(key_ptrs);
            free(key_sizes);
            free(value_ptrs);
            free(value_sizes);
        }

        ret = ReadSubset(sub_hndl, 0, 0, 0, 0, it_dir, default_first, debug, hex_dump, no_print, 0);
        if ( ret ) {
            fprintf(stderr, "ReadSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sess_hndl));
            r_ret = 1;
        }

        if ( sleep_usecs ) {
            usleep(sleep_usecs);
        }

        /*
         * Close subset
         */
        ret = CloseSubset(sess_hndl, sub_hndl);
        if ( ret ) {
            fprintf(stderr, "CloseSubset(%s) failed(%u)\n", snap_back_subset_name, SessionLastStatus(sess_hndl));
        }
        sub_hndl = 0;

        /*
         * Close store
         */
        CloseStore(sess_hndl, 0);
        sess_hndl = 0;

        printf("Regression(%u)\n", i);
    }

    printf("RegressionTest14 DONE\n");
}

static void RunTestRegression15(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    RegressionArgs args;
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess_hndl = sess_hndl;
    args.sub_hndl = sub_hndl;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.io_counter = 0;
    args.default_first = default_first;
    args.test_type = eRegression10;

    if ( thread_count == 1 ) {
        RegressionTest15(&args);
    } else {
        int i;
        pthread_t thread_ids[MAX_THREAD_COUNT];
        int rc;

        if ( thread_count > MAX_THREAD_COUNT ) {
            thread_count = MAX_THREAD_COUNT;
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            rc = pthread_create(&thread_ids[i], 0, RegressionStartTest15, &args);
            if ( rc ) {
                fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                break;
            }
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            int *ret;
            (void )pthread_join(thread_ids[i], (void **)&ret);
        }
    }
}

static void RunTestRegression16(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    RegressionArgs args;
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess_hndl = sess_hndl;
    args.sub_hndl = sub_hndl;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.io_counter = 0;
    args.default_first = default_first;
    args.test_type = eRegression10;

    if ( thread_count == 1 ) {
        RegressionTest16(&args);
    } else {
        int i;
        pthread_t thread_ids[MAX_THREAD_COUNT];
        int rc;

        if ( thread_count > MAX_THREAD_COUNT ) {
            thread_count = MAX_THREAD_COUNT;
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            rc = pthread_create(&thread_ids[i], 0, RegressionStartTest16, &args);
            if ( rc ) {
                fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                break;
            }
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            int *ret;
            (void )pthread_join(thread_ids[i], (void **)&ret);
        }
    }
}

static void RunTestRegression17(const char *cluster_name,
        const char *space_name,
        const char *store_name,
        SessionHND sess_hndl,
        SessionHND sub_hndl,
        SoeBool transactional,
        eSessionSyncMode sess_sync_mode,
        int sync_type,
        const char *provider_name,
        int provider,
        const char *key,
        size_t key_size,
        const char *end_key,
        size_t end_key_size,
        int it_dir,
        int default_first,
        char *value,
        size_t value_buffer_size,
        size_t value_size,
        uint32_t num_ops,
        uint32_t num_regressions,
        uint32_t regression_type,
        const char *snap_back_subset_name,
        uint32_t snap_back_id,
        int wait_for_key,
        int thread_count,
        int hex_dump,
        useconds_t sleep_usecs,
        int no_print,
        int group_write,
        int delete_from_group,
        int transaction_write,
        int debug)
{
    RegressionArgs args;
    args.cluster_name = cluster_name;
    args.space_name = space_name;
    args.store_name = store_name;
    args.provider = provider;
    args.provider_name = provider_name;
    args.key = key;
    args.key_size = key_size;
    args.end_key = end_key;
    args.end_key_size = end_key_size;
    args.it_dir = it_dir;
    args.value = value;
    args.value_buffer_size = value_buffer_size;
    args.value_size = value_size;
    args.num_ops = num_ops;
    args.num_regressions = num_regressions;
    args.snap_back_subset_name = snap_back_subset_name;
    args.snap_back_id = snap_back_id;
    args.transactional = transactional;
    args.sync_type = sync_type;
    args.debug = debug;
    args.wait_for_key = wait_for_key;
    args.sess_hndl = sess_hndl;
    args.sub_hndl = sub_hndl;
    args.sess_sync_mode = sess_sync_mode;
    args.hex_dump = hex_dump;
    args.sleep_usecs = sleep_usecs;
    args.no_print = no_print;
    args.io_counter = 0;
    args.default_first = default_first;
    args.test_type = eRegression10;

    if ( thread_count == 1 ) {
        RegressionTest17(&args);
    } else {
        int i;
        pthread_t thread_ids[MAX_THREAD_COUNT];
        int rc;

        if ( thread_count > MAX_THREAD_COUNT ) {
            thread_count = MAX_THREAD_COUNT;
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            rc = pthread_create(&thread_ids[i], 0, RegressionStartTest17, &args);
            if ( rc ) {
                fprintf(stderr, "pthread_create failed rc(%d) errno(%d)\n", rc, errno);
                break;
            }
        }

        for ( i = 0 ; i < thread_count ; i++ ) {
            int *ret;
            (void )pthread_join(thread_ids[i], (void **)&ret);
        }
    }
}






static int CharFromHex(char c)
{
    // toupper:
    if ( c >= 'a' && c <= 'f' ) {
        c -= ('a' - 'A');  // space
    }

    // validation
    if ( c < '0' || (c > '9' && (c < 'A' || c > 'F')) ) {
        return -1;
    }

    if ( c <= '9' ) {
        return c - '0';
    }

    return c - 'A' + 10;
}

/*
 * @ Decode key from hex
 */
static int DecodeKeyFromHex(const char *key, size_t *key_len, char *hex_key, size_t hex_key_len)
{
    const char *tmp;
    if ( !key || !key_len || !hex_key || !hex_key_len || (*key && hex_key_len < (*key_len)/2) ) {
        return -1;
    }

    *key_len = 0;
    if ( !*key ) {
        return *key_len;
    }
    for ( tmp = key ; *tmp ; ) {
        int h1 = CharFromHex(*tmp++);
        if ( h1 < 0 || !*tmp ) {
            return -1;
        }
        int h2 = CharFromHex(*tmp++);
        if ( h2 < 0 ) {
            return -1;
        }
        hex_key[(*key_len)++] = (char )((h1 << 4) | h2);
    }

    return *key_len;
}

int main(int argc, char *argv[])
{
    const char *cluster_name = "cluster_1";
    const char *space_name = "space_1";
    const char *store_name = "store_1";
    int provider = 0;
    const char *provider_name = 0;
    const char *key = "key_1";
    size_t key_size = (size_t )strlen(key);
    const char *end_key = "key_1";
    size_t end_key_size = (size_t )strlen(end_key);
    char hex_key[1024];
    char hex_end_key[1024];
    int it_dir = 0;
    int default_first = 1; // no default to 0
    const char *default_value = "value_1";
    char *value = 0;
    size_t value_buffer_size = 0;
    size_t value_size = 0;
    uint32_t num_ops = 1;
    uint32_t num_regressions = 1;
    uint32_t regression_type = 1;
    const char *snap_back_subset_name = "embedded_in_store";
    uint32_t snap_back_id = 102;
    int transactional = 0;
    int sync_type = 0;
    int debug = 0;
    int wait_for_key = 0;
    TestType test_type = eCreateStore;
    int thread_count = 1;
    int hex_dump = 0;
    char user_name[256];
    useconds_t sleep_usecs = 0;
    uint32_t sleep_multiplier = 1;
    int no_print = 0;
    int group_write = 0;
    int delete_from_group = 0;
    int transaction_write = 0;
    int key_in_hex = 0;
    int sync_async_destroy = 0;

    memset(hex_key, '\0', sizeof(hex_key));
    memset(hex_end_key, '\0', sizeof(hex_end_key));

    value = malloc(SOE_BIG_BUFFER);
    if ( !value ) {
        fprintf(stderr, "Can't malloc big buffer(%u)\n", SOE_BIG_BUFFER);
        return -1;
    }
    value_buffer_size = SOE_BIG_BUFFER;
    strcpy(value, default_value);
    value_size = (size_t )strlen(value);

    SessionHND sess_hndl = 0;
    SubsetableHND sub_hndl = 0;
    eSessionSyncMode sess_sync_mode = eSyncDefault;
    memset(user_name, '\0', sizeof(user_name));

    int ret = spo_getopt(argc, argv,
        &cluster_name,
        &space_name,
        &store_name,
        &provider,
        &key,
        &key_size,
        &end_key,
        &end_key_size,
        &it_dir,
        &default_first,
        value,
        value_buffer_size,
        &value_size,
        &num_ops,
        &num_regressions,
        &regression_type,
        &snap_back_subset_name,
        &snap_back_id,
        &transactional,
        &sync_type,
        &test_type,
        &wait_for_key,
        &thread_count,
        &hex_dump,
        user_name,
        &sleep_usecs,
        &no_print,
        &group_write,
        &delete_from_group,
        &transaction_write,
        &key_in_hex,
        &sleep_multiplier,
        &sync_async_destroy,
        &debug);

    if ( ret ) {
        if ( ret != 8 ) {
            fprintf(stderr, "Parse options failed\n");
            usage(argv[0]);
        }
        return ret;
    }

    if ( debug ) {
        printf("cluster_name(%s) space_name(%s) store_name(%s) provider(%d) provider_name(%s) key(%s) key_size(%lu) end_key(%s) end_key_size(%lu) hex_key(%s) hex_end_key(%s)"
            " it_dir(%d) default_first(%d) default_value(%s) value(%s) value_buffer_size(%lu) value_size(%lu) num_ops(%u) num_regressions(%u) regression_type(%u)"
            " snap_back_subset_name(%s) snap_back_id(%u) transactional(%d) sync_type(%d) debug(%d) wait_for_key(%d) test_type(%d) thread_count(%d)"
            " hex_dump(%d) user_name(%s) sleep_usecs(%u) sleep_multiplier(%u) no_print(%d) group_write(%d) delete_from_group(%d) transaction_write(%d)"
            " key_in_hex(%d) sync_async_destroy(%d)\n",
        cluster_name,
        space_name,
        store_name,
        provider,
        provider_name,
        key,
        key_size,
        end_key,
        end_key_size,
        hex_key,
        hex_end_key,
        it_dir,
        default_first,
        default_value,
        value,
        value_buffer_size,
        value_size,
        num_ops,
        num_regressions,
        regression_type,
        snap_back_subset_name,
        snap_back_id,
        transactional,
        sync_type,
        debug,
        wait_for_key,
        test_type,
        thread_count,
        hex_dump,
        user_name,
        sleep_usecs,
        sleep_multiplier,
        no_print,
        group_write,
        delete_from_group,
        transaction_write,
        key_in_hex,
        sync_async_destroy);
    }

    if ( debug ) {
        setenv("RC_LOGLVL", "ffffffffffffffff", 1);
        setenv("RC_SYSLOG", "stderr", 1);
    }

    /*
     * Key in hex
     */
    if ( key_in_hex ) {
        if ( DecodeKeyFromHex(key, &key_size, hex_key, sizeof(hex_key)) < 0 ) {
            fprintf(stderr, "Invalid hex key(%s)\n", key);
            return -1;
        }
        key = hex_key;
        if ( strcmp(end_key, "key_1") || !*end_key ) {
            if ( DecodeKeyFromHex(end_key, &end_key_size, hex_end_key, sizeof(hex_end_key)) < 0 ) {
                fprintf(stderr, "Invalid hex end_key(%s)\n", end_key);
                return -1;
            }
            end_key = hex_end_key;
        }
    }

    if ( !*user_name ) {
        strcpy(user_name, "lzsystem");
    }
    if ( SwitchUid(user_name) < 0 ) {
        usage(argv[0]);
        fprintf(stderr, "Run utility sudo c_test ....\n");
        return -1;
    }

    if ( value_buffer_size != SOE_BIG_BUFFER ) {
        if ( value_buffer_size >= SOE_BIG_BUFFER ) {
            value_buffer_size = SOE_BIG_BUFFER - 1;
        }
        value_size = value_buffer_size;
    }

    /*
     * @brief
     * Sync mode
     */
    sess_sync_mode = GetSyncMode(sync_type);

    /*
     * @brief
     * Provider
     */
    provider_name = GetProviderName(provider);

    if ( num_ops > MAX_NUM_OPS ) {
        fprintf(stderr, "number of ops too big\n");
        if ( value ) {
            free(value);
        }
        return -1;
    }

    if ( thread_count > MAX_THREAD_COUNT ) {
        thread_count = MAX_THREAD_COUNT;
    }

    if ( debug ) {
    printf("pr(%s) key(%s) value(%s) value_size(%lu) num_ops(%u) num_regressions(%u) thread_count(%u) gr_w(%d) tr_nal(%d) tr_w(%d)\n",
        provider_name, key, value, value_size, num_ops, num_regressions, thread_count, group_write, transactional, transaction_write);
    }

    switch ( test_type ) {
    case eCreateStore:
        RunTestCreateStore(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            provider_name,
            wait_for_key,
            debug);
    break;
    case eCreateSubset:
        RunTestCreateSubset(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            snap_back_subset_name,
            provider_name,
            wait_for_key,
            debug);
    break;
    case eTraverseNameSpace:
        RunTestTraverseNameSpace(cluster_name,
            space_name,
            store_name,
            transactional,
            sess_sync_mode,
            provider_name,
            wait_for_key,
            debug);
    break;
    case eRegression1: // create stores and subsets and concurrently write/read them
        RunTestRegression1(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eRegression2:   // create and destroy stores
        RunTestRegression2(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eRegression3:   // create and destroy subsets
        RunTestRegression3(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eRegression4: // create single store and subset with a name and concurrently write/read them
        RunTestRegression4(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eRegression5: // create single store with a name and concurrently write/read it
        RunTestRegression5(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eRegression6: // create single store with a name and concurrently write/read it
        RunTestRegression6(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eRegression7: // create single store and session in main thread with a name and concurrently write/read it
        RunTestRegression7(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eRegression8: // create single store and session in main thread, issues close after each put/merge
        RunTestRegression8(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eRegression9: // create/write/read different subset same store multi-threaded
        RunTestRegression9(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eRegression10: // create/write/read/close same store
        RunTestRegression10(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eRegression11: // create/write/read/close/destroy same subset/store
        RunTestRegression11(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eRegression12: // create/write/read/close wait/destroy same subset/store
        RunTestRegression12(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            sleep_multiplier,
            sync_async_destroy,
            debug);
    break;
    case eRegression13: // destory/create store, write async # ops
        RunTestRegression13(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            sleep_multiplier,
            sync_async_destroy,
            debug);
    break;
    case eRegression14: // mem leak test
        RunTestRegression14(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            sleep_multiplier,
            sync_async_destroy,
            debug);
    break;
    case eRegression15: // create/write/read/close same store
        RunTestRegression15(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eRegression16: // open/create then write/read then close
        RunTestRegression16(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eRegression17: // multiple sessions, same thread, open/open/close/close
        RunTestRegression17(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eWriteStore:
        RunTestWriteStore(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eWriteSubset:
        RunTestWriteSubset(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eMergeStore:
        RunTestMergeStore(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eMergeSubset:
        RunTestMergeSubset(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eReadStore:
        RunTestReadStore(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            provider_name,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            wait_for_key,
            hex_dump,
            no_print,
            debug);
    break;
    case eRepairStore:
        RunTestRepairStore(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            provider_name,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            wait_for_key,
            hex_dump,
            no_print,
            debug);
    break;
    case eReadSubset:
        RunTestReadSubset(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            provider_name,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            snap_back_subset_name,
            wait_for_key,
            hex_dump,
            no_print,
            debug);
    break;
    case eWriteStoreAsync:
        RunTestWriteStoreAsync(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eReadStoreAsync:
        RunTestReadStoreAsync(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            provider_name,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            wait_for_key,
            hex_dump,
            no_print,
            debug);
    break;
    case eMergeStoreAsync:
        RunTestMergeStoreAsync(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            sync_type,
            provider_name,
            provider,
            key,
            key_size,
            end_key,
            end_key_size,
            it_dir,
            default_first,
            value,
            value_buffer_size,
            value_size,
            num_ops,
            num_regressions,
            regression_type,
            snap_back_subset_name,
            snap_back_id,
            wait_for_key,
            thread_count,
            hex_dump,
            sleep_usecs,
            no_print,
            group_write,
            delete_from_group,
            transaction_write,
            debug);
    break;
    case eDeleteStoreAsync:
        RunTestDeleteStoreAsync(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            transactional,
            sess_sync_mode,
            provider_name,
            key,
            key_size,
            wait_for_key,
            debug);
    break;
    case eReadKVItemStore:
        RunTestReadKVItemStore(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            transactional,
            sess_sync_mode,
            provider_name,
            key,
            key_size,
            value,
            value_buffer_size,
            value_size,
            wait_for_key,
            hex_dump,
            no_print,
            debug);
    break;
    case eReadKVItemSubset:
        RunTestReadKVItemSubset(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            provider_name,
            key,
            key_size,
            snap_back_subset_name,
            wait_for_key,
            hex_dump,
            no_print,
            debug);
    break;
    case eDeleteKVItemStore:
        RunTestDeleteKVItemStore(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            transactional,
            sess_sync_mode,
            provider_name,
            key,
            key_size,
            wait_for_key,
            debug);
    break;
    case eDeleteKVItemSubset:
        RunTestDeleteKVItemSubset(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            provider_name,
            key,
            key_size,
            snap_back_subset_name,
            wait_for_key,
            debug);
    break;
    case eDestroyStore:
        RunTestDestroyStore(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            transactional,
            sess_sync_mode,
            provider_name,
            debug);
    break;
    case eDestroySubset:
        RunTestDestroySubset(cluster_name,
            space_name,
            store_name,
            sess_hndl,
            sub_hndl,
            transactional,
            sess_sync_mode,
            provider_name,
            snap_back_subset_name,
            wait_for_key,
            debug);
    break;
    default:
        fprintf(stderr, "Unsupported test type\n");
        usage(argv[0]);
        if ( value ) {
            free(value);
        }
        return -1;
    }

    if ( value ) {
        free(value);
    }
    return 0;
}


