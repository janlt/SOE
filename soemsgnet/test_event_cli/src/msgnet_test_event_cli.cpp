/*
 * msgnet_test_cli.cpp
 *
 *  Created on: Jan 10, 2017
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
#include "msgnetioevent.hpp"
#include "msgnettcpsocket.hpp"
#include "msgnetudpsocket.hpp"
#include "msgnetunixsocket.hpp"
#include "msgnetlistener.hpp"
#include "msgnetprocessor.hpp"
#include "msgnetipv4listener.hpp"
#include "msgnetipv4mcastlistener.hpp"
#include "msgnetsession.hpp"
#include "msgnetipv4tcpsession.hpp"
#include "msgnetipv4tcpclisession.hpp"
#include "msgneteventsession.hpp"
#include "msgneteventclisession.hpp"
#include "msgnetasynceventclisession.hpp"

#include <json/value.h>
#include <json/reader.h>

using namespace std;
using namespace MetaMsgs;
using namespace MetaNet;

//
// I/O Session
//
struct TestIov
{

};

class AsyncIOSession: public EventCliAsyncSession
{
public:
    IPv4Address    my_addr;
    uint64_t       counter;
    uint64_t       bytes_received;
    uint64_t       bytes_sent;
    bool           sess_debug;
    uint64_t       recv_count;
    uint64_t       sent_count;
    uint64_t       loop_count;
    boost::thread *io_session_thread;
    Session       *parent_session;
    uint32_t       msg_pause;
    boost::thread *in_thread;
    boost::thread *out_thread;
    static uint32_t      session_counter;
    uint32_t       session_id;

    AsyncIOSession(Point &point,
            Session *_parent,
            uint32_t _msg_pause = 0,
            bool _test = false)
        :EventCliAsyncSession(point, _test),
         counter(0),
         bytes_received(0),
         bytes_sent(0),
         sess_debug(false),
         recv_count(0),
         sent_count(0),
         loop_count(0),
         io_session_thread(0),
         parent_session(_parent),
         msg_pause(_msg_pause),
         in_thread(0),
         out_thread(0),
         session_id(session_counter++)
    {
        my_addr = IPv4Address::findNodeNoLocalAddr();
    }

    virtual ~AsyncIOSession();

    virtual int processInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev);
    virtual int sendInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev);
    virtual int processOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev);
    virtual int sendOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev);

    virtual void inLoop();
    virtual void outLoop();

    void setSessDebug(bool _sess_debug)
    {
        sess_debug = _sess_debug;
    }
    int ParseSetAddresses(const std::string &ad_json);
    int Connect();
    int testAllIOStream(const IPv4Address &from_addr, const MetaNet::Point *pt, uint64_t loop_count = -1);
    virtual void start() throw(std::runtime_error);
};

uint32_t AsyncIOSession::session_counter = 1;

AsyncIOSession::~AsyncIOSession()
{
    if ( in_thread ) {
        delete in_thread;
    }
    if ( out_thread ) {
        delete out_thread;
    }
}

int AsyncIOSession::processInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev)
{
    int ret = EventCliAsyncSession::processInboundStreamEvent(buf, from_addr, ev);
    if ( ret ) {
        std::cerr << __FUNCTION__ << " failed enqueue ret:" << ret << std::endl << std::flush;
    }
    if ( sess_debug == true ) std::cout << __FUNCTION__ << " Got and enqueued event" << std::endl << std::flush;
    return ret;
}

int AsyncIOSession::sendInboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev)
{
    int ret = EventCliAsyncSession::sendInboundStreamEvent(buf, dst_addr, ev);
    if ( ret ) {
        std::cerr << __FUNCTION__ << " failed dequeue ret:" << ret << std::endl << std::flush;
    }
    if ( sess_debug == true ) std::cout << __FUNCTION__ << " Got and dequeued event" << std::endl << std::flush;
    return ret;
}

int AsyncIOSession::processOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const IoEvent *ev)
{
    int ret = EventCliAsyncSession::processOutboundStreamEvent(buf, from_addr, ev);
    if ( ret ) {
        std::cerr << __FUNCTION__ << " failed enqueue ret:" << ret << std::endl << std::flush;
        return ret;
    }
    if ( sess_debug == true ) std::cout << __FUNCTION__ << " Got and enqueued event" << std::endl << std::flush;
    return ret;
}

int AsyncIOSession::sendOutboundStreamEvent(MetaMsgs::MsgBufferBase& buf, const IPv4Address *dst_addr, IoEvent *&ev)
{
    int ret = EventCliAsyncSession::sendInboundStreamEvent(buf, dst_addr, ev);
    if ( ret ) {
        std::cerr << __FUNCTION__ << " failed dequeue ret:" << ret << std::endl << std::flush;
        return ret;
    }
    if ( sess_debug == true ) std::cout << __FUNCTION__ << " Got and dequeued event" << std::endl << std::flush;
    return ret;
}

void AsyncIOSession::inLoop()
{
    EventCliAsyncSession::inLoop();
}

void AsyncIOSession::outLoop()
{
    EventCliAsyncSession::outLoop();
}

inline void AsyncIOSession::start() throw(std::runtime_error)
{
    //GetReq<TestIov> *gr = MetaMsgs::GetReq<TestIov>::poolT.get();

    for ( uint64_t i = 0 ; i < loop_count ; i++ ) {
        GetReq<TestIov> *gr = MetaMsgs::GetReq<TestIov>::poolT.get();

        std::string c_n = "DUPEK_CLUSTER_" + to_string(i);
        ::strcpy(gr->meta_desc.desc.cluster_name, c_n.c_str());
        gr->meta_desc.desc.cluster_id = 1234;
        if ( gr->SetIovSize(GetReq<TestIov>::cluster_name_idx, static_cast<uint32_t>(::strlen(gr->meta_desc.desc.cluster_name))) ) {
            std::cerr << __FUNCTION__ << " Data too big" << std::endl << std::flush;
        }

        ::strcpy(gr->meta_desc.desc.space_name, "DUPEK_SPACE");
        gr->meta_desc.desc.space_id = 5678;
        if ( gr->SetIovSize(GetReq<TestIov>::space_name_idx, static_cast<uint32_t>(::strlen(gr->meta_desc.desc.space_name))) ) {
            std::cerr << __FUNCTION__ << " Data too big" << std::endl << std::flush;
        }

        ::strcpy(gr->meta_desc.desc.store_name, "DUPAK_STORE");
        gr->meta_desc.desc.store_id = 9012;
        if ( gr->SetIovSize(GetReq<TestIov>::store_name_idx, static_cast<uint32_t>(::strlen(gr->meta_desc.desc.store_name))) ) {
            std::cerr << __FUNCTION__ << " Data too big" << std::endl << std::flush;
        }

        ::strcpy(gr->meta_desc.desc.key, "TESTKEY1");
        gr->meta_desc.desc.key_len = strlen(gr->meta_desc.desc.key);
        if ( gr->SetIovSize(GetReq<TestIov>::key_idx, gr->meta_desc.desc.key_len) ) {
            std::cerr << __FUNCTION__ << " Data too big" << std::endl << std::flush;
        }

        ::strcpy(gr->meta_desc.desc.end_key, "_ENDKEY1");
        if ( gr->SetIovSize(GetReq<TestIov>::end_key_idx, gr->meta_desc.desc.key_len) ) {
            std::cerr << __FUNCTION__ << " Data too big" << std::endl << std::flush;
        }

        ::memset(gr->meta_desc.desc.data, 'D', 8);
        gr->meta_desc.desc.data_len = 8; //1024;
        if ( gr->SetIovSize(GetReq<TestIov>::data_idx, gr->meta_desc.desc.data_len) ) {
            std::cerr << __FUNCTION__ << " Data too big" << std::endl << std::flush;
        }

        gr->meta_desc.desc.buf_len = 0;
        if ( gr->SetIovSize(GetReq<TestIov>::buf_idx, gr->meta_desc.desc.buf_len) ) {
            std::cerr << __FUNCTION__ << " Data too big" << std::endl << std::flush;
        }

        int ret = EventSession<IoEvent>::event_point.SendIov(*gr);
        if ( ret < 0 ) {
            std::cerr << __FUNCTION__ << " send failed type:" << gr->iovhdr.hdr.type << std::endl << std::flush;
        }

        sent_count++;
        MetaMsgs::GetReq<TestIov>::poolT.put(gr);
#ifdef ROUND_TRIP
        GetRsp<TestIov> *gp = MetaMsgs::GetRsp<TestIov>::poolT.get();
        ret = EventSession<IoEvent>::event_point.RecvIov(*gp);
        if ( ret < 0 ) {
            std::cerr << __FUNCTION__ << " recv failed type:" << gp->iovhdr.hdr.type << std::endl << std::flush;
        }
        recv_count++;

        if ( sess_debug == true ) std::cout << __FUNCTION__ << " Got GetRsp<TestIov> " <<
            " cluster: " << gp->meta_desc.desc.cluster_name <<
            " space: " << gp->meta_desc.desc.space_name <<
            " store: " << gp->meta_desc.desc.store_name <<
            std::endl << std::flush;

        MetaMsgs::GetRsp<TestIov>::poolT.put(gp);
#endif

        if ( msg_pause ) {
            usleep(msg_pause);
        }

        if ( !(i%0x40000) ) {
            std::cout << "(" << session_id << ")sent_count: " << sent_count << " recv_count: " << recv_count << std::endl << std::flush;
        }
    }
}




//
// Client Adm Session
//
class MySession: public IPv4TCPCliSession
{
public:
    IPv4Address          own_addr;
    uint64_t             counter;
    bool                 started;

    const static uint32_t                  max_sessions = 1024;
    std::shared_ptr<AsyncIOSession>        io_sessions[max_sessions];
    uint32_t                               num_sessions;

    typedef enum _eTestType {
        eAdmCont       = 0,
        eIOSession     = 1,
        eIOCont        = 2,
        eDead          = 3
    } eTestType;
    eTestType   test_type;
    uint64_t    run_count;
    bool        unix_sock_session;
    bool        ipv4_sock_session;
    uint32_t    msg_pause;

    IPv4Address io_peer_addr;

    MySession(TcpSocket &soc,
            eTestType _test_type,
            bool _unix_sock_session = true,
            bool _ipv4_sock_session = false,
            bool _debug = false,
            uint32_t _msg_pause = 0,
            bool _test = false)
        :IPv4TCPCliSession(soc, _test),
         counter(0),
         started(false),
         num_sessions(0),
         test_type(_test_type),
         run_count(-1),
         unix_sock_session(_unix_sock_session),
         ipv4_sock_session(_ipv4_sock_session),
         msg_pause(_msg_pause)
    {
        own_addr = IPv4Address::findNodeNoLocalAddr();
        setDebug(_debug);

        if ( soc.CreateStream() < 0 ) {
            std::cerr << "Can't create tcp socket: " << errno << " exiting ..." << std::flush;
            exit(-1);
        }
        if ( soc.SetNoDelay() < 0 ) {
            std::cerr << "Can't set nodelay on tcp socket: " << errno << " exiting ..." << std::flush;
            exit(-1);
        }
        if ( soc.SetRecvTmout(200) < 0 ) {
            std::cerr << "Can't set recv tmout on tcp socket: " << errno << " exiting ..." << std::flush;
            exit(-1);
        }
        if ( soc.SetSndTmout(200) < 0 ) {
            std::cerr << "Can't set snd tmout on tcp socket: " << errno << " exiting ..." << std::flush;
            exit(-1);
        }
        if ( soc.SetReuseAddr() < 0 ) {
            std::cerr << "Can't set reuse address on tcp socket: " << errno << " exiting ..." << std::flush;
            exit(-1);
        }
        if ( soc.SetNoLinger() < 0 ) {
            std::cerr << "Can't set no linger on tcp socket: " << errno << " exiting ..." << std::flush;
            exit(-1);
        }
    }

    virtual ~MySession() {}
    virtual int processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);

    virtual void start() throw(std::runtime_error);

    int ParseSetAddresses(const std::string &ad_json);
    int Connect();

    int CreateLocalUnixSockIOSession(const IPv4Address &_io_peer_addr, int _idx);
    int CreateLocalMMSockIOSession(const IPv4Address &_io_peer_addr, int _idx);
    int CreateRemoteIOSession(const IPv4Address &_io_peer_addr, int _idx);

    void contIOTest() throw(std::runtime_error);
    void ioSessionTest() throw(std::runtime_error);
    void setIOPeerAddr(const IPv4Address &_io_peer_addr);
    void setNumSessions(uint32_t _num_sessions);
};

void MySession::setIOPeerAddr(const IPv4Address &_io_peer_addr)
{
    io_peer_addr = _io_peer_addr;
}

void MySession::setNumSessions(uint32_t _num_sessions)
{
    num_sessions = _num_sessions;
}

void MySession::contIOTest() throw(std::runtime_error)
{
    for ( uint64_t i = 0 ; i < run_count ; i++ ) {
        if ( peer_addr == IPv4Address::null_address ) {
            std::cerr << __FUNCTION__ << " Server still disconnected or dead" << std::endl << std::flush;
            usleep(1000000);
        }

        for ( uint32_t s = 0 ; s < num_sessions ; s++ ) {
            io_sessions[s]->loop_count = -1;
            io_sessions[s]->io_session_thread = new boost::thread(&AsyncIOSession::start, io_sessions[s].get());
        }

        for ( uint32_t s = 0 ; s < num_sessions ; s++ ) {
            if ( io_sessions[s]->io_session_thread ) {
                io_sessions[s]->io_session_thread->join();
            }
        }
    }
}

int MySession::CreateLocalUnixSockIOSession(const IPv4Address &_io_peer_addr, int _idx)
{
    bool _clean_up = false;
    UnixSocket *ss = new UnixSocket;

    if ( ss->Create() < 0 ) {
        std::cerr << "Can't create tcp socket: " << errno << std::flush;
        _clean_up = true;
    }
    if ( ss->SetNoDelay() < 0 ) {
        std::cerr << "Can't set nodelay on tcp socket: " << errno << std::flush;
        _clean_up = true;
    }
    if ( ss->SetRecvTmout(200) < 0 ) {
        std::cerr << "Can't set recv tmout on tcp socket: " << errno << std::flush;
        _clean_up = true;
    }
    if ( ss->SetSndTmout(200) < 0 ) {
        std::cerr << "Can't set snd tmout on tcp socket: " << errno << std::flush;
        _clean_up = true;
    }
    if ( ss->SetReuseAddr() < 0 ) {
        std::cerr << "Can't set reuse address on tcp socket: " << errno << std::flush;
        _clean_up = true;
    }
    if ( ss->SetNoLinger() < 0 ) {
        std::cerr << "Can't set no linger on tcp socket: " << errno << std::flush;
        _clean_up = true;
    }

    io_sessions[_idx].reset(new AsyncIOSession(*ss, this, msg_pause));
    UnixAddress *uad = new UnixAddress(UnixAddress::findNodeNoLocalAddr());
    if ( uad->OpenName() < 0 ) {
        std::cerr << "Failed opening name for unix socket peer addr: " << uad->GetPath() << " errno: " << errno << std::endl << std::flush;
        delete ss;
        return -1;
    }

    try {
        io_sessions[_idx]->setPeerAddr(uad);
    } catch (const std::runtime_error &ex) {
        std::cerr << "Failed setting unix socket peer addr: " << ex.what() << std::endl << std::flush;
        delete ss;
        return -1;
    }
    io_sessions[_idx]->setSessDebug(debug);
    if ( io_sessions[_idx]->initialise(0, true) < 0 ) {
        std::cerr << "Failed initialize io session errno: " << errno << std::endl << std::flush;
        _clean_up = true;
        io_sessions[_idx].reset();
    }

    if ( _clean_up == true ) {
        delete ss;
        return -1;
    }

    io_sessions[_idx]->setSessDebug(debug);

    io_sessions[_idx]->in_thread = new boost::thread(&AsyncIOSession::inLoop, io_sessions[_idx].get());
    io_sessions[_idx]->out_thread = new boost::thread(&AsyncIOSession::outLoop, io_sessions[_idx].get());

    return 0;
}

int MySession::CreateLocalMMSockIOSession(const IPv4Address &_io_peer_addr, int _idx)
{
    bool _clean_up = false;
    MmPoint *ss = new MmPoint();

    if ( ss->Create() < 0 ) {
        std::cerr << "Can't create tcp socket: " << errno << std::flush;
        _clean_up = true;
    }
    if ( ss->SetRecvTmout(200) < 0 ) {
        std::cerr << "Can't set recv tmout on tcp socket: " << errno << std::flush;
        _clean_up = true;
    }
    if ( ss->SetSndTmout(200) < 0 ) {
        std::cerr << "Can't set snd tmout on tcp socket: " << errno << std::flush;
        _clean_up = true;
    }

    io_sessions[_idx].reset(new AsyncIOSession(*ss, this, msg_pause));
    MMAddress *mmad = new MMAddress(MMAddress::findNodeNoLocalAddr());
    if ( mmad->OpenName() < 0 ) {
        std::cerr << "Failed opening name for MM socket peer addr: " << mmad->GetPath() << " errno: " << errno << std::endl << std::flush;
        delete ss;
        return -1;
    }
    try {
        io_sessions[_idx]->setPeerAddr(mmad);
    } catch (const std::runtime_error &ex) {
        std::cerr << "Failed setting MM socket peer addr: " << ex.what() << std::endl << std::flush;
        delete ss;
        return -1;
    }
    io_sessions[_idx]->setSessDebug(debug);
    if ( io_sessions[_idx]->initialise(0, true) < 0 ) {
        std::cerr << "Failed initialize io session errno: " << errno << std::endl << std::flush;
        _clean_up = true;
        io_sessions[_idx].reset();
    }

    if ( _clean_up == true ) {
        delete ss;
        return -1;
    }

    io_sessions[_idx]->setSessDebug(debug);

    io_sessions[_idx]->in_thread = new boost::thread(&AsyncIOSession::inLoop, io_sessions[_idx].get());
    io_sessions[_idx]->out_thread = new boost::thread(&AsyncIOSession::outLoop, io_sessions[_idx].get());

    return 0;
}

int MySession::CreateRemoteIOSession(const IPv4Address &_io_peer_addr, int _idx)
{
    bool _clean_up = false;
    TcpSocket *ss = new TcpSocket;

    if ( ss->Create() < 0 ) {
        std::cerr << "Can't create tcp socket: " << errno << std::flush;
        _clean_up = true;
    }
    if ( ss->SetNoDelay() < 0 ) {
        std::cerr << "Can't set nodelay on tcp socket: " << errno << std::flush;
        _clean_up = true;
    }
    if ( ss->SetRecvTmout(200) < 0 ) {
        std::cerr << "Can't set recv tmout on tcp socket: " << errno << std::flush;
        _clean_up = true;
    }
    if ( ss->SetSndTmout(200) < 0 ) {
        std::cerr << "Can't set snd tmout on tcp socket: " << errno << std::flush;
        _clean_up = true;
    }
    if ( ss->SetReuseAddr() < 0 ) {
        std::cerr << "Can't set reuse address on tcp socket: " << errno << std::flush;
        _clean_up = true;
    }
    if ( ss->SetNoLinger() < 0 ) {
        std::cerr << "Can't set no linger on tcp socket: " << errno << std::flush;
        _clean_up = true;
    }
    if ( ss->SetNonBlocking(true) < 0 ) {
        std::cerr << "Can't set non blocking on tcp socket: " << errno << std::flush;
        _clean_up = true;
    }

    io_sessions[_idx].reset(new AsyncIOSession(*ss, this, msg_pause));
    io_sessions[_idx]->setPeerAddr(new IPv4Address(io_peer_addr));
    io_sessions[_idx]->setSessDebug(debug);
    if ( io_sessions[_idx]->initialise(0, true) < 0 ) {
        std::cerr << "Failed initialize io session errno: " << errno << std::endl << std::flush;
        _clean_up = true;
        io_sessions[_idx].reset();
    }

    if ( _clean_up == true ) {
        delete ss;
        return -1;
    }

    io_sessions[_idx]->setSessDebug(debug);

    io_sessions[_idx]->in_thread = new boost::thread(&AsyncIOSession::inLoop, io_sessions[_idx].get());
    io_sessions[_idx]->out_thread = new boost::thread(&AsyncIOSession::outLoop, io_sessions[_idx].get());

    return 0;
}

void MySession::ioSessionTest() throw(std::runtime_error)
{
    int ret = 0;

    // send AdmMsgAssignMMChannelReq and expect a response
    MetaMsgs::AdmMsgAssignMMChannelReq *mm_req = MetaMsgs::AdmMsgAssignMMChannelReq::poolT.get();
    mm_req->pid = getpid();

    ret = mm_req->send(tcp_sock.GetDesc());
    if ( ret < 0 ) {
        std::cerr << __FUNCTION__ << " Failed sending AdmMsgAssignMMChannelReq: " << ret << std::endl << std::flush;
        MetaMsgs::AdmMsgAssignMMChannelReq::poolT.put(mm_req);
        return;
    }

    MetaMsgs::AdmMsgAssignMMChannelReq::poolT.put(mm_req);

    MetaMsgs::AdmMsgBase base_msg;
    MetaMsgs::AdmMsgBuffer buf(true);
    buf.reset();
    ret = base_msg.recv(tcp_sock.GetDesc(), buf);
    if ( ret < 0 ) {
        std::cerr << __FUNCTION__ << " Can't receive AdmMsgBase hdr" << std::endl << std::flush;
        return;
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        std::cerr << __FUNCTION__ << " Can't receive AdmMsgBase hdr" << std::endl << std::flush;
    }
    recv_msg->hdr = base_msg.hdr;

    MetaMsgs::AdmMsgAssignMMChannelRsp *mm_rsp = dynamic_cast<MetaMsgs::AdmMsgAssignMMChannelRsp *>(recv_msg);
    ret = mm_rsp->recv(tcp_sock.GetDesc(), buf);
    if ( ret < 0 ) {
        std::cerr << __FUNCTION__ << " Can't receive AdmMsgAssignMMChannelRsp" << std::endl << std::flush;
    } else {
        if ( debug == true ) std::cout << __FUNCTION__ << " Got AdmMsgAssignMMChannelRsp req" <<
            " pid: " << mm_rsp->pid <<
            " path: " << mm_rsp->path <<
            " counter: " << counter++ << std::endl << std::flush;
    }

    // create io socket and IO session
    for ( uint32_t s = 0 ; s < num_sessions ; s++ ) {
        int ret = 0;
        if ( ipv4_sock_session == false ) {
            if ( std::string(own_addr.GetNTOASinAddr()) == std::string(io_peer_addr.GetNTOASinAddr()) && unix_sock_session == true ) {
                ret = CreateLocalUnixSockIOSession(io_peer_addr, s);
            } else if (  std::string(own_addr.GetNTOASinAddr()) == std::string(io_peer_addr.GetNTOASinAddr()) && unix_sock_session == false ) {
                ret = CreateLocalMMSockIOSession(io_peer_addr, s);
            } else {
                std::cerr << __FUNCTION__ << " Can't create requested session: " << own_addr.GetNTOASinAddr() <<
                    " io_peer_addr: " << io_peer_addr.GetNTOASinAddr() << std::endl << std::flush;
                exit(-1);
            }
        } else {
            ret = CreateRemoteIOSession(io_peer_addr, s);
        }

        if ( ret < 0 ) {
            std::cerr << __FUNCTION__ << " Create I/O session failed own_addr: " << own_addr.GetNTOASinAddr() <<
                " io_peer_addr: " << io_peer_addr.GetNTOASinAddr() << std::endl << std::flush;
            exit(-1);
        }
    }

    test_type = eIOCont;
}

void MySession::start() throw(std::runtime_error)
{
    // Run test all Adm msgs handler synchronously
    for ( ;; ) {
        switch (test_type ) {
        case MySession::eTestType::eIOSession:
            ioSessionTest();
            break;
        case MySession::eTestType::eIOCont:
            contIOTest();
            break;
        case MySession::eTestType::eDead:
            std::cerr << "Dead test type: " << test_type << std::endl << std::flush;
            usleep(100000);
            break;
        default:
            std::cerr << "Invalid test type: " << test_type << std::endl << std::flush;
            return;
        }
    }
}

// This client allows only one session at a time
static MySession *sess = 0;

//
// ret < 0 - error parsing
// ret == 0 - need to connect or reconnect (due to server address change or restart)
// ret > 0 - no change
//
int MySession::ParseSetAddresses(const std::string &ad_json)
{
    if ( debug == true ) std::cout << __FUNCTION__ << " Got json: " << ad_json << std::endl << std::flush;

    MsgNetMcastAdvertisement ad_msg(ad_json);
    int ret = ad_msg.Parse();
    if ( ret ) {
        std::cerr << __FUNCTION__ << " parse failed " << std::endl << std::flush;
        return -1;
    }
    ret = 1;

    AdvertisementItem *ad_item = ad_msg.FindItemByWhat("meta_srv_adm");
    if ( ad_item ) {
        uint16_t prot = ad_item->protocol == AdvertisementItem::eProtocol::eInet ? AF_INET : AF_INET;
        IPv4Address new_address(ad_item->address.c_str(), htons(ad_item->port), prot);
        if ( new_address != peer_addr ) {
            std::cout << "Got new srv json: " << ad_json <<
                " address: " << new_address.GetNTOASinAddr() <<
                " port: " << new_address.GetPort() <<
                " fam: " << new_address.GetFamily() << std::endl << std::flush;

            this->setPeerAddr(new_address);
            ret = 0;
        }
    }
    ad_item = ad_msg.FindItemByWhat("meta_srv_io");
    if ( ad_item ) {
        uint16_t prot = ad_item->protocol == AdvertisementItem::eProtocol::eInet ? AF_INET : AF_INET;
        IPv4Address io_new_address(ad_item->address.c_str(), htons(ad_item->port), prot);
        if ( io_new_address != io_peer_addr ) {
            std::cout << "Got new srv json: " << ad_json <<
                " address: " << io_new_address.GetNTOASinAddr() <<
                " port: " << io_new_address.GetPort() <<
                " fam: " << io_new_address.GetFamily() << std::endl << std::flush;

            this->setIOPeerAddr(io_new_address);
            ret = 0;
        }
    }

    return ret;
}

int MySession::processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( debug == true ) std::cout << __FUNCTION__ << " Invalid invocation" << std::endl << std::flush;
    return -1;
}

int MySession::processStreamMessage(MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( debug == true ) std::cout << __FUNCTION__ << " Got msg" << std::endl << std::flush;
    return 0;
}

int MySession::sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    if ( debug == true ) std::cout << __FUNCTION__ << " Invalid invocation" << std::endl << std::flush;
    return -1;
}

int MySession::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    if ( debug == true ) std::cout << __FUNCTION__ << " Sending msg" << std::endl << std::flush;
    return 0;
}

//
// Mcast msg Processor
//
struct McastlistCliProcessor: public Processor
{
    int processAdvertisement(MetaMsgs::McastMsgMetaAd *ad, const IPv4Address &from_addr, const MetaNet::Point *pt = 0)
    {
        int ret = 0;

        if ( debug == true ) std::cout << __FUNCTION__ << " Got ad msg: " << ad->json << std::endl << std::flush;

        // If the ad was from our own address (identified by function call!!!!!)
        // In real application it should come from a config file
        if ( from_addr.GetSinAddr() != own_addr.GetSinAddr() ) {
            if ( allow_remote_srv == false ) {
                return 0;
            }
        }

        // Create session
        if ( !sess ) {
            TcpSocket *ss = new TcpSocket;
            try {
                sess = new MySession(*ss, test_type, mm_sock_session == true ? false : true, allow_remote_srv, debug, msg_pause);
                sess->setNumSessions(num_io_sessions_per_session);
            } catch (const std::runtime_error &ex) {
                std::cerr << "Fatal std::runtime_error caught: " << ex.what() << " exiting ... " << std::endl << std::flush;
                exit(-1);
            }
        }

        if ( sess->peer_addr == IPv4Address::null_address ) {
            (void )sess->deinitialise(true);
            delete sess;

            TcpSocket *ss = new TcpSocket;
            try {
                sess = new MySession(*ss, test_type, mm_sock_session == true ? false : true, allow_remote_srv, debug, msg_pause);
                sess->setNumSessions(num_io_sessions_per_session);
            } catch (const std::runtime_error &ex) {
                std::cerr << "Fatal std::runtime_error caught: " << ex.what() << " exiting ... " << std::endl << std::flush;
                exit(-1);
            }

            ret = sess->ParseSetAddresses(ad->json);
            if ( ret < 0 ) {
                std::cerr << __FUNCTION__ << " Invalid mcast ad: " << ret << std::endl << std::flush;
                return -1;
            }

            // that means session has been deinitialized
            srv_addr = sess->peer_addr;

            ret = sess->initialise(0, true); // For client we pass NULL as listening socket since we are connection our own
            if ( ret < 0 ) {
                std::cerr << "Can't initialize session errno: " << errno << " exiting ..." << std::flush;
                exit(-1);
            }
        }

        if ( sess->started != true ) {
            sess->started = true;
             sess_thread = new boost::thread(&MySession::start, sess);
        }

        return ret;
    }

    // This callback runs session recovery logic as well
    virtual int processDgram(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0)
    {
        if ( !_buf.getSize() ) { // timeout - ignore
            return 0;
        }

        if ( debug == true ) std::cout << __FUNCTION__ << " Got dgram req" << std::endl << std::flush;
        int ret = 0;

        MetaMsgs::McastMsgMetaAd *ad = MetaMsgs::McastMsgMetaAd::poolT.get();
        ad->unmarshall(dynamic_cast<MetaMsgs::McastMsgBuffer &>(_buf));

        try {
            ret = processAdvertisement(ad, from_addr, pt);
        } catch (const std::runtime_error &ex) {
            std::cerr << "Failed initializing client session: " << ex.what() << std::endl << std::flush;
            MetaMsgs::McastMsgMetaAd::poolT.put(ad);
            return -1;
        }

        MetaMsgs::McastMsgMetaAd::poolT.put(ad);
        return ret;
    }

    virtual int processStream(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0)
    {
        if ( debug == true ) std::cout << __FUNCTION__ << " Got tsp req" << std::endl << std::flush;
        return 0;
    }

    virtual int sendDgram(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0)
    {
        if ( debug == true ) std::cout << __FUNCTION__ << " Sending dgram rsp" << std::endl << std::flush;
        return 0;
    }

    virtual int sendStream(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0)
    {
        if ( debug == true ) std::cout << __FUNCTION__ << " Sending tsp rsp" << std::endl << std::flush;
        return 0;
    }

    McastlistCliProcessor(uint32_t _num_io_sessions_per_session = 1,
            bool _mm_sock_session = false,
            bool _debug = false,
            bool _msg_debug = false,
            bool _allow_remote_srv = false,
            MySession::eTestType _test_type = MySession::eTestType::eAdmCont,
            uint32_t _msg_pause = 0)
        :counter(0),
         debug(_debug),
         msg_debug(_msg_debug),
         allow_remote_srv(_allow_remote_srv),
         mm_sock_session(_mm_sock_session),
         test_type(_test_type),
         msg_pause(_msg_pause),
         num_io_sessions_per_session(_num_io_sessions_per_session),
         sess_thread(0)
    {
        own_addr = IPv4Address::findNodeNoLocalAddr();
    }
    ~McastlistCliProcessor() { if ( sess_thread ) delete sess_thread; }

    IPv4Address            srv_addr;   // for communication
    IPv4Address            own_addr;
    uint64_t               counter;
    bool                   debug;
    bool                   msg_debug;
    bool                   allow_remote_srv;
    bool                   mm_sock_session;
    MySession::eTestType   test_type;
    uint32_t               msg_pause;
    uint32_t               num_io_sessions_per_session;
    boost::thread         *sess_thread;
};




static void usage(const char *prog)
{
    std::cerr << prog << ": -m (msg debug) -d (debug) -f (allow remote srv) -p (msg_pause=10000) -i (i/o test) -u (mm socket)" << std::endl << std::flush;
}

//
// Main
//
int main(int argc, char *argv[])
{
    bool debug_mode = false;
    bool msg_debug_mode = false;
    bool allow_remote_srv = false;
    bool mm_sock_session = false;
    uint32_t msg_pause = 0;
    MySession::eTestType test_type = MySession::eTestType::eAdmCont;
    uint32_t num_io_sessions = 1;

    if ( argc > 1 ) {
        for ( int i = 1 ; i < argc ; i++ ) {
            if ( std::string(argv[i]) == "-m" ) {
                msg_debug_mode = true;
            } else if ( std::string(argv[i]) == "-d" ) {
                debug_mode = true;
            } else if ( std::string(argv[i]) == "-f" ) {
                allow_remote_srv = true;
            } else if ( std::string(argv[i]) == "-i" ) {
                test_type = MySession::eTestType::eIOSession;
            } else if ( std::string(argv[i]) == "-p" ) {
                msg_pause = 10000;
            } else if ( std::string(argv[i]) == "-u" ) {
                mm_sock_session = true;
            } else if ( i > 1 && stoul(std::string(argv[i])) < 32 ) {
                if ( std::string(argv[i - 1]) == "-i" ) {
                    num_io_sessions = stoul(std::string(argv[i]));
                } else {
                    usage(argv[0]);
                    exit(-1);
                }
            } else {
                usage(argv[0]);
                exit(-1);
            }
        }
    }

    try {
        MetaNet::IPv4MulticastListener *ipv4mcastlis = MetaNet::IPv4MulticastListener::getInstance(true);
        ipv4mcastlis->setMcastClientMode(true);
        McastlistCliProcessor mcli_processor(num_io_sessions, mm_sock_session, debug_mode, msg_debug_mode, allow_remote_srv, test_type, msg_pause);

        boost::function<CallbackIPv4ProcessFun> m_p1 = boost::bind(&McastlistCliProcessor::processStream, &mcli_processor, _1, _2, _3, _4);
        ipv4mcastlis->setProcessStreamCB(m_p1);
        boost::function<CallbackIPv4ProcessFun> m_p2 = boost::bind(&McastlistCliProcessor::processDgram, &mcli_processor, _1, _2, _3, _4);
        ipv4mcastlis->setProcessDgramCB(m_p2);
        boost::function<CallbackIPv4SendFun> m_s1 = boost::bind(&McastlistCliProcessor::sendStream, &mcli_processor, _1, _2, _3, _4, _5);
        ipv4mcastlis->setSendStreamCB(m_s1);
        boost::function<CallbackIPv4SendFun> m_s2 = boost::bind(&McastlistCliProcessor::sendDgram, &mcli_processor, _1, _2, _3, _4, _5);
        ipv4mcastlis->setSendDgramCB(m_s2);

        boost::thread mcastlis_thread(&MetaNet::IPv4MulticastListener::start, ipv4mcastlis);

        mcastlis_thread.join();
    } catch (const std::runtime_error &ex) {
        std::cerr << "std::runtime_error caught: " << ex.what() << std::endl << std::flush;
    }

    return 0;
}
