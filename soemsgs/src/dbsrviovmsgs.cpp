/**
 * @file    dbsrviovmsgs.cpp
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
 * dbsrviovmsgs.cpp
 *
 *  Created on: Feb 27, 2017
 *      Author: Jan Lisowiec
 */

#include <sys/uio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

#include "soelogger.hpp"
using namespace SoeLogger;

#include "classalloc.hpp"
#include "generalpool.hpp"
#include "dbsrvmsgbuffer.hpp"
#include "dbsrvmarshallable.hpp"
#include "dbsrvmsgs.hpp"
#include "dbsrviovmsgsbase.hpp"
#include "dbsrviovmsgs.hpp"

#include <netinet/in.h>

using namespace std;
using namespace MemMgmt;

namespace MetaMsgs {

//#define __IOV_DEBUG__

//
// Iov
//
IovMetaDbDescriptor::IovMetaDbDescriptor()
{
    ::memset(&desc, '\0', sizeof(desc));
}

IovMetaDbDescriptor::~IovMetaDbDescriptor()
{}

uint8_t  IovMsgBase::something_anyway[MsgSomethingAnywayLength];
uint8_t  IovMsgBase::SOM[MsgSomLength];
IovMsgBase::Initializer iov_initalized;
bool IovMsgBase::Initializer::initialized = false;

IovMsgBase::Initializer::Initializer()
{
    if ( initialized != true ) {
        ::memcpy(reinterpret_cast<void *>(IovMsgBase::SOM), "DUPEKIOV", static_cast<size_t>(sizeof(IovMsgBase::SOM)));
        ::memcpy(reinterpret_cast<void *>(IovMsgBase::something_anyway), "SOMETHIN", static_cast<size_t>(sizeof(IovMsgBase::something_anyway)));
    }
    initialized = true;
}

IovMsgBase::IovMsgBase(eMsgType _type)
{
    ::memset(reinterpret_cast<void *>(&iovhdr), '\0', static_cast<size_t>(sizeof(iovhdr)));
    ::memset(reinterpret_cast<void *>(&iov), '\0', static_cast<size_t>(sizeof(iov)));
    ::memcpy(reinterpret_cast<void *>(iovhdr.hdr.som.marker), reinterpret_cast<const void *>(IovMsgBase::SOM), static_cast<size_t>(sizeof(AdmMsgBase::SOM)));
    iovhdr.hdr.type = static_cast<uint32_t>(_type);
    iovhdr.hdr.total_length = static_cast<uint32_t>(sizeof(iovhdr) + sizeof(iov));

    for ( uint32_t i = 0 ; i < MaxIovList ; i++ ) {
        if ( i == IovMsgBase::data_idx || i == IovMsgBase::buf_idx ) {
            iov[i].iov_base = static_cast<void *>(new uint8_t[iov_big_buf_size + 8]);
            iov[i].iov_len = iov_big_buf_size + 8;
            iov_sizes[i] = iov_big_buf_size + 8;
        } else {
            iov[i].iov_base = static_cast<void *>(new uint8_t[iov_buf_size + 8]);
            iov[i].iov_len = iov_buf_size + 8;
            iov_sizes[i] = iov_buf_size + 8;
        }
    }
    iov[0].iov_len = IovMsgBase::iov_zero_len;
    iov_sizes[0] = IovMsgBase::iov_zero_len;
}

IovMsgBase::~IovMsgBase()
{
    for ( uint32_t i = 0 ; i < MaxIovList ; i++ ) {
        delete[] static_cast<uint8_t *>(iov[i].iov_base);
        iov[i].iov_len = 0;
        iov_sizes[i] = 0;
    }
}

void IovMsgBase::marshall(uint8_t *&buf_ptr, uint32_t &val)
{
#if defined (__MULTI_PLATFORM__) && !defined (__GNUC__)
    uint8_t *tmp_ptr = buf_ptr;
    *tmp_ptr++ = val & 0xff; val = val >> 8;
    *tmp_ptr++ = val & 0xff; val = val >> 8;
    *tmp_ptr++ = val & 0xff; val = val >> 8;
    *tmp_ptr++ = val & 0xff;
#else
    *reinterpret_cast<uint32_t *>(buf_ptr) = val;
#endif
    buf_ptr += 4;
}

void IovMsgBase::unmarshall(uint8_t *&buf_ptr, uint32_t &val)
{
#if defined (__MULTI_PLATFORM__) && !defined (__GNUC__)
    uint8_t *tmp_ptr = buf_ptr + 3;
    val = *tmp_ptr--;
    val = val << 8 | *tmp_ptr--;
    val = val << 8 | *tmp_ptr--;
    val = val << 8 | *tmp_ptr--;
#else
    val = *reinterpret_cast<uint32_t *>(buf_ptr);
#endif
    buf_ptr += 4;

}

void IovMsgBase::marshall(uint8_t *&buf_ptr, uint64_t &val)
{
#if defined (__MULTI_PLATFORM__) && !defined (__GNUC__)
    uint8_t *tmp_ptr = buf_ptr;
    *tmp_ptr++ = val & 0xff; val = val >> 8;
    *tmp_ptr++ = val & 0xff; val = val >> 8;
    *tmp_ptr++ = val & 0xff; val = val >> 8;
    *tmp_ptr++ = val & 0xff; val = val >> 8;
    *tmp_ptr++ = val & 0xff; val = val >> 8;
    *tmp_ptr++ = val & 0xff; val = val >> 8;
    *tmp_ptr++ = val & 0xff; val = val >> 8;
    *tmp_ptr++ = val & 0xff;
#else
    *reinterpret_cast<uint64_t *>(buf_ptr) = val;
#endif
    buf_ptr += 8;
}

void IovMsgBase::unmarshall(uint8_t *&buf_ptr, uint64_t &val)
{
#if defined (__MULTI_PLATFORM__) && !defined (__GNUC__)
    uint8_t *tmp_ptr = buf_ptr + 7;
    val = *tmp_ptr--;
    val = val << 8 | *tmp_ptr--;
    val = val << 8 | *tmp_ptr--;
    val = val << 8 | *tmp_ptr--;
    val = val << 8 | *tmp_ptr--;
    val = val << 8 | *tmp_ptr--;
    val = val << 8 | *tmp_ptr--;
    val = val << 8 | *tmp_ptr--;
#else
    val = *reinterpret_cast<uint64_t *>(buf_ptr);
#endif
    buf_ptr += 8;
}

//
// iov[0].iov_base - contains iovhdr marshalled as described below
// iov[0].iov_len - always fixed (sizeof(MsgHdr) + sizeof(uint32_t) + MaxIovList*sizeof(uint32_t))
//
// Fields are unmarshalled from iov[0].iov_base in the following order:
// iovhdr.hdr.som (check the Start-Of-Msg)
// iovhdr.hdr.type
// iovhdr.hdr.total_length
// iovhdr.descr.list_length
// iovhdr.descr.list[0-length]
//
// iov[1-n] - contains data buffers and their respective lengths,
//            if buffer length is 0 then a filler will be used instead
//
int IovMsgBase::unmarshallHdr()
{
#if !defined(__clang__)
    if ( ::memcmp(iov[0].iov_base, IovMsgBase::SOM, sizeof(IovMsgBase::SOM)) ) {
#else
    if ( *reinterpret_cast<uint64_t *>(iov[0].iov_base) != *reinterpret_cast<uint64_t *>(IovMsgBase::SOM) ) {
#endif
#if !defined(__clang__)
        cerr << " invalid iov SOM" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " invalid iov SOM" << SoeLogger::LogError() << endl;
#endif
        return -1;
    }

    uint8_t *ptr = static_cast<uint8_t *>(iov[0].iov_base);
#if !defined(__clang__)
    ::memcpy(static_cast<void *>(iovhdr.hdr.som.marker), static_cast<void *>(ptr), sizeof(iovhdr.hdr.som.marker));
#else
    *reinterpret_cast<uint64_t *>(iovhdr.hdr.som.marker) = *reinterpret_cast<uint64_t *>(ptr);
#endif

    ptr += sizeof(iovhdr.hdr.som.marker);

    uint32_t l_l = 0;
    unmarshall(ptr, l_l);
    iovhdr.hdr.type = l_l;

    l_l = 0;
    unmarshall(ptr, l_l);
    iovhdr.hdr.total_length = l_l;

    // new field
    uint64_t ul_ul = 0;
    unmarshall(ptr, ul_ul);
    iovhdr.seq = ul_ul;

    l_l = 0;
    unmarshall(ptr, l_l);
    iovhdr.descr.status = l_l;

    l_l = 0;
    unmarshall(ptr, l_l);
    iovhdr.descr.list_length = l_l;

    if ( iovhdr.descr.list_length > MaxIovList - 1 ) {
#if !defined(__clang__)
        cerr << " number of iov's exceeds max len: " << iovhdr.descr.list_length << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " number of iov's exceeds max len: " << iovhdr.descr.list_length << SoeLogger::LogError() << endl;
#endif
        return -1;
    }

    for ( uint32_t i = 1 ; i < iovhdr.descr.list_length + 1 ; i++ ) {
        uint32_t l_l = 0;
        unmarshall(ptr, l_l);
        iovhdr.descr.list[i - 1] = l_l;
        iov[i].iov_len = l_l ? l_l : MsgSomethingAnywayLength;
        if ( i == IovMsgBase::data_idx || i == IovMsgBase::buf_idx ) {
            if ( iov[i].iov_len > iov_sizes[i] ) {
                if ( iov[i].iov_len < iov_nonsense_buf_size ) {
                    //cout << "i(0)" << i << " iov_len: " << iov[i].iov_len << endl;
                    iov_sizes[i] = static_cast<uint32_t>(iov[i].iov_len) + 8;
                    delete [] static_cast<uint8_t *>(iov[i].iov_base);
                    iov[i].iov_base = static_cast<void *>(new uint8_t[iov_sizes[i]]);
                    UpdateIovIndex(i);
                } else {
#if !defined(__clang__)
                    cerr << " data size exceeds max buffer size: " << endl;
#else
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " data size exceeds max buffer size: " << iov[i].iov_len << SoeLogger::LogError() << endl;
#endif
                    return -1;
                }
            }
        } else {
            if ( iov[i].iov_len > iov_sizes[i] ) {
                if ( iov[i].iov_len < iov_nonsense_buf_size ) {
                    //cout << "i(1)" << i << " iov_len: " << iov[i].iov_len << endl;
                    iov_sizes[i] = static_cast<uint32_t>(iov[i].iov_len) + 8;
                    delete [] static_cast<uint8_t *>(iov[i].iov_base);
                    iov[i].iov_base = static_cast<void *>(new uint8_t[iov_sizes[i]]);
                    UpdateIovIndex(i);
                } else {
#if !defined(__clang__)
                    cerr << " data size exceeds max buffer size: " << endl;
#else
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " data size exceeds max buffer size: " << iov[i].iov_len << SoeLogger::LogError() << endl;
#endif
                    return -1;
                }
            }
        }
        reinterpret_cast<char *>(iov[i].iov_base)[iov[i].iov_len] = '\0';
    }

    return 0;
}

//
// iov[0].iov_base - contains iovhdr marshalled as described below
// iov[0].iov_len - always fixed (sizeof(MsgHdr) + sizeof(uint32_t) + MaxIovList*sizeof(uint32_t))
//
// Fields are marshalled into iov[0].iov_base in the following order:
// iovhdr.hdr.som (check the Start-Of-Msg)
// iovhdr.hdr.type
// iovhdr.hdr.total_length
// iovhdr.descr.list_length
// iovhdr.descr.list[0-length]
//
// iov[1-n] - contains data buffers and their respective lengths,
//            if buffer length is 0 then a filler will be used instead
//
int IovMsgBase::marshallHdr()
{
#if !defined(__clang__)
    if ( ::memcmp(iovhdr.hdr.som.marker, IovMsgBase::SOM, sizeof(IovMsgBase::SOM)) ) {
#else
    if ( *reinterpret_cast<uint64_t *>(iovhdr.hdr.som.marker) != *reinterpret_cast<uint64_t *>(IovMsgBase::SOM) ) {
#endif
#if !defined(__clang__)
        cerr << " invalid iov SOM" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " invalid iov SOM" << SoeLogger::LogError() << endl;
#endif
        return -1;
    }

    uint8_t *ptr = static_cast<uint8_t *>(iov[0].iov_base);
#if !defined(__clang__)
    ::memcpy(static_cast<void *>(ptr), static_cast<const void *>(iovhdr.hdr.som.marker), sizeof(iovhdr.hdr.som.marker));
#else
    *reinterpret_cast<uint64_t *>(ptr) = *reinterpret_cast<uint64_t *>(iovhdr.hdr.som.marker);
#endif

    if ( iovhdr.descr.list_length > MaxIovList - 1 ) {
#if !defined(__clang__)
        cerr << " number of iov's exceeds max len: " << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " number of iov's exceeds max len: " << iovhdr.descr.list_length << SoeLogger::LogError() << endl;
#endif
        return -1;
    }

    const_cast<MsgHdr *>(&iovhdr.hdr)->total_length = IovMsgBase::iov_zero_len;
    ptr += sizeof(iovhdr.hdr.som.marker);

    uint32_t l_l = iovhdr.hdr.type;
    marshall(ptr, l_l);

    uint8_t *tl_ptr = ptr;
    ptr += sizeof(iovhdr.hdr.total_length);

    // new field sequence
    uint64_t ul_ul = iovhdr.seq;
    marshall(ptr, ul_ul);

    l_l = iovhdr.descr.status;
    marshall(ptr, l_l);

    l_l = iovhdr.descr.list_length;
    marshall(ptr, l_l);

    for ( uint32_t i = 1 ; i < iovhdr.descr.list_length + 1 ; i++ ) {
        uint32_t l_l = iovhdr.descr.list[i - 1];
        const_cast<MsgHdr *>(&iovhdr.hdr)->total_length += l_l;
        marshall(ptr, l_l);
    }

    l_l = iovhdr.hdr.total_length;
    marshall(tl_ptr, l_l);

    return 0;
}

//
// iov[0].iov_base - contains iovhdr marshalled as described below
// iov[0].iov_len - always fixed (sizeof(MsgHdr) + sizeof(uint32_t) + MaxIovList*sizeof(uint32_t))
//
// Fields are unmarshalled from iov[0].iov_base in the following order:
// iovhdr.hdr.som (check the Start-Of-Msg)
// iovhdr.hdr.type
// iovhdr.hdr.total_length
// iovhdr.descr.list_length
// iovhdr.descr.list[0-length]
//
// iov[1-n] - contains data buffers and their respective lengths,
//            if buffer length is 0 then a filler will be used instead
//
int IovMsgBase::unmarshallMMHdr()
{
#if !defined(__clang__)
    if ( ::memcmp(iov[0].iov_base, IovMsgBase::SOM, sizeof(IovMsgBase::SOM)) ) {
#else
    if ( *reinterpret_cast<uint64_t *>(iov[0].iov_base) != *reinterpret_cast<uint64_t *>(IovMsgBase::SOM) ) {
#endif
#if !defined(__clang__)
        cerr << " invalid iov SOM" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " invalid iov SOM" << SoeLogger::LogError() << endl;
#endif
        return -1;
    }

    uint8_t *ptr = static_cast<uint8_t *>(iov[0].iov_base);
#if !defined(__clang__)
    ::memcpy(static_cast<void *>(iovhdr.hdr.som.marker), static_cast<void *>(ptr), sizeof(iovhdr.hdr.som.marker));
#else
    *reinterpret_cast<uint64_t *>(iovhdr.hdr.som.marker) = *reinterpret_cast<uint64_t *>(ptr);
#endif

    ptr += sizeof(iovhdr.hdr.som.marker);

    uint32_t l_l = 0;
    unmarshallMM(ptr, l_l);
    iovhdr.hdr.type = l_l;

    l_l = 0;
    unmarshallMM(ptr, l_l);
    iovhdr.hdr.total_length = l_l;

    l_l = 0;
    unmarshallMM(ptr, l_l);
    iovhdr.descr.list_length = l_l;

    if ( iovhdr.descr.list_length > MaxIovList - 1 ) {
#if !defined(__clang__)
        cerr << " number of iov's exceeds max len: " << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " number of iov's exceeds max len: " << iovhdr.descr.list_length << SoeLogger::LogError() << endl;
#endif
        return -1;
    }

    for ( uint32_t i = 1 ; i < iovhdr.descr.list_length + 1 ; i++ ) {
        uint32_t l_l = 0;
        unmarshallMM(ptr, l_l);
        iovhdr.descr.list[i - 1] = l_l;
        iov[i].iov_len = l_l ? l_l : MsgSomethingAnywayLength;
        if ( i == IovMsgBase::data_idx || i == IovMsgBase::buf_idx ) {
            if ( iov[i].iov_len > iov_sizes[i] ) {
                if ( iov[i].iov_len < iov_nonsense_buf_size ) {
                    iov_sizes[i] = static_cast<uint32_t>(iov[i].iov_len) + 8;
                    delete [] static_cast<uint8_t *>(iov[i].iov_base);
                    iov[i].iov_base = static_cast<void *>(new uint8_t[iov_sizes[i]]);
                    UpdateIovIndex(i);
                } else {
#if !defined(__clang__)
                    cerr << " data size exceeds max buffer size: " << endl;
#else
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " data size exceeds max buffer size: " << iov[i].iov_len << SoeLogger::LogError() << endl;
#endif
                    return -1;
                }
            }
        } else {
            if ( iov[i].iov_len > iov_sizes[i] ) {
                if ( iov[i].iov_len < iov_nonsense_buf_size ) {
                    iov_sizes[i] = static_cast<uint32_t>(iov[i].iov_len) + 8;
                    delete [] static_cast<uint8_t *>(iov[i].iov_base);
                    iov[i].iov_base = static_cast<void *>(new uint8_t[iov_sizes[i]]);
                    UpdateIovIndex(i);
                } else {
#if !defined(__clang__)
                    cerr << " data size exceeds max buffer size: " << endl;
#else
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " data size exceeds max buffer size: " << iov[i].iov_len << SoeLogger::LogError() << endl;
#endif
                    return -1;
                }
            }
        }
    }

    return 0;
}

//
// iov[0].iov_base - contains iovhdr marshalled as described below
// iov[0].iov_len - always fixed (sizeof(MsgHdr) + sizeof(uint32_t) + MaxIovList*sizeof(uint32_t))
//
// Fields are marshalled into iov[0].iov_base in the following order:
// iovhdr.hdr.som (check the Start-Of-Msg)
// iovhdr.hdr.type
// iovhdr.hdr.total_length
// iovhdr.descr.list_length
// iovhdr.descr.list[0-length]
//
// iov[1-n] - contains data buffers and their respective lengths,
//            if buffer length is 0 then a filler will be used instead
//
int IovMsgBase::marshallMMHdr()
{
#if !defined(__clang__)
    if ( ::memcmp(iovhdr.hdr.som.marker, IovMsgBase::SOM, sizeof(IovMsgBase::SOM)) ) {
#else
    if ( *reinterpret_cast<uint64_t *>(iovhdr.hdr.som.marker) != *reinterpret_cast<uint64_t *>(IovMsgBase::SOM) ) {
#endif
#if !defined(__clang__)
        cerr << " invalid iov SOM" << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " invalid iov SOM" << SoeLogger::LogError() << endl;
#endif
        return -1;
    }

    uint8_t *ptr = static_cast<uint8_t *>(iov[0].iov_base);
#if !defined(__clang__)
    ::memcpy(static_cast<void *>(ptr), static_cast<const void *>(iovhdr.hdr.som.marker), sizeof(iovhdr.hdr.som.marker));
#else
    *reinterpret_cast<uint64_t *>(ptr) = *reinterpret_cast<uint64_t *>(iovhdr.hdr.som.marker);
#endif

    if ( iovhdr.descr.list_length > MaxIovList - 1 ) {
#if !defined(__clang__)
        cerr << " number of iov's exceeds max len: " << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " number of iov's exceeds max len: " << iovhdr.descr.list_length << SoeLogger::LogError() << endl;
#endif
        return -1;
    }

    const_cast<MsgHdr *>(&iovhdr.hdr)->total_length = IovMsgBase::iov_zero_len;
    ptr += sizeof(iovhdr.hdr.som.marker);

    uint32_t l_l = iovhdr.hdr.type;
    marshallMM(ptr, l_l);

    uint8_t *tl_ptr = ptr;
    ptr += sizeof(iovhdr.hdr.total_length);

    l_l = iovhdr.descr.list_length;
    marshallMM(ptr, l_l);

    for ( uint32_t i = 1 ; i < iovhdr.descr.list_length + 1 ; i++ ) {
        uint32_t l_l = iovhdr.descr.list[i - 1];
        const_cast<MsgHdr *>(&iovhdr.hdr)->total_length += l_l;
        marshallMM(ptr, l_l);
    }

    l_l = iovhdr.hdr.total_length;
    marshallMM(tl_ptr, l_l);

    return 0;
}

int IovMsgBase::recvSock(int fd)
{
    iov[0].iov_len = IovMsgBase::iov_zero_len;
#ifdef __IOV_DEBUG__
    cerr << __FUNCTION__ << " tr: " << iov_zero_len << endl;
#endif

    op_iov[0] = iov[0];
    int ret = recvSockExact(fd, iov_zero_len, 0, 1);
    if ( ret < 0 ) {
        return ret;
    }
#ifdef __IOV_DEBUG__
    int rret = ret;
#endif
    ret = unmarshallHdr();
#ifdef __IOV_DEBUG__
    cerr << __FUNCTION__ << " rret: " << rret << " tl: " << iovhdr.hdr.total_length << endl;
#endif
    if ( ret < 0 ) {
        return ret;
    }
    ::memcpy(&op_iov[1], &iov[1], sizeof(iov[0])*(MaxIovList - 1));
    return recvSockExact(fd, iovhdr.hdr.total_length - iov_zero_len, 1, static_cast<int>(iovhdr.descr.list_length));
}

int IovMsgBase::sendSock(int fd)
{
    int ret = marshallHdr();
#ifdef __IOV_DEBUG__
    cerr << __FUNCTION__ << " tl: " << iovhdr.hdr.total_length << endl;
#endif
    if ( ret < 0 ) {
        return ret;
    }

    ::memcpy(op_iov, iov, sizeof(iov[0])*MaxIovList);
    return sendSockExact(fd, iovhdr.hdr.total_length, 0, static_cast<int>(iovhdr.descr.list_length + 1));
}

//
// @brief shiftRecvIovs
//
// @param[in]  bytes_want          Total bytes wanted
// @param[in]  one_read            Last one bytes read
// @param[in]  bytes_read          Total bytes read
// @param[in/out]  start_iov_idx   Starting iov index from which to recalculate
// @param[in/out]  start_iov_cnt   iov count to recalculate
// @param[out] return              return status
//
int IovMsgBase::shiftRecvIovs(uint32_t bytes_want, uint32_t one_read, uint32_t bytes_read, int &start_iov_idx, int &start_iov_cnt)
{
#ifdef __IOV_DEBUG__
    cerr << __FUNCTION__ << " b_want: " << bytes_want << " o_read: " << one_read << " b_read: " << bytes_read <<
        " s_i_i: " << start_iov_idx << " s_i_c: "  << start_iov_cnt << endl;
#endif
    // covers most cases, no overhead
    if ( !one_read ) {
        return 0;
    }

    // very unlikely
    if ( bytes_want < bytes_read ) {
        return -1;
    }

    // case for big transfers, calculate running count
    int ret = -1;
    int r_c;
    for ( r_c = start_iov_idx ; r_c < MaxIovList ; r_c++ ) {
#ifdef __IOV_DEBUG__
        cerr << __FUNCTION__ << " op_iov[" << r_c << "].iov_len: "  << op_iov[r_c].iov_len << endl;
#endif
        if ( one_read >= op_iov[r_c].iov_len ) {
            one_read -= op_iov[r_c].iov_len;
            op_iov[r_c].iov_len = 0;
            start_iov_idx++;
            start_iov_cnt--;
            ret = 0;
            if( !one_read ) {
                break;
            }
            continue;
        }

        op_iov[r_c].iov_len -= one_read;
        char *tmp_base = static_cast<char *>(op_iov[r_c].iov_base);
        op_iov[r_c].iov_base = tmp_base + one_read;
        ret = 0;
        break;
    }

#ifdef __IOV_DEBUG__
    cerr << __FUNCTION__ << " remaining: " << bytes_want - bytes_read <<
        " s_i_i: " << start_iov_idx << " s_i_c: " << start_iov_cnt <<
        " op_iov[" << r_c << "].iov_len: " << op_iov[r_c].iov_len <<
        " op_iov[" << r_c << "].iov_base: " << reinterpret_cast<uint64_t>(op_iov[r_c].iov_base) << endl;
#endif

    return ret;
}

int IovMsgBase::recvSockExact(int fd, uint32_t bytes_want, int iov_idx, int iov_cnt)
{
    uint32_t bytes_read = 0;
    ssize_t ret = 0;
#ifdef __IOV_DEBUG__
    cerr << __FUNCTION__ << " bw: " << bytes_want << endl;
#endif
    while ( bytes_read < bytes_want ) {
        int c_ret = shiftRecvIovs(bytes_want, static_cast<uint32_t>(ret), bytes_read, iov_idx, iov_cnt);
        if ( c_ret < 0 ) {
#ifdef __IOV_DEBUG__
            cerr << __FUNCTION__ << " c_ret: " << c_ret << endl;
#endif
            return c_ret;
        }

        ret = ::readv(fd, &op_iov[iov_idx], iov_cnt);
        if ( ret < 0 ) {
            if ( errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK ) {
#ifdef __IOV_DEBUG__
                cerr << __FUNCTION__ << " ret: " << ret << " errno: " << errno << endl;
#endif
                ret = 0;
                usleep(50000);
                continue;
            } else {
                if ( errno == ECONNRESET || errno == EPIPE ) {
                    return -ECONNRESET;
                } else {
#if !defined(__clang__)
                    cerr << " readv failed errno: " << errno << endl;
#else
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " readv failed errno: " << errno << SoeLogger::LogError() << endl;
#endif
                    return ret;
                }
            }
        }
        if ( !ret ) {
            return -ECONNRESET;
        }
#ifdef __IOV_DEBUG__
        cerr << __FUNCTION__ << " ret: " << ret << endl;
#endif
        bytes_read += static_cast<uint32_t>(ret);
    }

    return static_cast<int>(bytes_read);
}

//
// @brief shiftSendIovs
//
// @param[in]  bytes_req           Total bytes requested
// @param[in]  one_write           Last one bytes written
// @param[in]  bytes_written       Total bytes written
// @param[in/out]  start_iov_idx   Starting iov index from which to recalculate
// @param[in/out]  start_iov_cnt   iov count to recalculate
// @param[out] return              return status
//
int IovMsgBase::shiftSendIovs(uint32_t bytes_req, uint32_t one_write, uint32_t bytes_written, int &start_iov_idx, int &start_iov_cnt)
{
#ifdef __IOV_DEBUG__
    cerr << __FUNCTION__ << " b_req: " << bytes_req << " o_write: " << one_write << " b_written: " << bytes_written <<
        " s_i_i: " << start_iov_idx << " s_i_c: "  << start_iov_cnt << endl;
#endif
    // covers most cases, no overhead
    if ( !bytes_written ) {
        return 0;
    }

    // very unlikely
    if ( bytes_req < bytes_written ) {
        return -1;
    }

    // case for big transfers, calculate running count
    int ret = -1;
    int r_c;
    for ( r_c = start_iov_idx ; r_c < MaxIovList ; r_c++ ) {
#ifdef __IOV_DEBUG__
        cerr << __FUNCTION__ << " op_iov[" << r_c << "].iov_len: "  << op_iov[r_c].iov_len << endl;
#endif
        if ( one_write >= op_iov[r_c].iov_len ) {
            one_write -= op_iov[r_c].iov_len;
            op_iov[r_c].iov_len = 0;
            start_iov_idx++;
            start_iov_cnt--;
            ret = 0;
            if( !one_write ) {
                break;
            }
            continue;
        }

        op_iov[r_c].iov_len -= one_write;
        char *tmp_base = static_cast<char *>(op_iov[r_c].iov_base);
        op_iov[r_c].iov_base = tmp_base + one_write;
        ret = 0;
        break;
    }

#ifdef __IOV_DEBUG__
    cerr << __FUNCTION__ << " remaining: " << bytes_req - bytes_written <<
        " s_i_i: " << start_iov_idx << " s_i_c: " << start_iov_cnt <<
        " op_iov[" << r_c << "].iov_len: " << op_iov[r_c].iov_len <<
        " op_iov[" << r_c << "].iov_base: " << reinterpret_cast<uint64_t>(op_iov[r_c].iov_base) << endl;
#endif

    return ret;
}

int IovMsgBase::sendSockExact(int fd, uint32_t bytes_req, int iov_idx, int iov_cnt)
{
    uint32_t bytes_written = 0;
    ssize_t ret = 0;
#ifdef __IOV_DEBUG__
    cerr << __FUNCTION__ << " br: " << bytes_req << endl;
#endif
    while ( bytes_written < bytes_req ) {
        int c_ret = shiftSendIovs(bytes_req, static_cast<uint32_t>(ret), bytes_written, iov_idx, iov_cnt);
        if ( c_ret < 0 ) {
#ifdef __IOV_DEBUG__
            cerr << __FUNCTION__ << " c_ret: " << c_ret << endl;
#endif
            return c_ret;
        }

        ret = ::writev(fd, &op_iov[iov_idx], iov_cnt);
        if ( ret < 0 ) {
            if ( errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK ) {
#ifdef __IOV_DEBUG__
                cerr << __FUNCTION__ << " ret: " << ret << " errno: " << ret << endl;
#endif
                ret = 0;
                usleep(50000);
                continue;
            } else {
                if ( errno == ECONNRESET || errno == EPIPE ) {
                    return -ECONNRESET;
                } else {
#if !defined(__clang__)
                    cerr << " writev failed errno: " << errno << endl;
#else
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " readv failed errno: " << errno << SoeLogger::LogError() << endl;
#endif
                    return ret;
                }
            }
        }
        if ( !ret ) {
            return -ECONNRESET;
        }
#ifdef __IOV_DEBUG__
        cerr << __FUNCTION__ << " ret: " << ret << endl;
#endif
        bytes_written += static_cast<uint32_t>(ret);
    }

    return static_cast<int>(bytes_written);
}

int IovMsgBase::recvMM(int fd, uint8_t *mm_ptr, size_t mm_size)
{
#ifdef __IOV_DEBUG__
    cerr << __FUNCTION__ << "rMM " << hex << "0x" << reinterpret_cast<uint64_t>(mm_ptr) << "/" << dec << mm_size << " SOM: ";
    uint8_t dbg_buf[32];
    memcpy(dbg_buf, mm_ptr, sizeof(iovhdr.hdr.som.marker));
    dbg_buf[sizeof(iovhdr.hdr.som.marker)] = '\0';
    cerr << dbg_buf << endl;
#endif

    iov[0].iov_len = IovMsgBase::iov_zero_len;
    int ret = recvMMExact(fd, iov_zero_len, 0, 1, mm_ptr, mm_size);
    if ( ret < 0 ) {
        return ret;
    }
    ret = unmarshallMMHdr();
    if ( ret < 0 ) {
        return ret;
    }
    return recvMMExact(fd, iovhdr.hdr.total_length - iov_zero_len, 1, static_cast<int>(iovhdr.descr.list_length), mm_ptr + iov_zero_len, mm_size - iov_zero_len);
}

int IovMsgBase::sendMM(int fd, uint8_t *mm_ptr, size_t mm_size)
{
#ifdef __IOV_DEBUG__
    cerr << "sMM " << hex << "0x" << reinterpret_cast<uint64_t>(mm_ptr) << "/" << dec << mm_size << " SOM: ";
    uint8_t dbg_buf[32];
    memcpy(dbg_buf, mm_ptr, sizeof(iovhdr.hdr.som.marker));
    dbg_buf[sizeof(iovhdr.hdr.som.marker)] = '\0';
    cerr << dbg_buf << endl;
#endif

    int ret = marshallMMHdr();
    if ( ret < 0 ) {
        return ret;
    }
    return sendMMExact(fd, iovhdr.hdr.total_length, 0, static_cast<int>(iovhdr.descr.list_length + 1), mm_ptr, mm_size);
}

int IovMsgBase::recvMMExact(int fd, uint32_t bytes_want, int iov_idx, int iov_cnt, uint8_t *mm_ptr, size_t mm_size)
{
    uint32_t bytes_read = 0;
    uint8_t *ptr = mm_ptr;

    if ( bytes_want > static_cast<uint32_t>(mm_size) ) {
#if !defined(__clang__)
        cerr << " MM size too small bytes_want: " << bytes_want <<
            " mm_size: " << mm_size << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " MM size too small bytes_want: " << bytes_want <<
            " mm_size: " << mm_size << SoeLogger::LogError() << endl;
#endif
        return -1;
    }

    for ( int i = 0 ; i < iov_cnt ; i++ ) {
        ::memcpy(iov[iov_idx + i].iov_base, ptr, iov[iov_idx + i].iov_len);
        ptr += iov[iov_idx + i].iov_len;
        bytes_read += static_cast<uint32_t>(iov[iov_idx + i].iov_len);
    }
    if ( bytes_read != bytes_want ) {
        return -1;
    }
    return static_cast<int>(bytes_read);
}

int IovMsgBase::sendMMExact(int fd, uint32_t bytes_req, int iov_idx, int iov_cnt, uint8_t *mm_ptr, size_t mm_size)
{
    uint32_t bytes_written = 0;
    uint8_t *ptr = mm_ptr;

    if ( bytes_req > static_cast<uint32_t>(mm_size) ) {
#if !defined(__clang__)
        cerr << " MM size too small bytes_want: " << bytes_req <<
            " mm_size: " << mm_size << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " MM size too small bytes_want: " << bytes_req <<
            " mm_size: " << mm_size << SoeLogger::LogError() << endl;
#endif
        return -1;
    }

    for ( int i = 0 ; i < iov_cnt ; i++ ) {
        ::memcpy(ptr, iov[iov_idx + i].iov_base, iov[iov_idx + i].iov_len);
        ptr += iov[iov_idx + i].iov_len;
        bytes_written += static_cast<uint32_t>(iov[iov_idx + i].iov_len);
    }
    if ( bytes_written != bytes_req ) {
        return -1;
    }
    return static_cast<int>(bytes_written);
}

//
// Non-template
//
int IovMsgReqNT::recv(int fd)
{
    return 0;
}

int IovMsgReqNT::send(int fd)
{
    return 0;
}

void IovMsgReqNT::UpdateIovIndex(uint32_t idx)
{}

int IovMsgRspNT::recv(int fd)
{
    return 0;
}

int IovMsgRspNT::send(int fd)
{
    return 0;
}

void IovMsgRspNT::UpdateIovIndex(uint32_t idx)
{}

}

