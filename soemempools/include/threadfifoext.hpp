/*
 * threadfifoext.hpp   (C) Copyright Jan Lisowiec, 2003-2017
 *
 */

/*
 * threadfifoext.hpp
 *
 *  Created on: Dec 22, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_MEMPOOLS_INCLUDE_THREADFIFOEXT_HPP_
#define SOE_SOE_SOE_MEMPOOLS_INCLUDE_THREADFIFOEXT_HPP_


#include <deque>
#include <queue>

#ifdef _VTHREAD_COMM_DEBUG
#include <iostream>
using namespace std;
#endif

#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

template<class T>
class Thread_FIFO_t
{
public:
    enum {
        BLOCK = 1, NOBLOCK = 0, PEEK = 1, NOPEEK = 0
    };

public:
    Thread_FIFO_t();
    virtual ~Thread_FIFO_t();
    void send(const T* aMsg);

    //    This call gets a pointer from the mutexed list.
    //    It can just "peek"
    //    It can also "block"
    //
    //    returns: NULL pointer is an error
    T* receive(const short peek = 0, const short block = 1);

    //    This call gets a pointer from the mutexed list.
    //    It can just "peek"
    //    It can also "block" for timespec
    //
    //    Supply timespec value
    //
    //    returns: NULL pointer is an error
    T* receiveWithTimeout(const timespec abstime, const short peek = 0,
            const short block = 1);

    //    Just peek at list...may block if set
    T* peek(const short block = 1);

    //    Argument in pointer to global Thread_FIFO.
    //
    //    used in conjuction with pthread_cleanup_ routines.
    //
    //    e.g. pthread_cleanup_push( Thread_FIFO::cleanup, arg );
    //    :
    //    pthread_exit( status );
    //    pthread_cleanup_pop(1);
    static void cleanup(void* arg);

    //    number of elements on the queue
    unsigned long size() const;

    // notify all waiters
    void reset();

private:
    Thread_FIFO_t(const Thread_FIFO_t<T> &right);
    Thread_FIFO_t<T> & operator=(const Thread_FIFO_t<T> &right);

private:
    std::queue<T*, std::deque<T*> >  data;
    boost::mutex                     mtx;
    boost::condition_variable        cond;
};

template<class T>
inline Thread_FIFO_t<T>::Thread_FIFO_t()
{
}

template<class T>
inline Thread_FIFO_t<T>::~Thread_FIFO_t()
{
}

template<class T>
inline void Thread_FIFO_t<T>::send(const T* aMsg)
{
    boost::mutex::scoped_lock lock(mtx);

    data.push(const_cast<T*>(aMsg));

#ifdef _VTHREAD_COMM_DEBUG
    cout<<"Thread_FIFO_t::send Added message to data FIFO"<<endl;
#endif

    cond.notify_all();
}

template<class T>
inline void Thread_FIFO_t<T>::reset()
{
    boost::mutex::scoped_lock lock(mtx);

#ifdef _VTHREAD_COMM_DEBUG
    cout<<"Thread_FIFO_t::reset Reset FIFO"<<endl;
#endif

    cond.notify_all();
}

template<class T>
inline T* Thread_FIFO_t<T>::receive(const short peek, const short block)
{
    T* tmpPtr = NULL;

    boost::mutex::scoped_lock lock(mtx);

    if (data.empty()) {
#ifdef _VTHREAD_COMM_DEBUG
        cout<<"Thread_FIFO_t::receive No data on FIFO"<<endl;
#endif
        if (block == BLOCK) {
            do {
#ifdef _VTHREAD_COMM_DEBUG
                cout<<"Thread_FIFO_t::receive blocked waiting for data on FIFO"<<endl;
#endif
                try {
                    cond.wait(lock);
                } catch (const boost::thread_resource_error &e) {
#ifdef _VTHREAD_COMM_DEBUG
                    cout<<"Thread_FIFO_t::receive problems with cond wait"<<" e: "<<e<<endl;
#endif
                }
            } while (data.empty());  // in case of spurious signals
        }
    }

    if (!data.empty()) {
        tmpPtr = data.front();
#ifdef _VTHREAD_COMM_DEBUG
        cout<<"Thread_FIFO_t::receive found front message"<<endl;
#endif

        if (peek == NOPEEK) {
            data.pop();
#ifdef _VTHREAD_COMM_DEBUG
            cout<<"Thread_FIFO_t::receive removed front message"<<endl;
#endif
        }
    }

    return tmpPtr;
}

template<class T>
inline T* Thread_FIFO_t<T>::receiveWithTimeout(const timespec abstime,
        const short peek, const short block)
{
    T* tmpPtr = NULL;

    boost::mutex::scoped_lock lock(mtx);

    if (data.empty()) {
#ifdef _VTHREAD_COMM_DEBUG
        cout<<"Thread_FIFO_t::receiveWithTimeout No data on FIFO"<<endl;
#endif
        if (block == BLOCK) {
            bool go_on = false;
            do {
#ifdef _VTHREAD_COMM_DEBUG
                cout<<"Thread_FIFO_t::receiveWithTimeout blocked waiting for data on FIFO"<<endl;
#endif
                int retCode = cond.wait(lock, abstime);
                if (retCode == ETIMEDOUT) {
                    go_on = true;
#ifdef _VTHREAD_COMM_DEBUG
                    cout<<"Thread_FIFO_t::receiveWithTimeout timedout"<<endl;
#endif
                } else if (retCode != 0) {
                    go_on = false;
#ifdef _VTHREAD_COMM_DEBUG
                    cout<<"Thread_FIFO_t::receiveWithTimeout problems with timed wait"<<endl;
#endif
                } else {
                    if (!data.empty()) { // in case of spurious signals
                        go_on = true;
                    }
                }
            } while (!go_on);
        }
    }

    if (!data.empty()) {
        tmpPtr = data.front();
#ifdef _VTHREAD_COMM_DEBUG
        cout<<"Thread_FIFO_t::receiveWithTimeout found front message"<<endl;
#endif

        if (peek == NOPEEK) {
            data.pop();
#ifdef _VTHREAD_COMM_DEBUG
            cout<<"Thread_FIFO_t::receiveWithTimeout removed front message"<<endl;
#endif
        }
    }

    return tmpPtr;
}

template<class T>
inline T* Thread_FIFO_t<T>::peek(const short block)
{
    return (Thread_FIFO_t<T>::receive(PEEK, block));
}

template<class T>
inline void Thread_FIFO_t<T>::cleanup(void* arg)
{
    Thread_FIFO_t* me = (Thread_FIFO_t*) arg;
    (void )me->mtx.try_lock();
    me->mtx.unlock();
}

template<class T>
inline unsigned long Thread_FIFO_t<T>::size() const
{
    return data.size();
}



#endif /* SOE_SOE_SOE_MEMPOOLS_INCLUDE_THREADFIFOEXT_HPP_ */
