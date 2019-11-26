/**
 * @file    dbsrvmarshallable.hpp
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
 * dbsrvmarshallable.hpp
 *
 *  Created on: Jan 9, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMARSHALLABLE_HPP_
#define SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMARSHALLABLE_HPP_

namespace MetaMsgs {

class AdmMsgBuffer;
class McastMsgBuffer;

class AdmMsgMarshallable
{
public:
    AdmMsgMarshallable();
    virtual ~AdmMsgMarshallable();
    virtual void marshall (AdmMsgBuffer& buf) = 0;
    virtual void unmarshall (AdmMsgBuffer& buf) = 0;
    friend MetaMsgs::AdmMsgBuffer& operator << (MetaMsgs::AdmMsgBuffer& buffer, AdmMsgMarshallable& rval);
    friend MetaMsgs::AdmMsgBuffer& operator >> (MetaMsgs::AdmMsgBuffer& buffer, AdmMsgMarshallable& rval);

private:
    AdmMsgMarshallable(const AdmMsgMarshallable &_r);
    const AdmMsgMarshallable & operator=(const AdmMsgMarshallable &_r);
};

class McastMsgMarshallable
{
public:
    McastMsgMarshallable();
    virtual ~McastMsgMarshallable();
    virtual void marshall (McastMsgBuffer& buf) = 0;
    virtual void unmarshall (McastMsgBuffer& buf) = 0;
    friend MetaMsgs::McastMsgBuffer& operator << (MetaMsgs::McastMsgBuffer& buffer, McastMsgMarshallable& rval);
    friend MetaMsgs::McastMsgBuffer& operator >> (MetaMsgs::McastMsgBuffer& buffer, McastMsgMarshallable& rval);

private:
    McastMsgMarshallable(const McastMsgMarshallable &_r);
    const McastMsgMarshallable & operator=(const McastMsgMarshallable &_r);
};

}


#endif /* SOE_SOE_SOE_MSGS_INCLUDE_DBSRVMARSHALLABLE_HPP_ */
