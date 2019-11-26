/**
 * @file    dbsrvmsgviofactory.hpp
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
 * dbsrvmsgviofactory.hpp
 *
 *  Created on: Jan 13, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGVIOFACTORY_HPP_
#define SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGVIOFACTORY_HPP_

namespace MetaMsgs {

class IovMsgBase;

class IovMsgFactory
{
public:
    static IovMsgBase::GetMsgBaseFunction get_factories[eMsgMax];
    static IovMsgBase::PutMsgBaseFunction put_factories[eMsgMax];

    static IovMsgFactory *instance;
    static IovMsgFactory *getInstance();

    IovMsgBase *get(eMsgType typ) throw(std::runtime_error);
    void put(IovMsgBase *msg) throw(std::runtime_error);

    IovMsgFactory();
    virtual ~IovMsgFactory();

    void registerGetFactory(eMsgType typ, IovMsgBase::GetMsgBaseFunction fac) throw(std::runtime_error);
    void registerPutFactory(eMsgType typ, IovMsgBase::PutMsgBaseFunction fac) throw(std::runtime_error);
};

inline IovMsgBase *IovMsgFactory::get(eMsgType typ) throw(std::runtime_error)
{
    if ( FactoryResolver::isIov(typ) && IovMsgFactory::get_factories[typ] ) {
        return (*IovMsgFactory::get_factories[typ])();
    }
    throw std::runtime_error((std::string("No get factory for typ: ") + to_string(typ)).c_str());
}

inline void IovMsgFactory::put(IovMsgBase *msg) throw(std::runtime_error)
{
    if ( FactoryResolver::isIov(static_cast<eMsgType>(msg->iovhdr.hdr.type)) == true && IovMsgFactory::put_factories[static_cast<eMsgType>(msg->iovhdr.hdr.type)] ) {
        (*IovMsgFactory::put_factories[static_cast<eMsgType>(msg->iovhdr.hdr.type)])(msg);
        return;
    }
    throw std::runtime_error((std::string("No put factory for typ: ") + to_string(msg->iovhdr.hdr.type)).c_str());
}

}

#endif /* SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMSGVIOFACTORY_HPP_ */
