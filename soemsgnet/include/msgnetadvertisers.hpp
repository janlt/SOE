/**
 * @file    msgnetadvertisers.hpp
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
 * msgnetadvertisers.hpp
 *
 *  Created on: Mar 1, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETADVERTISERS_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETADVERTISERS_HPP_

namespace MetaNet {

class MsgNetMcastAdvertisement;

class AdvertisementItem
{
    friend class MsgNetMcastAdvertisement;

public:
    typedef enum _eProtocol {
        eNone          = 0,
        eInet          = 1,
        eIPv4Tcp       = 2,
        eIPv4Udp       = 3,
        eMM            = 4,
        eAll           = 5,
        eUnix          = 6
    } eProtocol;

    std::string     what;
    std::string     description;
    std::string     address;
    uint16_t        port;
    eProtocol       protocol;

public:
    AdvertisementItem(const std::string &_what = "",
        const std::string &_desc = "",
        std::string _addr = "",
        uint16_t _port = 0,
        eProtocol _prot = eNone);
};

class Advertisement
{
protected:
    std::string json;
    bool        debug;

public:
    Advertisement(const std::string &_json);
    virtual ~Advertisement();

    virtual std::string AsJson() = 0;
    virtual std::string GetJson() = 0;
    virtual void AddItem(const AdvertisementItem &_item) = 0;
    virtual int Parse() = 0;

    void SetDebug(bool _debug);
};

//
// Our own thing
//
class MsgNetMcastAdvertisement: public Advertisement
{
    uint32_t                        ttl;
    bool                            secure;
    std::vector<AdvertisementItem>  items;

public:
    MsgNetMcastAdvertisement(const std::string &_json = "");
    virtual ~MsgNetMcastAdvertisement();

    virtual std::string AsJson();
    virtual std::string GetJson();
    virtual void AddItem(const AdvertisementItem &_item);
    virtual int Parse();

    uint32_t GetTtl(uint32_t _ttl);
    bool GetSecure(bool _req_sec);
    AdvertisementItem *FindItemByWhat(const std::string &_what);
};

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETADVERTISERS_HPP_ */
