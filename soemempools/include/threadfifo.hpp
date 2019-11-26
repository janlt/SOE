/*
 * threadfifo.hpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * threadfifo.hpp
 *
 *  Created on: Dec 22, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MEMPOOLS_INCLUDE_THREADFIFO_HPP_
#define SOE_SOE_SOE_MEMPOOLS_INCLUDE_THREADFIFO_HPP_


#include <deque>
#include <queue>

#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

//  This is an inter-thread (intra-process) communications
//  link.
//  It uses a FIFO of pointers to pass data.  The
//  instantiation of this class should probably be global.

class Thread_FIFO {
public:
    enum {
        BLOCK = 1, NOBLOCK = 0, PEEK = 1, NOPEEK = 0
    };

public:
    Thread_FIFO();
    virtual ~Thread_FIFO();

    // This call adds a pointer to a mutexed list.
    void send(const void* aMsg);

    // This call gets a pointer from the mutexed list.
    // It can just "peek"
    // It can also "block"
    //
    // returns: NULL pointer is an error
    void* receive(const short peek = 0, const short block = 1);

    // Just peek at list...may block if set
    void* peek(const short block = 1);

    // Argument in pointer to global Thread_FIFO.
    //
    // used in conjuction with pthread_cleanup_ routines.
    //
    // e.g. pthread_cleanup_push( Thread_FIFO::cleanup, arg );
    // :
    // pthread_exit( status );
    // pthread_cleanup_pop(1);
    static void cleanup(void* arg);

private:
    Thread_FIFO(const Thread_FIFO &right);
    const Thread_FIFO & operator=(const Thread_FIFO &right);

private:
    std::queue<void*, std::deque<void*> > data;
    boost::condition_variable             cond;
    boost::mutex                          mtx;
};


#endif /* SOE_SOE_SOE_MEMPOOLS_INCLUDE_THREADFIFO_HPP_ */
