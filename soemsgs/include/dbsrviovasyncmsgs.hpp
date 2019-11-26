/**
 * @file    dbsrviovasyncmsgs.hpp
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
 * dbsrviovasyncmsgs.hpp
 *
 *  Created on: Nov 30, 2017
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_MSGS_INCLUDE_DBSRVIOVASYNCMSGS_HPP_
#define RENTCONTROL_SOE_SOE_SOE_MSGS_INCLUDE_DBSRVIOVASYNCMSGS_HPP_

namespace MetaMsgs {

template <class T>
class IovMsgAsyncReq: public IovMsgReq<T>
{
public:
    uint64_t   seq;

    IovMsgAsyncReq(eMsgType _type = eMsgInvalid);
    virtual ~IovMsgAsyncReq() {}

public:
    int recvMsg(T &rcv);
    int sendMsg(T &snd);
    virtual void HitchIds();
    virtual void UnHitchIds();
    void setSeq(uint64_t _seq);
    uint64_t getSeq();
};

template <class T>
inline IovMsgAsyncReq<T>::IovMsgAsyncReq(eMsgType _type)
    :IovMsgReq<T>(_type),
     seq(0)
{}

template <class T>
inline int IovMsgAsyncReq<T>::recvMsg(T &rcv)
{
    return rcv.recv(this);
}

template <class T>
inline int IovMsgAsyncReq<T>::sendMsg(T &snd)
{
    return snd.send(this);
}

template <class T>
inline void IovMsgAsyncReq<T>::setSeq(uint64_t _seq)
{
    seq = _seq;
    IovMsgBase::iovhdr.seq = seq;
}

template <class T>
inline uint64_t IovMsgAsyncReq<T>::getSeq()
{
    seq = IovMsgBase::iovhdr.seq;
    return seq;
}

template <class T>
inline void IovMsgAsyncReq<T>::HitchIds()
{
    return IovMsgReq<T>::HitchIds();
}

template <class T>
inline void IovMsgAsyncReq<T>::UnHitchIds()
{
    return IovMsgReq<T>::UnHitchIds();
}




template <class T>
class IovMsgAsyncRsp: public IovMsgRsp<T>
{
public:
    uint64_t   seq;

    IovMsgAsyncRsp(eMsgType _type = eMsgInvalid);
    virtual ~IovMsgAsyncRsp() {}

public:
    int recvMsg(T &rcv);
    int sendMsg(T &snd);
    virtual void HitchIds();
    virtual void UnHitchIds();
    void setSeq(uint64_t _seq);
    uint64_t getSeq();
};

template <class T>
inline IovMsgAsyncRsp<T>::IovMsgAsyncRsp(eMsgType _type)
    :IovMsgRsp<T>(_type),
     seq(0)
{}

template <class T>
inline int IovMsgAsyncRsp<T>::recvMsg(T &rcv)
{
    return rcv.recv(this);
}

template <class T>
inline int IovMsgAsyncRsp<T>::sendMsg(T &snd)
{
    return snd.send(this);
}

template <class T>
inline void IovMsgAsyncRsp<T>::setSeq(uint64_t _seq)
{
    seq = _seq;
    IovMsgBase::iovhdr.seq = seq;
}

template <class T>
inline uint64_t IovMsgAsyncRsp<T>::getSeq()
{
    seq = IovMsgBase::iovhdr.seq;
    return seq;
}

template <class T>
inline void IovMsgAsyncRsp<T>::HitchIds()
{
    return IovMsgRsp<T>::HitchIds();
}

template <class T>
inline void IovMsgAsyncRsp<T>::UnHitchIds()
{
    return IovMsgRsp<T>::UnHitchIds();
}




//
// NT versions
//
template <class T>
class IovMsgAsyncReqNT: public IovMsgReq<T>
{
public:
    uint64_t   seq;

    IovMsgAsyncReqNT(eMsgType _type = eMsgInvalid): IovMsgReq<T>(_type), seq(0) {}
    virtual ~IovMsgAsyncReqNT() {}

public:
    int recv(int fd);
    int send(int fd);
    void setSeq(uint64_t _seq);
    uint64_t getSeq();
};

template <class T>
int IovMsgAsyncReqNT<T>::recv(int fd)
{
    return 0;
}

template <class T>
int IovMsgAsyncReqNT<T>::send(int fd)
{
    return 0;
}

template <class T>
void IovMsgAsyncReqNT<T>::setSeq(uint64_t _seq)
{
    seq = _seq;
    IovMsgBase::iovhdr.seq = seq;
}

template <class T>
uint64_t IovMsgAsyncReqNT<T>::getSeq()
{
    seq = IovMsgBase::iovhdr.seq;
    return seq;
}




template <class T>
class IovMsgAsyncRspNT: public IovMsgRsp<T>
{
public:
    uint64_t   seq;

    IovMsgAsyncRspNT(eMsgType _type = eMsgInvalid): IovMsgRsp<T>(_type), seq(0) {}
    virtual ~IovMsgAsyncRspNT() {}

public:
    int recv(int fd);
    int send(int fd);
    void setSeq(uint64_t _seq);
    uint64_t getSeq();
};

template <class T>
int IovMsgAsyncRspNT<T>::recv(int fd)
{
    return 0;
}

template <class T>
int IovMsgAsyncRspNT<T>::send(int fd)
{
    return 0;
}

template <class T>
void IovMsgAsyncRspNT<T>::setSeq(uint64_t _seq)
{
    seq = _seq;
    IovMsgBase::iovhdr.seq = seq;
}

template <class T>
uint64_t IovMsgAsyncRspNT<T>::getSeq()
{
    seq = IovMsgBase::iovhdr.seq;
    return seq;
}

}

#endif /* RENTCONTROL_SOE_SOE_SOE_MSGS_INCLUDE_DBSRVIOVASYNCMSGS_HPP_ */
