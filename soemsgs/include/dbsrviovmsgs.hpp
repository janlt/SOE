/**
 * @file    dbsrviovmsgs.hpp
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
 * dbsrviovmsgs.hpp
 *
 *  Created on: Feb 27, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGS_INCLUDE_DBSRVIOVMSGS_HPP_
#define SOE_SOE_SOE_MSGS_INCLUDE_DBSRVIOVMSGS_HPP_



namespace MetaMsgs {

template <class T>
class IovMsgReq: public IovMsgBase
{
public:
    IovMsgReq(eMsgType _type = eMsgInvalid);
    virtual ~IovMsgReq() {}

public:
    int recvMsg(T &rcv);
    int sendMsg(T &snd);
    void SetUpIovIndexes();
    virtual void UpdateIovIndex(uint32_t idx);
    virtual void HitchIds();
    virtual void UnHitchIds();
    virtual void HitchStatusBits(StatusBits status_bits);
    virtual void UnhitchStatusBits(StatusBits &status_bits);
    int SetIovSize(uint32_t idx, uint32_t size);
    int GetIovSize(uint32_t idx, uint32_t &size);

public:
    IovMetaDbDescriptor meta_desc;

    static const uint32_t cluster_name_idx = 1;
    static const uint32_t space_name_idx   = 2;
    static const uint32_t store_name_idx   = 3;
    static const uint32_t key_idx          = 4;
    static const uint32_t end_key_idx      = 5;
    static const uint32_t data_idx         = 6;
    static const uint32_t buf_idx          = 7;
};

template <class T>
IovMsgReq<T>::IovMsgReq(eMsgType _type)
    :IovMsgBase(_type)
{
    SetUpIovIndexes();
}

template <class T>
void IovMsgReq<T>::HitchIds()
{
    *reinterpret_cast<uint64_t *>(static_cast<char *>(iov[meta_desc.desc.cluster_iov].iov_base) +
        iov[meta_desc.desc.cluster_iov].iov_len) =  meta_desc.desc.cluster_id;
    (void )SetIovSize(cluster_name_idx, iov[meta_desc.desc.cluster_iov].iov_len + sizeof(meta_desc.desc.cluster_id));

    *reinterpret_cast<uint64_t *>(static_cast<char *>(iov[meta_desc.desc.space_iov].iov_base) +
        iov[meta_desc.desc.space_iov].iov_len) =  meta_desc.desc.space_id;
    (void )SetIovSize(space_name_idx, iov[meta_desc.desc.space_iov].iov_len + sizeof(meta_desc.desc.space_id));

    *reinterpret_cast<uint64_t *>(static_cast<char *>(iov[meta_desc.desc.store_iov].iov_base) +
        iov[meta_desc.desc.store_iov].iov_len) =  meta_desc.desc.store_id;
    (void )SetIovSize(store_name_idx, iov[meta_desc.desc.store_iov].iov_len + sizeof(meta_desc.desc.store_id));
}

template <class T>
void IovMsgReq<T>::UnHitchIds()
{
    meta_desc.desc.cluster_id = *reinterpret_cast<uint64_t *>(static_cast<char *>(iov[meta_desc.desc.cluster_iov].iov_base) +
        iov[meta_desc.desc.cluster_iov].iov_len - sizeof(meta_desc.desc.cluster_id));
    *(static_cast<char *>(iov[meta_desc.desc.cluster_iov].iov_base) + iov[meta_desc.desc.cluster_iov].iov_len - sizeof(meta_desc.desc.cluster_id)) = '\0';
    (void )SetIovSize(cluster_name_idx, iov[meta_desc.desc.cluster_iov].iov_len - sizeof(meta_desc.desc.cluster_id));

    meta_desc.desc.space_id = *reinterpret_cast<uint64_t *>(static_cast<char *>(iov[meta_desc.desc.space_iov].iov_base) +
        iov[meta_desc.desc.space_iov].iov_len - sizeof(meta_desc.desc.space_id));
    *(static_cast<char *>(iov[meta_desc.desc.space_iov].iov_base) + iov[meta_desc.desc.space_iov].iov_len - sizeof(meta_desc.desc.space_id)) = '\0';
    (void )SetIovSize(space_name_idx, iov[meta_desc.desc.space_iov].iov_len - sizeof(meta_desc.desc.space_id));

    meta_desc.desc.store_id = *reinterpret_cast<uint64_t *>(static_cast<char *>(iov[meta_desc.desc.store_iov].iov_base) +
        iov[meta_desc.desc.store_iov].iov_len - sizeof(meta_desc.desc.store_id));
    *(static_cast<char *>(iov[meta_desc.desc.store_iov].iov_base) + iov[meta_desc.desc.store_iov].iov_len - sizeof(meta_desc.desc.store_id)) = '\0';
    (void )SetIovSize(store_name_idx, iov[meta_desc.desc.store_iov].iov_len - sizeof(meta_desc.desc.store_id));
}

template <class T>
void IovMsgReq<T>::SetUpIovIndexes()
{
    ::memset(&meta_desc.desc, '\0', sizeof(meta_desc.desc));

    meta_desc.desc.cluster_iov = cluster_name_idx;
    meta_desc.desc.cluster_name = static_cast<char *>(iov[meta_desc.desc.cluster_iov].iov_base);

    meta_desc.desc.space_iov = space_name_idx;
    meta_desc.desc.space_name = static_cast<char *>(iov[meta_desc.desc.space_iov].iov_base);

    meta_desc.desc.store_iov = store_name_idx;
    meta_desc.desc.store_name = static_cast<char *>(iov[meta_desc.desc.store_iov].iov_base);

    meta_desc.desc.key_iov = key_idx;
    meta_desc.desc.key = static_cast<char *>(iov[meta_desc.desc.key_iov].iov_base);

    meta_desc.desc.end_key_iov = end_key_idx;
    meta_desc.desc.end_key = static_cast<char *>(iov[meta_desc.desc.end_key_iov].iov_base);

    meta_desc.desc.data_iov = data_idx;
    meta_desc.desc.data = static_cast<char *>(iov[meta_desc.desc.data_iov].iov_base);

    meta_desc.desc.buf_iov = buf_idx;
    meta_desc.desc.buf = static_cast<char *>(iov[meta_desc.desc.buf_iov].iov_base);

    iovhdr.descr.list_length = buf_idx;
}

template <class T>
void IovMsgReq<T>::UpdateIovIndex(uint32_t idx)
{
    switch ( idx ) {
    case cluster_name_idx:
        meta_desc.desc.cluster_name = static_cast<char *>(iov[cluster_name_idx].iov_base);
        break;
    case space_name_idx:
        meta_desc.desc.space_name = static_cast<char *>(iov[space_name_idx].iov_base);
        break;
    case store_name_idx:
        meta_desc.desc.store_name = static_cast<char *>(iov[store_name_idx].iov_base);
        break;
    case key_idx:
        meta_desc.desc.key = static_cast<char *>(iov[key_idx].iov_base);
        break;
    case end_key_idx:
        meta_desc.desc.end_key = static_cast<char *>(iov[end_key_idx].iov_base);
        break;
    case data_idx:
        meta_desc.desc.data = static_cast<char *>(iov[data_idx].iov_base);
        break;
    case buf_idx:
        meta_desc.desc.buf = static_cast<char *>(iov[buf_idx].iov_base);
        break;
    default:
        break;
    }
}

template <class T>
void IovMsgReq<T>::HitchStatusBits(StatusBits status_bits)
{
    iovhdr.descr.status = status_bits.status_bits;
}

template <class T>
void IovMsgReq<T>::UnhitchStatusBits(StatusBits &status_bits)
{
    status_bits.status_bits = iovhdr.descr.status;
}

template <class T>
inline int IovMsgReq<T>::SetIovSize(uint32_t idx, uint32_t size)
{
    if ( idx > buf_idx || size >= static_cast<size_t>(iov_nonsense_buf_size) ) {
#ifdef __GNUC__
        cerr << "index out of range or size too big: " << idx << " " << size << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "index out of range or size too big: " << idx << " " << size << SoeLogger::LogError() << endl;
#endif
        return -1;
    }

    if ( size ) {
        if ( size > iov_sizes[idx] ) {
            //cout << "IDX(0)" << idx << " size: " << size << endl;
            delete[] static_cast<uint8_t *>(iov[idx].iov_base);
            iov[idx].iov_base = static_cast<void *>(new uint8_t[size + 8]);
            iov[idx].iov_len = static_cast<size_t>(size);
            iov_sizes[idx] = size + 8;
            UpdateIovIndex(idx);
        } else {
            iov[idx].iov_len = static_cast<size_t>(size);
        }
        iovhdr.descr.list[idx - 1] = size;
    } else {
        iov[idx].iov_len = static_cast<size_t>(MsgSomethingAnywayLength);
        iovhdr.descr.list[idx - 1] = static_cast<size_t>(MsgSomethingAnywayLength);
#ifdef __GNUC__
        ::memcpy(iov[idx].iov_base, IovMsgBase::something_anyway, sizeof(IovMsgBase::something_anyway));
#else
        *reinterpret_cast<uint64_t *>(iov[idx].iov_base) = *reinterpret_cast<uint64_t *>(IovMsgBase::something_anyway);
#endif
    }
    return 0;
}

template <class T>
inline int IovMsgReq<T>::GetIovSize(uint32_t idx, uint32_t &size)
{
    if ( idx > buf_idx || !idx ) {
#ifdef __GNUC__
        cerr << "index out of range: " << idx << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "index out of range: " << idx << SoeLogger::LogError() << endl;
#endif
        return -1;
    }

    size = iovhdr.descr.list[idx - 1];
    return 0;
}

template <class T>
inline int IovMsgReq<T>::recvMsg(T &rcv)
{
    return rcv.recv(this);
}

template <class T>
inline int IovMsgReq<T>::sendMsg(T &snd)
{
    return snd.send(this);
}



template <class T>
class IovMsgRsp: public IovMsgBase
{
public:
    uint32_t   status;

    IovMsgRsp(eMsgType _type = eMsgInvalid);
    virtual ~IovMsgRsp() {}

public:
    int recvMsg(T &rcv);
    int sendMsg(T &snd);
    void SetUpIovIndexes();
    virtual void UpdateIovIndex(uint32_t idx);
    virtual void HitchIds();
    virtual void UnHitchIds();
    int SetIovSize(uint32_t idx, uint32_t size);
    int GetIovSize(uint32_t idx, uint32_t &size);

public:
    IovMetaDbDescriptor meta_desc;
};

template <class T>
inline IovMsgRsp<T>::IovMsgRsp(eMsgType _type)
    :IovMsgBase(_type),
     status(0)
{
    SetUpIovIndexes();
}

template <class T>
void IovMsgRsp<T>::HitchIds()
{
    *reinterpret_cast<uint64_t *>(static_cast<char *>(iov[meta_desc.desc.cluster_iov].iov_base) +
        iov[meta_desc.desc.cluster_iov].iov_len) =  meta_desc.desc.cluster_id;
    (void )SetIovSize(cluster_name_idx, iov[meta_desc.desc.cluster_iov].iov_len + sizeof(meta_desc.desc.cluster_id));

    *reinterpret_cast<uint64_t *>(static_cast<char *>(iov[meta_desc.desc.space_iov].iov_base) +
        iov[meta_desc.desc.space_iov].iov_len) =  meta_desc.desc.space_id;
    (void )SetIovSize(space_name_idx, iov[meta_desc.desc.space_iov].iov_len + sizeof(meta_desc.desc.space_id));

    *reinterpret_cast<uint64_t *>(static_cast<char *>(iov[meta_desc.desc.store_iov].iov_base) +
        iov[meta_desc.desc.store_iov].iov_len) =  meta_desc.desc.store_id;
    (void )SetIovSize(store_name_idx, iov[meta_desc.desc.store_iov].iov_len + sizeof(meta_desc.desc.store_id));
}

template <class T>
void IovMsgRsp<T>::UnHitchIds()
{
    meta_desc.desc.cluster_id = *reinterpret_cast<uint64_t *>(static_cast<char *>(iov[meta_desc.desc.cluster_iov].iov_base) +
        iov[meta_desc.desc.cluster_iov].iov_len - sizeof(meta_desc.desc.cluster_id));
    *(static_cast<char *>(iov[meta_desc.desc.cluster_iov].iov_base) + iov[meta_desc.desc.cluster_iov].iov_len - sizeof(meta_desc.desc.cluster_id)) = '\0';
    iov[meta_desc.desc.cluster_iov].iov_len -= sizeof(meta_desc.desc.cluster_id);

    meta_desc.desc.space_id = *reinterpret_cast<uint64_t *>(static_cast<char *>(iov[meta_desc.desc.space_iov].iov_base) +
        iov[meta_desc.desc.space_iov].iov_len - sizeof(meta_desc.desc.space_id));
    *(static_cast<char *>(iov[meta_desc.desc.space_iov].iov_base) + iov[meta_desc.desc.space_iov].iov_len - sizeof(meta_desc.desc.space_id)) = '\0';
    iov[meta_desc.desc.space_iov].iov_len -= sizeof(meta_desc.desc.space_id);

    meta_desc.desc.store_id = *reinterpret_cast<uint64_t *>(static_cast<char *>(iov[meta_desc.desc.store_iov].iov_base) +
        iov[meta_desc.desc.store_iov].iov_len - sizeof(meta_desc.desc.store_id));
    *(static_cast<char *>(iov[meta_desc.desc.store_iov].iov_base) + iov[meta_desc.desc.store_iov].iov_len - sizeof(meta_desc.desc.store_id)) = '\0';
    iov[meta_desc.desc.store_iov].iov_len -= sizeof(meta_desc.desc.store_id);
}

template <class T>
void IovMsgRsp<T>::SetUpIovIndexes()
{
    ::memset(&meta_desc.desc, '\0', sizeof(meta_desc.desc));

    meta_desc.desc.cluster_iov = cluster_name_idx;
    meta_desc.desc.cluster_name = static_cast<char *>(iov[meta_desc.desc.cluster_iov].iov_base);

    meta_desc.desc.space_iov = space_name_idx;
    meta_desc.desc.space_name = static_cast<char *>(iov[meta_desc.desc.space_iov].iov_base);

    meta_desc.desc.store_iov = store_name_idx;
    meta_desc.desc.store_name = static_cast<char *>(iov[meta_desc.desc.store_iov].iov_base);

    meta_desc.desc.key_iov = key_idx;
    meta_desc.desc.key = static_cast<char *>(iov[meta_desc.desc.key_iov].iov_base);

    meta_desc.desc.end_key_iov = end_key_idx;
    meta_desc.desc.end_key = static_cast<char *>(iov[meta_desc.desc.end_key_iov].iov_base);

    meta_desc.desc.data_iov = data_idx;
    meta_desc.desc.data = static_cast<char *>(iov[meta_desc.desc.data_iov].iov_base);

    meta_desc.desc.buf_iov = buf_idx;
    meta_desc.desc.buf = static_cast<char *>(iov[meta_desc.desc.buf_iov].iov_base);

    iovhdr.descr.list_length = buf_idx;
}

template <class T>
void IovMsgRsp<T>::UpdateIovIndex(uint32_t idx)
{
    switch ( idx ) {
    case cluster_name_idx:
        meta_desc.desc.cluster_name = static_cast<char *>(iov[cluster_name_idx].iov_base);
        break;
    case space_name_idx:
        meta_desc.desc.space_name = static_cast<char *>(iov[space_name_idx].iov_base);
        break;
    case store_name_idx:
        meta_desc.desc.store_name = static_cast<char *>(iov[store_name_idx].iov_base);
        break;
    case key_idx:
        meta_desc.desc.key = static_cast<char *>(iov[key_idx].iov_base);
        break;
    case end_key_idx:
        meta_desc.desc.end_key = static_cast<char *>(iov[end_key_idx].iov_base);
        break;
    case data_idx:
        meta_desc.desc.data = static_cast<char *>(iov[data_idx].iov_base);
        break;
    case buf_idx:
        meta_desc.desc.buf = static_cast<char *>(iov[buf_idx].iov_base);
        break;
    default:
        break;
    }
}

template <class T>
inline int IovMsgRsp<T>::SetIovSize(uint32_t idx, uint32_t size)
{
    if ( idx > buf_idx || size >= static_cast<size_t>(iov_nonsense_buf_size) ) {
#ifdef __GNUC__
        cerr << "index out of range or size too big: " << idx << " " << size << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "index out of range or size too big: " << idx << " " << size << SoeLogger::LogError() << endl;
#endif
        return -1;
    }

    if ( size ) {
        if ( size > iov_sizes[idx] ) {
            //cout << "IDX(1)" << idx << " size: " << size << endl;
            delete[] static_cast<uint8_t *>(iov[idx].iov_base);
            iov[idx].iov_base = static_cast<void *>(new uint8_t[size + 8]);
            iov[idx].iov_len = static_cast<size_t>(size);
            iov_sizes[idx] = size + 8;
            UpdateIovIndex(idx);
        } else {
            iov[idx].iov_len = static_cast<size_t>(size);
        }
        iovhdr.descr.list[idx - 1] = size;
    } else {
        iov[idx].iov_len = static_cast<size_t>(MsgSomethingAnywayLength);
        iovhdr.descr.list[idx - 1] = static_cast<size_t>(MsgSomethingAnywayLength);
#ifdef __GNUC__
        ::memcpy(iov[idx].iov_base, IovMsgBase::something_anyway, sizeof(IovMsgBase::something_anyway));
#else
        *reinterpret_cast<uint64_t *>(iov[idx].iov_base) = *reinterpret_cast<uint64_t *>(IovMsgBase::something_anyway);
#endif
    }
    return 0;
}

template <class T>
inline int IovMsgRsp<T>::GetIovSize(uint32_t idx, uint32_t &size)
{
    if ( idx > buf_idx || !idx ) {
#ifdef __GNUC__
        cerr << "index out of range: " << idx << endl;
#else
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "index out of range: " << idx << SoeLogger::LogError() << endl;
#endif
        return -1;
    }

    size = iovhdr.descr.list[idx - 1];
    return 0;
}

template <class T>
int IovMsgRsp<T>::recvMsg(T &rcv)
{
    return rcv.recv(this);
}

template <class T>
int IovMsgRsp<T>::sendMsg(T &snd)
{
    return snd.send(this);
}



//
// Non template versions
//
class IovMsgReqNT: public IovMsgBase
{
public:
    IovMsgReqNT(eMsgType _type = eMsgInvalid): IovMsgBase(_type) {}
    virtual ~IovMsgReqNT() {}

public:
    int recv(int fd);
    int send(int fd);
    virtual void UpdateIovIndex(uint32_t idx);

private:
    IovMetaDbDescriptor meta_desc;
};



class IovMsgRspNT: public IovMsgBase
{
public:
    uint32_t   status;

    IovMsgRspNT(eMsgType _type = eMsgInvalid): IovMsgBase(_type), status(0) {}
    virtual ~IovMsgRspNT() {}

public:
    int recv(int fd);
    int send(int fd);
    virtual void UpdateIovIndex(uint32_t idx);

private:
    IovMetaDbDescriptor meta_desc;
};

}

#endif /* SOE_SOE_SOE_MSGS_INCLUDE_DBSRVIOVMSGS_HPP_ */
