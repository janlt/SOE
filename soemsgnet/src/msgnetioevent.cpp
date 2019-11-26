/**
 * @file    msgnetioevent.cpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe Session API
 *
 *
 * @section Copyright
 *
 *   Copyright Notice:
 *
 *    Copyright  2014-2019 Jan Lisowiec jlisowiec@gmail.com
 *
 *
 * This product is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * This software is distributed in the hope that it will be useful.
 *
 * @section Licence
 *
 *
 * @section Description
 *
 *   soe_soe
 *
 * @section Source
 *
 *
 */

/*
 * msgnetioevent.cpp
 *
 *  Created on: Feb 9, 2018
 *      Author: Jan Lisowiec
 */

#include <sys/epoll.h>
#include <cstring>

#include "msgnetioevent.hpp"

namespace MetaNet {

//
// Produce HUP epoll event
//
IoEvent *IoEvent::RDHUOEvent()
{
    IoEpollEvent ep_ev;
    ::memset(&ep_ev, '\0', sizeof(ep_ev));
    MetaNet::IoEvent *ev = new MetaNet::IoEvent(ep_ev);
    ev->ev.events |= EPOLLRDHUP;

    return ev;
}

}


