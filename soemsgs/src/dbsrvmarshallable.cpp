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
#include "dbsrvmarshallable.hpp"

#include <netinet/in.h>

namespace MetaMsgs {

MetaMsgs::AdmMsgMarshallable::AdmMsgMarshallable ()
{
}

MetaMsgs::AdmMsgMarshallable::~AdmMsgMarshallable()
{
}

MetaMsgs::AdmMsgBuffer &operator << (MetaMsgs::AdmMsgBuffer &buffer,MetaMsgs::AdmMsgMarshallable &rval)
{
    rval.marshall(buffer);
    return buffer;
}

MetaMsgs::AdmMsgBuffer &operator >> (MetaMsgs::AdmMsgBuffer &buffer, MetaMsgs::AdmMsgMarshallable &rval)
{
    rval.unmarshall(buffer);
    return buffer;
}



MetaMsgs::McastMsgMarshallable::McastMsgMarshallable ()
{
}

MetaMsgs::McastMsgMarshallable::~McastMsgMarshallable()
{
}

MetaMsgs::McastMsgBuffer &operator << (MetaMsgs::McastMsgBuffer &buffer, MetaMsgs::McastMsgMarshallable &rval)
{
    rval.marshall(buffer);
    return buffer;
}

MetaMsgs::McastMsgBuffer &operator >> (MetaMsgs::McastMsgBuffer &buffer, MetaMsgs::McastMsgMarshallable &rval)
{
    rval.unmarshall(buffer);
    return buffer;
}

}

