/**
 * @file    msgnetipv4address.cpp
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
 * msgnetipv4address.cpp
 *
 *  Created on: Feb 7, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <poll.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <stdexcept>
using namespace std;

#include <unistd.h>
#include <stdlib.h>

#include "thread.hpp"
#include "threadfifoext.hpp"
#include "classalloc.hpp"
#include "generalpool.hpp"
#include "dbsrvmarshallable.hpp"
#include "dbsrvmsgbuffer.hpp"

#include "soelogger.hpp"
using namespace SoeLogger;

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
#include "msgnetipv4address.hpp"
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnetudpsocket.hpp"

namespace MetaNet {

sockaddr_in IPv4Address::null_address = {0};

bool IPv4Address::operator==(const Address &r)
{
    const IPv4Address &ipv4a = dynamic_cast<const IPv4Address &>(r);
    return this->addr.sin_addr.s_addr == ipv4a.addr.sin_addr.s_addr &&
        this->addr.sin_port == ipv4a.addr.sin_port &&
        this->addr.sin_family == ipv4a.addr.sin_family;
}

bool IPv4Address::operator!=(const Address &r)
{
    return !(*this == r);
}

IPv4Address &IPv4Address::operator=(const sockaddr_in _sockaddr_in)
{
    this->addr.sin_addr.s_addr = _sockaddr_in.sin_addr.s_addr;
    this->addr.sin_port = _sockaddr_in.sin_port;
    this->addr.sin_family = _sockaddr_in.sin_family;
    return *this;
}

bool IPv4Address::operator==(const sockaddr_in _sockaddr_in)
{
    return *this == IPv4Address(_sockaddr_in);
}

bool IPv4Address::operator!=(const sockaddr_in _sockaddr_in)
{
    return *this != IPv4Address(_sockaddr_in);
}

IPv4Address IPv4Address::findNodeNoLocalAddr(bool debug)
{
    IPv4Address this_nodes_no_local_addr;

    // Get the hostname/domainname
    char hostname[NI_MAXHOST + 1];
    ::memset(hostname, '\0', sizeof(hostname));
    ::gethostname(hostname, sizeof(hostname));

    // Now get the entry from the inet database
    struct hostent host_entry;
    ::memset(&host_entry, '\0', sizeof(host_entry));

    struct ifaddrs *ifaddr, *ifa;
    int family, s, n;


    s = ::getifaddrs(&ifaddr);

    struct hostent *rslt = 0;
    if ( s ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "getaddrinfo failed, errno: " << errno << SoeLogger::LogError() << endl;

        rslt = ::gethostbyname(hostname); // Linux uses thread-specific storage for result
        char** q;
        if ( rslt ) {
            q = rslt->h_addr_list;
            if ( *q ) {
                this_nodes_no_local_addr.addr.sin_family = AF_INET;
                this_nodes_no_local_addr.addr.sin_port = htons(0);
                this_nodes_no_local_addr.addr.sin_addr.s_addr = ((in_addr*) *q)->s_addr;
            }
        } else {
            struct addrinfo hints;
            struct addrinfo *result, *rp;
            ::memset(&hints, 0, sizeof(struct addrinfo));
            hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 we filter later on */
            hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
            hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
            hints.ai_protocol = 0;          /* Any protocol */
            hints.ai_canonname = NULL;
            hints.ai_addr = NULL;
            hints.ai_next = NULL;

            int ss = ::getaddrinfo(hostname, 0, &hints, &result);
            if ( !ss ) {
                for ( rp = result ; rp ; rp = rp->ai_next ) {
                    if ( rp->ai_family != AF_INET ||
                        ::inet_ntoa(reinterpret_cast<sockaddr_in *>(rp->ai_addr)->sin_addr) == std::string("127.0.0.1") ) {
                        continue;
                    }
                    UdpSocket try_sock;
                    int sfd = try_sock.Create(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
                    if ( sfd < 0 ) {
                        continue;
                    }

                    if ( !try_sock.Bind(rp->ai_addr) ) {
                        try_sock.Close();
                        // struct addrinfo
                        // {
                        //   int ai_flags;         /* Input flags.  */
                        //   int ai_family;        /* Protocol family for socket.  */
                        //   int ai_socktype;      /* Socket type.  */
                        //   int ai_protocol;      /* Protocol for socket.  */
                        //   socklen_t ai_addrlen;     /* Length of socket address.  */
                        //   struct sockaddr *ai_addr; /* Socket address for socket.  */
                        //   char *ai_canonname;       /* Canonical name for service location.  */
                        //   struct addrinfo *ai_next; /* Pointer to next in list.  */
                        // };
                        this_nodes_no_local_addr.addr.sin_addr = reinterpret_cast<sockaddr_in *>(rp->ai_addr)->sin_addr;
                        this_nodes_no_local_addr.addr.sin_port = reinterpret_cast<sockaddr_in *>(rp->ai_addr)->sin_port;
                        this_nodes_no_local_addr.addr.sin_family = rp->ai_family;
                        break; /* Success - use this address */
                    }
                }
            } else {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Can't get this machine's address: " << ::gai_strerror(ss) << " errno: " << SoeLogger::LogError() << endl;
            }
        }
    } else {
        char hostname_1[NI_MAXHOST + 1];

        // try to bind each address, first bound will be used
        for ( ifa = ifaddr, n = 0 ; ifa ; ifa = ifa->ifa_next, n++ ) {
            if ( !ifa->ifa_addr ) {
                continue;
            }

            family = ifa->ifa_addr->sa_family;
            if ( family == AF_INET ) {
                s = ::getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), hostname_1, NI_MAXHOST, 0, 0, NI_NUMERICHOST);
                if ( s < 0 ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "getnameinfo filed: " << ::gai_strerror(s) << " errno: " << errno << SoeLogger::LogError() << endl;
                }
                if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "hostname_1: "<< hostname_1 << SoeLogger::LogDebug() << endl;
                if ( hostname_1 == std::string("127.0.0.1") ) {
                    continue;
                }

                this_nodes_no_local_addr.addr.sin_addr = reinterpret_cast<sockaddr_in *>(ifa->ifa_addr)->sin_addr;
                this_nodes_no_local_addr.addr.sin_port = reinterpret_cast<sockaddr_in *>(ifa->ifa_addr)->sin_port;
                this_nodes_no_local_addr.addr.sin_family = ifa->ifa_addr->sa_family;
                break;
            }
        }
    }

    ::freeifaddrs(ifaddr);

    return this_nodes_no_local_addr;
}

}


