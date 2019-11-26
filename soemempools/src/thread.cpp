/*
 * thread.cpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * thread.cpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#ifdef _VTHREAD_DEBUG
#include <iostream>
using namespace std;
#endif

#include "thread.hpp"

unsigned int Thread::max_thread = 0;
unsigned int Thread::max_pool = 0;
unsigned int Thread::next_range = 0;

Thread::Thread(bool using_pools_):
    using_pools(using_pools_),
    start_pool(-1),  // so it fails without setting this separately
    range(0),
    next_pool(0)
{}

Thread::~Thread()
{}

int Thread::operator==(const Thread &right) const
{
   return( pthread_equal( threadId, right.threadId ) );
}

int Thread::operator!=(const Thread &right) const
{
   return( !pthread_equal( threadId, right.threadId ) );
}

void Thread::run ()
{
#ifdef _VTHREAD_DEBUG
    cerr<<"Thread::run() time to create the thread"<<endl;
#endif
    //
    //  Create The thread
    //
    pthread_attr_t tmpAttr = thr_attr.get_attr();
    pthread_create( &threadId, &tmpAttr, Thread::threadStart, this );
    //
    //  ignore any return codes for now
    //
}

void* Thread::threadStart (void* arg)
{
#ifdef _VTHREAD_DEBUG
    cerr<<"Thread::threadStart() is starting"<<endl;
#endif

    ((Thread*)arg)->start();
 
    return NULL;
}

void Thread::join ()
{
    //
    //  Join on this thread
    //
    void* tmpStat;
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_join( threadId, &tmpStat );
#else
    pthread_join( threadId, &tmpStat );
#endif
    //
    //  probably should check for errors here
    //
#ifdef _VTHREAD_DEBUG
    cerr<<"Thread::join() returned from join "<<iRetCode<<endl;
#endif
}

void Thread::cancel ()
{
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_cancel( threadId );
#else
    pthread_cancel( threadId );
#endif
    //
    //  probably should check for errors here
    //
#ifdef _VTHREAD_DEBUG
    cerr<<"Thread::cancel() returned from cancel "<<iRetCode<<endl;
#endif
}

void Thread::detach ()
{
    pthread_detach( threadId );
    //
    //  probably should check for errors here
    //
#ifdef _VTHREAD_DEBUG
    cerr<<"Thread::detach() returned from detach "<<endl;
#endif
}

Thread_Attr::Thread_Attr()
{
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_attr_init( &attr );
#else
    pthread_attr_init( &attr );
#endif
    //
    //  Ignore errors for now
    //
#ifdef _VTHREAD_DEBUG
    cout<<"Thread_Attr::Thread_Attr created new attribute: "<<iRetCode<<endl;
#endif
}

Thread_Attr::Thread_Attr(const Thread_Attr &right)
    :attr( right.attr )
{}

Thread_Attr::~Thread_Attr()
{
#ifdef _VTHREAD_DEBUG
    int iRetCode = pthread_attr_destroy( &attr );
#else
    pthread_attr_destroy( &attr );
#endif
    //
    //  Return code should always be valid
    //
#ifdef _VTHREAD_DEBUG
    cout<<"Thread_Attr::~Thread_Attr destroyed attribute: "<<iRetCode<<endl;
#endif
}

const Thread_Attr & Thread_Attr::operator=(const Thread_Attr &right)
{
    if ( this == &right ) return *this;

    attr = right.attr;

    return *this;
}

void Thread::set_pool_range(unsigned int range_)
{
    range = range_;
    next_pool = 0;

    if ( using_pools == true ) {
        //assert(Thread::max_pool && Thread::max_thread);
        start_pool = Thread::next_range;
        Thread::next_range += range;
    }
    //assert(Thread::next_range <= Thread::max_pool);
}
