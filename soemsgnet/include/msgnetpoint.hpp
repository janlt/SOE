/**
 * @file    msgnetpoint.hpp
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
 * msgnetpoint.hpp
 *
 *  Created on: Feb 6, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETPOINT_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETPOINT_HPP_

namespace MetaNet {

class Point;

struct PointHashEntry
{
    int          fd;
    Point       *point;
};

class Point
{
public:
    typedef enum _eType
    {
        eUndef  = 0,
        eTcp    = 1,
        eUdp    = 2,
        eUnix   = 3,
        eMM     = 4,
        eFifo   = 5
    } eType;

    typedef enum _eFdState
    {
        eInit    = 0,
        eOpened  = 1,
        eClosed  = 2,
        eInvalid = 3
    } eFdState;

public:
    eType        type;

private:
    int          desc;
    eFdState     state;

public:
    Point(eType _type, int _desc = -1);
    virtual ~Point();

    class Initializer
    {
    public:
        static bool initialized;
        Initializer();
        ~Initializer() {}
    };
    static Initializer points_initalized;

    virtual int Create(bool _srv = false) = 0;
    virtual int Close() = 0;
    virtual int Connect(const Address *addr) = 0;
    virtual int Accept(Address *addr) = 0;
    virtual int Create(int domain, int type, int protocol) = 0;
    virtual int SetSendBufferLen(int _buf_len) = 0;
    virtual int SetReceiveBufferLen(int _buf_len) = 0;
    virtual int SetReuseAddr() = 0;
    virtual int SetAddMcastMembership(const ip_mreq *_ip_mreq) = 0;
    virtual int SetMcastTtl(unsigned int _ttl_val) = 0;
    virtual int SetNonBlocking(bool _blocking) = 0;
    virtual int Cork() = 0;
    virtual int Uncork() = 0;
    virtual int SetMcastLoop() = 0;
    virtual int SetNoDelay() = 0;
    virtual int Bind(const sockaddr *_addr) = 0;
    virtual int SetNoSigPipe() = 0;
    virtual int SetRecvTmout(uint32_t msecs) = 0;
    virtual int SetSndTmout(uint32_t msecs) = 0;
    virtual int SetNoLinger(uint32_t tmout = 0) = 0;

    virtual int Register(bool _ignore = false, bool _force_register = false) throw(std::runtime_error);
    virtual int Deregister(bool _ignore = false, bool _force_deregister = false) throw(std::runtime_error);

    virtual int SendIov(MetaMsgs::IovMsgBase &io) = 0;
    virtual int RecvIov(MetaMsgs::IovMsgBase &io) = 0;

    virtual int GetDesc();
    virtual int GetConstDesc() const;
    virtual void SetDesc(int _desc);

    virtual int Send(const uint8_t *io, uint32_t io_size) = 0;
    virtual int Recv(uint8_t *io, uint32_t io_size) = 0;

    virtual bool IsConnected() = 0;

    const static uint32_t   max_points = 100000;
    static PointHashEntry   points[max_points];

    static Point *Find(int fd);

    eType GetType();
    void SetType(eType _type);
    bool Created();

    void SetStateOpened();
    void SetStateClosed();
    void SetStateInvalid();
    Point::eFdState GetState();

    int CheckFd();

public:
    static uint64_t   open_counter;
    static uint64_t   close_counter;
};

inline int Point::GetDesc()
{
    return desc;
}

inline int Point::GetConstDesc() const
{
    return desc;
}

inline void Point::SetDesc(int _desc)
{
    desc = _desc;
}

inline bool Point::Created()
{
    return desc < 0 ? false : true;
}

inline Point::eType Point::GetType()
{
    return type;
}

inline void Point::SetType(Point::eType _type)
{
    type = _type;
}

inline void Point::SetStateOpened()
{
    state = Point::eFdState::eOpened;
}

inline void Point::SetStateClosed()
{
    state = Point::eFdState::eClosed;
}

inline void Point::SetStateInvalid()
{
    state = Point::eFdState::eInvalid;
}

inline Point::eFdState Point::GetState()
{
    return state;
}
}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETPOINT_HPP_ */
