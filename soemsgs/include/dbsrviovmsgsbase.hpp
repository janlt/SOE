/**
 * @file    dbsrviovmsgsbase.hpp
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
 * dbsrviovmsgsbase.hpp
 *
 *  Created on: Feb 27, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGS_INCLUDE_DBSRVIOVMSGSBASE_HPP_
#define SOE_SOE_SOE_MSGS_INCLUDE_DBSRVIOVMSGSBASE_HPP_

namespace MetaMsgs {

//
// Iov messages
//

const uint16_t MaxIovList = 8;

//
// Transmit this info so the recipient knows how many elems to receive in iov[] and
// can reconstruct IovDbDescriptor
// It's marshalled <list_length><uint32_t><uint32_t>....
// Index on the list corresponds to a position within IovDbDescriptor
// Values in list hold the data length of the corresponding IovDbDescriptor field
//     if list[idx] = 0 skip the position
//

//
// Bit fields for status when sent from client -> server
//
#define STORE_RANGE_DELETE     1
#define SUBSET_RANGE_DELETE    2
#define STORE_RANGE_GET        3
#define SUBSET_RANGE_GET       4
#define STORE_RANGE_PUT        5
#define SUBSET_RANGE_PUT       6
#define STORE_RANGE_MERGE      7
#define SUBSET_RANGE_MERGE     8

typedef union _StatusBits
{
    uint32_t         status_bits;
    typedef struct _Bits {
        uint32_t     override_dups:1;      // override dups ( 0 - no, 1 - override)
        uint32_t     range_type:4;         // range op type (see above)
        uint32_t     async_io_session:1;   // if it's a msg sent on io_session_async
        uint32_t     rest:26;              // remaining
    } Bits;
    Bits             bits;
} StatusBits;

typedef struct _IovMsgDescriptor
{
    uint32_t         status;                // !!! Overloaded field:
                                            // Here the bit fields will be sent(client)->received(server)
                                            // Here the status(high 16 bits)/valid(low 16 bits) will be sent(server)->received(client)
    uint32_t         list_length;           // length of the iov[n] list, i.e. iov[1-n]
                                            // the iov[0].iov_len is always 16 + 4 + 8 + 8*4 = 60
    uint32_t         list[MaxIovList];      // each elem corresponds to a position within IovDbDescriptor
} PACKED_ALIGNED_CODE(IovMsgDescriptor);

typedef  struct _IovMsgHdr
{
    MsgHdr           hdr;                   // hdr.total_length has the total lengths of all iov's
    uint64_t         seq;                   // reserved for sequenced iov's, i.e. async i/os
    IovMsgDescriptor descr;                 // Iov descriptor, see above
} PACKED_ALIGNED_CODE(IovMsgHdr);

//
// Some fields are overloaded
//
typedef struct _IovDbDescriptor
{
    uint32_t              cluster_iov;          // iov index (optional)
    uint64_t              cluster_id;           // cluster id
    char                 *cluster_name;         // cluster name

    uint32_t              space_iov;            // iov index (optional)
    uint64_t              space_id;             // space id
    char                 *space_name;           // space name

    uint32_t              store_iov;            // iov index (optional)
    uint64_t              store_id;             // store id
    char                 *store_name;           // store name

    uint32_t              key_iov;              // iov index (optional)
    char                 *key;                  // key
    uint32_t              end_key_iov;          // iov index (optional)
    char                 *end_key;              // end key if get range is called
    uint32_t              key_len;              // key length
    uint32_t              end_key_len;          // end key length, or Iterator valid

    uint32_t              data_iov;             // iov index (optional)
    char                 *data;                 // data holding data for Put
    uint32_t              data_len;             // data length

    uint32_t              buf_iov;              // iov index (optional)
    char                 *buf;                  // buffer to receive data from Get
    uint32_t              buf_len;              // buffer length
} IovDbDescriptor;

class IovMetaDbDescriptor
{
public:
    IovDbDescriptor   desc;

    IovMetaDbDescriptor();
    ~IovMetaDbDescriptor();
};

class IovMsgBase
{
public:

    static const uint32_t cluster_name_idx = 1;
    static const uint32_t space_name_idx   = 2;
    static const uint32_t store_name_idx   = 3;
    static const uint32_t key_idx          = 4;
    static const uint32_t end_key_idx      = 5;
    static const uint32_t data_idx         = 6;
    static const uint32_t buf_idx          = 7;

public:
    IovMsgHdr        iovhdr;                         // iovhdr.hdr has total length
    struct iovec     iov[MaxIovList + 1];            // iov[0] has marshalled header
    struct iovec     op_iov[MaxIovList + 1];         // used as a copy, readv/writev may return partial messages
    uint32_t         iov_sizes[MaxIovList + 1];      // to keep track of iov sizes in case we need to reallocate bigger buffgers

    static uint8_t   SOM[MsgSomLength];
    static uint8_t   something_anyway[MsgSomethingAnywayLength];
    static const uint32_t iov_zero_len = sizeof(MsgHdr) + 2*sizeof(uint32_t) + sizeof(uint64_t) + MaxIovList*sizeof(uint32_t);
    static const uint32_t iov_buf_size = 100000;
    static const uint32_t iov_big_buf_size = 10000000;
    static const uint32_t iov_nonsense_buf_size = 4000000000;

    class Initializer
    {
    public:
        static bool initialized;
        Initializer();
        ~Initializer() {}
    };
    static Initializer iov_initalized;

    IovMsgBase(eMsgType _type = eMsgInvalid);

    virtual ~IovMsgBase();

    typedef IovMsgBase *(*GetMsgBaseFunction)();
    typedef void (*PutMsgBaseFunction)(IovMsgBase *);

    int recvSock(int fd);
    int sendSock(int fd);

    int recvMM(int fd, uint8_t *mm_ptr, size_t mm_size);
    int sendMM(int fd, uint8_t *mm_ptr, size_t mm_size);

    bool isAsync();
    bool isSync();

    virtual void HitchIds();
    virtual void UnHitchIds();

    virtual void HitchStatusBits(StatusBits status_bits);
    virtual void UnhitchStatusBits(StatusBits &status_bits);
    virtual void UpdateIovIndex(uint32_t idx) = 0;

protected:
    int shiftRecvIovs(uint32_t bytes_want, uint32_t one_read, uint32_t bytes_read, int &start_iov_idx, int &start_iov_cnt);
    int shiftSendIovs(uint32_t bytes_req, uint32_t one_write, uint32_t bytes_written, int &start_iov_idx, int &start_iov_cnt);

    int recvSockExact(int fd, uint32_t bytes_want, int iov_idx, int iov_cnt);
    int sendSockExact(int fd, uint32_t bytes_req, int iov_idx, int iov_cnt);

    int recvMMExact(int fd, uint32_t bytes_want, int iov_idx, int iov_cnt, uint8_t *mm_ptr, size_t mm_size);
    int sendMMExact(int fd, uint32_t bytes_req, int iov_idx, int iov_cnt, uint8_t *mm_ptr, size_t mm_size);

    int unmarshallHdr();
    int marshallHdr();

    int unmarshallMMHdr();
    int marshallMMHdr();

    void marshall(uint8_t *&buf_ptr, uint32_t &val);
    void unmarshall(uint8_t *&buf_ptr, uint32_t &val);
    void marshall(uint8_t *&buf_ptr, uint64_t &val);
    void unmarshall(uint8_t *&buf_ptr, uint64_t &val);

    void marshallMM(uint8_t *&buf_ptr, uint32_t &val);
    void unmarshallMM(uint8_t *&buf_ptr, uint32_t &val);
    void marshallMM(uint8_t *&buf_ptr, uint64_t &val);
    void unmarshallMM(uint8_t *&buf_ptr, uint64_t &val);

public:
    void SetCombinedStatus(uint32_t _status_1, uint32_t _status_2);
    void GetCombinedStatus(uint32_t &_status_1, uint32_t &_status_2);
};

inline void IovMsgBase::SetCombinedStatus(uint32_t _status_1, uint32_t _status_2)
{
    iovhdr.descr.status = (_status_1 << 16) + _status_2;
}

inline void IovMsgBase::GetCombinedStatus(uint32_t &_status_1, uint32_t &_status_2)
{
    _status_1 = (iovhdr.descr.status >> 16) & 0xffff;
    _status_2 = iovhdr.descr.status & 0xffff;
}

inline void IovMsgBase::marshallMM(uint8_t *&buf_ptr, uint32_t &val)
{
    *reinterpret_cast<uint32_t *>(buf_ptr) = val;
    buf_ptr += 4;
}

inline void IovMsgBase::unmarshallMM(uint8_t *&buf_ptr, uint32_t &val)
{
    val = *reinterpret_cast<uint32_t *>(buf_ptr);
    buf_ptr += 4;
}

inline void IovMsgBase::marshallMM(uint8_t *&buf_ptr, uint64_t &val)
{
    *reinterpret_cast<uint64_t *>(buf_ptr) = val;
    buf_ptr += 8;
}

inline void IovMsgBase::unmarshallMM(uint8_t *&buf_ptr, uint64_t &val)
{
    val = *reinterpret_cast<uint64_t *>(buf_ptr);
    buf_ptr += 8;
}

inline bool IovMsgBase::isAsync()
{
    return iovhdr.hdr.type >= eMsgGetAsyncReq && iovhdr.hdr.type <= eMsgMergeAsyncRspNT;
}

inline bool IovMsgBase::isSync()
{
    return !isAsync();
}

inline void IovMsgBase::HitchIds()
{}

inline void IovMsgBase::UnHitchIds()
{}

inline void IovMsgBase::HitchStatusBits(StatusBits status_bits)
{}

inline void IovMsgBase::UnhitchStatusBits(StatusBits &status_bits)
{}

}



#endif /* SOE_SOE_SOE_MSGS_INCLUDE_DBSRVIOVMSGSBASE_HPP_ */
