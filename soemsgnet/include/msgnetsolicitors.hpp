/**
 * @file    msgnetsolicitors.hpp
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
 * msgnetsolicitors.hpp
 *
 *  Created on: Mar 1, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETSOLICITORS_HPP_
#define SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETSOLICITORS_HPP_

namespace MetaNet {

class SolicitationItem
{
    std::string     what;
    std::string     description;

public:
    SolicitationItem(const std::string &_what = "",
        const std::string &_desc = "");
};

class Solicitation
{
protected:
    std::string json;
    bool        debug;

public:
    Solicitation(const std::string &_json);
    virtual ~Solicitation();

    virtual std::string AsJson() = 0;
    virtual void AddItem(const SolicitationItem &_item) = 0;
    virtual int Parse() = 0;

    void SetDebug(bool _debug);
};

class MsgNetMcastSolicitation: public Solicitation
{
    std::vector<SolicitationItem>   items;
    uint32_t                        requested_ttl;
    bool                            requested_secure;

public:
    MsgNetMcastSolicitation(const std::string &_json = "");
    virtual ~MsgNetMcastSolicitation();

    virtual std::string AsJson();
    virtual void AddItem(const SolicitationItem &_item);
    virtual int Parse();

    void SetRequestedTtl(uint32_t _ttl);
    void SetRequestedSecure(bool _req_sec);
};

}

#endif /* SOE_SOE_SOE_MSGNET_INCLUDE_MSGNETSOLICITORS_HPP_ */
