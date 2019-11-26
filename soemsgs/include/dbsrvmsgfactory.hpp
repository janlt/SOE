/**
 * @file    dbsrvmsgfactory.hpp
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
 * dbsrvmsgfactory.hpp
 *
 *  Created on: Jan 13, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGFACTORY_HPP_
#define SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGFACTORY_HPP_

namespace MetaMsgs {

typedef enum _eFactoryType
{
    eFactoryInvalid      = 0,
    eFactoryAdm          = 1,
    eFactoryIov          = 2,
    eFactoryAsyncIov     = 3,
    eFactoryMcast        = 4,
    eFactoryMax          = 5
} eFactoryType;

class AdmMsgFactoryIdent
{
    static int factory_id;
};

class FactoryResolver
{
    FactoryResolver();
    virtual ~FactoryResolver();

    static int start_adm_type;
    static int end_adm_type;

    static int start_iov_type;
    static int end_iov_type;

    static int start_mcast_type;
    static int end_mcast_type;

public:

    static bool isAdm(eMsgType typ)
    {
        return typ >= start_adm_type && typ <= end_adm_type;
    }

    static bool isIov(eMsgType typ)
    {
        return typ >= start_iov_type && typ <= end_iov_type;
    }

    static bool isMcast(eMsgType typ)
    {
        return typ >= start_mcast_type && typ <= end_mcast_type;
    }
};

}

#endif /* SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGFACTORY_HPP_ */
