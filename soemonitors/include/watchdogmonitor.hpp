/**
 * @file    watchdogmonitor.hpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe monitors
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
 * watchdogmmonitor.hpp
 *
 *  Created on: Nov 28, 2017
 *      Author: Jan Lisowiec
 */

#ifndef RENTCONTROL_SOE_SOE_SOE_SOEMONITORS_INCLUDE_WATCHDOGMONITOR_HPP_
#define RENTCONTROL_SOE_SOE_SOE_SOEMONITORS_INCLUDE_WATCHDOGMONITOR_HPP_

using namespace std;

namespace SoeMonitors {

class WatchdogBase
{
    pid_t      process_pid;
    const static pid_t invalid_pid = 0;
    std::vector<ResourceMonitorBase *> resource_monitors;

public:
    WatchdogBase(pid_t _pid = WatchdogBase::invalid_pid):
        process_pid(_pid)
    {}
    virtual ~WatchdogBase() {}
    pid_t GetProcessPid() { return process_pid; }
    virtual void AddResourceMonitor(ResourceMonitorBase *_monitor) = 0;
    virtual void Monitor() = 0;
};

class ForkedWatchdog: public WatchdogBase
{
    string file_name;
    char **argv;
    char **env;

    std::vector<ResourceMonitorBase *> resource_monitors;

public:
    ForkedWatchdog(const char *_file_name = 0, char **_argv = 0, char **_env = 0) throw(std::runtime_error);
    virtual ~ForkedWatchdog();
    void AddResourceMonitor(ResourceMonitorBase *_monitor);
    virtual void Monitor();
};

}

#endif /* RENTCONTROL_SOE_SOE_SOE_SOEMONITORS_INCLUDE_WATCHDOGMONITOR_HPP_ */
