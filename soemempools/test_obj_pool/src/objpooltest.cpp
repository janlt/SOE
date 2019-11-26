/*
 * objpooltest.cpp
 *
 *  Created on: Dec 21, 2016
 *      Author: Jan Lisowiec
 */

#include <iostream>
using namespace std;
#include <unistd.h>
#include "mmdefs.hpp"
#include "thread.hpp"
#include "generalpool.hpp"
using namespace MemMgmt;

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

MyThread::MyThread(unsigned long l_c): Thread(true), allocs_counter(0), delets_counter(0), loop_counter(l_c) {}
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
    static GeneralPool<TestA, ClassAlloc<TestA> > poolT;

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

GeneralPool<TestA, ClassAlloc<TestA> > TestA::poolT(10, "TestA");

class TestB: public TestBase {
private:
    char name[25];

public:
    static GeneralPool<TestB, ClassAlloc<TestB> > poolT;

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

GeneralPool<TestB, ClassAlloc<TestB> > TestB::poolT(11, "TestB");

class TestC: public TestBase
{
private:
    char name[93];

public:
    static GeneralPool<TestC, ClassAlloc<TestC> > poolT;

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

GeneralPool<TestC, ClassAlloc<TestC> > TestC::poolT(12, "TestC");

class TestD: public TestBase
{
private:
    char name[278];

public:
    static GeneralPool<TestD, ClassAlloc<TestD> > poolT;

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

GeneralPool<TestD, ClassAlloc<TestD> > TestD::poolT(13, "TestD");

class TestE: public TestBase
{
private:
    char name[1793];

public:
    static GeneralPool<TestE, ClassAlloc<TestE> > poolT;

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

GeneralPool<TestE, ClassAlloc<TestE> > TestE::poolT(14, "TestE");

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
    for (unsigned long  cnt = 0 ; cnt < loop_counter ; cnt++ ) {
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
                        atab1[aa++] = TestA::poolT.get(this->get_range_pool());
                        allocs_counter++;
                    }
                    break;
                case 1:
                    if (bb < oneShot) {
                        btab1[bb++] = TestB::poolT.get(this->get_range_pool());
                        allocs_counter++;
                    }
                    break;
                case 2:
                    if (cc < oneShot) {
                        ctab1[cc++] = TestC::poolT.get(this->get_range_pool());
                        allocs_counter++;
                    }
                    break;
                case 3:
                    if (dd < oneShot) {
                        dtab1[dd++] = TestD::poolT.get(this->get_range_pool());
                        allocs_counter++;
                    }
                    break;
                case 4:
                    if (ee < oneShot) {
                        etab1[ee++] = TestE::poolT.get(this->get_range_pool());
                        allocs_counter++;
                    }
                    break;
                }
            }
        }

        usleep(usleepInt);

        for (aa--; aa >= 0;) {
            TestA::poolT.put(static_cast<TestA*>(atab1[aa--]), this->get_range_pool());
            delets_counter++;
        }
        for (bb--; bb >= 0;) {
            TestB::poolT.put(static_cast<TestB*>(btab1[bb--]), this->get_range_pool());
            delets_counter++;
        }
        for (cc--; cc >= 0;) {
            TestC::poolT.put(static_cast<TestC*>(ctab1[cc--]), this->get_range_pool());
            delets_counter++;
        }
        for (dd--; dd >= 0;) {
            TestD::poolT.put(static_cast<TestD*>(dtab1[dd--]), this->get_range_pool());
            delets_counter++;
        }
        for (ee--; ee >= 0;) {
            TestE::poolT.put(static_cast<TestE*>(etab1[ee--]), this->get_range_pool());
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
        loopCounter = static_cast<unsigned long>(atoi(argv[4]));
        oneShot = min(ONE_SHOT, atoi(argv[1]));
        usleepInt = atoi(argv[2]);
        numThreads = atoi(argv[3]);
    }
    numThreads = numThreads > 32 ? 32 : numThreads;

    Thread::max_pool = TestA::poolT.getPoolSize();
    Thread::max_thread = numThreads;

    // create threads
    for ( int i = 0 ; i < numThreads ; i++ ) {
        threads[i] = new MyThread(loopCounter);
        threads[i]->set_pool_range(Thread::max_pool/Thread::max_thread);
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
