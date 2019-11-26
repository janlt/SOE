/*
 * generalpoolbase.cpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * generalpoolbase.cpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#include <stdio.h>
#include <string.h>

#include "generalpoolbase.hpp"
#include "poolmanager.hpp"

namespace MemMgmt {
unsigned short GeneralPoolBase::idGener = 1;

GeneralPoolBase::GeneralPoolBase(unsigned short id_, const char *name_) :
    stackTraceFile(0),
    userId(id_)
{
    // set the id
    {
    WriteLock w_lock(generMutex);
    id = GeneralPoolBase::idGener++;
    }

    // set the name
    if (name_) {
        strncpy(name, name_, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
    } else {
        name[0] = '\0';
    }

    // initialize rttiName
    rttiName[0] = '\0';

    // register with the PoolManager
    PoolManager &pm = PoolManager::getInstance();
    pm.registerPool(this);
}

GeneralPoolBase::~GeneralPoolBase()
{
    // unregister with the PoolManager
    PoolManager &pm = PoolManager::getInstance();
    pm.unregisterPool(this);

    // close the traceStatsFile
    if (stackTraceFile) {
        fflush(stackTraceFile);
        fclose(stackTraceFile);
    }
}

void GeneralPoolBase::setRttiName(const char *_rttiName)
{
    strncpy(rttiName, _rttiName, sizeof(rttiName) - 1);
    rttiName[sizeof(rttiName) - 1] = '\0';
}
} // namespace MemMgmt
