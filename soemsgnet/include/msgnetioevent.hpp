/**
 * @file    msgnetioevent.hpp
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
 * msgnetioevent.hpp
 *
 *  Created on: Mar 17, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIOEVENT_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIOEVENT_HPP_

namespace MetaNet {

//
//typedef union epoll_data {
//    void        *ptr;
//    int          fd;
//    uint32_t     u32;
//    uint64_t     u64;
//} epoll_data_t;
//
//struct epoll_event {
//    uint32_t     events;      /* Epoll events */
//    epoll_data_t data;        /* User data variable */
//};
//
// ptr is used to pass ap data
//

typedef struct epoll_event IoEpollEvent;

struct IoEvent
{
    IoEpollEvent          ev;
    int                   epoll_fd;
    void                 *io;

    IoEvent(const IoEpollEvent &_ev, int _epoll_fd = -1);
    IoEvent(const IoEvent &_ev);
    ~IoEvent() {}

    static IoEvent *RDHUOEvent();
};

inline IoEvent::IoEvent(const IoEpollEvent &_ev, int _epoll_fd)
    :ev(_ev),
     epoll_fd(_epoll_fd),
     io(0)
{}

inline IoEvent::IoEvent(const IoEvent &_ev)
    :ev(_ev.ev),
     epoll_fd(_ev.epoll_fd),
     io(_ev.io)
{}

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETIOEVENT_HPP_ */
