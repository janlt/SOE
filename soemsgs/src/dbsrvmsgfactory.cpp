/**
 * @file    dbsrvmsgfactory.cpp
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
 * dbsrvmsgfactory.cpp
 *
 *  Created on: Jan 13, 2017
 *      Author: Jan Lisowiec
 */

#include <sys/uio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h>

#include <string>

#include "classalloc.hpp"
#include "generalpool.hpp"
#include "dbsrvmsgbuffer.hpp"
#include "dbsrvmarshallable.hpp"
#include "dbsrvmsgs.hpp"
#include "dbsrvmsgtypes.hpp"
#include "dbsrvmsgfactory.hpp"

namespace MetaMsgs {

int FactoryResolver::start_adm_type = eMsgAssignMMChannelReq;
int FactoryResolver::end_adm_type = eMsgDeleteSnapshotEngineRsp;

int FactoryResolver::start_iov_type = eMsgGetReq;
int FactoryResolver::end_iov_type = eMsgAbortTransactionRsp;

int FactoryResolver::start_mcast_type = eMsgMetaAd;
int FactoryResolver::end_mcast_type = eMsgMetaSolicit;

}

