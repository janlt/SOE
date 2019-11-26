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
 * resourcemonitors.hpp
 *
 *  Created on: Nov 28, 2017
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_SOEMONITORS_INCLUDE_RESOURCEMONITORS_HPP_
#define RENTCONTROL_SOE_SOE_SOE_SOEMONITORS_INCLUDE_RESOURCEMONITORS_HPP_

using namespace std;

namespace SoeMonitors {

class LifeMonitor: public ResourceMonitorBase
{
public:
    LifeMonitor();
    virtual ~LifeMonitor();
    virtual ResourceMonitorBase::ResourceStatus Monitor(pid_t proc);
    virtual const char *Name();
};

class MemoryMonitor: public ResourceMonitorBase
{
public:
    MemoryMonitor();
    virtual ~MemoryMonitor();
    virtual ResourceMonitorBase::ResourceStatus Monitor(pid_t proc);
    virtual const char *Name();
};

class CpuMonitor: public ResourceMonitorBase
{
public:
    CpuMonitor();
    virtual ~CpuMonitor();
    virtual ResourceMonitorBase::ResourceStatus Monitor(pid_t proc);
    virtual const char *Name();
};

class FdMonitor: public ResourceMonitorBase
{
public:
    FdMonitor();
    virtual ~FdMonitor();
    virtual ResourceMonitorBase::ResourceStatus Monitor(pid_t proc);
    virtual const char *Name();
};

}

#endif /* RENTCONTROL_SOE_SOE_SOE_SOEMONITORS_INCLUDE_RESOURCEMONITORS_HPP_ */
