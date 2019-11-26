/**
 * @file    msgnetpoint.cpp
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
 * msgnetpoint.cpp
 *
 *  Created on: Feb 6, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

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

namespace MetaNet {

PointHashEntry  Point::points[Point::max_points];

Point::Initializer points_initalized;

bool      Point::Initializer::initialized = false;
uint64_t  Point::open_counter = 0;
uint64_t  Point::close_counter = 0;

Point::Initializer::Initializer()
{
    if ( initialized != true ) {
        ::memset(Point::points, '\0', sizeof(Point::points));
    }
    initialized = true;
}

Point::Point(Point::eType _type, int _desc)
    :type(_type),
    desc(_desc),
    state(Point::eFdState::eInit)
{}

Point::~Point()
{}

// Linux is guaranteeing fd uniqueness
int Point::Register(bool _ignore, bool _force_register) throw(std::runtime_error)
{
    int idx = GetDesc();
    if ( idx < 0 || idx >= static_cast<int>(Point::max_points) ) {
        if ( _ignore == false ) {
            throw std::runtime_error((std::string(__FUNCTION__) + " Point hash corrupt fd: " + to_string(idx)).c_str());
        }
        return -1;
    } else {
        if ( (Point::points[idx].fd || Point::points[idx].point) && _force_register == false ) {
            if ( _ignore == false ) {
                throw std::runtime_error((std::string(__FUNCTION__) + " Point hash corrupt fd: " + to_string(idx)).c_str());
            }
            return -1;
        }
    }
    Point::points[idx].fd = idx;
    Point::points[idx].point = this;
    return 0;
}

int Point::Deregister(bool _ignore, bool _force_deregister) throw(std::runtime_error)
{
    int idx = GetDesc();
    if ( idx < 0 || idx >= static_cast<int>(Point::max_points) ) {
        if ( _ignore == false ) {
            throw std::runtime_error((std::string(__FUNCTION__) + " Point hash corrupt fd: " + to_string(idx)).c_str());
        }
        return -1;
    } else {
        if ( (!Point::points[idx].fd || !Point::points[idx].point) && _force_deregister == false ) {
            if ( _ignore == false ) {
                throw std::runtime_error((std::string(__FUNCTION__) + " Point hash corrupt fd: " + to_string(idx)).c_str());
            }
            return -1;
        }
    }
    Point::points[idx].fd = 0;
    Point::points[idx].point = 0;
    return 0;
}

Point *Point::Find(int fd)
{
    if ( fd < 0 || fd >= static_cast<int>(Point::max_points) || Point::points[fd].fd != fd ) {
        return 0;
    }

    return Point::points[fd].point;

}

int Point::CheckFd()
{
    if ( GetState() != Point::eFdState::eOpened ) {
        return 2;
    }

    int ret = ::fcntl(desc, F_GETFD);
    if ( ret < 0 || errno == EBADF ) {
        return -1;
    }

    return 0;
}

}

