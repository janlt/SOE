/**
 * @file    dbsrvmarshallable.cpp
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
 * dbsrvmarshallable.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jan Lisowiec
 */

#include <stdio.h>
#include <string.h>

#include <string>

#include "generalpool.hpp"
#include "dbsrvmsgbuffer.hpp"

#include <netinet/in.h>

using namespace std;

namespace MetaMsgs {

MemMgmt::GeneralPool<AdmMsgBuffer::MsgBuffer, MemMgmt::ClassAlloc<AdmMsgBuffer::MsgBuffer> > AdmMsgBuffer::MsgBuffer::pool;
MemMgmt::GeneralPool<AdmMsgBuffer::MsgBigBuffer, MemMgmt::ClassAlloc<AdmMsgBuffer::MsgBigBuffer> > AdmMsgBuffer::MsgBigBuffer::pool;

AdmMsgBuffer::AdmMsgBuffer(const AdmMsgBuffer &right)
    :msg_buffer(0),
    msg_big_buffer(0)
{
    offset = right.offset;
    size = right.size;
    manage_buf = right.manage_buf;

    if ( size == maxMsgBufSize ) {
        msg_buffer = AdmMsgBuffer::MsgBuffer::pool.get();
        buf = msg_buffer->mem_buf;
    } else if ( size == maxMsgBigBufSize ) {
        msg_big_buffer = AdmMsgBuffer::MsgBigBuffer::pool.get();
        buf = msg_big_buffer->mem_buf;
    } else {
        ;
    }

    if (size) {
        ::memcpy(buf, right.buf, offset);
    }
}

AdmMsgBuffer::AdmMsgBuffer (bool _big_buffer)
    :msg_buffer(0),
    msg_big_buffer(0)
{
    buf = 0;
    offset = 0;
    size = 0;
    manage_buf = true;

    if ( _big_buffer == true ) {
        msg_big_buffer = AdmMsgBuffer::MsgBigBuffer::pool.get();
        buf = msg_big_buffer->mem_buf;
        size = maxMsgBigBufSize;
    } else {
        msg_buffer = AdmMsgBuffer::MsgBuffer::pool.get();
        buf = msg_buffer->mem_buf;
        size = maxMsgBufSize;
    }
}

AdmMsgBuffer::AdmMsgBuffer (uint8_t* buf_, uint32_t len)
    :msg_buffer(0),
    msg_big_buffer(0)
{
    buf = buf_;
    offset = 0;
    size = len;
    manage_buf = false;
}

AdmMsgBuffer::~AdmMsgBuffer()
{
    if ( manage_buf == true ) {
        if ( size == maxMsgBufSize && msg_buffer ) {
            AdmMsgBuffer::MsgBuffer::pool.put(msg_buffer);
        } else if ( size == maxMsgBigBufSize && msg_big_buffer ) {
            AdmMsgBuffer::MsgBigBuffer::pool.put(msg_big_buffer);
        } else {
            ;
        }

        buf = 0;
        msg_buffer = 0;
        msg_big_buffer = 0;
        size = 0;
    }
}

void AdmMsgBuffer::reset()
{
    offset = 0;
}

AdmMsgBuffer& AdmMsgBuffer::operator << (uint64_t rval)
{
    uint8_t *ptr = buf + offset;

    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff;

    offset += 8;

    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator >> (uint64_t &rval)
{
    uint8_t* ptr = buf + offset + 7;

    rval = *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;

    offset += 8;

    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator << (uint32_t rval)
{
    uint8_t* ptr = buf + offset;

    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff;

    offset += 4;

    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator >> (uint32_t &rval)
{
    uint8_t* ptr = buf + offset + 3;

    rval = *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;

    offset += 4;

    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator << (uint16_t rval)
{
    uint8_t* ptr = buf + offset;

    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff;

    offset += 2;

    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator >> (uint16_t &rval)
{
    uint8_t* ptr = buf + offset + 1;

    rval = *ptr--;
    rval = rval << 8 | *ptr--;

    offset += 2;

    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator << (uint8_t rval)
{
    buf[offset++] = rval;
    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator >> (uint8_t &rval)
{
    rval = buf[offset++];
    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator << (const string &rval)
{
    const uint16_t length = static_cast<uint16_t>(rval.length());
    operator<<(length);
    if ( length ) {
        const char* t = rval.data();
        ::memcpy(&buf[offset], t, length);
        offset += length;
    }
    /*
    for (int i = 0; i < length; i++)
    buf[offset++] = *t++;
    */
    return *this;
    }

AdmMsgBuffer& AdmMsgBuffer::operator >> (string &rval)
{
    uint16_t length;
    operator>>(length);

    if ( length ) {
        char save = buf[offset + length];
        buf[offset + length] = '\0';
        rval = (char*)&buf[offset];
        buf[offset + length] = save;
    }
    offset += length;
    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator << (int64_t rval)
{
    uint8_t *ptr = buf + offset;

    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff;

    offset += 8;

    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator >> (int64_t &rval)
{
    uint8_t *ptr = buf + offset + 7;

    rval = *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;

    offset += 8;

    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator << (int32_t rval)
{
    uint8_t *ptr = buf + offset;

    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff;

    offset += 4;

    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator >> (int32_t &rval)
{
    uint8_t *ptr = buf + offset + 3;

    rval = *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;

    offset += 4;

    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator << (int16_t rval)
{
    uint8_t *ptr = buf + offset;

    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff;

    offset += 2;

    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator >> (int16_t &rval)
{
    uint8_t *ptr = buf + offset + 1;

    rval = *ptr--;
    rval = rval << 8 | *ptr--;

    offset += 2;

    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator << (int8_t rval)
{
    buf[offset++] = rval;
    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator >> (int8_t &rval)
{
    rval = buf[offset++];
    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator << (bool rval)
{
    buf[offset++] = rval;
    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator >> (bool& rval)
{
    rval = buf[offset++];
    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator <<(const Uint8Vector &rval)
{
    ::memcpy(buf + offset, rval.ptr, rval.size);
    offset += rval.size;
    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator >>(Uint8Vector &rval)
{
    ::memcpy(rval.ptr, buf + offset, rval.size);
    offset += rval.size;
    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator << (AdmMsgBuffer& rval)
{
    // We copy the data from rval buffer from the beginning to its current offset.
    // The size of copied data is marshalled into this buffer to allow reconstruction.
    // Offset of this buffer is adjusted.
    uint32_t rsz = rval.getOffset();
    *this << rsz;
    ::memcpy( &buf[offset], &rval.buf[0], rsz );
    offset += rsz;
    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator >> (AdmMsgBuffer& rval)
{
    // We reconstruct rval data from this data.
    // Offset of this buffer is adjusted, offset of rval buffer points to the end of restored data.
    // Thie size of data to be reconstructed is extracted from data itself (first 4 bytes)
    uint32_t rsz;
    *this >> rsz;
    ::memcpy(&rval.buf[0], &buf[offset], rsz);
    offset += rsz;
    rval.offset += rsz;
    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator << (const char *rval)
{
    const uint16_t length = strlen(rval);
    operator<<(length);
    ::memcpy(buf + offset, rval, length);
    offset += length;
    return *this;
}

AdmMsgBuffer& AdmMsgBuffer::operator >> (char* rval)
{
    uint16_t length;
    operator>>(length);

    memcpy(rval, buf + offset, length);
    rval[length] = '\0';
    offset += length;
    return *this;
}




MemMgmt::GeneralPool<McastMsgBuffer::MsgBuffer, MemMgmt::ClassAlloc<McastMsgBuffer::MsgBuffer> > McastMsgBuffer::MsgBuffer::pool;
MemMgmt::GeneralPool<McastMsgBuffer::MsgBigBuffer, MemMgmt::ClassAlloc<McastMsgBuffer::MsgBigBuffer> > McastMsgBuffer::MsgBigBuffer::pool;

McastMsgBuffer::McastMsgBuffer(const McastMsgBuffer &right)
    :msg_buffer(0),
    msg_big_buffer(0)
{
    offset = right.offset;
    size = right.size;
    manage_buf = right.manage_buf;

    if ( size == maxMsgBufSize ) {
        msg_buffer = McastMsgBuffer::MsgBuffer::pool.get();
        buf = msg_buffer->mem_buf;
    } else if ( size == maxMsgBigBufSize ) {
        msg_big_buffer = McastMsgBuffer::MsgBigBuffer::pool.get();
        buf = msg_big_buffer->mem_buf;
    } else {
        ;
    }

    if (size) {
        ::memcpy(buf, right.buf, offset);
    }
}

McastMsgBuffer::McastMsgBuffer (bool _big_buffer)
    :msg_buffer(0),
    msg_big_buffer(0)
{
    buf = 0;
    offset = 0;
    size = 0;
    manage_buf = true;

    if ( _big_buffer == true ) {
        msg_big_buffer = McastMsgBuffer::MsgBigBuffer::pool.get();
        buf = msg_big_buffer->mem_buf;
        size = maxMsgBigBufSize;
    } else {
        msg_buffer = McastMsgBuffer::MsgBuffer::pool.get();
        buf = msg_buffer->mem_buf;
        size = maxMsgBufSize;
    }
}

McastMsgBuffer::McastMsgBuffer (uint8_t* buf_, uint32_t len)
    :msg_buffer(0),
    msg_big_buffer(0)
{
    buf = buf_;
    offset = 0;
    size = len;
    manage_buf = false;
}

McastMsgBuffer::~McastMsgBuffer()
{
    if ( manage_buf == true ) {
        if ( size == maxMsgBufSize && msg_buffer ) {
            McastMsgBuffer::MsgBuffer::pool.put(msg_buffer);
        } else if ( size == maxMsgBigBufSize && msg_big_buffer ) {
            McastMsgBuffer::MsgBigBuffer::pool.put(msg_big_buffer);
        } else {
            ;
        }

        buf = 0;
        msg_buffer = 0;
        msg_big_buffer = 0;
        size = 0;
    }
}

void McastMsgBuffer::reset()
{
    offset = 0;
}

McastMsgBuffer& McastMsgBuffer::operator << (uint64_t rval)
{
    uint8_t *ptr = buf + offset;

    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff;

    offset += 8;

    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator >> (uint64_t &rval)
{
    uint8_t* ptr = buf + offset + 7;

    rval = *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;

    offset += 8;

    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator << (uint32_t rval)
{
    uint8_t* ptr = buf + offset;

    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff;

    offset += 4;

    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator >> (uint32_t &rval)
{
    uint8_t* ptr = buf + offset + 3;

    rval = *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;

    offset += 4;

    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator << (uint16_t rval)
{
    uint8_t* ptr = buf + offset;

    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff;

    offset += 2;

    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator >> (uint16_t &rval)
{
    uint8_t* ptr = buf + offset + 1;

    rval = *ptr--;
    rval = rval << 8 | *ptr--;

    offset += 2;

    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator << (uint8_t rval)
{
    buf[offset++] = rval;
    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator >> (uint8_t &rval)
{
    rval = buf[offset++];
    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator << (const string &rval)
{
    const uint16_t length = static_cast<uint16_t>(rval.length());
    operator<<(length);
    if ( length ) {
        const char* t = rval.data();
        ::memcpy(&buf[offset], t, length);
        offset += length;
    }
    /*
    for (int i = 0; i < length; i++)
    buf[offset++] = *t++;
    */
    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator >> (string &rval)
{
    uint16_t length;
    operator>>(length);

    if ( length ) {
        char save = buf[offset + length];
        buf[offset + length] = '\0';
        rval = string((char*)&buf[offset]);
        buf[offset + length] = save;
    }
    offset += length;
    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator << (int64_t rval)
{
    uint8_t *ptr = buf + offset;

    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff;

    offset += 8;

    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator >> (int64_t &rval)
{
    uint8_t *ptr = buf + offset + 7;

    rval = *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;

    offset += 8;

    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator << (int32_t rval)
{
    uint8_t *ptr = buf + offset;

    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff;

    offset += 4;

    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator >> (int32_t &rval)
{
    uint8_t *ptr = buf + offset + 3;

    rval = *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;
    rval = rval << 8 | *ptr--;

    offset += 4;

    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator << (int16_t rval)
{
    uint8_t *ptr = buf + offset;

    *ptr++ = rval & 0xff; rval = rval >> 8;
    *ptr++ = rval & 0xff;

    offset += 2;

    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator >> (int16_t &rval)
{
    uint8_t *ptr = buf + offset + 1;

    rval = *ptr--;
    rval = rval << 8 | *ptr--;

    offset += 2;

    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator << (int8_t rval)
{
    buf[offset++] = rval;
    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator >> (int8_t &rval)
{
    rval = buf[offset++];
    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator << (bool rval)
{
    buf[offset++] = rval;
    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator >> (bool& rval)
{
    rval = buf[offset++];
    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator <<(const Uint8Vector &rval)
{
    ::memcpy(buf + offset, rval.ptr, rval.size);
    offset += rval.size;
    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator >>(Uint8Vector &rval)
{
    ::memcpy(rval.ptr, buf + offset, rval.size);
    offset += rval.size;
    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator << (McastMsgBuffer& rval)
{
    // We copy the data from rval buffer from the beginning to its current offset.
    // The size of copied data is marshalled into this buffer to allow reconstruction.
    // Offset of this buffer is adjusted.
    uint32_t rsz = rval.getOffset();
    *this << rsz;
    ::memcpy( &buf[offset], &rval.buf[0], rsz );
    offset += rsz;
    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator >> (McastMsgBuffer& rval)
{
    // We reconstruct rval data from this data.
    // Offset of this buffer is adjusted, offset of rval buffer points to the end of restored data.
    // Thie size of data to be reconstructed is extracted from data itself (first 4 bytes)
    uint32_t rsz;
    *this >> rsz;
    ::memcpy(&rval.buf[0], &buf[offset], rsz);
    offset += rsz;
    rval.offset += rsz;
    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator << (const char *rval)
{
    const uint16_t length = strlen(rval);
    operator<<(length);
    ::memcpy(buf + offset, rval, length);
    offset += length;
    return *this;
}

McastMsgBuffer& McastMsgBuffer::operator >> (char* rval)
{
    uint16_t length;
    operator>>(length);

    memcpy(rval, buf + offset, length);
    rval[length] = '\0';
    offset += length;
    return *this;
}
}



