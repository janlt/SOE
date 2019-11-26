/**
 * @file    mainbase.h
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
 * mainbase.h
 *
 *  Created on: Jan 24, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_EPOCH_SOE_SOE_TEST_SRC_MAINBASE_H_
#define SOE_EPOCH_SOE_SOE_TEST_SRC_MAINBASE_H_

#define DEPOCH_SOE

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

#define LZDBD      fprintf
#define LZFLUSH    fflush(stderr)
#define DBG        stderr

typedef enum _eSpoTestType {
    eSpoKV           = 0,
    eSpoSG           = 1,
    eSpoMKV          = 2,
    eSpoMSG          = 3,
    eSpoConMKV       = 4,
    eSpoConMSG       = 5,
    eSpoConTran      = 6,
    eSpoFailConTran  = 7,
    eSpoCreaSnap     = 8,
    eSpoDelSnap      = 9,
    eSpoListSnap     = 10,
    eSpoCreaTran     = 11,
    eSpoCreaBackEng  = 12,
    eSpoCreaBack     = 13,
    eSpoDelBackEng   = 14,
    eSpoListBacks    = 15,
    eSpoRepair       = 16,
    eSpoConDelete    = 17,
    eSpoCreaSubset   = 18,
    eSpoDelSubset    = 19,
    eSpoQuerySubset  = 20,
    eSpoSubsetLoop   = 21,
    eSpoWriteSubset  = 22,
    eSpoReadSubset   = 23,
    eSpoTestSubset   = 24,
    eSpoQueryDevice  = 25,
    eSpoNone         = 26
} eSpoTestType;

typedef enum _eDbProvider {
    eKvsProvider       = 0,
    eRcsdbemProvider   = 1,
    eMetadbcliProvider = 2,
    ePostgresProvider  = 3
} eDbProvider;

void debugPrintHexBuf(FILE *fp, const char buf[], unsigned int len);
void debugPrintAsciiHexBuf(FILE *fp, const char buf[], unsigned int len);

void *soeTestConnect(const char *hostname,
    int port,
    const char *cluster,
    const char *space,
    const char *store,
    eDbProvider prov,
    eSpoTestType con_type,
    bool create,
    int sync_type,
    bool debug);

void soeTestDisconnect(void *connection);

bool soeTestWriteKVBlock(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t block,
    const unsigned char *buf,
    uint32_t length);

bool soeTestReadKVBlock(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t block,
    unsigned char *buf,
    uint32_t *length);

bool soeTestInitialiseKVDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint32_t blksize,
    uint64_t nblocks);

bool soeTestQueryKVDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint32_t *blksize,
    uint64_t *nblocks);

bool soeTestDeleteKVBlock(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t block);

bool soeTestWriteMultipleKVBlocks(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t start,
    uint32_t nblocks,
    const unsigned char *buf,
    uint32_t len);

bool MultipleKVBlockCalback(const char *key,
    size_t key_len,
    const char *data,
    size_t data_len,
    void *arg);

bool soeTestReadMultipleKVBlocks(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    int use_iter,
    uint64_t start,
    uint32_t nblocks,
    unsigned char *buf,
    uint32_t len);

bool soeTestDeleteMultipleKVBlocks(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t start,
    uint32_t nblocks);

void *soeTestCreateSGDevice(void *retval_c,
    const char *cluster,
    const char *space,
    const char *store,
    const bool create);

bool soeTestQuerySGDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint32_t *blksize,
    uint64_t *nblocks);

bool soeTestReadSGBlock(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t block,
    unsigned char *buf,
    uint32_t *length);

bool soeTestWriteSGBlock(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t block,
    const unsigned char *buf,
    uint32_t length);

bool soeTestWriteMultipleSGBlocks(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t start,
    uint32_t nblocks,
    const unsigned char *buf,
    uint32_t len);

bool soeTestReadMultipleSGBlocks(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    int use_iter,
    uint64_t start,
    uint32_t nblocks,
    unsigned char *buf,
    uint32_t len);

bool soeTestDeleteMultipleSGBlocks(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t start,
    uint32_t nblocks);

bool soeTestWriteTransactionMultipleKVBlocks(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t start,
    uint32_t nblocks,
    const unsigned char *buf,
    uint32_t len,
    bool fail);

bool soeTestWriteTransactionKVBlock(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    uint64_t block,
    const unsigned char *buf,
    uint32_t length,
    bool fail);

bool soeTestCreateSnapshot(const char *cluster,
    const char *space,
    const char *store,
    const char *snap_name);

bool soeTestDeleteSnapshot(const char *cluster,
    const char *space,
    const char *store,
    const char *snap_name,
    uint32_t snap_id);

bool soeTestDeleteSnapshotEngine(const char *cluster,
    const char *space,
    const char *store,
    const char *snap_name,
    uint32_t snap_id);

bool soeTestListSnapshots(const char *cluster,
    const char *space,
    const char *store);

bool soeTestCreateBackupEngine(const char *cluster,
    const char *space,
    const char *store,
    const char *back_name);

bool soeTestCreateBackup(const char *cluster,
    const char *space,
    const char *store,
    const char *back_name,
    uint32_t back_id);

bool soeTestDeleteBackupEngine(const char *cluster,
    const char *space,
    const char *store,
    const char *back_name,
    uint32_t back_id);

bool soeTestListBackups(const char *cluster,
    const char *space,
    const char *store);

bool soeTestContainerRepair(const char *cluster,
    const char *space,
    const char *store);

bool soeTestContainerDelete(void *connection,
    const char *cluster,
    const char *space,
    const char *store);

bool soeTestCreateSubsetDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    const char *subset,
    bool create);

bool soeTestDeleteSubsetDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    const char *subset);

bool soeTestQuerySubsetDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    const char *subset,
    uint32_t *blksize,
    uint64_t *nblocks,
    int dump_hex);

bool soeTestReadSubsetDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    const char *subset,
    uint64_t block,
    unsigned char *buf,
    uint32_t *length);

bool soeTestReadMultipleSubsetDevice(void *conn,
    const char *cluster,
    const char *space,
    const char *store,
    const char *subset,
    const uint64_t,
    uint32_t,
    unsigned char *,
    uint32_t );

bool soeTestWriteMultipleSubsetDevice(void *conn,
    const char *cluster,
    const char *space,
    const char *store,
    const char *subset,
    const uint64_t,
    uint32_t,
    const unsigned char *,
    uint32_t );

bool soeTestWriteSubsetDevice(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    const char *subset,
    uint64_t block,
    const unsigned char *buf,
    uint32_t length);

bool soeTestQueryContainer(void *connection,
    const char *cluster,
    const char *space,
    const char *store,
    int dump_hex);

#ifdef __cplusplus
}
#endif


#endif /* SOE_EPOCH_SOE_SOE_TEST_SRC_MAINBASE_H_ */
