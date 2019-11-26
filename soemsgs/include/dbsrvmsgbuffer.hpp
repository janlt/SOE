/**
 * @file    dbsrvmsgbuffer.hpp
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
 * dbsrvmsgbuffer.hpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGBUFFER_HPP_
#define SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGBUFFER_HPP_

namespace MetaMsgs {

const unsigned int maxMsgBufSize = 8192;
const unsigned int maxMsgBigBufSize = 524288;

const unsigned int maxMsgIovecBufSize = maxMsgBigBufSize;

struct Uint8Vector
{
    uint8_t    *ptr;
    uint32_t    size;

    Uint8Vector(uint8_t *_ptr, uint32_t _size): ptr(_ptr), size(_size) {}
};



//
// MsgBuffer for polymorphism
//
class MsgBufferBase
{
public:
    MsgBufferBase() {}
    virtual ~MsgBufferBase() {}

    uint8_t *getBuf();
    const uint8_t *getCBuf();
    uint32_t getOffset();
    uint32_t getSize();

public:
    uint8_t        *buf;
    uint32_t        offset;
    uint32_t        size;
    bool            manage_buf;

    void setManageBuf(bool _manage_buf);
};

inline uint8_t* MsgBufferBase::getBuf()
{
    return buf;
}

inline const uint8_t* MsgBufferBase::getCBuf()
{
    return buf;
}

inline uint32_t MsgBufferBase::getOffset()
{
    return offset;
}

inline uint32_t MsgBufferBase::getSize()
{
    return size;
}

inline void MsgBufferBase::setManageBuf(bool _manage_buf)
{
    manage_buf = _manage_buf;
}




//
// AdmMsgBuffer
//
class AdmMsgBuffer: public MsgBufferBase
{
public:
    struct MsgBuffer
    {
        uint8_t         mem_buf[maxMsgBufSize];
        static MemMgmt::GeneralPool<AdmMsgBuffer::MsgBuffer,
            MemMgmt::ClassAlloc<AdmMsgBuffer::MsgBuffer> > pool;
    };

    struct MsgBigBuffer
    {
        uint8_t         mem_buf[maxMsgBigBufSize];
        static MemMgmt::GeneralPool<AdmMsgBuffer::MsgBigBuffer,
            MemMgmt::ClassAlloc<AdmMsgBuffer::MsgBigBuffer> > pool;
    };

    AdmMsgBuffer(const AdmMsgBuffer &right);
    AdmMsgBuffer(bool _big_buffer = false);
    AdmMsgBuffer(uint8_t *buf_, uint32_t len);
    virtual ~AdmMsgBuffer();
    void reset();

    AdmMsgBuffer& operator <<(uint64_t rval);
    AdmMsgBuffer& operator >>(uint64_t& rval);
    AdmMsgBuffer& operator <<(uint32_t rval);
    AdmMsgBuffer& operator >>(uint32_t& rval);
    AdmMsgBuffer& operator <<(uint16_t rval);
    AdmMsgBuffer& operator >>(uint16_t& rval);
    AdmMsgBuffer& operator <<(uint8_t rval);
    AdmMsgBuffer& operator >>(uint8_t& rval);
    AdmMsgBuffer& operator <<(const std::string& rval);
    AdmMsgBuffer& operator >>(std::string& rval);
    AdmMsgBuffer& operator <<(int64_t rval);
    AdmMsgBuffer& operator >>(int64_t& rval);
    AdmMsgBuffer& operator <<(int32_t rval);
    AdmMsgBuffer& operator >>(int32_t& rval);
    AdmMsgBuffer& operator <<(int16_t rval);
    AdmMsgBuffer& operator >>(int16_t& rval);
    AdmMsgBuffer& operator <<(int8_t rval);
    AdmMsgBuffer& operator >>(int8_t& rval);
    AdmMsgBuffer& operator <<(const char *rval);
    AdmMsgBuffer& operator >>(char* rval);
    AdmMsgBuffer& operator <<(const Uint8Vector &rval);
    AdmMsgBuffer& operator >>(Uint8Vector &rval);
    AdmMsgBuffer& operator <<(bool rval);
    AdmMsgBuffer& operator >>(bool& rval);

    // Data is appended to this buffer from argument buffer
    // from the beginning to its current offset.
    // Offset of this buffer is adjusted.
    // First 4 bytes written to the raw buffer contain size of
    // the following data and allow reconstruction.
    AdmMsgBuffer& operator <<(AdmMsgBuffer& rval);
    AdmMsgBuffer& operator >>(AdmMsgBuffer& rval);

    MsgBigBuffer *getMsgBigBuffer();

private:
    AdmMsgBuffer & operator=(const AdmMsgBuffer &right);

    MsgBuffer      *msg_buffer;
    MsgBigBuffer   *msg_big_buffer;
};

inline AdmMsgBuffer::MsgBigBuffer *AdmMsgBuffer::getMsgBigBuffer()
{
    return msg_big_buffer;
}




//
// McastMsgBuffer
//
class McastMsgBuffer: public MsgBufferBase
{
public:
    struct MsgBuffer
    {
        uint8_t         mem_buf[maxMsgBufSize];
        static MemMgmt::GeneralPool<McastMsgBuffer::MsgBuffer,
            MemMgmt::ClassAlloc<McastMsgBuffer::MsgBuffer> > pool;
    };

    struct MsgBigBuffer
    {
        uint8_t         mem_buf[maxMsgBigBufSize];
        static MemMgmt::GeneralPool<McastMsgBuffer::MsgBigBuffer,
            MemMgmt::ClassAlloc<McastMsgBuffer::MsgBigBuffer> > pool;
    };

    McastMsgBuffer(const McastMsgBuffer &right);
    McastMsgBuffer(bool _big_buffer = false);
    McastMsgBuffer(uint8_t *buf_, uint32_t len);
    virtual ~McastMsgBuffer();
    void reset();

    McastMsgBuffer& operator <<(uint64_t rval);
    McastMsgBuffer& operator >>(uint64_t& rval);
    McastMsgBuffer& operator <<(uint32_t rval);
    McastMsgBuffer& operator >>(uint32_t& rval);
    McastMsgBuffer& operator <<(uint16_t rval);
    McastMsgBuffer& operator >>(uint16_t& rval);
    McastMsgBuffer& operator <<(uint8_t rval);
    McastMsgBuffer& operator >>(uint8_t& rval);
    McastMsgBuffer& operator <<(const std::string& rval);
    McastMsgBuffer& operator >>(std::string& rval);
    McastMsgBuffer& operator <<(int64_t rval);
    McastMsgBuffer& operator >>(int64_t& rval);
    McastMsgBuffer& operator <<(int32_t rval);
    McastMsgBuffer& operator >>(int32_t& rval);
    McastMsgBuffer& operator <<(int16_t rval);
    McastMsgBuffer& operator >>(int16_t& rval);
    McastMsgBuffer& operator <<(int8_t rval);
    McastMsgBuffer& operator >>(int8_t& rval);
    McastMsgBuffer& operator <<(const char *rval);
    McastMsgBuffer& operator >>(char* rval);
    McastMsgBuffer& operator <<(const Uint8Vector &rval);
    McastMsgBuffer& operator >>(Uint8Vector &rval);
    McastMsgBuffer& operator <<(bool rval);
    McastMsgBuffer& operator >>(bool& rval);

    // Data is appended to this buffer from argument buffer
    // from the beginning to its current offset.
    // Offset of this buffer is adjusted.
    // First 4 bytes written to the raw buffer contain size of
    // the following data and allow reconstruction.
    McastMsgBuffer& operator <<(McastMsgBuffer& rval);
    McastMsgBuffer& operator >>(McastMsgBuffer& rval);

    MsgBigBuffer *getMsgBigBuffer();

private:
    McastMsgBuffer & operator=(const McastMsgBuffer &right);

    MsgBuffer      *msg_buffer;
    MsgBigBuffer   *msg_big_buffer;
};

inline McastMsgBuffer::MsgBigBuffer *McastMsgBuffer::getMsgBigBuffer()
{
    return msg_big_buffer;
}

}


#endif /* SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGBUFFER_HPP_ */
