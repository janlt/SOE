/**
 * @file    resourcemonitorbase.cpp
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
 * resourcemonitors.cpp
 *
 *  Created on: Nov 28, 2017
 *      Author: Jan Lisowiec
 */

#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/wait.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <new>

using namespace std;

#include "resourcemonitorbase.hpp"
#include "resourcemonitors.hpp"

namespace SoeMonitors {

LifeMonitor::LifeMonitor() {}
LifeMonitor::~LifeMonitor() {}
ResourceMonitorBase::ResourceStatus LifeMonitor::Monitor(pid_t proc)
{
    int stat_val;
    pid_t ret_pid = waitpid(proc, &stat_val, WNOHANG);

    ResourceMonitorBase::ResourceStatus proc_status = ResourceMonitorBase::ResourceStatus::eUnknown;
    if ( ret_pid == -1 ) {
        return proc_status;
    }

    if ( ret_pid == proc ) {
        if ( WIFEXITED(stat_val) ) {
            cout << "LifeMonitor::" << __FUNCTION__ << " pid: " << proc << " process terminated normally status: " << WEXITSTATUS(stat_val) << endl;
            proc_status = ResourceMonitorBase::ResourceStatus::eNormal;
        } else if ( WIFSIGNALED(stat_val) ) {
            cout << "LifeMonitor::" << __FUNCTION__ << " pid: " << proc << " process terminated due to uncaught signal: " << WTERMSIG(stat_val) << endl;
            proc_status = ResourceMonitorBase::ResourceStatus::eCritical;
        } else if ( WIFSTOPPED(stat_val) ) {
            cout << "LifeMonitor::" << __FUNCTION__ << " pid: " << proc << " process is currently stopped on signal: " << WIFSTOPPED(stat_val) << endl;
            proc_status = ResourceMonitorBase::ResourceStatus::eAlarm;
        } else if ( WIFCONTINUED(stat_val) ) {
            cout << "LifeMonitor::" << __FUNCTION__ << " pid: " << proc << " process continued " << endl;
            proc_status = ResourceMonitorBase::ResourceStatus::eNormal;
        }
    }

    return proc_status;
}
const char *LifeMonitor::Name()
{
    return boost::typeindex::type_id<LifeMonitor>().pretty_name().c_str();
}


MemoryMonitor::MemoryMonitor() {}
MemoryMonitor::~MemoryMonitor() {}
ResourceMonitorBase::ResourceStatus MemoryMonitor::Monitor(pid_t proc)
{
    return ResourceMonitorBase::ResourceStatus::eNormal;
}
const char *MemoryMonitor::Name()
{
    return boost::typeindex::type_id<MemoryMonitor>().pretty_name().c_str();
}


CpuMonitor::CpuMonitor() {}
CpuMonitor::~CpuMonitor() {}
ResourceMonitorBase::ResourceStatus CpuMonitor::Monitor(pid_t proc)
{
    return ResourceMonitorBase::ResourceStatus::eNormal;
}
const char *CpuMonitor::Name()
{
    return boost::typeindex::type_id<CpuMonitor>().pretty_name().c_str();
}


FdMonitor::FdMonitor() {}
FdMonitor::~FdMonitor() {}
ResourceMonitorBase::ResourceStatus FdMonitor::Monitor(pid_t proc)
{
    return ResourceMonitorBase::ResourceStatus::eNormal;
}
const char *FdMonitor::Name()
{
    return boost::typeindex::type_id<FdMonitor>().pretty_name().c_str();
}


}


