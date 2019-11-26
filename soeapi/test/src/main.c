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
 *  Created on: Jan 24, 2017
 *      Author: Jan Lisowiec
 */

#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include "mainbase.h"


typedef struct {
    const char  *blkspecial;
    uint32_t     blksize;
    uint64_t     nblocks;
    uint64_t     blockno;
    void        *conn;
    const char  *host;
    uint16_t     port;
    char         cluster_name[512];
    char         space_name[512];
    char         store_name[512];
    char         back_snap_subset_name[512];
    uint32_t     snap_back_subset_id;
    uint32_t     subset_test_count;
    int          new;
    int          testtype;
    eDbProvider  provider;
    int          sync_type;
    int          useiter;
    int          debug;
    int          dump_hex;
} device_t;

static int spo_kv_read(device_t *device, void *buf, uint64_t off, uint32_t len)
{
    uint64_t block = off / device->blksize;
    uint32_t count = len / device->blksize;
    uint32_t length = device->blksize;
    int ret = 0;

    if ( off % device->blksize || len % device->blksize || off+len > device->blksize*device->nblocks ) {
        LZDBD(DBG,"spo_kv_read(%lu, %u) invalid request\n",off,len);LZFLUSH;
        return -1;
    }

    if ( count == 1 ) {
        ret = soeTestReadKVBlock(device->conn, device->cluster_name, device->space_name, device->store_name, block, buf, &length);
        if ( !length ) {
            memset(buf, 0x00, device->blksize);
        } else if( length != device->blksize ) {
            LZDBD(DBG,"spo_kv_read(%lu, %u) invalid block length(%u)\n",off,len,length);LZFLUSH;
            return -1;
        }
    } else {
        ret = soeTestReadMultipleKVBlocks(device->conn, device->cluster_name, device->space_name, device->store_name, device->useiter, block, count, buf, length);
    }

    return !ret ? -1 : 0;;
}

static int spo_kv_write(device_t *device, const void *buf, uint64_t off, uint32_t len)
{
    uint64_t block = off / device->blksize;
    uint32_t count = len / device->blksize;
    int ret = 0;

    if ( off % device->blksize || len % device->blksize || off+len > device->blksize*device->nblocks ) {
        LZDBD(DBG,"spo_kv_write(%lu, %u) invalid request\n",off,len);LZFLUSH;
        return -1;
    }

    if ( count == 1 ) {
        ret = soeTestWriteKVBlock(device->conn, device->cluster_name, device->space_name, device->store_name, block, buf, device->blksize);
    } else {
        ret = soeTestWriteMultipleKVBlocks(device->conn, device->cluster_name, device->space_name, device->store_name, block, count, buf, device->blksize);
    }

    return !ret ? -1 : 0;;
}

static int spo_kv_tr_write(device_t *device, const void *buf, uint64_t off, uint32_t len, int fail)
{
    uint64_t block = off / device->blksize;
    uint32_t count = len / device->blksize;
    int ret = 0;

    if ( off % device->blksize || len % device->blksize || off+len > device->blksize*device->nblocks ) {
        LZDBD(DBG,"spo_kv_write(%lu, %u) invalid request\n",off,len);LZFLUSH;
        return -1;
    }

    if ( count == 1 ) {
        ret = soeTestWriteTransactionKVBlock(device->conn, device->cluster_name, device->space_name, device->store_name, block, buf, device->blksize, fail);
    } else {
        ret = soeTestWriteTransactionMultipleKVBlocks(device->conn, device->cluster_name, device->space_name, device->store_name, block, count, buf, device->blksize, fail);
    }

    return !ret ? -1 : 0;;
}

static int spo_kv_create_snap(device_t *device, const char *snap_name)
{
    int ret;
    ret = soeTestCreateSnapshot(device->cluster_name, device->space_name, device->store_name, snap_name);
    return !ret ? -1 : 0;
}

static int spo_kv_delete_snap(device_t *device, const char *snap_name, uint32_t snap_id)
{
    int ret;
    ret = soeTestDeleteSnapshot(device->cluster_name, device->space_name, device->store_name, snap_name, snap_id);
    return !ret ? -1 : 0;
}

static int spo_kv_list_snaps(device_t *device)
{
    int ret;
    ret = soeTestListSnapshots(device->cluster_name, device->space_name, device->store_name);
    return !ret ? -1 : 0;
}

static int spo_kv_create_back_eng(device_t *device, const char *back_name)
{
    int ret;
    ret = soeTestCreateBackupEngine(device->cluster_name, device->space_name, device->store_name, back_name);
    return !ret ? -1 : 0;
}

static int spo_kv_create_back(device_t *device, const char *back_name, uint32_t back_id)
{
    int ret;
    ret = soeTestCreateBackup(device->cluster_name, device->space_name, device->store_name, back_name, back_id);
    return !ret ? -1 : 0;
}

static int spo_kv_delete_back_eng(device_t *device, const char *back_name, uint32_t back_id)
{
    int ret;
    ret = soeTestDeleteBackupEngine(device->cluster_name, device->space_name, device->store_name, back_name, back_id);
    return !ret ? -1 : 0;
}

static int spo_kv_list_backs(device_t *device)
{
    int ret;
    ret = soeTestListBackups(device->cluster_name, device->space_name, device->store_name);
    return !ret ? -1 : 0;
}

static int spo_kv_repair(device_t *device)
{
    int ret;
    ret = soeTestContainerRepair(device->cluster_name, device->space_name, device->store_name);
    return !ret ? -1 : 0;
}

static int spo_sg_check(device_t *device)
{
    int ret;

    ret = soeTestQuerySGDevice(device->conn, device->cluster_name, device->space_name, device->store_name, &device->blksize, &device->nblocks);

    if ( !ret ) {
        void *state;
        state = soeTestCreateSGDevice(device->conn, device->cluster_name, device->space_name, device->store_name, 1);
        if ( !state ) {
            LZDBD(DBG,"Creating SG cluster(%s) space(%s) store(%s) failed\n", device->cluster_name, device->space_name, device->store_name);LZFLUSH;
            ret = 0;
        } else {
            ret = 1;
        }

    }

    return ret;
}

static int spo_sg_read(device_t *device, void *buf, uint64_t off, uint32_t len)
{
    uint64_t block = off / device->blksize;
    uint32_t count = len / device->blksize;
    uint32_t length = device->blksize;
    int ret = 0;

    if ( off % device->blksize || len % device->blksize || off+len > device->blksize*device->nblocks ) {
        LZDBD(DBG,"spo_sg_read(%lu, %u) invalid request\n",off,len);LZFLUSH;
        return -1;
    }

    if( count == 1 ) {
        soeTestReadSGBlock(device->conn, device->cluster_name, device->space_name, device->store_name, block, buf, &length);
        if ( !length ) {
            memset(buf, 0x00, device->blksize);
        } else if ( length != device->blksize ) {
             LZDBD(DBG,"spo_sg_read(%lu, %u) invalid block length(%u)\n",off,len,length);LZFLUSH;
             return -1;
        }
    } else {
        ret = soeTestReadMultipleSGBlocks(device->conn, device->cluster_name, device->space_name, device->store_name, device->useiter, block, count, buf, length);
    }

    return !ret ? -1 : 0;
}

static int spo_sg_write(device_t *device, const void *buf, uint64_t off, uint32_t len)
{
    uint64_t block = off / device->blksize;
    uint32_t count = len / device->blksize;
    int ret = 0;

    if ( off % device->blksize || len % device->blksize || off+len > device->blksize*device->nblocks ) {
        LZDBD(DBG,"spo_sg_write(%lu, %u) invalid request\n",off,len);LZFLUSH;
        return -1;
    }

    if ( count == 1 ) {
        ret = soeTestWriteSGBlock(device->conn, device->cluster_name, device->space_name, device->store_name, block, buf, device->blksize);
    } else {
        ret = soeTestWriteMultipleSGBlocks(device->conn, device->cluster_name, device->space_name, device->store_name, block, count, buf, device->blksize);
    }

    return !ret ? -1 : 0;
}

static int spo_kv_delete(device_t *device)
{
    int ret = 0;

    ret = soeTestContainerDelete(device->conn, device->cluster_name, device->space_name, device->store_name);

    return !ret ? -1 : 0;
}

//
// Subset tests
//
static int spo_kv_create_subset(device_t *device)
{
    int ret = 0;

    ret = soeTestCreateSubsetDevice(device->conn, device->cluster_name, device->space_name, device->store_name, device->back_snap_subset_name, 1);

    return !ret ? -1 : 0;
}

static int spo_kv_delete_subset(device_t *device)
{
    int ret = 0;

    ret = soeTestDeleteSubsetDevice(device->conn, device->cluster_name, device->space_name, device->store_name, device->back_snap_subset_name);

    return !ret ? -1 : 0;
}

static int spo_kv_query_subset(device_t *device)
{
    int ret = 0;

    ret = soeTestQuerySubsetDevice(device->conn, device->cluster_name, device->space_name, device->store_name, device->back_snap_subset_name, &device->blksize, &device->nblocks, device->dump_hex);

    return !ret ? -1 : 0;
}

static int spo_kv_query_container(device_t *device)
{
    int ret = 0;

    ret = soeTestQueryContainer(device->conn, device->cluster_name, device->space_name, device->store_name, device->dump_hex);

    return !ret ? -1 : 0;
}

static int spo_kv_read_subset(device_t *device, void *buf, uint64_t off, uint32_t len)
{
    uint64_t block = off / device->blksize;
    uint32_t count = len / device->blksize;
    uint32_t length = device->blksize;
    int ret = 0;

    if ( off % device->blksize || len % device->blksize || off+len > device->blksize*device->nblocks ) {
        LZDBD(DBG,"spo_kv_read_subset(%lu, %u) invalid request\n", off, len);LZFLUSH;
        return -1;
    }

    if ( count == 1 ) {
        ret = soeTestReadSubsetDevice(device->conn, device->cluster_name, device->space_name, device->store_name, device->back_snap_subset_name, block, buf, &length);
        if ( !length ) {
            memset(buf, 0x00, device->blksize);
        } else if( length != device->blksize ) {
            LZDBD(DBG,"spo_kv_read_subset(%lu, %u) invalid block length(%u)\n",off,len,length);LZFLUSH;
            return -1;
        }
    } else {
        ret = soeTestReadMultipleSubsetDevice(device->conn, device->cluster_name, device->space_name, device->store_name, device->back_snap_subset_name, block, count, buf, length);
    }

    return !ret ? -1 : 0;
}

static int spo_kv_write_subset(device_t *device, const void *buf, uint64_t off, uint32_t len)
{
    uint64_t block = off / device->blksize;
    uint32_t count = len / device->blksize;
    int ret = 0;

    if ( off % device->blksize || len % device->blksize || off+len > device->blksize*device->nblocks ) {
        LZDBD(DBG,"spo_kv_write(%lu, %u) invalid request\n",off,len);LZFLUSH;
        return -1;
    }

    if ( count == 1 ) {
        ret = soeTestWriteSubsetDevice(device->conn, device->cluster_name, device->space_name, device->store_name, device->back_snap_subset_name, block, buf, device->blksize);
    } else {
        ret = soeTestWriteMultipleSubsetDevice(device->conn, device->cluster_name, device->space_name, device->store_name, device->back_snap_subset_name, 0, count, buf, device->blksize);
    }

    return !ret ? -1 : 0;;
}

//
// Test argument parsing section
//
static uint64_t atoul(const char *a)
{
    uint64_t f = 1;
    int l = strlen(a);

    
    if ( l ) {
        switch(a[l-1])
        {
        case 'k':
        case 'K':
            f = 1024UL;
            break;

        case 'm':
        case 'M':
            f = 1024UL*1024;
            break;

        case 'g':
        case 'G':
            f = 1024UL*1024*1024;
            break;

        case 't':
        case 'T':
            f = 1024UL*1024*1024*1024;
            break;

        case 'p':
        case 'P':
            f = 1024UL*1024*1024*1024*1024;
            break;

        case 'e':
        case 'E':
            f = 1024UL*1024*1024*1024*1024*1024;
            break;
        }
    }

    return atol(a)*f;
}

void debugPrintHexBuf(FILE *fp,
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

static char read_buf[100000000];
static char write_buf[100000000];

#define BBBB 0x0B
#define MMMM 0x0A
#define EEEE 0x0F

static void runKVTest(device_t *dev, uint32_t num_blocks)
{
    int ret;
    uint64_t op_size = dev->blksize*num_blocks;
    uint64_t off = dev->blockno*dev->blksize;

    if ( op_size > sizeof(write_buf) ) {
        LZDBD(DBG, "Buffers too small\n");LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    memset(write_buf, BBBB, 16);
    memset(write_buf + 16, MMMM, op_size - 16);
    memset(write_buf + op_size - 16, EEEE, 16);

    ret = spo_kv_write(dev, write_buf, off, op_size);

    if ( ret < 0 ) {
        LZDBD(DBG, "spo_kv_write failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s write done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

    memset(read_buf, '\0', op_size);
    ret = spo_kv_read(dev, read_buf, off, op_size);
    if ( ret < 0 ) {
        LZDBD(DBG, "spo_kv_read failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

    if ( memcmp(write_buf, read_buf, op_size) ) {
        debugPrintHexBuf(stderr, read_buf, op_size);
    }

    LZDBD(DBG,"%s read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;
}

static void runSGTest(device_t *dev, uint32_t num_blocks)
{
    int ret;
    uint64_t op_size = dev->blksize*num_blocks;
    uint64_t off = dev->blockno*dev->blksize;

    if ( op_size > sizeof(write_buf) ) {
        LZDBD(DBG, "Buffers too small\n");LZFLUSH;
        return;
    }

    memset(write_buf, BBBB, 16);
    memset(write_buf + 16, MMMM, op_size - 16);
    memset(write_buf + op_size - 16, EEEE, 16);

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    device_t c_dev = *dev;

    ret = spo_sg_check(dev);
    if ( !ret ) {
        LZDBD(DBG, "spo_sg_check failed(%d)\n", ret);LZFLUSH;
        return;
    }

    *dev = c_dev;

    ret = spo_sg_write(dev, write_buf, off, op_size);
    if ( ret < 0 ) {
        LZDBD(DBG, "spo_sg_write failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s write done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

    memset(read_buf, '\0', op_size);
    ret = spo_sg_read(dev, read_buf, off, op_size);
    if ( ret < 0 ) {
        LZDBD(DBG, "spo_sg_read failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

    if ( memcmp(write_buf, read_buf, op_size) ) {
        debugPrintHexBuf(stderr, read_buf, op_size);
    }

    LZDBD(DBG,"%s read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;
}

static void runMKVTest(device_t *dev, uint32_t num_blocks)
{
    int ret;
    uint64_t op_size = dev->blksize*num_blocks;
    uint64_t off = dev->blockno*dev->blksize;

    if ( op_size > sizeof(write_buf) ) {
        LZDBD(DBG, "Buffers too small\n");LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    memset(write_buf, BBBB, 16);
    memset(write_buf + 16, MMMM, op_size - 16);
    memset(write_buf + op_size - 16, EEEE, 16);

    ret = spo_kv_write(dev, write_buf, off, op_size);

    if ( ret < 0 ) {
        LZDBD(DBG, "spo_kv_write failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s multi-write done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

    memset(read_buf, '\0', op_size);
    ret = spo_kv_read(dev, read_buf, off, op_size);
    if ( ret < 0 ) {
        LZDBD(DBG, "spo_kv_read failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s multi-read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

    if ( memcmp(write_buf, read_buf, op_size) ) {
        debugPrintHexBuf(stderr, read_buf, op_size);
    }

    LZDBD(DBG,"%s multi-read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;
}

static void runMSGTest(device_t *dev, uint32_t num_blocks)
{
    int ret;
    uint64_t op_size = dev->blksize*num_blocks;
    uint64_t off = dev->blockno*dev->blksize;

    if ( op_size > sizeof(write_buf) ) {
        LZDBD(DBG, "Buffers too small\n");LZFLUSH;
        return;
    }

    memset(write_buf, BBBB, 16);
    memset(write_buf + 16, MMMM, op_size - 16);
    memset(write_buf + op_size - 16, EEEE, 16);

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    device_t c_dev = *dev;

    ret = spo_sg_check(dev);
    if ( !ret ) {
        LZDBD(DBG, "spo_sg_check failed(%d)\n", ret);LZFLUSH;
        return;
    }

    *dev = c_dev;

    ret = spo_sg_write(dev, write_buf, off, op_size);
    if ( ret < 0 ) {
        LZDBD(DBG, "spo_sg_write failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s multi-write done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

    memset(read_buf, '\0', op_size);
    ret = spo_sg_read(dev, read_buf, off, op_size);
    if ( ret < 0 ) {
        LZDBD(DBG, "spo_sg_read failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s multi-read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

    if ( memcmp(write_buf, read_buf, op_size) ) {
        debugPrintHexBuf(stderr, read_buf, op_size);
    }

    LZDBD(DBG,"%s multi-read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;
}

static void runConMKVTest(device_t *dev, uint32_t num_blocks)
{
    int ret;
    uint64_t op_size = dev->blksize*num_blocks;
    uint64_t off = dev->blockno*dev->blksize;

    if ( op_size > sizeof(write_buf) ) {
        LZDBD(DBG, "Buffers too small\n");LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    for ( ;; ) {
        memset(write_buf, BBBB, 16);
        memset(write_buf + 16, MMMM, op_size - 16);
        memset(write_buf + op_size - 16, EEEE, 16);

        ret = spo_kv_write(dev, write_buf, off, op_size);

        if ( ret < 0 ) {
            LZDBD(DBG, "spo_kv_write failed(%d)\n", ret);LZFLUSH;
            return;
        }

        LZDBD(DBG,"%s multi-write done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

        memset(read_buf, '\0', op_size);
        ret = spo_kv_read(dev, read_buf, off, op_size);
        if ( ret < 0 ) {
            LZDBD(DBG, "spo_kv_read failed(%d)\n", ret);LZFLUSH;
            return;
        }

        LZDBD(DBG,"%s multi-read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

        if ( memcmp(write_buf, read_buf, op_size) ) {
            debugPrintHexBuf(stderr, read_buf, op_size);
            break;
        }
        off += op_size;
        off %= (dev->nblocks*dev->blksize);
    }

    LZDBD(DBG,"%s multi-read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;
}

static void runConMSGTest(device_t *dev, uint32_t num_blocks)
{
    int ret;
    uint64_t op_size = dev->blksize*num_blocks;
    uint64_t off = dev->blockno*dev->blksize;

    if ( op_size > sizeof(write_buf) ) {
        LZDBD(DBG, "Buffers too small\n");LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    device_t c_dev = *dev;

    ret = spo_sg_check(dev);
    if ( !ret ) {
        LZDBD(DBG, "spo_sg_check failed(%d)\n", ret);LZFLUSH;
        return;
    }

    *dev = c_dev;

    for ( ;; ) {
        memset(write_buf, BBBB, 16);
        memset(write_buf + 16, MMMM, op_size - 16);
        memset(write_buf + op_size - 16, EEEE, 16);

        ret = spo_sg_write(dev, write_buf, off, op_size);
        if ( ret < 0 ) {
            LZDBD(DBG, "spo_sg_write failed(%d)\n", ret);LZFLUSH;
            return;
        }

        LZDBD(DBG,"%s multi-write done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

        memset(read_buf, '\0', op_size);
        ret = spo_sg_read(dev, read_buf, off, op_size);
        if ( ret < 0 ) {
            LZDBD(DBG, "spo_sg_read failed(%d)\n", ret);LZFLUSH;
            return;
        }

        LZDBD(DBG,"%s multi-read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

        if ( memcmp(write_buf, read_buf, op_size) ) {
            debugPrintHexBuf(stderr, read_buf, op_size);
            break;
        }
        off += op_size;
        off %= (dev->nblocks*dev->blksize);
    }

    LZDBD(DBG,"%s multi-read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;
}

static void runConMVSTranTest(device_t *dev, uint32_t num_blocks, int fail)
{
    int ret;
    uint64_t op_size = dev->blksize*num_blocks;
    uint64_t off = dev->blockno*dev->blksize;

    if ( op_size > sizeof(write_buf) ) {
        LZDBD(DBG, "Buffers too small\n");LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    for ( ;; ) {
        memset(write_buf, BBBB, 16);
        memset(write_buf + 16, MMMM, op_size - 16);
        memset(write_buf + op_size - 16, EEEE, 16);

        ret = spo_kv_tr_write(dev, write_buf, off, op_size, fail);

        if ( ret < 0 ) {
            LZDBD(DBG, "spo_kv_write failed(%d)\n", ret);LZFLUSH;
            return;
        }

        LZDBD(DBG,"%s multi-transaction write done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

        memset(read_buf, '\0', op_size);
        ret = spo_kv_read(dev, read_buf, off, op_size);
        if ( ret < 0 ) {
            LZDBD(DBG, "spo_kv_read failed(%d)\n", ret);LZFLUSH;
            return;
        }

        LZDBD(DBG,"%s multi-transaction read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

        if ( memcmp(write_buf, read_buf, op_size) ) {
            debugPrintHexBuf(stderr, read_buf, op_size);
            break;
        }
        off += op_size;
        off %= (dev->nblocks*dev->blksize);
    }

    LZDBD(DBG,"%s multi-transaction read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;
}

static void runConCreaSnap(device_t *dev, const char *snap_name)
{
    int ret;

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    ret = spo_kv_create_snap(dev, snap_name);
    if ( ret < 0 ) {
        LZDBD(DBG, "Snapshot create failed\n");LZFLUSH;
        return;
    }
}

static void runConDelSnap(device_t *dev, const char *snap_name, uint32_t snap_id)
{
    int ret;

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    ret = spo_kv_delete_snap(dev, snap_name, snap_id);
    if ( ret < 0 ) {
        LZDBD(DBG, "Snapshot delete failed\n");LZFLUSH;
        return;
    }
}

static void runConListSnap(device_t *dev)
{
    int ret;

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    ret = spo_kv_list_snaps(dev);
    if ( ret < 0 ) {
        LZDBD(DBG, "Snapshot list failed\n");LZFLUSH;
        return;
    }
}

static void runConCreaBackEng(device_t *dev, const char *back_name)
{
    int ret;

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    ret = spo_kv_create_back_eng(dev, back_name);
    if ( ret < 0 ) {
        LZDBD(DBG, "Backup create engine failed\n");LZFLUSH;
        return;
    }
}

static void runConCreaBack(device_t *dev, const char *back_name, uint32_t back_id)
{
    int ret;

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    ret = spo_kv_create_back(dev, back_name, back_id);
    if ( ret < 0 ) {
        LZDBD(DBG, "Backup create failed\n");LZFLUSH;
        return;
    }
}

static void runConDelBackEng(device_t *dev, const char *back_name, uint32_t back_id)
{
    int ret;

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    ret = spo_kv_delete_back_eng(dev, back_name, back_id);
    if ( ret < 0 ) {
        LZDBD(DBG, "Backup delete engine failed\n");LZFLUSH;
        return;
    }
}

static void runConListBacks(device_t *dev)
{
    int ret;

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    ret = spo_kv_list_backs(dev);
    if ( ret < 0 ) {
        LZDBD(DBG, "Backup list failed\n");LZFLUSH;
        return;
    }
}

static void runConRepair(device_t *dev)
{
    int ret;

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    ret = spo_kv_repair(dev);
    if ( ret < 0 ) {
        LZDBD(DBG, "Container repair failed\n");LZFLUSH;
        return;
    }
}

static void runConDelete(device_t *dev)
{
    int ret;

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    ret = spo_kv_delete(dev);

    if ( ret < 0 ) {
        LZDBD(DBG, "spo_kv_delete failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s delete done\n", __FUNCTION__);LZFLUSH;
}

static void runConCreaSubset(device_t *dev)
{
    int ret;

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    ret = spo_kv_create_subset(dev);

    if ( ret < 0 ) {
        LZDBD(DBG, "spo_kv_create_subset failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s spo_kv_create_subset done\n", __FUNCTION__);LZFLUSH;
}

static void runConDelSubset(device_t *dev)
{
    int ret;

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    ret = spo_kv_delete_subset(dev);

    if ( ret < 0 ) {
        LZDBD(DBG, "spo_kv_delete_subset failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s spo_kv_delete_subset done\n", __FUNCTION__);LZFLUSH;
}

static void runConQuerySubset(device_t *dev)
{
    int ret;

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    ret = spo_kv_query_subset(dev);

    if ( ret < 0 ) {
        LZDBD(DBG, "spo_kv_query_subset failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s spo_kv_query_subset done\n", __FUNCTION__);LZFLUSH;
}

static void runConWriteSubset(device_t *dev, uint32_t num_blocks)
{
    int ret;
    uint64_t op_size = dev->blksize*num_blocks;
    uint64_t off = dev->blockno*dev->blksize;

    if ( op_size > sizeof(write_buf) ) {
        LZDBD(DBG, "Buffers too small\n");LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    memset(write_buf, BBBB, 16);
    memset(write_buf + 16, MMMM, op_size - 16);
    memset(write_buf + op_size - 16, EEEE, 16);

    ret = spo_kv_write_subset(dev, write_buf, off, op_size);

    if ( ret < 0 ) {
        LZDBD(DBG, "spo_kv_write_subset failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s spo_kv_write_subset done\n", __FUNCTION__);LZFLUSH;
}

static void runConReadSubset(device_t *dev, uint32_t num_blocks)
{
    int ret;
    uint64_t op_size = dev->blksize*num_blocks;
    uint64_t off = dev->blockno*dev->blksize;

    if ( op_size > sizeof(read_buf) ) {
        LZDBD(DBG, "Buffers too small\n");LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    ret = spo_kv_read_subset(dev, read_buf, off, op_size);

    if ( ret < 0 ) {
        LZDBD(DBG, "spo_kv_read_subset failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s spo_kv_read_subset done\n", __FUNCTION__);LZFLUSH;
}

static void runConTestSubset(device_t *dev, uint32_t num_blocks)
{
    int ret;
    uint64_t op_size = dev->blksize*num_blocks;
    uint64_t off = dev->blockno*dev->blksize;

    if ( op_size > sizeof(write_buf) ) {
        LZDBD(DBG, "Buffers too small\n");LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    memset(write_buf, BBBB, 16);
    memset(write_buf + 16, MMMM, op_size - 16);
    memset(write_buf + op_size - 16, EEEE, 16);

    ret = spo_kv_write_subset(dev, write_buf, off, op_size);

    if ( ret < 0 ) {
        LZDBD(DBG, "spo_kv_write_subset failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s Subset multi-write done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

    memset(read_buf, '\0', op_size);
    ret = spo_kv_read_subset(dev, read_buf, off, op_size);
    if ( ret < 0 ) {
        LZDBD(DBG, "spo_kv_read_subset failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s multi-read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;

    if ( memcmp(write_buf, read_buf, op_size) ) {
        debugPrintHexBuf(stderr, read_buf, op_size);
    }

    LZDBD(DBG,"%s subset multi-read done off(%lu) op_size(%lu)\n", __FUNCTION__, off, op_size);LZFLUSH;
}

static void runConQueryContainer(device_t *dev)
{
    int ret;

    LZDBD(DBG,"%s\n", __FUNCTION__);LZFLUSH;

    ret = spo_kv_query_container(dev);

    if ( ret < 0 ) {
        LZDBD(DBG, "spo_kv_query_container failed(%d)\n", ret);LZFLUSH;
        return;
    }

    LZDBD(DBG,"%s spo_kv_query_container done\n", __FUNCTION__);LZFLUSH;
}

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage %s [options]  <blkspecial path>\n"
        "  -o, --cluster-name             Soe/Epoch cluster name\n"
        "  -z, --space-name               Soe/Epoch space name\n"
        "  -c, --container                Soe/Epoch container to use for this device\n"
        "  -l, --trans-container          Soe/Epoch transactional container to use for this device\n"
        "  -b, --numblocks                Num blocks (default = 1)\n"
        "  -r, --subset-loop-count        Number of Subset loop tests\n"
        "  -a, --blockno                  Block number (default = 0)\n"
        "  -d  --snap-backup-subset-name  Snapshot or backup name (default snapshot_1)\n"
        "  -f  --snap-back-id             Snapshot or backup id (default 0)\n"
        "  -s, --space                    Blocks on device\n"
        "  -j, --debug                    Debug (default 0)\n"
        "  -e, --epoch-host               Epoch hostname / IP address\n"
        "  -p, --epoch-port               Epoch port number\n"
        "  -k, --kv                       KV test (default)\n"
        "  -g, --sg                       SG test\n"
        "  -t, --mkv                      MKV test\n"
        "  -u, --msg                      MSG test\n"
        "  -m, --noiter                   Do not use range iterator for multi read\n"
        "  -n, --new                      Create new container\n"
        "  -y, --erase                    Delete container\n"
        "  -i, --sync                     Sync type (0 - default, 1 - fdatasync, 2 - async WAL)\n"
        "  -x, --contkv                   Continuous KV test\n"
        "  -v, --provider                 Store provider (0 - KVS, 1 - RCSDBEM(default), 2 - METADBCLI, 3 - POSTGRES)\n"
        "  -R  --repair                   Repair Soe/Epoch container test\n"
        "  -T  --transaction              Continuous transaction test\n"
        "  -F  --failed-trans             Failed transaction test\n"
        "  -S  --create-snapshot          Snapshot create test\n"
        "  -U  --delete-snapshot          Snapshot delete test\n"
        "  -W  --list-snapshots           Snapshot list test\n"
        "  -M  --create-backup engine     Backup engine create test\n"
        "  -N  --create-backup            Backup create test\n"
        "  -O  --delete-backup-engine     Backup engine delete test\n"
        "  -P  --list-backup-engines      Backup engine list test\n"
        "  -A  --create-subset            Subset create test\n"
        "  -B  --delete-subset            Subset delete test\n"
        "  -C  --query-subset             Subset query (list all KV pairs) test\n"
        "  -L  --subset-loop              Subset loop /create/write/query/delete query test\n"
        "  -D  --write-subset             Subset write test\n"
        "  -E  --read-subset              Subset read test\n"
        "  -G  --write-read-subset        Subset write/read test\n"
        "  -Q  --query-container          Device's container query (list all KV pairs) test\n"
        "  -H, --hex-dump                 Do hex dump of read records\n"
        "  -h, --help                     Print this help\n", prog);
}

static void spo_getopt(int argc,
    char *argv[],
    device_t *device,
    uint32_t *num_blocks,
    char user_name[])
{
    int c = 0;

    while (1) {
        static char short_options[] = "o:z:c:l:b:a:d:f:s:e:p:v:i:j:r:X:klgtuxywmnRMNOPTFDSUWABCLDEGQHfrh?";
        static struct option long_options[] = {
            {"cluster",           1, 0, 'o'},
            {"spacename",         1, 0, 'z'},
            {"container",         1, 0, 'c'},
            {"trans_container",   1, 0, 'l'},
            {"numblocks",         1, 0, 'b'},
            {"subset_loop_cnt",   1, 0, 'r'},
            {"snap_back_name",    1, 0, 'd'},
            {"snap_back_id",      1, 0, 'f'},
            {"blockno",           1, 0, 'a'},
            {"space",             1, 0, 's'},
            {"debug",             1, 0, 'j'},
            {"epoch-host",        1, 0, 'e'},
            {"epoch-port",        1, 0, 'p'},
            {"kv-test",           1, 0, 'k'},
            {"sg-test",           1, 0, 'g'},
            {"mkv-test",          1, 0, 't'},
            {"msg-test",          1, 0, 'u'},
            {"new",               0, 0, 'n'},
            {"provider",          1, 0, 'v'},
            {"user-name",         1, 0, 'X'},
            {"sync_type",         1, 0, 'h'},
            {"help",              0, 0, '?'},
            {0, 0, 0, 0}
        };

        if ( (c = getopt_long(argc, argv, short_options, long_options, NULL)) == -1 ) {
            break;
        }

        switch ( c ) {
        case 'a':
            device->blockno = atoul(optarg);
            break;

        case 'l':
            strcpy(device->store_name, optarg);
            device->testtype = c;
            break;

        case 'o':
            strcpy(device->cluster_name, optarg);
            break;

        case 'z':
            strcpy(device->space_name, optarg);
            break;

        case 'c':
            strcpy(device->store_name, optarg);
            break;

        case 'j':
            device->debug = atoi(optarg);
            break;

        case 'b':
            *num_blocks = atoul(optarg);
            break;

        case 'r':
            device->subset_test_count = (uint32_t )atoi(optarg);
            break;

        case 'd':
            strcpy(device->back_snap_subset_name, optarg);
            break;

        case 'f':
            device->snap_back_subset_id =atoul(optarg);
            break;

        case 's':
            device->nblocks = atoul(optarg);
            break;

        case 'e':
            device->host = optarg;
            break;

        case 'p':
            device->port = atoi(optarg);
            break;

        case 'i':
            device->sync_type = atoi(optarg);
            break;

        case 'k':
            device->testtype = c;
            break;

        case 'g':
            device->testtype = c;
            break;

        case 't':
            *num_blocks = device->nblocks > 64 ? 64 : device->nblocks;
            device->testtype = c;
            break;

        case 'u':
            *num_blocks = device->nblocks > 64 ? 64 : device->nblocks;
            device->testtype = c;
            break;

        case 'x':
            *num_blocks = device->nblocks > 64 ? 64 : device->nblocks;
            device->testtype = c;
            break;

        case 'y':
            strcpy(device->store_name, optarg);
            device->testtype = c;
            break;

        case 'w':
            *num_blocks = device->nblocks > 64 ? 64 : device->nblocks;
            device->testtype = c;
            break;

        case 'n':
            device->new = 1;
            break;

        case 'm':
            device->useiter = 0;
            break;

        case 'v':
            device->provider = (eDbProvider )atoi(optarg);
            break;

        case 'R':
            device->testtype = c;
            break;

        case 'T':
            device->testtype = c;
            break;

        case 'F':
            device->testtype = c;
            break;

        case 'S':
            device->testtype = c;
            break;

        case 'U':
            device->testtype = c;
            break;

        case 'M':
            device->testtype = c;
            break;

        case 'N':
            device->testtype = c;
            break;

        case 'O':
            device->testtype = c;
            break;

        case 'P':
            device->testtype = c;
            break;

        case 'W':
            device->testtype = c;
            break;

        case 'A':
            device->testtype = c;
            break;

        case 'B':
            device->testtype = c;
            break;

        case 'C':
            device->testtype = c;
            break;

        case 'L':
            device->testtype = c;
            break;

        case 'D':
            device->testtype = c;
            break;

        case 'E':
            device->testtype = c;
            break;

        case 'G':
            device->testtype = c;
            break;

        case 'Q':
            device->testtype = c;
            break;

        case 'H':
            device->dump_hex = 1;
            break;

        case 'X':
            strcpy(user_name, optarg);
            break;

        case 'h':
        case '?':
            usage(argv[0]);
            exit(EXIT_SUCCESS);
            break;
        }
    }

    device->blkspecial = argv[optind];

    if ( !strcmp(device->store_name, "") ) {
        strcpy(device->store_name, device->blkspecial);
    }
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

int main(int argc, char *argv[])
{
    char user_name[256];

    device_t device = {
            .blkspecial = NULL,
            .host = "127.0.0.1",
            .port = 3680,
            .blksize = 1024,
            .nblocks = 1024*1024,
            .blockno = 0,
            .testtype = 'k',
            .provider = eRcsdbemProvider,
            .sync_type = 0,
            .useiter = 1,
            .subset_test_count = 1,
            .debug = 0,
            .dump_hex = 0
    };

    memset(user_name, '\0', sizeof(user_name));

    const char *spo_default_cluster_name = "spo_cluster";
    const char *spo_default_space_name = "spo_space";
    const char *spo_default_store_name = "spo_store";
    const char *spo_default_subset_name = "spo_subset";

    strcpy(device.cluster_name, spo_default_cluster_name);
    strcpy(device.space_name, spo_default_space_name);
    strcpy(device.store_name, spo_default_store_name);
    strcpy(device.back_snap_subset_name, spo_default_subset_name);

    uint32_t num_blocks = 1;

    eSpoTestType type;

    spo_getopt(argc, argv, &device, &num_blocks, user_name);

    if ( !*user_name ) {
        strcpy(user_name, "lzsystem");
    }
    if ( SwitchUid(user_name) < 0 ) {
        fprintf(stderr, "Run utility sudo c_test ....\n");
        return -1;
    }

    if ( device.testtype == 'k' ) {
        type = eSpoKV;
    } else if ( device.testtype == 'g' ) {
        type = eSpoSG;
    } else if ( device.testtype == 't' ) {
        type = eSpoMKV;
    } else if ( device.testtype == 'u' ) {
        type = eSpoMSG;
    } else if ( device.testtype == 'x' ) {
        type = eSpoConMKV;
    } else if ( device.testtype == 'y' ) {
        type = eSpoConDelete;
    } else if ( device.testtype == 'w' ) {
        type = eSpoConMSG;
    } else if ( device.testtype == 'R' ) {
        type = eSpoRepair;
    } else if ( device.testtype == 'T' ) {
        type = eSpoConTran;
    } else if ( device.testtype == 'F' ) {
        type = eSpoFailConTran;
    } else if ( device.testtype == 'S' ) {
        type = eSpoCreaSnap;
    } else if ( device.testtype == 'U' ) {
        type = eSpoDelSnap;
    } else if ( device.testtype == 'W' ) {
        type = eSpoListSnap;
    } else if ( device.testtype == 'M' ) {
        type = eSpoCreaBackEng;
    } else if ( device.testtype == 'N' ) {
        type = eSpoCreaBack;
    } else if ( device.testtype == 'O' ) {
        type = eSpoDelBackEng;
    } else if ( device.testtype == 'P' ) {
        type = eSpoListBacks;
    } else if ( device.testtype == 'A' ) {
        type = eSpoCreaSubset;
    } else if ( device.testtype == 'B' ) {
        type = eSpoDelSubset;
    } else if ( device.testtype == 'L' ) {
        type = eSpoSubsetLoop;
    } else if ( device.testtype == 'C' ) {
        type = eSpoQuerySubset;
    } else if ( device.testtype == 'D' ) {
        type = eSpoWriteSubset;
    } else if ( device.testtype == 'E' ) {
        type = eSpoReadSubset;
    } else if ( device.testtype == 'G' ) {
        type = eSpoTestSubset;
    } else if ( device.testtype == 'Q' ) {
        type = eSpoQueryDevice;
    } else if ( device.testtype == 'l' ) {
        type = eSpoCreaTran;
    } else {
        LZDBD(DBG, "Invalid testtype(%c)\n", device.testtype);LZFLUSH;
        exit(EXIT_FAILURE);
    }

    if ( !(device.conn = soeTestConnect(device.host,
            device.port,
            device.cluster_name,
            device.space_name,
            device.store_name,
            device.provider, type,
            device.new,
            device.sync_type,
            device.debug)) ) {
        LZDBD(DBG, "Cannot initialise container(%s)\n", device.store_name);LZFLUSH;
        exit(EXIT_FAILURE);
    }

    if ( type == eSpoKV ) {
        runKVTest(&device, num_blocks);
    } else if ( type == eSpoSG ) {
        runSGTest(&device, num_blocks);
    } else if ( type == eSpoMKV ) {
        runMKVTest(&device, num_blocks);
    } else if ( type == eSpoMSG ) {
        runMSGTest(&device, num_blocks);
    } else if ( type == eSpoConMKV ) {
        runConMKVTest(&device, num_blocks);
    } else if ( type == eSpoConMSG ) {
        runConMSGTest(&device, num_blocks);
    } else if ( type == eSpoConTran ) {
        runConMVSTranTest(&device, num_blocks, 0);
    } else if ( type == eSpoFailConTran ) {
        runConMVSTranTest(&device, num_blocks, 1);
    } else if ( type == eSpoCreaSnap ) {
        runConCreaSnap(&device, device.back_snap_subset_name);
    } else if ( type == eSpoDelSnap ) {
        runConDelSnap(&device, device.back_snap_subset_name, device.snap_back_subset_id);
    } else if ( type == eSpoListSnap ) {
        runConListSnap(&device);
    } else if ( type == eSpoCreaBackEng ) {
        runConCreaBackEng(&device, device.back_snap_subset_name);
    } else if ( type == eSpoCreaBack ) {
        runConCreaBack(&device, device.back_snap_subset_name, device.snap_back_subset_id);
    } else if ( type == eSpoDelBackEng ) {
        runConDelBackEng(&device, device.back_snap_subset_name, device.snap_back_subset_id);
    } else if ( type == eSpoListBacks ) {
        runConListBacks(&device);
    } else if ( type == eSpoConDelete ) {
        runConDelete(&device);
    } else if ( type == eSpoRepair ) {
        runConRepair(&device);
    } else if ( type == eSpoCreaSubset ) {
        runConCreaSubset(&device);
    } else if ( type == eSpoDelSubset ) {
        runConDelSubset(&device);
    } else if ( type == eSpoQuerySubset ) {
        runConQuerySubset(&device);
    } else if ( type == eSpoSubsetLoop ) {
        uint32_t i;
        char s_buf[1024];
        char init_s_name[1024];
        strcpy(init_s_name, device.back_snap_subset_name);

        for ( i = 0 ; i < device.subset_test_count ; i++ ) {
            sprintf(s_buf, "%s_%d", init_s_name, i);
            strcpy(device.back_snap_subset_name, s_buf);
            runConWriteSubset(&device, num_blocks);
            runConQuerySubset(&device);
            runConDelSubset(&device);
        }
    } else if ( type == eSpoWriteSubset ) {
        runConWriteSubset(&device, num_blocks);
    } else if ( type == eSpoReadSubset ) {
        runConReadSubset(&device, num_blocks);
    } else if ( type == eSpoTestSubset ) {
        runConTestSubset(&device, num_blocks);
    } else if ( type == eSpoQueryDevice ) {
        runConQueryContainer(&device);
    } else {
        LZDBD(DBG, "Invalid testtype(%c)\n", device.testtype);LZFLUSH;
    }

    soeTestDisconnect(device.conn);

    exit(EXIT_SUCCESS);
}
