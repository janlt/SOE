/*
 * generalpoolbase.hpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * generalpoolbase.hpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MEMPOOLS_INCLUDE_GENERALPOOLBASE_HPP_
#define SOE_SOE_SOE_MEMPOOLS_INCLUDE_GENERALPOOLBASE_HPP_


#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

typedef boost::shared_mutex Lock;
typedef boost::unique_lock<Lock> WriteLock;
typedef boost::shared_lock<Lock> ReadLock;

namespace MemMgmt {
class GeneralPoolBase {
public:
    GeneralPoolBase(unsigned short id_, const char *name_);
    virtual ~GeneralPoolBase();
    virtual void manage() = 0;
    virtual void dump() = 0;
    virtual void stats(FILE &file_) = 0;
    void setRttiName(const char *_rttiName);
    const unsigned short getId() const;
    const char* getName() const;
    const char* getRttiName() const;
    const FILE* getStackTraceFile() const;
    void setStackTraceFile(FILE* value);
    const unsigned short getUserId() const;

    unsigned short id;
    char           name[32];
    char           rttiName[128];
    FILE          *stackTraceFile;

private:
    GeneralPoolBase(const GeneralPoolBase &right);
    GeneralPoolBase & operator=(const GeneralPoolBase &right);

    static unsigned short idGener;
    unsigned short        userId;
    Lock                  generMutex;
};

inline const unsigned short GeneralPoolBase::getId() const
{
    return id;
}

inline const char* GeneralPoolBase::getName() const
{
    return name;
}

inline const char* GeneralPoolBase::getRttiName() const
{
    return rttiName;
}

inline const FILE* GeneralPoolBase::getStackTraceFile() const
{
    return stackTraceFile;
}

inline void GeneralPoolBase::setStackTraceFile(FILE* value)
{
    stackTraceFile = value;
}

inline const unsigned short GeneralPoolBase::getUserId() const
{
    return userId;
}
} // namespace MemMgmt



#endif /* SOE_SOE_SOE_MEMPOOLS_INCLUDE_GENERALPOOLBASE_HPP_ */
