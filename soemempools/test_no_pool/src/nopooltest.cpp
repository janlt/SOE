/*
 * nopooltest.cpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#include <iostream>
using namespace std;
#include <unistd.h>
#include "mmdefs.hpp"
#include "thread.hpp"

class MyThread: public Thread
{
public:
    MyThread(unsigned long l_c = -1);
    virtual ~MyThread();
    virtual void start();

    unsigned long allocs_counter;
    unsigned long delets_counter;
    unsigned long loop_counter;
};

MyThread::MyThread(unsigned long l_c): allocs_counter(0), delets_counter(0), loop_counter(l_c) {}
MyThread::~MyThread() {}

class TestBase
{
private:
    int         a;
    long        b;
    int         c;
    long        d;
    std::string mem;

public:
    TestBase(int a_ = 1, long b_ = 2, int c_ = 3, long d_ = 4): a(a_), b(b_), c(c_), d(d_), mem("TEST")
    {
    }
    virtual ~TestBase()
    {
    }
    virtual void Nothing()
    {
    }
    void Finalize()
    {
        a = 1;
        b = 2;
        c = 3;
        d = 4;
        mem = "TEST";
    }
};

class TestA: public TestBase
{
private:
    char name[16];

public:
    virtual ~TestA() {
    }
    virtual void Nothing() {
    }

public:
    //TestA() { memset(name, 'A', sizeof(name)); }
    char * getName() {
        return name;
    }
};

class TestB: public TestBase {
private:
    char name[25];

public:
    virtual ~TestB() {
    }
    virtual void Nothing() {
    }

public:
    //TestB() { memset(name, 'A', sizeof(name)); }
    char * getName() {
        return name;
    }
};

class TestC: public TestBase
{
private:
    char name[93];

public:
   virtual ~TestC() {
    }
    virtual void Nothing() {
    }

public:
    //TestC() { memset(name, 'C', sizeof(name)); }
    char * getName() {
        return name;
    }
};

class TestD: public TestBase
{
private:
    char name[278];

public:
    virtual ~TestD() {
    }
    virtual void Nothing() {
    }

public:
    //TestD() { memset(name, 'D', sizeof(name)); }
    char * getName() {
        return name;
    }
};

class TestE: public TestBase
{
private:
    char name[1793];

public:
    virtual ~TestE() {
    }
    virtual void Nothing() {
    }

public:
    //TestE() { memset(name, 'E', sizeof(name)); }
    char * getName() {
        return name;
    }
};

extern int oneShot;
extern int usleepInt;

typedef TestBase* Testptr;

void MyThread::start()
{
    Testptr *atab1 = new Testptr[ONE_SHOT];
    Testptr *btab1 = new Testptr[ONE_SHOT];
    Testptr *ctab1 = new Testptr[ONE_SHOT];
    Testptr *dtab1 = new Testptr[ONE_SHOT];
    Testptr *etab1 = new Testptr[ONE_SHOT];

    cout << "######### starting a thread " << endl << flush;

    int aa, bb, cc, dd, ee;
    for ( unsigned long  cnt = 0 ; cnt < loop_counter ; cnt++ ) {
        cout << "######### get a thread shot " << cnt << endl << flush;

        aa = 0;
        bb = 0;
        cc = 0;
        dd = 0;
        ee = 0;
        for (int ii = 0; ii < oneShot; ii++) {
            for (int jj = 0; jj < 5; jj++) {
                int idx = jj % 5;
                switch (idx) {
                case 0:
                    if (aa < oneShot) {
                        atab1[aa++] = new TestA();
                        allocs_counter++;
                    }
                    break;
                case 1:
                    if (bb < oneShot) {
                        btab1[bb++] = new TestB();
                        allocs_counter++;
                    }
                    break;
                case 2:
                    if (cc < oneShot) {
                        ctab1[cc++] = new TestC();
                        allocs_counter++;
                    }
                    break;
                case 3:
                    if (dd < oneShot) {
                        dtab1[dd++] = new TestD();
                        allocs_counter++;
                    }
                    break;
                case 4:
                    if (ee < oneShot) {
                        etab1[ee++] = new TestE();
                        allocs_counter++;
                    }
                    break;
                }
            }
        }

        usleep(usleepInt);

        for (aa--; aa >= 0;) {
            delete static_cast<TestA*>(atab1[aa--]);
            delets_counter++;
        }
        for (bb--; bb >= 0;) {
            delete static_cast<TestB*>(btab1[bb--]);
            delets_counter++;
        }
        for (cc--; cc >= 0;) {
            delete static_cast<TestC*>(ctab1[cc--]);
            delets_counter++;
        }
        for (dd--; dd >= 0;) {
            delete static_cast<TestD*>(dtab1[dd--]);
            delets_counter++;
        }
        for (ee--; ee >= 0;) {
            delete static_cast<TestE*>(etab1[ee--]);
            delets_counter++;
        }

        usleep(usleepInt);
    }
}

#define MAX_TEST_THREADS 32

int main(int argc, char **argv)
{
    int numThreads = 1;
    unsigned long loopCounter = -1;
    unsigned long total_allocs = 0;
    unsigned long total_delets = 0;
    MyThread *threads[MAX_TEST_THREADS];

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
        loopCounter = static_cast<unsigned long>(atoi(argv[4]));
    }
    numThreads = numThreads > 32 ? 32 : numThreads;

    // create threads
    for ( int i = 0 ; i < numThreads ; i++ ) {
        threads[i] = new MyThread(loopCounter);
        threads[i]->run();
    }

    for ( int i = 0 ; i < numThreads ; i++ ) {
        threads[i]->join();
        total_allocs += threads[i]->allocs_counter;
        total_delets += threads[i]->delets_counter;
    }

    cout << "total_allocs: " << total_allocs << " total_delets: " << total_delets << endl << flush;

    return 0;
}
