/*
 * thread.hpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * thread.hpp
 *
 *  Created on: Dec 22, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MEMPOOLS_INCLUDE_THREAD_HPP_
#define SOE_SOE_SOE_MEMPOOLS_INCLUDE_THREAD_HPP_


#include <iostream>

using namespace std;

#include <pthread.h>

class Thread_Attr
{
public:
    Thread_Attr();

    Thread_Attr(const Thread_Attr &right);

    virtual ~Thread_Attr();

    const Thread_Attr & operator=(const Thread_Attr &right);

    void setScope(const int &scope);
    void setDetachState(const int &detachState);
    void setInheritSched(const int &inheritSched);
    void setSchedParam(const sched_param &param);
    void setSchedPolicy(const int &policy);
    void setStackSize(const size_t &stackSize);

    int getScope() const;
    int getDetachState() const;
    int getInheritSched() const;
    sched_param getSchedParam() const;
    int getSchedPolicy() const;
    size_t getStackSize() const;

    const pthread_attr_t get_attr() const;
private:
    pthread_attr_t attr;
};

//  This class provides a simple abstraction for a thread.
//
//  In practice it will be implemented by using pthreads.
//
//  Once created the thread can be started by using the
//  run() method. Execution will start in the start() method
//  which must be provided by an implementation of this
//  class.

class Thread
{
public:
    Thread(bool using_pools_ = false);
    virtual ~Thread();
    int operator==(const Thread &right) const;
    int operator!=(const Thread &right) const;

    //    This operation is used to start a thread running. This
    //    call will equate to a pthread_create call.
    void run();
    void join();
    void cancel();
    void detach();

    //    Contains the thread ID that the operating system has
    //    assigned to this thread. Can later be used to identify
    //    this thread.
    const pthread_t get_threadId() const;

    Thread_Attr thr_attr;

    void set_pool_range(unsigned int range_);
    unsigned int get_range_pool()
    {
        return start_pool + (++next_pool)%range;
    }

protected:
    //    This pure virtual method will need to be implemented by
    //    specialising classes. After the run() method is called
    //    execution will start in this method().
    virtual void start() = 0;

    pthread_t threadId;

public:
    static unsigned int max_pool;
    static unsigned int max_thread;
    static unsigned int next_range;

    bool using_pools;
    unsigned int start_pool;
    unsigned int range;
    unsigned int next_pool;

private:
    static void* threadStart(void* arg);
};

inline void Thread_Attr::setScope (const int &scope)
{
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_attr_setscope( &attr, scope );
#else
    pthread_attr_setscope( &attr, scope );
#endif
    //
    //  ignore Return code for now
    //
#ifdef _VTHREAD_DEBUG
    cout<<"Thread_Attr::setScope set: "<<scope<<" return: "<<iRetCode<<endl;
#endif
}

inline void Thread_Attr::setDetachState (const int &detachState)
{
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_attr_setdetachstate( &attr, detachState );
#else
    pthread_attr_setdetachstate( &attr, detachState );
#endif
    //
    //  ignore Return code for now
    //
#ifdef _VTHREAD_DEBUG
    cout<<"Thread_Attr::setDetachState set: "<<detachState<<" return: "<<iRetCode<<endl;
#endif
}

inline void Thread_Attr::setInheritSched (const int &inheritSched)
{
#ifdef _VTHREAD_DEBUG
   int iRetCode = pthread_attr_setinheritsched( &attr, inheritSched );
#else
   pthread_attr_setinheritsched( &attr, inheritSched );
#endif
   //
   //  ignore Return code for now
   //
#ifdef _VTHREAD_DEBUG
   cout<<"Thread_Attr::setInheritSched set: "<<inheritSched<<" return: "<<iRetCode<<endl;
#endif
}

inline void Thread_Attr::setSchedParam (const sched_param &param)
{
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_attr_setschedparam( &attr, &param );
#else
    pthread_attr_setschedparam( &attr, &param );
#endif
    //
    //  ignore Return code for now
    //
#ifdef _VTHREAD_DEBUG
    cout<<"Thread_Attr::setSchedParam return: "<<iRetCode<<endl;
#endif
}

inline void Thread_Attr::setSchedPolicy (const int &policy)
{
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_attr_setschedpolicy( &attr, policy );
#else
    pthread_attr_setschedpolicy( &attr, policy );
#endif
    //
    //  ignore Return code for now
    //
#ifdef _VTHREAD_DEBUG
    cout<<"Thread_Attr::setSchedPolicy set: "<<policy<<" return: "<<iRetCode<<endl;
#endif
}

inline void Thread_Attr::setStackSize (const size_t &stackSize)
{
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_attr_setstacksize( &attr, stackSize );
#else
    pthread_attr_setstacksize( &attr, stackSize );
#endif
    //
    //  ignore Return code for now
    //
#ifdef _VTHREAD_DEBUG
    cout<<"Thread_Attr::setStackSize set: "<<stackSize<<" return: "<<iRetCode<<endl;
#endif
}

inline int Thread_Attr::getScope () const
{
    int scope;
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_attr_getscope( &attr, &scope );
#else
    pthread_attr_getscope( &attr, &scope );
#endif
    //
    //  ignore Return code for now
    //
#ifdef _VTHREAD_DEBUG
    cout<<"Thread_Attr::getScope state: "<<scope<<" return: "<<iRetCode<<endl;
#endif

    return scope;
}

inline int Thread_Attr::getDetachState () const
{
    int detachState;
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_attr_getdetachstate( &attr, &detachState );
#else
    pthread_attr_getdetachstate( &attr, &detachState );
#endif
    //
    //  ignore Return code for now
    //
#ifdef _VTHREAD_DEBUG
    cout<<"Thread_Attr::getDetachState state: "<<detachState<<" return: "<<iRetCode<<endl;
#endif

    return detachState;
}

inline int Thread_Attr::getInheritSched () const
{
    int inheritSched;
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_attr_getinheritsched( &attr, &inheritSched );
#else
    pthread_attr_getinheritsched( &attr, &inheritSched );
#endif
    //
    //  ignore Return code for now
    //
#ifdef _VTHREAD_DEBUG
    cout<<"Thread_Attr::getInheritSched state: "<<inheritSched<<" return: "<<iRetCode<<endl;
#endif

    return inheritSched;
}

inline sched_param Thread_Attr::getSchedParam () const
{
    sched_param schedParam;
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_attr_getschedparam( &attr, &schedParam );
#else
    pthread_attr_getschedparam( &attr, &schedParam );
#endif
    //
    //  ignore Return code for now
    //
#ifdef _VTHREAD_DEBUG
    cout<<"Thread_Attr::getSchedParam return: "<<iRetCode<<endl;
#endif

    return schedParam;
}

inline int Thread_Attr::getSchedPolicy () const
{
    int schedPolicy;
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_attr_getschedpolicy( &attr, &schedPolicy );
#else
    pthread_attr_getschedpolicy( &attr, &schedPolicy );
#endif
    //
    //  ignore Return code for now
    //
#ifdef _VTHREAD_DEBUG
    cout<<"Thread_Attr::getSchedPolicy state: "<<schedPolicy<<" return: "<<iRetCode<<endl;
#endif

    return schedPolicy;
}

inline size_t Thread_Attr::getStackSize () const
{
    size_t stackSize;
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_attr_getstacksize( &attr, &stackSize );
#else
    pthread_attr_getstacksize( &attr, &stackSize );
#endif
    //
    //  ignore Return code for now
    //
#ifdef _VTHREAD_DEBUG
    cout<<"Thread_Attr::getStackSize state: "<<stackSize<<" return: "<<iRetCode<<endl;
#endif

    return stackSize;
}

inline const pthread_attr_t Thread_Attr::get_attr () const
{
    return attr;
}

inline const pthread_t Thread::get_threadId () const
{
    return threadId;
}


#endif /* SOE_SOE_SOE_MEMPOOLS_INCLUDE_THREAD_HPP_ */
