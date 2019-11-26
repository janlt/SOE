/*
 * poolmanager.hpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * poolmanager.hpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MEMPOOLS_INCLUDE_POOLMANAGER_HPP_
#define SOE_SOE_SOE_MEMPOOLS_INCLUDE_POOLMANAGER_HPP_


#include <map>

using namespace std;

#include "generalpoolbase.hpp"

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread.hpp>

typedef boost::shared_mutex Lock;
typedef boost::unique_lock<Lock> WriteLock;
typedef boost::shared_lock<Lock> ReadLock;

namespace MemMgmt {
class PoolManager
{
public:
    typedef map<unsigned int, GeneralPoolBase *, less<unsigned int> > GeneralPoolMap;

public:
    ~PoolManager();
    static void start(PoolManager *arg);
    static PoolManager & getInstance();
    void registerPool(GeneralPoolBase *pool_);
    void unregisterPool(GeneralPoolBase *pool_);
    void dump();

#ifdef _MEM_STATS_
    char *printDateTime(const time_t &dtm);
#endif

private:
    PoolManager();
    //PoolManager(const PoolManager &right);
    //PoolManager & operator=(const PoolManager &right);

    static PoolManager *instance;
    Lock                mutex;
    GeneralPoolMap      pools;
    static Lock         instMutex;

    boost::thread      *collector;

#ifdef _MEM_STATS_
    FILE *statsFile;
    int statsInterval;
    bool statsOn;
#endif
};
} // namespace MemMgmt


#endif /* SOE_SOE_SOE_MEMPOOLS_INCLUDE_POOLMANAGER_HPP_ */
