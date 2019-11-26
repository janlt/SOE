/**
 * @file    dbsrvmsgadmfactory.hpp
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
 * dbsrvmsgadmfactory.hpp
 *
 *  Created on: Feb 7, 2017
 *      Author:  Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGADMFACTORY_HPP_
#define SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGADMFACTORY_HPP_

namespace MetaMsgs {

class AdmMsgFactory
{
public:
    static AdmMsgBase::GetMsgBaseFunction get_factories[eMsgMax];
    static AdmMsgBase::PutMsgBaseFunction put_factories[eMsgMax];

    static AdmMsgFactory *instance;
    static AdmMsgFactory *getInstance();

    AdmMsgBase *get(eMsgType typ) const throw(std::runtime_error);
    void put(AdmMsgBase *msg) throw(std::runtime_error);

    AdmMsgFactory();
    virtual ~AdmMsgFactory();

    void registerGetFactory(eMsgType typ, AdmMsgBase::GetMsgBaseFunction fac) throw(std::runtime_error);
    void registerPutFactory(eMsgType typ, AdmMsgBase::PutMsgBaseFunction fac) throw(std::runtime_error);
};

inline AdmMsgBase *AdmMsgFactory::get(eMsgType typ) const throw(std::runtime_error)
{
    if ( FactoryResolver::isAdm(typ) && AdmMsgFactory::get_factories[typ] ) {
        return (*AdmMsgFactory::get_factories[typ])();
    }
    throw std::runtime_error((std::string("No get factory for typ: ") + to_string(typ)).c_str());
}

inline void AdmMsgFactory::put(AdmMsgBase *msg) throw(std::runtime_error)
{
    if ( FactoryResolver::isAdm(static_cast<eMsgType>(msg->hdr.type)) == true && AdmMsgFactory::put_factories[static_cast<eMsgType>(msg->hdr.type)] ) {
        (*AdmMsgFactory::put_factories[static_cast<eMsgType>(msg->hdr.type)])(msg);
        return;
    }
    throw std::runtime_error((std::string("No put factory for typ: ") + to_string(msg->hdr.type)).c_str());
}

}

#endif /* SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGADMFACTORY_HPP_ */
