/**
 * @file    watchdogmonitor.cpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe Monitor
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
 * watchdogmonitor.cpp
 *
 *  Created on: Nov 28, 2017
 *      Author: Jan Lisowiec
 */

#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <dlfcn.h>

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

#include "resourcemonitorbase.hpp"
#include "resourcemonitors.hpp"
#include "watchdogmonitor.hpp"

using namespace std;

namespace SoeMonitors {

ForkedWatchdog::ForkedWatchdog(const char *_file_name, char **_argv, char **_env) throw(std::runtime_error)
    :WatchdogBase(fork()),
    file_name(_file_name ? _file_name : ""),
    argv(_argv),
    env(_env)
{
    // parent
    if ( GetProcessPid() ) {
        if ( file_name.empty() == false ) {
            int ret = execve(file_name.c_str(), argv, env);
            if ( ret < 0 ) {
                ostringstream str_buf;
                str_buf << "ForkedWatchdog::" << __FUNCTION__ << " failed file_name: " <<
                    file_name << " errno: " << errno << endl;
                throw std::runtime_error(str_buf.str().c_str());
            }
        }
    }
}

ForkedWatchdog::~ForkedWatchdog()
{}

void ForkedWatchdog::Monitor()
{
    if ( !GetProcessPid() ) {
        return;
    }

    for ( uint32_t i = 0 ; i < resource_monitors.size() ; i++ ) {
        ResourceMonitorBase::ResourceStatus ret = resource_monitors[i]->Monitor(GetProcessPid());
        cout << "ForkedWatchdog::" << __FUNCTION__ << " " << resource_monitors[i]->Name() << " ret: " << ret << endl;
    }
}

void ForkedWatchdog::AddResourceMonitor(ResourceMonitorBase *_monitor)
{
    resource_monitors.push_back(_monitor);
}

}


