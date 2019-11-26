/*
 * threadfifo.cpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * threadfifo.cpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#include <iostream>

using namespace std;

#include "threadfifo.hpp"

Thread_FIFO::Thread_FIFO()
{}

Thread_FIFO::~Thread_FIFO()
{}

void Thread_FIFO::send (const void* aMsg)
{
    boost::mutex::scoped_lock lock(mtx);

    data.push( (void*)aMsg );

#ifdef _VTHREAD_COMM_DEBUG
    cout<<"Thread_FIFO::send Added message to data FIFO"<<endl;
#endif

    cond.notify_all();
}

void* Thread_FIFO::receive (const short peek, const short block)
{
    void* tmpPtr = NULL;

    boost::mutex::scoped_lock lock(mtx);

    if( data.empty() ) {
#ifdef _VTHREAD_COMM_DEBUG
        cout<<"Thread_FIFO::receive No data on FIFO"<<endl;
#endif
        if ( block == BLOCK ) {
            do {
#ifdef _VTHREAD_COMM_DEBUG
                cout<<"Thread_FIFO::receive blocked waiting for data on FIFO"<<endl;
#endif
                try {
                    cond.wait(lock);
                } catch (const boost::thread_resource_error &e) {
#ifdef _VTHREAD_COMM_DEBUG
                    cout<<"Thread_FIFO::receive problems with cond wait"<<" e: " <<e<<endl;
#endif
                }
            }
         while( data.empty() );  // in case of spurious signals
        }
    }

    if( !data.empty() ) {
        tmpPtr = data.front();
#ifdef _VTHREAD_COMM_DEBUG
        cout<<"Thread_FIFO::receive found front message"<<endl;
#endif
      
        if ( peek == NOPEEK ) {
            data.pop();
#ifdef _VTHREAD_COMM_DEBUG
            cout<<"Thread_FIFO::receive removed front message"<<endl;
#endif
        }
    }

    return tmpPtr;
}

void* Thread_FIFO::peek (const short block)
{
    return( Thread_FIFO::receive( PEEK, block ) );
}

void Thread_FIFO::cleanup (void* arg)
{
    Thread_FIFO* me = (Thread_FIFO*)arg;
    (void )me->mtx.try_lock();
    me->mtx.unlock();
}
