/**
 * @file    dbsrvmcastfactory.hpp
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
 * dbsrvmcastfactory.hpp
 *
 *  Created on: Feb 7, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMCASTFACTORY_HPP_
#define SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMCASTFACTORY_HPP_

namespace MetaMsgs {

class McastMsgFactory
{
public:
    static McastMsgBase::GetMsgBaseFunction get_factories[eMsgMax];
    static McastMsgBase::PutMsgBaseFunction put_factories[eMsgMax];

    static McastMsgFactory *instance;
    static McastMsgFactory *getInstance();

    McastMsgBase *get(eMsgType typ) const throw(std::runtime_error);
    void put(McastMsgBase *msg) throw(std::runtime_error);

    McastMsgFactory();
    virtual ~McastMsgFactory();

    void registerGetFactory(eMsgType typ, McastMsgBase::GetMsgBaseFunction fac) throw(std::runtime_error);
    void registerPutFactory(eMsgType typ, McastMsgBase::PutMsgBaseFunction fac) throw(std::runtime_error);
};

inline McastMsgBase *McastMsgFactory::get(eMsgType typ) const throw(std::runtime_error)
{
    if ( FactoryResolver::isMcast(typ) && McastMsgFactory::get_factories[typ] ) {
        return (*McastMsgFactory::get_factories[typ])();
    }
    throw std::runtime_error((std::string("No get factory for typ: ") + to_string(typ)).c_str());
}

inline void McastMsgFactory::put(McastMsgBase *msg) throw(std::runtime_error)
{
    if ( FactoryResolver::isMcast(static_cast<eMsgType>(msg->hdr.type)) == true && McastMsgFactory::put_factories[static_cast<eMsgType>(msg->hdr.type)] ) {
        (*McastMsgFactory::put_factories[static_cast<eMsgType>(msg->hdr.type)])(msg);
        return;
    }
    throw std::runtime_error((std::string("No put factory for typ: ") + to_string(msg->hdr.type)).c_str());
}

}

#endif /* SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMCASTFACTORY_HPP_ */
