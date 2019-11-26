/**
 * @file    dbsrvmcasttypes.hpp
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
 * dbsrvmcasttypes.hpp
 *
 *  Created on: Feb 7, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMCASTTYPES_HPP_
#define SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMCASTTYPES_HPP_

namespace MetaMsgs {

class McastMsgPoolInitializer
{
public:
    static bool initialized;
    McastMsgPoolInitializer();
    ~McastMsgPoolInitializer() {}
};

// -----------------------------------------------------------------------------------------

class McastMsgMetaAd: public McastMsg
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<McastMsgMetaAd, MemMgmt::ClassAlloc<McastMsgMetaAd> > poolT;

public:
    McastMsgMetaAd();
    virtual ~McastMsgMetaAd();

    virtual void marshall (McastMsgBuffer& buf);
    virtual void unmarshall (McastMsgBuffer& buf);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static McastMsgMetaAd *get();
    static void put(McastMsgMetaAd *obj);
    static void registerPool();
};

// -----------------------------------------------------------------------------------------

class McastMsgMetaSolicit: public McastMsg
{
public:
    static int    class_msg_type;
    static MemMgmt::GeneralPool<McastMsgMetaSolicit, MemMgmt::ClassAlloc<McastMsgMetaSolicit> > poolT;

public:
    McastMsgMetaSolicit();
    virtual ~McastMsgMetaSolicit();

    virtual void marshall (McastMsgBuffer& buf);
    virtual void unmarshall (McastMsgBuffer& buf);

    virtual std::string toJson();
    virtual void fromJson(const std::string &_sjon);

    static McastMsgMetaSolicit *get();
    static void put(McastMsgMetaSolicit *obj);
    static void registerPool();
};

}

#endif /* SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMCASTTYPES_HPP_ */
