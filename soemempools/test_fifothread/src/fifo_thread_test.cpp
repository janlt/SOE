/*
 * fifo_thread_test.cpp
 *
 *  Created on: Mar 2, 2017
 *      Author: Jan Lisowiec
 */


#include <iostream>
using namespace std;
#include <stdlib.h>
#include <unistd.h>

#include  "thread.hpp"
#include  "threadfifoext.hpp"

template <class T>
class MyThread: public Thread
{
public:
    MyThread(unsigned long l_c = -1, unsigned long us_v = 0, bool _debug = false);
    virtual ~MyThread();
    virtual void start();

    Thread_FIFO_t<T>   in_fifo;
    Thread_FIFO_t<T>   out_fifo;

    unsigned long enqueue_counter;
    unsigned long dequeue_counter;
    unsigned long loop_counter;
    unsigned long usleep_val;

    bool          debug;
};

template <class T>
MyThread<T>::MyThread(unsigned long l_c, unsigned long us_v, bool _debug)
     :enqueue_counter(0),
      dequeue_counter(0),
      loop_counter(l_c),
      usleep_val(us_v),
      debug(_debug)
{}

template <class T>
MyThread<T>::~MyThread()
{}

template <class T>
void MyThread<T>::start()
{
    cout << "TestThread running" << endl;
    for ( unsigned long i = 0 ; i < loop_counter ; i++ ) {
        if ( usleep_val ) {
            usleep(usleep_val);
        }

        T *obj = in_fifo.receive();
        if ( debug == true ) cout << "th_id: " << this->threadId << " loop: " << i << " val: " << obj->val << endl << flush;

        if ( !(i%0x10000) ) {
            cout << "th_id: " << this->threadId << " loop: " << i << " val: " << obj->val << endl << flush;
        }
        dequeue_counter++;
        obj->val++;
        out_fifo.send(obj);
        enqueue_counter++;
    }
}

//=======================================================
class Contents
{
public:
    unsigned long    val;
    static unsigned long val_counter;

    Contents();
    ~Contents();
};

unsigned long Contents::val_counter = 0;

Contents::Contents()
    :val(val_counter++)
{}

Contents::~Contents()
{}

//=======================================================
const int ONE_SHOT = 10000000;
const int MAX_TEST_THREADS = 32;

int main(int argc, char **argv)
{
    int numThreads = 8;
    unsigned long loop_counter = -1;   // forever
    unsigned long total_enqueues = 0;
    unsigned long total_dequeues = 0;
    int oneShot = ONE_SHOT;
    unsigned long usleepInt = 0;
    MyThread<Contents> *threads[MAX_TEST_THREADS];

    if (argc == 2)
        oneShot = min(ONE_SHOT, atoi(argv[1]));
    else if (argc == 3) {
        oneShot = min(ONE_SHOT, atoi(argv[1]));
        usleepInt = atoi(argv[2]);
    } else if (argc == 4) {
        oneShot = min(ONE_SHOT, atoi(argv[1]));
        usleepInt = atoi(argv[2]);
        numThreads = atoi(argv[3]);
    } else if (argc == 5) {
        oneShot = min(ONE_SHOT, atoi(argv[1]));
        usleepInt = atoi(argv[2]);
        numThreads = atoi(argv[3]);
        loop_counter = static_cast<unsigned long>(atoi(argv[4]));
    }
    numThreads = numThreads > 32 ? 32 : numThreads;

    // create threads
    for ( int i = 0 ; i < numThreads ; i++ ) {
        threads[i] = new MyThread<Contents>(loop_counter, usleepInt, false);
        threads[i]->run();
    }

    Contents *obj = new Contents;
    for ( unsigned long l = 0 ; l < loop_counter ; l++ ) {
        for ( int i = 0 ; i < numThreads ; i++ ) {
            threads[i]->in_fifo.send(obj);
            threads[i]->out_fifo.receive();
        }
    }

    for ( int i = 0 ; i < numThreads ; i++ ) {
        threads[i]->join();
        total_enqueues += threads[i]->enqueue_counter;
        total_dequeues += threads[i]->dequeue_counter;
    }

    cout << "total_enqueues: " << total_enqueues << " total_dequeues: " << total_dequeues << endl << flush;

    return 0;
}

