/**
 * @file    msgnetsocket.cpp
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
 * msgnetsocket.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <stdexcept>
using namespace std;
#include <poll.h>
#include <netdb.h>

#include <boost/function.hpp>
#include <boost/thread/locks.hpp>

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
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"

namespace MetaNet {

Socket::Socket(Point::eType _type)
    :Point(_type),
    sock_opt_type(-1),
    async(false)
{}

Socket::~Socket()
{}

bool Socket::Created()
{
    return Point::Created();
}

int Socket::Close()
{
    int ret = CheckFd();

    if ( ret == 2 ) {
        return 0;
    }

    if ( ret < 0 && errno != EBADF && errno != ENOTCONN ) {
        //cout << "Socket::" << __FUNCTION__ << " fd=" << GetDesc() << " errno=" << errno << endl;
        return -1;
    }

    while ( 1 ) {
        ret = ::close(GetDesc());
        if ( ret < 0 ) {
            if ( errno == EINTR ) {
                continue;
            } else {
                SetStateClosed();
                return ret;
            }
        }
        break;
    }
    SetStateClosed();
    return ret;
}

int Socket::SetSockOptType()
{
    int optlen = sizeof(sock_opt_type);
    int ret = ::getsockopt(GetDesc(), SOL_SOCKET, SO_TYPE, static_cast<void*>(&sock_opt_type), reinterpret_cast<socklen_t *>(&optlen));
    if ( ret < 0 ) {
        sock_opt_type = -1;
    }
    return ret;
}

int Socket::GetName(sockaddr *_addr)
{
    socklen_t size = sizeof(*_addr);
    return ::getsockname(GetDesc(), _addr, &size);
}

int Socket::SetSendBufferLen(int _buf_len)
{
    return ::setsockopt(GetDesc(), SOL_SOCKET, SO_SNDBUF, static_cast<void*>(&_buf_len), static_cast<socklen_t>(sizeof(_buf_len)));
}

int Socket::SetReceiveBufferLen(int _buf_len)
{
    return ::setsockopt(GetDesc(), SOL_SOCKET, SO_RCVBUF, static_cast<void*>(&_buf_len), static_cast<socklen_t>(sizeof(_buf_len)));
}

int Socket::SetReuseAddr()
{
    int val = 1;
    return ::setsockopt(GetDesc(), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&val), static_cast<socklen_t>(sizeof(val)));
}

int Socket::SetAddMcastMembership(const ip_mreq *_ip_mreq)
{
    return ::setsockopt(GetDesc(), IPPROTO_IP, IP_ADD_MEMBERSHIP, _ip_mreq, static_cast<socklen_t>(sizeof(*_ip_mreq)));
}

int Socket::SetMcastTtl(unsigned int _ttl_val)
{
    return ::setsockopt(GetDesc(), SOL_IP, IP_MULTICAST_TTL, &_ttl_val, static_cast<socklen_t>(sizeof(_ttl_val)));
}

int Socket::SetNoDelay()
{
    int val = 1;
    if ( type == eUnix ) {
        return 0;
    }
    return ::setsockopt(GetDesc(), SOL_TCP, TCP_NODELAY, &val, static_cast<socklen_t>(sizeof(val)));
}

int Socket::Cork()
{
    int val = 1;
    return ::setsockopt(GetDesc(), SOL_TCP, TCP_CORK, &val, static_cast<socklen_t>(sizeof(val)));
}

int Socket::Uncork()
{
    int val = 0;
    return ::setsockopt(GetDesc(), SOL_TCP, TCP_CORK, &val, static_cast<socklen_t>(sizeof(val)));
}

int Socket::SetMcastLoop()
{
    u_char loop = 1;
    return ::setsockopt(GetDesc(), SOL_IP, IP_MULTICAST_LOOP, &loop, static_cast<socklen_t>(sizeof(loop)));
}

int Socket::Bind(const sockaddr *_addr)
{
    return ::bind(GetDesc(), _addr, static_cast<socklen_t>(sizeof(*_addr)));
}

int Socket::SetNonBlocking(bool _blocking)
{
    int flags = ::fcntl(GetDesc(), F_GETFL, 0);
    if ( flags < 0 ) {
        return -1;
    }
    flags = _blocking == false ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return ::fcntl(GetDesc(), F_SETFL, flags);
}

int Socket::SetNoSigPipe()
{
#ifdef _SET_SO_NOSIGPIPE_
    int val = 1;
    return ::setsockopt(GetDesc(), SOL_SOCKET, SO_NOSIGPIPE, reinterpret_cast<const char *>(&val), static_cast<socklen_t>(sizeof(val)));
#else
    sigset_t sigpipe_mask;
    sigemptyset(&sigpipe_mask);
    sigaddset(&sigpipe_mask, SIGPIPE);
    sigset_t saved_mask;
    int ret = pthread_sigmask(SIG_BLOCK, &sigpipe_mask, &saved_mask);
    if ( ret < 0 ) {
        return ret;
    }
    return 0;
#endif
}

int Socket::SetRecvTmout(uint32_t msecs)
{
    struct timeval timeout;
    if ( msecs < 1000 ) {
        timeout.tv_sec = 0;
        timeout.tv_usec = msecs*1000;
    } else {
        timeout.tv_sec = msecs/1000;
        timeout.tv_usec = 0;
    }
    return ::setsockopt(GetDesc(), SOL_SOCKET, SO_RCVTIMEO, static_cast<const void *>(&timeout), static_cast<int>(sizeof(timeout)));
}

int Socket::SetSndTmout(uint32_t msecs)
{
    struct timeval timeout;
    if ( msecs < 1000 ) {
        timeout.tv_sec = 0;
        timeout.tv_usec = msecs*1000;
    } else {
        timeout.tv_sec = msecs/1000;
        timeout.tv_usec = 0;
    }
    return ::setsockopt(GetDesc(), SOL_SOCKET, SO_SNDTIMEO, static_cast<const void *>(&timeout), static_cast<int>(sizeof(timeout)));
}

int Socket::SetNoLinger(uint32_t tmout)
{
    struct linger linger;
    linger.l_onoff = !tmout ? 0 : 1;
    linger.l_linger = tmout;
    return ::setsockopt(GetDesc(), SOL_SOCKET, SO_LINGER, static_cast<const void *>(&linger), static_cast<int>(sizeof(linger)));
}

int Socket::RecvExact(uint8_t *buf, uint32_t bytes_want, bool _ignore_sig)
{
    uint32_t bytes_read = 0;
    int ret;
    int fd = GetDesc();

    while ( bytes_read < bytes_want ) {
        ret = ::recv(fd, buf + bytes_read, bytes_want - bytes_read, _ignore_sig == false ? 0 : MSG_NOSIGNAL);
        if ( ret < 0 ) {
            if ( errno == EINTR || errno == EAGAIN ) {
                continue;
            } else {
                if ( errno == ECONNRESET ) {
                    return -ECONNRESET;
                } else if ( _ignore_sig == true && errno != EAGAIN && errno != EWOULDBLOCK ) {
                    return -ECONNRESET;
                } else {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " read failed errno: " << errno << SoeLogger::LogError() << endl;
                    return ret;
                }
            }
        }
        if ( !ret ) {
            if ( errno == EBADF ) {
                return -ECONNRESET;
            } else if ( errno ) {
                return -1;
            } else {
                ;
            }
        }

        bytes_read += static_cast<uint32_t>(ret);
    }

    return static_cast<int>(bytes_read);
}

int Socket::SendExact(const uint8_t *data, uint32_t bytes_req, bool _ignore_sig)
{
    uint32_t bytes_written = 0;
    int ret;
    int fd = GetConstDesc();

    while ( bytes_written < bytes_req ) {
        ret = ::send(fd, data + bytes_written, bytes_req - bytes_written, _ignore_sig == false ? 0 : MSG_NOSIGNAL);
        if ( ret < 0 ) {
            if ( errno == EINTR || errno == EAGAIN ) {
                continue;
            } else {
                if ( errno == ECONNRESET ) {
                    return -ECONNRESET;
                } else if ( _ignore_sig == true && errno != EAGAIN && errno != EWOULDBLOCK ) {
                    return -ECONNRESET;
                } else {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " read failed errno: " << errno << SoeLogger::LogError() << endl;
                    return ret;
                }
            }
        }
        if ( !ret ) {
            if ( errno == EBADF ) {
                return -ECONNRESET;
            } else if ( errno ) {
                return -1;
            } else {
                ;
            }
        }

        bytes_written += static_cast<uint32_t>(ret);
    }

    return static_cast<int>(bytes_written);
}

bool Socket::IsConnected()
{
    return false;
}

}
