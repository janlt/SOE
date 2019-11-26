/**
 * @file    main.hpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe Session API
 *
 *
 * @section Copyright
 *
 *   Copyright Notice:
 *
 *    Copyright 2014-2019 Jan Lisowiec jlisowiec@gmail.com,
 *                   
 *
 *    This product is free software; you can redistribute it and/or,
 *    modify it under the terms of the GNU Library General Public
 *    License as published by the Free Software Foundation; either
 *    version 2 of the License, or (at your option) any later version.
 *    This software is distributed in the hope that it will be useful.
 *
 *
 * @section Licence
 *
 *   GPL 2 or later
 *
 *
 * @section Description
 *
 *   soe_soe
 *
 * @section Source
 *
 *    
 *    
 *
 */

/*
 * main.cpp
 *
 *  Created on: Jan 5, 2017
 *      Author: Jan Lisowiec
 */

#include <dirent.h>

#include <vector>
#include <map>
#include <fstream>
#include <string>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <chrono>
#include <boost/random.hpp>

extern "C" {
#include "soeutil.h"
}

#include "soelogger.hpp"
using namespace SoeLogger;

#include "soe_bind_api.hpp"
#include "rcsdbargdefs.hpp"
#include "rcsdbfacadeapi.hpp"
#include "rcsdbproxyfacadebase.hpp"

#include <errno.h>
#include <pwd.h>
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
#include <getopt.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <stdexcept>
using namespace std;
#include <poll.h>
#include <sys/epoll.h>
#include <netdb.h>

#include <boost/thread.hpp>
#include <boost/function.hpp>

#include "soelogger.hpp"

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
#include "msgnetioevent.hpp"
#include "msgnettcpsocket.hpp"
#include "msgnetudpsocket.hpp"
#include "msgnetunixsocket.hpp"
#include "msgnetlistener.hpp"
#include "msgnetprocessor.hpp"
#include "msgnetipv4listener.hpp"
#include "msgnetipv4mcastlistener.hpp"
#include "msgnetlistener.hpp"
#include "msgneteventlistener.hpp"
#include "msgnetsession.hpp"
#include "msgnetipv4tcpsession.hpp"
#include "msgnetipv4tcpclisession.hpp"
#include "msgneteventsession.hpp"
#include "msgneteventclisession.hpp"
#include "msgneteventsession.hpp"
#include "msgneteventsrvsession.hpp"
#include "msgnetasynceventsrvsession.hpp"

using namespace MetaMsgs;
using namespace MetaNet;
using namespace RcsdbFacade;

#include <rocksdb/cache.h>
#include <rocksdb/compaction_filter.h>
#include <rocksdb/slice.h>
#include <rocksdb/table.h>
#include <rocksdb/utilities/options_util.h>
#include <rocksdb/db.h>
#include <rocksdb/utilities/backupable_db.h>
#include <rocksdb/merge_operator.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/utilities/checkpoint.h>

using namespace rocksdb;

#include "rcsdbargdefs.hpp"
#include "rcsdbidgener.hpp"
#include "rcsdbfacadeapi.hpp"
#include "rcsdbfacadebase.hpp"
#include "rcsdbconfigbase.hpp"
#include "rcsdbconfig.hpp"
#include "rcsdbfilters.hpp"

#include "metadbsrvmcastprocessor.hpp"
#include "metadbsrvadmprocessor.hpp"
#include "metadbsrveventprocessor.hpp"

#include "metadbsrvrcsdbname.hpp"
#include "metadbsrvrcsdbstoreiterator.hpp"
#include "metadbsrvrangecontext.hpp"
#include "metadbsrvrcsdbstore.hpp"
#include "metadbsrvrcsdbspace.hpp"
#include "metadbsrvrcsdbcluster.hpp"
#include "metadbsrvrcsdbstorebackup.hpp"
#include "metadbsrvrcsdbsnapshot.hpp"
#include "metadbsrvaccessors1.hpp"
#include "metadbsrvfacadeapi.hpp"
#include "metadbsrvasynciosession.hpp"
#include "metadbsrvdefaultstore.hpp"
#include "metadbsrvfacade1.hpp"

using namespace std;
using namespace MetaNet;
using namespace Metadbsrv;

/*
 * @brief usage()
 */
static void usage(SoeLogger::Logger &logger, const char *prog)
{
    fprintf(stderr,
        "Usage %s [options]\n"
        "  -d, --debug             debug on\n"
        "  -m, --msg-debug         msg debug on\n"
        "  -u, --user_name         username\n"
        "  -h, --help              Print this help\n", prog);
    logger.Clear(CLEAR_DEFAULT_ARGS) << "Usage " << prog <<
        " [options]\n"
        "  -d, --debug             debug on\n"
        "  -m, --msg-debug         msg debug on\n"
        "  -u, --user_name         username\n"
        "  -h, --help              Print this help\n" << SoeLogger::LogError() << endl;
}

static int metasrv_getopt(SoeLogger::Logger &logger, int argc, char *argv[],
    bool *debug,
    bool *msg_debug,
    char user_name[])
{
    int c = 0;
    int ret = 0;

    while (1) {
        static char short_options[] = "u:dm";
        static struct option long_options[] = {
            {"debug",           1, 0, 'd'},
            {"msg-debug",       1, 0, 'm'},
            {"user-name",       1, 0, 'u'},
            {0, 0, 0, 0}
        };

        if ( (c = getopt_long(argc, argv, short_options, long_options, NULL)) == -1 ) {
            break;
        }

        switch ( c ) {
        case 'd':
            *debug = true;
            break;

        case 'm':
            *msg_debug = true;
            break;

        case 'u':
            ::strcpy(user_name, optarg);
            break;

        case 'h':
        case '?':
            usage(logger, argv[0]);
            ret = 0;
            break;
        default:
            ret = -1;
            break;
        }

        if ( ret ) {
            break;
        }
    }

    return ret;
}

static int SwitchUid(SoeLogger::Logger &logger, const char *username)
{
    int ret;
    size_t buffer_len;
    char pwd_buffer[1024];

    struct passwd pwd;
    struct passwd *pwd_res = 0;
    memset(&pwd, '\0', sizeof(struct passwd));

    buffer_len = sysconf(_SC_GETPW_R_SIZE_MAX) * sizeof(char);

    ret = getpwnam_r(username, &pwd, pwd_buffer, buffer_len, &pwd_res);
    if ( !pwd_res || ret ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "getpwnam_r failed to find requested entry" << SoeLogger::LogError() << endl;
        return -3;
    }
    //printf("uid: %d\n", pwd.pw_uid);
    //printf("gid: %d\n", pwd.pw_gid);

    if ( getuid() == pwd_res->pw_uid ) {
        return 0;
    }

    ret = setegid(pwd_res->pw_gid);
    if ( ret ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "can't setegid() errno(" << errno << ")" << SoeLogger::LogError() << endl;
        return -4;
    }
    ret = seteuid(pwd_res->pw_uid);
    if ( ret ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "can't setegid() errno(" << errno << ")" << SoeLogger::LogError() << endl;
        return -4;
    }

    return 0;
}

int main(int argc, char **argv)
{
    char user_name[256];

    IPv4Address addr;
    addr.addr.sin_family = AF_INET;
    addr.addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.addr.sin_port = htons(6111);

    IPv4Address event_addr;
    event_addr.addr.sin_family = AF_INET;
    event_addr.addr.sin_addr.s_addr = htonl(INADDR_ANY);
    event_addr.addr.sin_port = htons(7111);

    bool debug_mode = false;
    bool msg_debug_mode = false;
    SoeLogger::Logger logger;

    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << argv[0] << " starting main services ..." << SoeLogger::LogInfo() << endl;

    ::memset(user_name, '\0', sizeof(user_name));
    int ret = metasrv_getopt(logger, argc, argv,
        &debug_mode,
        &msg_debug_mode,
        user_name);
    if ( ret ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Parse options failed" << SoeLogger::LogError() << endl;
        usage(logger, argv[0]);
        return -1;
    }

    if ( !*user_name ) {
        strcpy(user_name, "lzsystem");
    }
    ret = SwitchUid(logger, user_name);
    if ( ret < 0 ) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "Can't swtich uid, run it under lzsystem or sudo soemetadbsrv " << SoeLogger::LogError() << endl;
        return -1;
    }

    if ( debug_mode == true ) {
        ClusterProxy1::global_debug = true;
    }

    try {
        MetaNet::IPv4MulticastListener *ipv4mcastlis = 0;
        McastlistSrvProcessor msrv_processor(false);
        try {
            ipv4mcastlis = MetaNet::IPv4MulticastListener::getInstance(true);
            ipv4mcastlis->setMcastClientMode(true);

            boost::function<CallbackIPv4ProcessFun> m_p1 = boost::bind(&McastlistSrvProcessor::processStream, &msrv_processor, _1, _2, _3, _4);
            ipv4mcastlis->setProcessStreamCB(m_p1);
            boost::function<CallbackIPv4ProcessFun> m_p2 = boost::bind(&McastlistSrvProcessor::processDgram, &msrv_processor, _1, _2, _3, _4);
            ipv4mcastlis->setProcessDgramCB(m_p2);
            boost::function<CallbackIPv4SendFun> m_s1 = boost::bind(&McastlistSrvProcessor::sendStream, &msrv_processor, _1, _2, _3, _4, _5);
            ipv4mcastlis->setSendStreamCB(m_s1);
            boost::function<CallbackIPv4SendFun> m_s2 = boost::bind(&McastlistSrvProcessor::sendDgram, &msrv_processor, _1, _2, _3, _4, _5);
            ipv4mcastlis->setSendDgramCB(m_s2);
        } catch (const std::runtime_error &ex) {
            delete ipv4mcastlis;
            ipv4mcastlis = 0;
            logger.Clear(CLEAR_DEFAULT_ARGS) << "runtime_error caught: " << ex.what() << ", will try again" << SoeLogger::LogError() << endl;
        }

        TcpSocket ipv4_sock;
        ipv4_sock.SetNoSigPipe();
        MetaNet::IPv4Listener *ipv4lis = MetaNet::IPv4Listener::setInstance(&ipv4_sock);
        ipv4lis->setDebug(msg_debug_mode);
        AdmSrvProcessor srv_processor(msg_debug_mode);
        boost::function<CallbackIPv4ProcessFun> l_p1 = boost::bind(&AdmSrvProcessor::processStream, &srv_processor, _1, _2, _3, _4);
        ipv4lis->setProcessStreamCB(l_p1);
        boost::function<CallbackIPv4ProcessFun> l_p2 = boost::bind(&AdmSrvProcessor::processDgram, &srv_processor, _1, _2, _3, _4);
        ipv4lis->setProcessDgramCB(l_p2);
        boost::function<CallbackIPv4SendFun> l_s1 = boost::bind(&AdmSrvProcessor::sendStream, &srv_processor, _1, _2, _3, _4, _5);
        ipv4lis->setSendStreamCB(l_s1);
        boost::function<CallbackIPv4SendFun> l_s2 = boost::bind(&AdmSrvProcessor::sendDgram, &srv_processor, _1, _2, _3, _4, _5);
        ipv4lis->setSendDgramCB(l_s2);
        boost::function<CallbackIPv4ProcessTimerFun> l_tm = boost::bind(&AdmSrvProcessor::processTimeout, &srv_processor);
        ipv4lis->setProcessTimerCB(l_tm);

        ipv4lis->setAssignedAddress(addr);
        TcpSocket *tcp_sock = dynamic_cast<TcpSocket *>(ipv4lis->getSock());
        if ( !tcp_sock ) {
            throw runtime_error(string(string(__FUNCTION__) + " Wrong socket type").c_str());
        }
        int ret = tcp_sock->Listen(10);
        if ( ret < 0 ) {
            throw runtime_error((string(__FUNCTION__) +
                " Listen failed addr: " + addr.GetNTOASinAddr() + " errno: " + to_string(errno)).c_str());
        }


        TcpSocket event_sock;
        event_sock.SetNoSigPipe();
        event_sock.SetNonBlocking(true);
        EventLisSrvProcessor eventsrv_processor(debug_mode);
        eventsrv_processor.eventlis = new MetaNet::EventListener(event_sock);
        eventsrv_processor.eventlis->setDebug(debug_mode);
        boost::function<CallbackEventProcessSessionFun> el_pe1 = boost::bind(&EventLisSrvProcessor::processStreamListenerEvent, &eventsrv_processor, _1, _2, _3, _4);
        eventsrv_processor.eventlis->setEventProcessSessionCB(el_pe1);
        boost::function<CallbackEventSendSessionFun> el_se1 = boost::bind(&EventLisSrvProcessor::sendStreamListenerEvent, &eventsrv_processor, _1, _2, _3, _4, _5);
        eventsrv_processor.eventlis->setEventSendSessionCB(el_se1);


        eventsrv_processor.eventlis->setAssignedAddress(event_addr);
        tcp_sock = dynamic_cast<TcpSocket *>(eventsrv_processor.eventlis->getSock());
        if ( !tcp_sock ) {
            throw runtime_error(string(string(__FUNCTION__) + " Wrong socket type").c_str());
        }
        ret = tcp_sock->Listen(10);
        if ( ret < 0 ) {
            throw runtime_error((string(__FUNCTION__) +
                " Listen on TCP socket failed addr: " + addr.GetNTOASinAddr() + " errno: " + to_string(errno)).c_str());
        }
        UnixSocket *unix_sock = dynamic_cast<UnixSocket *>(eventsrv_processor.eventlis->getUnixSock());
        if ( !unix_sock ) {
            throw runtime_error(string(string(__FUNCTION__) + " Wrong socket type").c_str());
        }
        ret = unix_sock->Listen(10);
        if ( ret < 0 ) {
            throw runtime_error((string(__FUNCTION__) +
                " Listen on Unix socket failed addr: " + addr.GetNTOASinAddr() + " errno: " + to_string(errno)).c_str());
        }
        MmPoint *mm_point = dynamic_cast<MmPoint *>(eventsrv_processor.eventlis->getMmSock());
        if ( !mm_point ) {
            throw runtime_error(string(string(__FUNCTION__) + " Wrong socket type").c_str());
        }
        ret = mm_point->Listen(10);
        if ( ret < 0 ) {
            throw runtime_error((string(__FUNCTION__) +
                " Listen on MM socket failed addr: " + addr.GetNTOASinAddr() + " errno: " + to_string(errno)).c_str());
        }

        boost::thread lis_thread(&MetaNet::IPv4Listener::start, ipv4lis);
        boost::thread eventlis_thread1(&MetaNet::EventListener::startAsync, eventsrv_processor.eventlis);
        boost::thread eventlis_thread2(&MetaNet::EventListener::startAsync, eventsrv_processor.eventlis);
        boost::thread eventlis_thread3(&MetaNet::EventListener::startAsync, eventsrv_processor.eventlis);
        boost::thread eventlis_thread4(&MetaNet::EventListener::startAsync, eventsrv_processor.eventlis);

        // if we don't have mcast lis try to start it up again
        if ( !ipv4mcastlis ) {
            for ( int i = 0 ; i < 1000 ; i++ ) {
                try {
                    ipv4mcastlis = MetaNet::IPv4MulticastListener::getInstance(true);
                    ipv4mcastlis->setMcastClientMode(true);

                    boost::function<CallbackIPv4ProcessFun> m_p1 = boost::bind(&McastlistSrvProcessor::processStream, &msrv_processor, _1, _2, _3, _4);
                    ipv4mcastlis->setProcessStreamCB(m_p1);
                    boost::function<CallbackIPv4ProcessFun> m_p2 = boost::bind(&McastlistSrvProcessor::processDgram, &msrv_processor, _1, _2, _3, _4);
                    ipv4mcastlis->setProcessDgramCB(m_p2);
                    boost::function<CallbackIPv4SendFun> m_s1 = boost::bind(&McastlistSrvProcessor::sendStream, &msrv_processor, _1, _2, _3, _4, _5);
                    ipv4mcastlis->setSendStreamCB(m_s1);
                    boost::function<CallbackIPv4SendFun> m_s2 = boost::bind(&McastlistSrvProcessor::sendDgram, &msrv_processor, _1, _2, _3, _4, _5);
                    ipv4mcastlis->setSendDgramCB(m_s2);
                    break;
                } catch (const std::runtime_error &ex) {
                    delete ipv4mcastlis;
                    ipv4mcastlis = 0;
                    logger.Clear(CLEAR_DEFAULT_ARGS) << "runtime_error caught: " << ex.what() << ", will try again" << SoeLogger::LogError() << endl;
                    usleep(100000);
                }
            }
        }

        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << argv[0] << " starting mcast service ..." << SoeLogger::LogInfo() << endl;

        boost::thread *mcastlis_thread = 0;
        if ( ipv4mcastlis ) {
            mcastlis_thread = new boost::thread(&MetaNet::IPv4MulticastListener::start, ipv4mcastlis);
        }

        lis_thread.join();
        eventlis_thread1.join();
        eventlis_thread2.join();
        eventlis_thread3.join();
        eventlis_thread4.join();
        if ( ipv4mcastlis ) {
            mcastlis_thread->join();
            delete mcastlis_thread;
        }
    } catch (const runtime_error &ex) {
        logger.Clear(CLEAR_DEFAULT_ARGS) << "runtime_error caught: " << ex.what() << SoeLogger::LogError() << endl;
    }

    return 0;
}


