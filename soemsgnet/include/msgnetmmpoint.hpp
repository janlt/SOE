/**
 * @file    msgnetmmpoint.hpp
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
 * msgnetmmpoint.hpp
 *
 *  Created on: Feb 6, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETMMPOINT_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETMMPOINT_HPP_

typedef boost::shared_mutex Lock;
typedef boost::unique_lock<Lock> WriteLock;
typedef boost::shared_lock<Lock> ReadLock;

namespace MetaNet {

class MmPoint: public Point
{
public:
    typedef enum _eSession {
        eSessionStart    = 0,
        eSessionContinue = 1,
        eSessionEnd      = 2,
        eSessionLogin    = 3,
        eSessionLogout   = 4,
        eSessionNone     = 5
    } eSession;

    const static uint32_t   max_direct_buffer  = 128;
    const static uint32_t   max_opts_buffer    = 64;
    const static uint32_t   max_session_nme    = 64;

    const static uint32_t   invalid_channel_idx = -1;

    struct SessionArgs {
        char                 buf[max_direct_buffer];      // direct buffer for search key, username, etc
        char                 key_opts[max_opts_buffer];   // key search options, or magic word, or session ID (md5 hash)
        std::string          name;                        // mm transport session name (output), session_id (input)
        size_t               size;                        // size of direct buffer data
        uint32_t             mm_size;                     // size of mm file
        uint32_t             transfer_size;               // transfer size in mm file
        uint32_t             transfer_seq;                // transfer sequence
        uint32_t             channel_idx;                 // transfer channel index (handle)
        eSession             session_cmd;                 // command for a session phase,
                                                          // Note: server will indicate there is more data by setting it to Continue
        uint64_t             offset;                      // additional param to serve as an offset if multi-transfer session is used

        SessionArgs()
            :size(0),
            mm_size(0),
            transfer_size(0),
            transfer_seq(0),
            channel_idx(invalid_channel_idx),
            session_cmd(eSessionNone),
            offset(0)
        {}
        ~SessionArgs() {}
    };

    static const char *mm_file_prefix;

    typedef enum _ePointState {
        eFree      = 0,
        eInuse     = 1,
        eConnected = 2,
        eAccepted  = 3
    } ePointState;

    typedef int (*TransferSessDataHandler)(
        int fi,
        void *chan,
        SessionArgs *args);

    const static uint32_t     default_tmout = 10000000;

    struct TransferChannel {
        std::string             mm_file_path;                  // mm file path
        int                     mm_fd;                         // mm file fd
        size_t                  mm_file_size;                  // as above
        void                   *mm_mapping;                    // mapping pointer
        size_t                  mm_user_send_offset;
        size_t                  mm_user_recv_offset;
        size_t                  mm_user_send_size;
        size_t                  mm_user_recv_size;
        ePointState             mm_state;                      // state of the point, i.e. in use, free
        uint32_t                mm_transfer_size;              // size of a given transfer
        uint32_t                mm_transfer_seq;               // starts from 0 on the Start request and is incremented on each Continue
        TransferSessDataHandler data_handler;                  // data handler for this channel
        int                     timer;                         // this channel's timer
        uint32_t                timeout;                       // timeout value

        TransferChannel(const std::string &_path)
            :mm_file_path(_path),
            mm_file_size(0),
            mm_mapping(0),
            mm_user_send_offset(0),
            mm_user_recv_offset(0),
            mm_user_send_size(0),
            mm_user_recv_size(0),
            mm_state(eFree),
            mm_transfer_size(0),
            mm_transfer_seq(0),
            data_handler(0),
            timer(0),
            timeout(default_tmout)
        {}
        ~TransferChannel() {}
    };

public:
    class Initializer {
    public:
        static bool initialized;
        Initializer(bool _no_lock = false);
        void InitializerNoLock();
        ~Initializer();
    };
    static Initializer *mmpoint_initalized;


    const static uint32_t            max_mmpoints = 16; //512;            // max MmPoints preallocated
    static std::shared_ptr<MmPoint>  points[max_mmpoints];          // preallocated MmPoint array
    static Lock                      global_lock;                   // global lock to allocate/deallocate MmPoints

    const static uint32_t            max_mps = 128;                 // max idxs per session
    struct Handshake {
        uint32_t   num_idxs;
        uint16_t   idxs[max_mps];
    };
    bool                             idx_set;                       // true when idxs[] has been set
    bool                             srv_side;                      // true when on server side
    uint8_t                          idx_buf[sizeof(Handshake)];    // signal_point buffer for handshake
    int                              idx;                           // needed when preallocating
    TransferChannel                  channel;                       // MM channel mappings

    struct XmitSyn {
        uint32_t   xmit_idx;
    };
    struct MmMapping {
        int      idx;
        int      mm_fd;
    };
    MmMapping                        mps[MmPoint::max_mps];         // indexes/fds into MmPoint::points
    MmMapping                        connect_mps[MmPoint::max_mps]; // new connection indexes/fds into MmPoint::points
    uint32_t                         idx_cnt;                       // actual idx count
    uint32_t                         connect_idx_cnt;               // actual idx count
    uint32_t                         idx_mask;                      // idx bit mask
    uint32_t                         search_start;                  // to randomize the search for a free idx

    static UnixAddress               signal_addr;                   // Unix address for signaling socket
    Point::eType                     signal_type;                   // type of signal_point
    Point                           *signal_point;                  // for signaling, use UnixSocket, TcpSocket or FifoPoint
    SessionArgs                      args;                          // session args

    uint64_t                         send_count;                    // statistics
    uint64_t                         recv_count;                    // statistics
    struct IdxHits {
        uint64_t  s_hit_count;
        uint64_t  c_hit_count;
        uint64_t  s_miss_count;
        uint64_t  c_miss_count;
    };
    IdxHits                          mps_hits[MmPoint::max_mps];    // statistics

public:
    MmPoint(uint32_t ch_cnt = 1, const std::string &_path = "", Point::eType _signal_type = Point::eType::eUnix);
    MmPoint(const MMAddress &ch_addr, const MMAddress &sig_addr, uint32_t ch_cnt = 1, Point::eType _signal_type = Point::eType::eUnix);
    virtual ~MmPoint();

    virtual int Create(bool _srv = false);
    virtual int Close();
    virtual int Create(int domain, int type, int protocol) { return -1; }
    virtual int SetSendBufferLen(int _buf_len) { return -1; }
    virtual int SetReceiveBufferLen(int _buf_len) { return -1; }
    virtual int SetReuseAddr() { return -1; }
    virtual int SetAddMcastMembership(const ip_mreq *_ip_mreq) { return -1; }
    virtual int SetMcastTtl(unsigned int _ttl_val) { return -1; }
    virtual int Cork() { return -1; }
    virtual int Uncork() { return -1; }
    virtual int SetMcastLoop() { return -1; }
    virtual int SetNoSigPipe() { return -1; }
    virtual int SetNoLinger(uint32_t tmout = 0) { return -1; }
    virtual int SetNonBlocking(bool _blocking);
    virtual int SetNoDelay();
    virtual int Connect(const Address *addr);
    virtual int Accept(Address *addr);
    virtual int SetRecvTmout(uint32_t msecs);
    virtual int SetSndTmout(uint32_t msecs);
    virtual int Bind(const sockaddr *_addr);
    virtual int Shutdown();
    virtual int Register(bool _ignore = false, bool _force_register = false) throw(std::runtime_error);
    virtual int Deregister(bool _ignore = false, bool _force_deregister = false) throw(std::runtime_error);

    virtual int SendIov(MetaMsgs::IovMsgBase &io);
    virtual int RecvIov(MetaMsgs::IovMsgBase &io);

    virtual int Send(const uint8_t *io, uint32_t io_size) { return -1; }
    virtual int Recv(uint8_t *io, uint32_t io_size) { return -1; }

    virtual bool IsConnected() { return false; }

    virtual int GetDesc();
    virtual int GetConstDesc() const;
    virtual void SetDesc(int _desc);

    int Listen(int backlog);
    void SetConnectMps(const MmPoint &mm_pt);

protected:
    static MmPoint *CreateMapping(uint32_t idx, uint32_t _tmout, const std::string &_name);
    static void DestroyMapping(MmPoint *mmp);
    void Initialize(const char *_path);
    int Check();
    int CheckConst() const;
    int UnmarshallMps(const Handshake *hnd);
    int MarshallMps(Handshake *hnd);
    int WaitFreeXmitIdx(uint8_t *&mm_ptr, size_t &mm_size);
    int CheckXmitIdx(int _xmit_idx);
    int CheckConnectMps();
    void PrintStats() const;
    void PrintMps() const;

    int WriteState();
};

inline int MmPoint::Check()
{
    if ( GetConstDesc() < 0 ) {
        return -1;
    }
    for ( uint32_t i = 0 ; i < idx_cnt ; i++ ) {
        if ( mps[i].idx < 0 || mps[i].idx >= static_cast<int>(MmPoint::max_mmpoints) ) {
            return -1;
        }
    }
    return 0;
}

inline int MmPoint::CheckConst() const
{
    return const_cast<MmPoint *>(this)->Check();
}

inline int MmPoint::CheckXmitIdx(int _xmit_idx)
{
    for ( uint32_t i = 0 ; i < idx_cnt ; i++ ) {
        if ( mps[i].idx == _xmit_idx ) {
            return 0;
        }
    }
    return -1;
}

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETMMPOINT_HPP_ */
