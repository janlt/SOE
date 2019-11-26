/**
 * @file    msgnetmmpoint.cpp
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
 * msgnetmmpoint.cpp
 *
 *  Created on: Feb 6, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <list>
#include <stdexcept>
using namespace std;
#include <poll.h>
#include <netdb.h>

#include "soelogger.hpp"
using namespace SoeLogger;

#include "thread.hpp"
#include "threadfifoext.hpp"
#include "classalloc.hpp"
#include "generalpool.hpp"
#include "dbsrvmarshallable.hpp"
#include "dbsrvmsgbuffer.hpp"

#include "dbsrvmsgs.hpp"
#include "dbsrvmcasttypes.hpp"
#include "dbsrvmsgadmtypes.hpp"
#include "dbsrvmsgfactory.hpp"
#include "dbsrvmsgadmfactory.hpp"

#include "dbsrviovmsgsbase.hpp"
#include "dbsrviovmsgs.hpp"
#include "dbsrvmsgiovfactory.hpp"
#include "dbsrvmsgiovtypes.hpp"

#include "msgnetaddress.hpp"
#include "msgnetmmaddress.hpp"
#include "msgnetunixaddress.hpp"
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnetunixsocket.hpp"
#include "msgnetmmpoint.hpp"

namespace MetaNet {

bool MmPoint::Initializer::initialized = false;
MmPoint::Initializer *MmPoint::mmpoint_initalized = 0;
std::shared_ptr<MmPoint>  MmPoint::points[MmPoint::max_mmpoints];
UnixAddress MmPoint::signal_addr = UnixAddress::null_address;
const char *MmPoint::mm_file_prefix = "/tmp/soemmdir_dont_touch";
Lock MmPoint::global_lock;

//#define __MM_POINT_DEBUG__

MmPoint::Initializer::Initializer(bool _no_lock)
{
    if ( _no_lock == false ) {
        WriteLock w_lock(MmPoint::global_lock);
        Initializer::InitializerNoLock();
    } else {
        Initializer::InitializerNoLock();
    }
}

void MmPoint::Initializer::InitializerNoLock()
{
    int ret = ::mkdir(MmPoint::mm_file_prefix, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if ( ret && errno != EEXIST ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't create path, path: " << MmPoint::mm_file_prefix <<
            " errno: " << errno << SoeLogger::LogError() << endl;
        return;
    }

    if ( Initializer::initialized == false ) {
        for ( uint32_t i = 0 ; i < MmPoint::max_mmpoints ; i++ ) {
            std::string file_name = std::string("mmpoint_file_") + to_string(i);

            MmPoint::points[i].reset(CreateMapping(i, default_tmout, file_name));
            if ( !MmPoint::points[i].get() ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't mmap state, errno: " << errno <<
                    SoeLogger::LogError() << endl;
                break;
            }
            MmPoint::points[i]->idx = i;

            if ( MmPoint::points[i]->WriteState() < 0 ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Wrong state, path: " << MmPoint::points[i]->channel.mm_file_path <<
                    SoeLogger::LogError() << endl;
            }
        }
    }
    Initializer::initialized = true;
}

MmPoint::Initializer::~Initializer()
{
    WriteLock w_lock(MmPoint::global_lock);

    if ( Initializer::initialized == true ) {
        for ( uint32_t i = 0 ; i < MmPoint::max_mmpoints ; i++ ) {
            MmPoint::points[i].reset();
        }
    }
    Initializer::initialized = false;
}

void MmPoint::Initialize(const char *_path)
{
    signal_addr = UnixAddress(_path);

    ::memset(idx_buf, 0xff, sizeof(idx_buf));
    for ( uint32_t i = 0 ; i < MmPoint::max_mps ; i++ ) {
        mps[i].idx = -1;
        mps[i].mm_fd = -1;
        ::memset(&mps_hits[i], '\0', sizeof(IdxHits));
    }
}

MmPoint::MmPoint(uint32_t ch_cnt, const std::string &_path, Point::eType _signal_type)
    :Point(Point::eType::eMM),
     idx_set(false),
     srv_side(false),
     idx(-1),
     channel(_path),
     idx_cnt(ch_cnt),
     connect_idx_cnt(0),
     idx_mask(0),
     search_start(0),
     signal_type(_signal_type),
     signal_point(0),
     args(),
     send_count(0),
     recv_count(0)
{
    Initialize(_path.c_str());
}

MmPoint::MmPoint(const MMAddress &ch_addr, const MMAddress &sig_addr, uint32_t ch_cnt, Point::eType _signal_type)
    :Point(Point::eType::eMM),
     idx_set(false),
     srv_side(false),
     idx(-1),
     channel(ch_addr.GetPath()),
     idx_cnt(ch_cnt),
     connect_idx_cnt(0),
     idx_mask(0),
     search_start(0),
     signal_type(_signal_type),
     signal_point(0),
     args(),
     send_count(0),
     recv_count(0)
{
    Initialize(sig_addr.GetPath());
}

MmPoint::~MmPoint()
{
    MmPoint::DestroyMapping(this);
}

void MmPoint::PrintMps() const
{
    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " ("<< srv_side << ") idx_cnt: " << idx_cnt << SoeLogger::LogDebug() << endl;
    for ( uint32_t i = 0 ; i < idx_cnt ; i++ ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " idx/fd: " << mps[i].idx << "/" << mps[i].mm_fd << SoeLogger::LogDebug() << endl;
    }
    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
}

int MmPoint::Create(bool _srv)
{
    WriteLock w_lock(MmPoint::global_lock);

    if ( !MmPoint::mmpoint_initalized ) {
        MmPoint::mmpoint_initalized = new MmPoint::Initializer(true);
    }

    // for signaling we use AF_UNIX socket
    switch ( signal_type ) {
    case Point::eType::eUnix:
        signal_point = new UnixSocket();
        break;
    case Point::eType::eTcp:
        //signal_point = new TcpSocket();
        //break;
    default:
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Unsupported signal type: " << signal_type <<
            SoeLogger::LogError() << endl;
        return -1;
        break;
    }

    if ( signal_point->Create() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Create signaling socket failed, errno: " << errno <<
            SoeLogger::LogError() << endl;
        return -1;
    }
    SetDesc(signal_point->GetDesc());

    if ( _srv == false ) {
        return GetDesc();
    }

    srv_side = _srv;

    if ( idx_cnt > MmPoint::max_mps ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid idx_cnt: " << idx_cnt << SoeLogger::LogError() << endl;
        return -1;
    }

    uint32_t i = 0;
    uint32_t idx_allocated = 0;
    for ( i = 0 ; i < MmPoint::max_mmpoints ; i++ ) {
        if ( !MmPoint::points[i] ) {
            return -1;
        }
        if ( MmPoint::points[i]->channel.mm_state == MmPoint::ePointState::eFree ) {
            mps[idx_allocated].idx = i;
            mps[idx_allocated].mm_fd = MmPoint::points[i]->channel.mm_fd;
            MmPoint::points[i]->channel.mm_state = MmPoint::ePointState::eInuse;

            if ( MmPoint::points[i]->WriteState() < 0 ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Wrong state, path: " << MmPoint::points[i]->channel.mm_file_path <<
                    SoeLogger::LogError() << endl;
                return -1;
            }

            volatile uint32_t *ch_sts_ptr = reinterpret_cast<uint32_t *>(static_cast<uint8_t *>(MmPoint::points[i]->channel.mm_mapping) +
                MmPoint::points[i]->channel.mm_user_send_offset - sizeof(uint32_t));
            *ch_sts_ptr = 0;
            ch_sts_ptr = reinterpret_cast<uint32_t *>(static_cast<uint8_t *>(MmPoint::points[i]->channel.mm_mapping) +
                MmPoint::points[i]->channel.mm_user_recv_offset - sizeof(uint32_t));
            *ch_sts_ptr = 0;

            idx_mask |= (1 << idx_allocated);

            idx_allocated++;
            if ( idx_allocated >= idx_cnt ) {
                break;
            }
        }
    }

    if ( i >= MmPoint::max_mmpoints || idx_allocated < idx_cnt ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "No MmPoints available" << SoeLogger::LogError() << endl;
        return -1;
    }

    idx_set = true;

    //PrintMps();

    return GetDesc();
}

int MmPoint::Shutdown()
{
    return Close();
}

int MmPoint::Close()
{
    WriteLock w_lock(MmPoint::global_lock);

    for ( uint32_t i = 0 ; i < idx_cnt ; i++ ) {
        if ( MmPoint::points[mps[i].idx] &&
                MmPoint::points[mps[i].idx]->channel.mm_state != MmPoint::ePointState::eFree ) {
            MmPoint::points[mps[i].idx]->channel.mm_state = MmPoint::ePointState::eFree;
            SetDesc(-1);

            if ( MmPoint::points[mps[i].idx]->WriteState() < 0 ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Wrong state, path: " << MmPoint::points[mps[i].idx]->channel.mm_file_path <<
                    SoeLogger::LogError() << endl;
            }
            mps[i].idx = -1;
            mps[i].mm_fd = -1;

            if ( signal_point ) {
                signal_point->Close();
                signal_point = 0;
            }
            return 0;
        }
    }
    idx_mask = 0;

    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "No MmPoint found" << SoeLogger::LogError() << endl;
    return -1;
}

int MmPoint::SetNonBlocking(bool _blocking)
{
    return signal_point->SetNonBlocking(_blocking);
}

int MmPoint::SetNoDelay()
{
    return signal_point->SetNoDelay();
}

MmPoint *MmPoint::CreateMapping(uint32_t idx, uint32_t _tmout, const std::string &_name)
{
    MmPoint *mmp = new MmPoint(1, std::string(MmPoint::mm_file_prefix) + "/" + _name);
    mmp->channel.mm_file_size = 1 << 20;

    mmp->channel.mm_fd = ::open(mmp->channel.mm_file_path.c_str(), O_RDWR | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if ( mmp->channel.mm_fd < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error opening file for mm " <<
            " path: " << mmp->channel.mm_file_path << " errno: " << errno << SoeLogger::LogError() << endl;
        delete mmp;
        return 0;
    }

    int ret = ::lseek(mmp->channel.mm_fd, mmp->channel.mm_file_size - 1, SEEK_SET);
    if ( ret < 0 ) {
        ::close(mmp->channel.mm_fd);
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error seeking file for mm " <<
            " path: " << mmp->channel.mm_file_path << " errno: " << errno << SoeLogger::LogError() << endl;
        delete mmp;
        return 0;
    }

    ret = ::write(mmp->channel.mm_fd, " ", 1);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error writing file for mm " <<
            " path: " << mmp->channel.mm_file_path << " errno: " << errno << SoeLogger::LogError() << endl;
        delete mmp;
        return 0;
    }

    mmp->channel.mm_mapping = static_cast<unsigned char *>(::mmap(0, mmp->channel.mm_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, mmp->channel.mm_fd, 0));
    if ( mmp->channel.mm_mapping == MAP_FAILED ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error mapping file for mm " <<
            " path: " << mmp->channel.mm_file_path << " errno: " << errno << SoeLogger::LogError() << endl;
        delete mmp;
        return 0;
    }

    ret = ::madvise(mmp->channel.mm_mapping, mmp->channel.mm_file_size, MADV_SEQUENTIAL);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error madvicing file for mm " <<
            " path: " << mmp->channel.mm_file_path << " errno: " << errno << SoeLogger::LogError() << endl;
        delete mmp;
        return 0;
    }

    mmp->channel.mm_user_send_size = mmp->channel.mm_user_recv_size = (mmp->channel.mm_file_size - 32)/2;

    mmp->channel.mm_user_send_offset = 16;
    mmp->channel.mm_user_recv_offset = 16 + mmp->channel.mm_user_send_size + 16;

    mmp->channel.timer = -1;
    mmp->channel.timeout = _tmout;
    mmp->channel.mm_state = eFree;

    uint32_t *ch_sts_ptr = reinterpret_cast<uint32_t *>(mmp->channel.mm_mapping);
    *(ch_sts_ptr) = 0;

#ifdef __MM_POINT_DEBUG__
    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "MMP idx: " << idx << std::hex << " 0x" << reinterpret_cast<uint64_t>(mmp->channel.mm_mapping) << std::dec << " r/s: " <<
        std::hex << "0x" << reinterpret_cast<uint64_t>(static_cast<uint8_t *>(mmp->channel.mm_mapping) + mmp->channel.mm_user_recv_offset) << "/" <<
        std::hex << "0x" << reinterpret_cast<uint64_t>(static_cast<uint8_t *>(mmp->channel.mm_mapping) + mmp->channel.mm_user_send_offset) <<
        std::dec << " rs/ss: " << mmp->channel.mm_user_recv_size << "/" << mmp->channel.mm_user_send_size <<
        SoeLogger::LogDebug() << endl;
#endif
    return mmp;
}

void MmPoint::DestroyMapping(MmPoint *mmp)
{

    int ret = ::munmap(mmp->channel.mm_mapping, mmp->channel.mm_file_size);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error unmapping file " <<
            " path: " << mmp->channel.mm_file_path << " errno: " << errno << SoeLogger::LogError() << endl;
    }
    mmp->channel.mm_mapping = 0;

    (void )::close(mmp->channel.mm_fd);
    mmp->channel.mm_fd = -1;
}

int MmPoint::UnmarshallMps(const Handshake *hnd)
{
    WriteLock w_lock(MmPoint::global_lock);

    idx_mask = 0;
    idx_cnt = hnd->num_idxs;
    for ( uint32_t i = 0 ; i < idx_cnt ; i++ ) {
        uint32_t cur_idx = hnd->idxs[i];

        if ( !MmPoint::points[cur_idx] || MmPoint::points[cur_idx]->channel.mm_state != MmPoint::ePointState::eFree ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Null or invalid MmPoint, idx: " << cur_idx <<
                " desc: " << GetDesc() << SoeLogger::LogError() << endl;
            return -1;
        }

        MmPoint::points[cur_idx]->channel.mm_state = MmPoint::ePointState::eInuse;
        MmPoint::points[cur_idx]->channel.mm_state = MmPoint::ePointState::eConnected;
        mps[i].idx = cur_idx;
        mps[i].mm_fd = MmPoint::points[cur_idx]->channel.mm_fd;
        idx_mask |= (1 << i);
    }

    if ( idx_set == false ) {
        idx_set = true;
    }
    return 0;
}

int MmPoint::MarshallMps(Handshake *hnd)
{
    hnd->num_idxs = connect_idx_cnt;
    if ( hnd->num_idxs > MmPoint::max_mps ) {
        return -1;
    }
    for ( uint32_t i = 0 ; i < connect_idx_cnt ; i++ ) {
        hnd->idxs[i] = connect_mps[i].idx;

        if ( !MmPoint::points[connect_mps[i].idx] || MmPoint::points[connect_mps[i].idx]->channel.mm_state == MmPoint::ePointState::eFree ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Null or invalid MmPoint, idx: " <<
                connect_mps[i].idx << " desc: " << GetDesc() << SoeLogger::LogError() << endl;
            return -1;
        }
    }
    return 0;
}

int MmPoint::Connect(const Address *addr)
{
    if ( idx_set == true && Check() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid MmPoint desc: " << GetDesc() << SoeLogger::LogError() << endl;
        return -1;
    }

    UnixAddress uad(addr ? dynamic_cast<const MMAddress *>(addr)->GetPath() : "");
    int ret = signal_point->Connect(addr ? &uad : 0);
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Connect failed, UnixAddress MmPoint idx: " <<
            idx << " desc: " << GetConstDesc() <<
            " errno: " << errno << SoeLogger::LogError() << endl;
        return -1;
    }

    idx_buf[2] = 0xaa;
    ret = signal_point->Send(idx_buf, 4);
    if ( ret != 4 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error Send MmPoint fd: " << GetDesc() << SoeLogger::LogError() << endl;
        return -1;
    }

    ret = signal_point->Recv(idx_buf, sizeof(idx_buf));
    if ( ret != sizeof(idx_buf) ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error Recv MmPoint fd: " << GetDesc() << SoeLogger::LogError() << endl;
        return -1;
    }
    ret = UnmarshallMps(reinterpret_cast<Handshake *>(idx_buf));
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Can't assign mps desc: " << GetDesc() << SoeLogger::LogError() << endl;
        return -1;
    }

    if ( WriteState() < 0 ) {
         SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error WriteState: " << GetDesc() << SoeLogger::LogError() << endl;
         return -1;
    }

    if ( Check() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid MmPoint idx: " << idx << " desc: " << GetDesc() << SoeLogger::LogError() << endl;
        return -1;
    }

    //PrintMps();

    return ret >= 0 ? 0 : ret;
}

int MmPoint::Accept(Address *addr)
{
    if ( Check() < 0 || CheckConnectMps() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid MmPoint desc: " << GetDesc() << SoeLogger::LogError() << endl;
        return -1;
    }

    srv_side = true;

    UnixAddress uad(addr ? dynamic_cast<MMAddress *>(addr)->GetPath() : "");
    int fd_ret = signal_point->Accept(addr ? &uad : 0);
    if ( fd_ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Accept failed, UnixAddress MmPoint desc: " << GetDesc() <<
            " errno: " << errno << SoeLogger::LogError() << endl;
        return -1;
    }

    UnixSocket tmp_u_sock;
    tmp_u_sock.SetDesc(fd_ret);

    int ret = tmp_u_sock.Recv(idx_buf, 4);
    if ( ret != 4 || idx_buf[2] != 0xaa ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error Recv MmPoint desc: " << GetDesc() << SoeLogger::LogError() << endl;
        return -1;
    }

    if ( CheckConnectMps() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid connect_idxs desc: " << GetDesc() << SoeLogger::LogError() << endl;
        return -1;
    }
    ret = MarshallMps(reinterpret_cast<Handshake *>(idx_buf));
    if ( ret < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error marshall mps desc: " << GetDesc() << SoeLogger::LogError() << endl;
        return -1;
    }
    ret = tmp_u_sock.Send(idx_buf, sizeof(Handshake));
    if ( ret != sizeof(idx_buf) ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error Send MmPoint desc: " << GetDesc() << SoeLogger::LogError() << endl;
        return -1;
    }

    if ( WriteState() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error WriteState: " << GetDesc() << SoeLogger::LogError() << endl;
        return -1;
    }

    return fd_ret;
}

void MmPoint::SetConnectMps(const MmPoint &mm_pt)
{
    connect_idx_cnt = mm_pt.idx_cnt;
    for ( uint32_t i = 0 ; i < mm_pt.idx_cnt ; i++ ) {
        connect_mps[i].idx = mm_pt.mps[i].idx;
    }
}

int MmPoint::CheckConnectMps()
{
    for ( uint32_t i = 0 ; i < connect_idx_cnt ; i++ ) {
        if ( !MmPoint::points[connect_mps[i].idx] || MmPoint::points[connect_mps[i].idx]->channel.mm_state == MmPoint::ePointState::eFree ) {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Null or invalid MmPoint, idx: " <<
                connect_mps[i].idx << " desc: " << GetDesc() << SoeLogger::LogError() << endl;
            return -1;
        }
    }
    return 0;
}

int MmPoint::Bind(const sockaddr *_addr)
{
    return signal_point->Bind(_addr);
}

int MmPoint::Listen(int backlog)
{
    return ::listen(GetDesc(), backlog);
}

int MmPoint::GetDesc()
{
    return Point::GetDesc();
}

int MmPoint::GetConstDesc() const
{
    return Point::GetConstDesc();
}

void MmPoint::SetDesc(int _desc)
{
    Point::SetDesc(_desc);
    if ( signal_point ) {
        signal_point->SetDesc(_desc);
    }
}

int MmPoint::Register(bool _ignore, bool _force_register) throw(std::runtime_error)
{
    return Point::Register(_ignore, _force_register);
}

int MmPoint::Deregister(bool _ignore, bool _force_deregister) throw(std::runtime_error)
{
    return Point::Deregister(_ignore, _force_deregister);
}

int MmPoint::WaitFreeXmitIdx(uint8_t *&mm_ptr, size_t &mm_size)
{
    // First mm channel has info which channels are available for transfer
    for ( uint32_t i = search_start ; i < idx_cnt + search_start ; i++ ) {
        uint32_t t_idx = mps[i%idx_cnt].idx;

        MmPoint::TransferChannel *ch = &MmPoint::points[t_idx]->channel;
        uint8_t *comm_ptr;
        size_t comm_size;
        if ( srv_side == false ) {
            comm_ptr = static_cast<uint8_t *>(ch->mm_mapping) + ch->mm_user_send_offset;
            comm_size = ch->mm_user_send_size;
        } else {
            comm_ptr = static_cast<uint8_t *>(ch->mm_mapping) + ch->mm_user_recv_offset;
            comm_size = ch->mm_user_recv_size;
        }

        volatile uint32_t *ch_sts_ptr = reinterpret_cast<uint32_t *>(comm_ptr - sizeof(uint32_t));
        if ( !(*ch_sts_ptr) ) {
            *ch_sts_ptr = 10000 + t_idx;
            mm_ptr = comm_ptr;
            mm_size = comm_size;

            if ( srv_side == true ) {
                mps_hits[t_idx].s_hit_count++;
            } else {
                mps_hits[t_idx].c_hit_count++;
            }

            search_start++;
            search_start %= idx_cnt;

            return t_idx;
        } else {
            if ( srv_side == true ) {
                mps_hits[t_idx].s_miss_count++;
            } else {
                mps_hits[t_idx].c_miss_count++;
            }

            for ( uint32_t l = 0 ; l < 1024 ; l++ ) {
                struct timespec tim;
                tim.tv_sec = 0;
                tim.tv_nsec = 2*l;
                (void )nanosleep(&tim , 0);
                if ( !(*ch_sts_ptr) ) {
                    *ch_sts_ptr = 10000 + t_idx;
                    mm_ptr = comm_ptr;
                    mm_size = comm_size;

                    if ( srv_side == true ) {
                        mps_hits[t_idx].s_hit_count++;
                    } else {
                        mps_hits[t_idx].c_hit_count++;
                    }

                    search_start++;
                    search_start %= idx_cnt;

                    return t_idx;
                }

                if ( srv_side == true ) {
                    mps_hits[t_idx].s_miss_count++;
                } else {
                    mps_hits[t_idx].c_miss_count++;
                }
            }
            return -1;
        }
    }

    return -1;
}

void MmPoint::PrintStats() const
{
    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "idx_cnt: " << idx_cnt << SoeLogger::LogDebug() << endl;
    for ( uint32_t i = 0 ; i < idx_cnt ; i++ ) {
        uint32_t j = mps[i].idx;
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " h " << mps_hits[j].s_hit_count << "/" << mps_hits[j].c_hit_count << SoeLogger::LogDebug() << endl;
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " m " << mps_hits[j].s_miss_count << "/" << mps_hits[j].c_miss_count << SoeLogger::LogDebug() << endl;
    }
    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << SoeLogger::LogDebug() << endl;
}

//#define __MM_POINT_ACK__
int MmPoint::SendIov(MetaMsgs::IovMsgBase &io)
{
    if ( CheckConst() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid MmPoint desc: " << GetConstDesc() << SoeLogger::LogError() << endl;
        return -1;
    }

    uint8_t *comm_ptr = 0;
    size_t comm_size = 0;
    int free_idx = -1;
    for ( ;; ) {
        if ( (free_idx = const_cast<MmPoint *>(this)->WaitFreeXmitIdx(comm_ptr, comm_size)) >= 0 ) {
            break;
        }
    }

    //if ( !((const_cast<MmPoint *>(this)->send_count++)%0x100000) ) {
        //PrintStats();
    //}

#ifdef __MM_POINT_DEBUG__
    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "MMPSend idx: " << idx << " s_s: " << srv_side << std::hex << " 0x" << reinterpret_cast<uint64_t>(comm_ptr) <<
        std::dec << " cs: " << comm_size << SoeLogger::LogError() << endl;
#endif
    int sent_cnt =  io.sendMM(MmPoint::points[free_idx]->channel.mm_fd, comm_ptr, comm_size);

    int ret = 0;
    if ( srv_side == false && idx_set == false ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " idx not set, free_idx: " << free_idx << " desc: " << GetConstDesc() << SoeLogger::LogError() << endl;
        ret = -1;
    }

    XmitSyn x_s;
    x_s.xmit_idx = static_cast<uint32_t>(free_idx);
    ret = signal_point->Send(reinterpret_cast<uint8_t *>(&x_s), sizeof(x_s));
    if ( ret != sizeof(x_s) ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error Send MmPoint idx: " << idx << " desc: " << GetConstDesc() << SoeLogger::LogError() << endl;
    }
#ifdef __MM_POINT_ACK__
    ret = signal_point->Recv(ack_buf, 4);
    if ( ret != 4 || ack_buf[0] != 1 || ack_buf[1] != idx ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error Recv ack MmPoint idx: " << idx << " desc: " << GetConstDesc() << SoeLogger::LogError() << endl;
        return -1;
    }
#endif

    return !ret ? sent_cnt : ret;
}

int MmPoint::RecvIov(MetaMsgs::IovMsgBase &io)
{
    if ( Check() < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Invalid MmPoint desc: " << GetDesc() << SoeLogger::LogError() << endl;
        return -1;
    }

    XmitSyn x_s;
    int ret = signal_point->Recv(reinterpret_cast<uint8_t *>(&x_s), sizeof(x_s));
    if ( ret != sizeof(x_s) || CheckXmitIdx(x_s.xmit_idx) < 0 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error Recv syn MmPoint desc: " << GetDesc() << SoeLogger::LogError() << endl;
    }

    MmPoint::TransferChannel *ch = &MmPoint::points[x_s.xmit_idx]->channel;
    uint8_t *comm_ptr;
    size_t comm_size;
    if ( srv_side == false ) {
        comm_ptr = static_cast<uint8_t *>(ch->mm_mapping) + ch->mm_user_recv_offset;
        comm_size = ch->mm_user_recv_size;
    } else {
        comm_ptr = static_cast<uint8_t *>(ch->mm_mapping) + ch->mm_user_send_offset;
        comm_size = ch->mm_user_send_size;
    }
#ifdef __MM_POINT_DEBUG__
    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "MMPRecv idx: " << idx << " s_s: " << srv_side << std::hex << " 0x" << reinterpret_cast<uint64_t>(comm_ptr) <<
        std::dec << " cs: " << comm_size << SoeLogger::LogError() << endl;
#endif
    int recv_cnt =  io.recvMM(MmPoint::points[x_s.xmit_idx]->channel.mm_fd, comm_ptr, comm_size);

    if ( srv_side == false && idx_set == false ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " idx not set, xmit_idx: " << x_s.xmit_idx << " desc: " << GetDesc() << SoeLogger::LogError() << endl;
        ret = -1;
    }
#ifdef __MM_POINT_ACK__
    ret = signal_point->Send(idx_buf, 4);
    if ( ret != 4 ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Error Send MmPoint xmit_idx: " << x_s.xmit_idx << " desc: " << GetConstDesc() << SoeLogger::LogError() << endl;
        return -1;
    }
#endif

    volatile uint32_t *ch_sts_ptr = reinterpret_cast<uint32_t *>(comm_ptr - sizeof(uint32_t));
    *ch_sts_ptr = 0;

    return !ret ? recv_cnt : ret;
}

int MmPoint::SetRecvTmout(uint32_t msecs)
{
    return signal_point->SetRecvTmout(msecs);
}

int MmPoint::SetSndTmout(uint32_t msecs)
{
    return signal_point->SetSndTmout(msecs);
}

//
// State machine
// eFree -> eInuse -> eConnected ->eAccepted -> eFree
// eFree -> eInuse -> eInuse -> as above
// eFree -> eFree -> as above
//
int MmPoint::WriteState()
{
    return 0;

    int t_idx = -1;
    for ( uint32_t i = 0 ; i < idx_cnt ; i++ ) {
        if ( mps[i].idx >= 0 && mps[i].idx < static_cast<int>(MmPoint::max_mmpoints) ) {
            t_idx = mps[i].idx;
        } else {
            return -1;
        }

        MmPoint::TransferChannel *ch = &MmPoint::points[t_idx]->channel;
        MmPoint::ePointState st = *static_cast<MmPoint::ePointState *>(ch->mm_mapping);
        switch ( st ) {
        case MmPoint::ePointState::eFree:
            if ( ch->mm_state == MmPoint::ePointState::eFree ) {
                break;
            }
            if ( ch->mm_state != MmPoint::ePointState::eInuse ) {
                return -1;
            }
            *static_cast<MmPoint::ePointState *>(ch->mm_mapping) = ch->mm_state;
            break;
        case MmPoint::ePointState::eInuse:
            if ( ch->mm_state == MmPoint::ePointState::eInuse ) {
                break;
            }
            if ( ch->mm_state != MmPoint::ePointState::eConnected ) {
                return -1;
            }
            *static_cast<MmPoint::ePointState *>(ch->mm_mapping) = ch->mm_state;
            break;
        case MmPoint::ePointState::eConnected:
            if ( ch->mm_state != MmPoint::ePointState::eAccepted ) {
                return -1;
            }
            *static_cast<MmPoint::ePointState *>(ch->mm_mapping) = ch->mm_state;
            break;
        case MmPoint::ePointState::eAccepted:
            if ( ch->mm_state != MmPoint::ePointState::eFree ) {
                return -1;
            }
            *static_cast<MmPoint::ePointState *>(ch->mm_mapping) = ch->mm_state;
            break;
        default:
            return -1;
            break;
        }
    }
    return 0;
}
}


