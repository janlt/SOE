/**
 * @file    metadbclifacadeapi.hpp
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
 * metadbclifacadeapi.hpp
 *
 *  Created on: Feb 3, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBCLI_INCLUDE_METADBCLIFACADEAPI_HPP_
#define SOE_SOE_SOE_METADBCLI_INCLUDE_METADBCLIFACADEAPI_HPP_

namespace MetadbcliFacade {

class Exception: public RcsdbFacade::Exception
{
public:
    Exception(const std::string err = "Know nothing", eStatus sts = eInternalError)
        :RcsdbFacade::Exception(err, sts)
    {}
    virtual ~Exception() {}
    virtual const std::string &what() const { return RcsdbFacade::Exception::what(); }
};

}

#endif /* SOE_SOE_SOE_METADBCLI_INCLUDE_METADBCLIFACADEAPI_HPP_ */
