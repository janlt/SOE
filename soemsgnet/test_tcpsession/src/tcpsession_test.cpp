/*
 * tcpsession_test.cpp
 *
 *  Created on: Jan 11, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <stdexcept>
using namespace std;
#include <poll.h>
#include <sys/epoll.h>
#include <netdb.h>

#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>

#include "thread.hpp"
#include "threadfifoext.hpp"
#include "classalloc.hpp"
#include "generalpool.hpp"
#include "dbsrvmarshallable.hpp"
#include "dbsrvmsgbuffer.hpp"
#include "dbsrvmsgs.hpp"
#include "dbsrvmcasttypes.hpp"
#include "dbsrvmsgadmtypes.hpp"
#include "dbsrvmsgfactory.hpp"
#include "dbsrvmsgadmfactory.hpp"

#include "dbsrviovmsgsbase.hpp"
#include "dbsrviovmsgs.hpp"
#include "dbsrvmsgiovfactory.hpp"
#include "dbsrvmsgiovtypes.hpp"

#include "msgnetadkeys.hpp"
#include "msgnetadvertisers.hpp"
#include "msgnetsolicitors.hpp"
#include "msgnetaddress.hpp"
#include "msgnetipv4address.hpp"
#include "msgnetunixaddress.hpp"
#include "msgnetmmaddress.hpp"
#include "msgnetpoint.hpp"
#include "msgnetsocket.hpp"
#include "msgnetunixsocket.hpp"
#include "msgnetmmpoint.hpp"
#include "msgnettcpsocket.hpp"
#include "msgnetudpsocket.hpp"
#include "msgnetioevent.hpp"
#include "msgnetsession.hpp"
#include "msgnetprocessor.hpp"
#include "msgnetlistener.hpp"
#include "msgneteventlistener.hpp"
#include "msgnetipv4listener.hpp"
#include "msgnetipv4mcastlistener.hpp"
#include "msgnetipv4tcpsession.hpp"
#include "msgnetipv4tcpclisession.hpp"
#include "msgnetipv4tcpsrvsession.hpp"
#include "msgneteventsession.hpp"
#include "msgneteventsrvsession.hpp"

const int ONE_SHOT = 10000000;
int oneShot = ONE_SHOT;
int usleepInt = 200000;

using namespace MemMgmt;
using namespace MetaMsgs;
using namespace MetaNet;

typedef enum _eSessionType
{
    eSessionSrv = 0,
    eSessionCli = 1
} eSessionType;

class MyThread: public Thread
{
public:
    MyThread(int ind, unsigned long l_c = -1, eSessionType typ = eSessionSrv);
    virtual ~MyThread();
    virtual void start();
    void check(uint32_t show);

    eSessionType  type;
    unsigned long regis_counter;
    unsigned long deregis_counter;
    unsigned long loop_counter;
    int           index;           // thread 0 cleans up entries, threads > 0 add entries
    uint32_t      spread[16];
    uint32_t      regisses;
    uint32_t      last_regisses;
};

MyThread::MyThread(int ind, unsigned long l_c, eSessionType typ)
    :Thread(true),
    type(typ),
    regis_counter(0),
    deregis_counter(0),
    loop_counter(l_c),
    index(ind),
    regisses(0),
    last_regisses(0) {}

MyThread::~MyThread() {}


extern int oneShot;
extern int usleepInt;

void MyThread::start()
{
    cout << "######### starting a thread " << endl << flush;

    for (unsigned long  cnt = 0 ; cnt < loop_counter ; cnt++ ) {
        if ( index ) {
            cout << "######### get a thread shot " << cnt << endl << flush;
            int prog = 1;
            for (int ii = 0; ii < oneShot; ii++) {
                TcpSocket ss;
                int suck = ss.CreateStream();
                if ( suck < 0 ) {
                    //cout << "soc errno: " << errno << endl << flush;
                    usleep(usleepInt*prog);
                    prog += 1;
                    if ( prog > 64 ) {
                        prog = 1;
                    }
                    continue;
                }
                prog = 1;
                usleep(usleepInt);
                try {
                    IPv4TCPSession *ptr;
                    if ( type == eSessionSrv ) {
                        ptr = new IPv4TCPSrvSession(ss, true);
                    } else {
                        ptr = new IPv4TCPCliSession(ss, true);
                    }
                    ptr->initialise();
                    ptr->registerSession();
                } catch (const std::runtime_error &ex) {
                    cout << "ex cnt: " << cnt << " ii: " << ii << " what: " << ex.what() << endl << flush;
                }
            }
        } else {
            cout << "@@@@@@@@@ get a thread shot " << cnt << endl << flush;
            int regissesUsleepInt = 100;
            int ii;
            for (int jj = 0 ; jj < oneShot ; jj++) {
                ii = 100;
                for (uint32_t kk = 0 ; kk < MetaNet::session_max ; kk++) {
                    if ( IPv4TCPSession::sessions[kk].fd && IPv4TCPSession::sessions[kk].sess ) {
                        try {
                            IPv4TCPSession *ptr = IPv4TCPSession::sessions[kk].sess;
                            IPv4TCPSession::sessions[kk].sess->deinitialise();
                            IPv4TCPSession::sessions[kk].sess->deregisterSession();
                            delete ptr;
                            ii--;
                        } catch (const std::runtime_error &ex) {
                            cout << "ex cnt: " << cnt << " ii: " << ii << " what: " << ex.what() << endl << flush;
                        }
                    }
                    if ( !(kk%100) ) {
                        check(kk%200);
                    }
                    if ( ii <= 0 ) {
                        break;
                    }
                }
                check(0);
                if ( regisses > last_regisses ) {
                    regissesUsleepInt -= 10;
                } else {
                    regissesUsleepInt += 10;
                }
                if ( regissesUsleepInt < 100 ) {
                    regissesUsleepInt = 100;
                }
                if ( regissesUsleepInt > 100000 ) {
                    regissesUsleepInt = 100000;
                }
                if ( regissesUsleepInt ) {
                    usleep(regissesUsleepInt);
                }
            }
        }
    }
}

void MyThread::check(uint32_t show)
{
    last_regisses = regisses;
    regisses = 0;
    ::memset(spread, '\0', sizeof(spread));
    int range = MetaNet::session_max/16;
    for (uint32_t kk = 0 ; kk < MetaNet::session_max ; kk++) {
        int s_idx = kk/range;
        if ( IPv4TCPSession::sessions[kk].fd && IPv4TCPSession::sessions[kk].sess ) {
            regisses++;
            if ( s_idx < 16 ) {
                spread[s_idx]++;
            }
        }
    }
    if ( !show ) {
        cout << endl << flush;
        cout << "---> regisses: " << regisses << " spread[ ";
        for ( int ll = 0 ; ll < 16 ; ll++ ) {
            cout << spread[ll] << " ";
        }
        cout << " ] open_c: " << IPv4TCPSession::open_counter << " close_c: " << IPv4TCPSession::close_counter << endl << flush;
    }
}

#define MAX_TEST_THREADS 32

int main(int argc, char **argv)
{
    int numThreads = 1;
    unsigned long loopCounter = -1;
    unsigned long total_regis = 0;
    unsigned long total_deregis = 0;
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
    numThreads++;
    numThreads = numThreads > 32 ? 32 : numThreads;

    Thread::max_thread = numThreads;

    // create threads + 1
    for ( int i = 0 ; i < numThreads ; i++ ) {
        threads[i] = new MyThread(i, loopCounter, eSessionSrv);
        threads[i]->run();
    }

    for ( int i = 0 ; i < numThreads ; i++ ) {
        threads[i]->join();
        total_regis += threads[i]->regis_counter;
        total_deregis += threads[i]->deregis_counter;
    }

    cout << "total_registrations: " << total_regis << " total_deregistrations: " << total_deregis << endl << flush;

    return 0;
}
