/**
 * @file    resourcemonitorbase.hpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe Monitors
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
 * resourcemonitorbase.hpp
 *
 *  Created on: Nov 28, 2017
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_SOEMONITORS_INCLUDE_RESOURCEMONITORBASE_HPP_
#define RENTCONTROL_SOE_SOE_SOE_SOEMONITORS_INCLUDE_RESOURCEMONITORBASE_HPP_

namespace SoeMonitors {

class ResourceMonitorBase
{
public:
    typedef enum _ResourceStatus {
        eNormal   = 0,
        eAlarm    = 1,
        eCritical = 2,
        eUnknown  = 3
    } ResourceStatus;

public:
    ResourceMonitorBase() {}
    virtual ~ResourceMonitorBase() {}
    virtual ResourceMonitorBase::ResourceStatus Monitor(pid_t proc) = 0;
    virtual const char *Name() = 0;
};

}

#endif /* RENTCONTROL_SOE_SOE_SOE_SOEMONITORS_INCLUDE_RESOURCEMONITORBASE_HPP_ */
