/*
 * main.cpp
 *
 *  Created on: Nov 29, 2017
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
using namespace SoeMonitors;

int main(int argc, char **argv)
{
    char *newargv[16];
    char *newenviron[] = { 0 };
    ::memset(newargv, '\0', sizeof(newargv));

    newargv[0] = argv[1];
    for ( int i = 1 ; i < argc && i < 16 ; i++ ) {
        newargv[i] = argv[i + 1];
    }

    int j = 0;
    for ( const char *ptr = newargv[j] ; ptr ; ptr = newargv[j++] ) {
        cout << "newargv[" << j << "] " << newargv[j] << endl;
    }

    ForkedWatchdog *fw;
    try {
        fw = new ForkedWatchdog(argv[1], newargv, newenviron);
    } catch (const std::runtime_error &ex) {
        cout << "Exception caught what: " << ex.what() << endl;
        return -1;
    }

    if ( fw->GetProcessPid() ) {
        LifeMonitor life_monitor;
        MemoryMonitor mem_monitor;
        CpuMonitor cpu_monitor;
        FdMonitor fd_monitor;

        fw->AddResourceMonitor(&life_monitor);
        fw->AddResourceMonitor(&mem_monitor);
        fw->AddResourceMonitor(&cpu_monitor);
        fw->AddResourceMonitor(&fd_monitor);

        for ( ;; ) {
            sleep(1);
            fw->Monitor();
        }
    } else {
        for ( int i = 0 ; i < 3 ; i++ ) {
            sleep(1);
        }
        char *ptr = 0;
        memcpy(ptr, " ", 1);
    }

    delete fw;
    return 0;
}

