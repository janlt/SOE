/**
 * @file    soestatistics.hpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe Stats API
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
 * soestatistics.hpp
 *
 *  Created on: Sep 20, 2018
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOESTATISTICS_HPP_
#define RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOESTATISTICS_HPP_

namespace Soe {

class Session;
class Manager;

class Statistic
{
public:
    Statistic() = default;
    virtual ~Statistic() = 0;
};

}



#endif /* RENTCONTROL_SOE_SOE_SOE_SOEAPI_INCLUDE_SOESTATISTICS_HPP_ */
