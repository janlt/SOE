/**
 * @file    metadbcliiterator.cpp
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
 * metadbcliiterator.cpp
 *
 *  Created on: Aug 7, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>

#include "metadbcliiterator.hpp"

namespace MetadbcliFacade {

ProxyIterator::ProxyIterator(uint32_t _id)
    :id(_id),
     key_buf(0),
     key_buf_len(0),
     buf_buf(0),
     buf_buf_len(0)
{}

ProxyIterator::~ProxyIterator()
{
    if ( key_buf && key_buf_len ) {
        delete [] key_buf;
    }
    if ( buf_buf && buf_buf_len ) {
        delete [] buf_buf;
    }
}

}


