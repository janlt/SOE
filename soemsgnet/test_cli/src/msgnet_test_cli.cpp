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

template <class T>
class MyIOSession: public EventCliSession<T>
{
public:
    IPv4Address my_addr;
    uint64_t    counter;
    uint64_t    bytes_received;
    uint64_t    bytes_sent;
    bool        sess_debug;

    uint64_t       recv_count;
    uint64_t       sent_count;
    uint64_t       loop_count;
    boost::thread *io_session_thread;

    Session *parent_session;

    MyIOSession(Point &point, Session *_parent, bool _test = false)
        :EventCliSession<T>(point, _test),
         counter(0),
         bytes_received(0),
         bytes_sent(0),
         sess_debug(false),
         recv_count(0),
         sent_count(0),
         loop_count(0),
         io_session_thread(0),
         parent_session(_parent)
    {
        my_addr = IPv4Address::findNodeNoLocalAddr();
    }

    virtual ~MyIOSession() {}
    virtual int processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);

    void setSessDebug(bool _sess_debug)
    {
        sess_debug = _sess_debug;
    }
    int ParseSetAddresses(const std::string &ad_json);
    int Connect();
    int testAllIOStream(const IPv4Address &from_addr, const MetaNet::Point *pt, uint64_t loop_count = -1);
    virtual void start() throw(std::runtime_error);
};

template <class T>
inline int MyIOSession<T>::processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( sess_debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got msg" << std::endl << std::flush;

    return 0;
}

template <class T>
inline int MyIOSession<T>::processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( sess_debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got msg" << std::endl << std::flush;

    return 0;
}

template <class T>
inline int MyIOSession<T>::sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    if ( sess_debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got msg" << std::endl << std::flush;

    return 0;
}

template <class T>
inline int MyIOSession<T>::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    if ( sess_debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got msg" << std::endl << std::flush;

    return 0;
}

template <class T>
inline int MyIOSession<T>::testAllIOStream(const IPv4Address &from_addr, const MetaNet::Point *pt, uint64_t _loop_count)
{
    loop_count = _loop_count;

    io_session_thread = new boost::thread(&MyIOSession<T>::start, this);
    return 0;
}

template <class T>
inline void MyIOSession<T>::start() throw(std::runtime_error)
{
    for ( uint64_t i = 0 ; i < loop_count ; i++ ) {
        GetReq<TestIov> *gr = MetaMsgs::GetReq<TestIov>::poolT.get();

        std::string c_n = "DUPEK_CLUSTER_" + to_string(i);
        ::strcpy(gr->meta_desc.desc.cluster_name, c_n.c_str());
        gr->meta_desc.desc.cluster_id = 1234;
        if ( gr->SetIovSize(GetReq<TestIov>::cluster_name_idx, static_cast<uint32_t>(::strlen(gr->meta_desc.desc.cluster_name))) ) {
            std::cerr << __PRETTY_FUNCTION__ << " Data too big" << std::endl << std::flush;
        }

        ::strcpy(gr->meta_desc.desc.space_name, "DUPEK_SPACE");
        gr->meta_desc.desc.space_id = 5678;
        if ( gr->SetIovSize(GetReq<TestIov>::space_name_idx, static_cast<uint32_t>(::strlen(gr->meta_desc.desc.space_name))) ) {
            std::cerr << __PRETTY_FUNCTION__ << " Data too big" << std::endl << std::flush;
        }

        ::strcpy(gr->meta_desc.desc.store_name, "DUPAK_STORE");
        gr->meta_desc.desc.store_id = 9012;
        if ( gr->SetIovSize(GetReq<TestIov>::store_name_idx, static_cast<uint32_t>(::strlen(gr->meta_desc.desc.store_name))) ) {
            std::cerr << __PRETTY_FUNCTION__ << " Data too big" << std::endl << std::flush;
        }

        ::strcpy(gr->meta_desc.desc.key, "TESTKEY");
        gr->meta_desc.desc.key_len = strlen(gr->meta_desc.desc.key);
        if ( gr->SetIovSize(GetReq<TestIov>::key_idx, gr->meta_desc.desc.key_len) ) {
            std::cerr << __PRETTY_FUNCTION__ << " Data too big" << std::endl << std::flush;
        }

        ::strcpy(gr->meta_desc.desc.end_key, "TESTENDKEY");
        if ( gr->SetIovSize(GetReq<TestIov>::end_key_idx, gr->meta_desc.desc.key_len) ) {
            std::cerr << __PRETTY_FUNCTION__ << " Data too big" << std::endl << std::flush;
        }

        ::memset(gr->meta_desc.desc.data, 'D', 1024);
        gr->meta_desc.desc.data_len = 1024;
        if ( gr->SetIovSize(GetReq<TestIov>::data_idx, gr->meta_desc.desc.data_len) ) {
            std::cerr << __PRETTY_FUNCTION__ << " Data too big" << std::endl << std::flush;
        }

        gr->meta_desc.desc.buf_len = 0;
        if ( gr->SetIovSize(GetReq<TestIov>::buf_idx, gr->meta_desc.desc.buf_len) ) {
            std::cerr << __PRETTY_FUNCTION__ << " Data too big" << std::endl << std::flush;
        }

        int ret = EventSession<T>::event_point.SendIov(*gr);
        if ( ret < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " send failed type:" << gr->iovhdr.hdr.type << std::endl << std::flush;
        }
        sent_count++;
        MetaMsgs::GetReq<TestIov>::poolT.put(gr);

        GetRsp<TestIov> *gp = MetaMsgs::GetRsp<TestIov>::poolT.get();
        ret = EventSession<T>::event_point.RecvIov(*gp);
        if ( ret < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " recv failed type:" << gp->iovhdr.hdr.type << std::endl << std::flush;
        }
        recv_count++;

        if ( sess_debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got GetRsp<TestIov> " <<
            " cluster: " << gp->meta_desc.desc.cluster_name <<
            " space: " << gp->meta_desc.desc.space_name <<
            " store: " << gp->meta_desc.desc.store_name <<
            std::endl << std::flush;

        if ( !(i%0x10000) ) {
            std::cout << "sent_count: " << sent_count << " recv_count: " << recv_count << std::endl << std::flush;
        }
        MetaMsgs::GetRsp<TestIov>::poolT.put(gp);
    }
}




//
// Client Session
//
class MySession: public IPv4TCPCliSession
{
public:
    IPv4Address          own_addr;
    uint64_t             counter;
    bool                 started;

    const static uint32_t                  max_sessions = 1024;
    std::shared_ptr<MyIOSession<TestIov> > io_sessions[max_sessions];
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

    IPv4Address io_peer_addr;

    MySession(TcpSocket &soc, eTestType _test_type, bool _unix_sock_session = true, bool _debug = false, bool _test = false)
        :IPv4TCPCliSession(soc, _test),
         counter(0),
         started(false),
         num_sessions(0),
         test_type(_test_type),
         run_count(-1),
         unix_sock_session(_unix_sock_session)
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

    void contAdmTest() throw(std::runtime_error);
    void contIOTest() throw(std::runtime_error);
    void ioSessionTest() throw(std::runtime_error);
    int testAllAdmStream(const IPv4Address &from_addr, const MetaNet::Point *pt, uint64_t loop_count = -1);
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

void MySession::contAdmTest() throw(std::runtime_error)
{
    int ret = 0;
    for ( uint64_t i = 0 ; i < run_count ; i++ ) {
        if ( peer_addr != IPv4Address::null_address ) {
            ret = testAllAdmStream(peer_addr, &tcp_sock);
            if ( debug == true ) std::cout << "Got ret: " << ret << " errno: " << errno << std::endl << std::flush;

            // Upon server disconnect we deinitialize->close
            if ( ret == -ECONNRESET ) {
                std::cerr << "Server disconnected or dead" << std::endl << std::flush;
                deinitialise(true);
                peer_addr = IPv4Address::null_address;  // reset server address to NULL address
            }
        } else {
            std::cerr << "Server still disconnected or dead" << std::endl << std::flush;
            usleep(1000000);
        }
    }
}

void MySession::contIOTest() throw(std::runtime_error)
{
    int ret = 0;
    for ( uint64_t i = 0 ; i < run_count ; i++ ) {
        if ( peer_addr == IPv4Address::null_address ) {
            std::cerr << __PRETTY_FUNCTION__ << " Server still disconnected or dead" << std::endl << std::flush;
            usleep(1000000);
        }

        for ( uint32_t s = 0 ; s < num_sessions ; s++ ) {
            ret = io_sessions[s]->testAllIOStream(io_peer_addr, &tcp_sock);
            if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got ret: " << ret << " errno: " << errno << std::endl << std::flush;

            // Upon server disconnect we deinitialize->close
            if ( ret == -ECONNRESET ) {
                std::cerr << __PRETTY_FUNCTION__ << " Server disconnected or dead" << std::endl << std::flush;
                deinitialise(true);
                peer_addr = IPv4Address::null_address;  // reset server address to NULL address
            }
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

    io_sessions[_idx].reset(new MyIOSession<TestIov>(*ss, this));
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
    return 0;
}

int MySession::CreateLocalMMSockIOSession(const IPv4Address &_io_peer_addr, int _idx)
{
    bool _clean_up = false;
    MmPoint *ss = new MmPoint;

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

    io_sessions[_idx].reset(new MyIOSession<TestIov>(*ss, this));
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

    io_sessions[_idx].reset(new MyIOSession<TestIov>(*ss, this));
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
        std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgAssignMMChannelReq: " << ret << std::endl << std::flush;
        MetaMsgs::AdmMsgAssignMMChannelReq::poolT.put(mm_req);
        return;
    }

    MetaMsgs::AdmMsgAssignMMChannelReq::poolT.put(mm_req);

    MetaMsgs::AdmMsgBase base_msg;
    MetaMsgs::AdmMsgBuffer buf(true);
    buf.reset();
    ret = base_msg.recv(tcp_sock.GetDesc(), buf);
    if ( ret < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgBase hdr" << std::endl << std::flush;
        return;
    }

    MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
    if ( !recv_msg ) {
        std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgBase hdr" << std::endl << std::flush;
    }
    recv_msg->hdr = base_msg.hdr;

    MetaMsgs::AdmMsgAssignMMChannelRsp *mm_rsp = dynamic_cast<MetaMsgs::AdmMsgAssignMMChannelRsp *>(recv_msg);
    ret = mm_rsp->recv(tcp_sock.GetDesc(), buf);
    if ( ret < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgAssignMMChannelRsp" << std::endl << std::flush;
    } else {
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgAssignMMChannelRsp req" <<
            " pid: " << mm_rsp->pid <<
            " path: " << mm_rsp->path <<
            " counter: " << counter++ << std::endl << std::flush;
    }

    // create io socket and IO session
    for ( uint32_t s = 0 ; s < num_sessions ; s++ ) {
        int ret = 0;
        if ( std::string(own_addr.GetNTOASinAddr()) == std::string(io_peer_addr.GetNTOASinAddr()) && unix_sock_session == true ) {
            ret = CreateLocalUnixSockIOSession(io_peer_addr, s);
        } else if (  std::string(own_addr.GetNTOASinAddr()) == std::string(io_peer_addr.GetNTOASinAddr()) && unix_sock_session == false ) {
            ret = CreateLocalMMSockIOSession(io_peer_addr, s);
        } else {
            ret = CreateRemoteIOSession(io_peer_addr, s);
        }

        if ( ret < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " Create I/O session failed own_addr: " << own_addr.GetNTOASinAddr() <<
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
        case MySession::eTestType::eAdmCont:
            contAdmTest();
            break;
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
    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got json: " << ad_json << std::endl << std::flush;

    MsgNetMcastAdvertisement ad_msg(ad_json);
    int ret = ad_msg.Parse();
    if ( ret ) {
        std::cerr << __PRETTY_FUNCTION__ << " parse failed " << std::endl << std::flush;
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
    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Invalid invocation" << std::endl << std::flush;
    return -1;
}

int MySession::processStreamMessage(MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got msg" << std::endl << std::flush;
    return 0;
}

int MySession::sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Invalid invocation" << std::endl << std::flush;
    return -1;
}

int MySession::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Sending msg" << std::endl << std::flush;
    return 0;
}

int MySession::testAllAdmStream(const IPv4Address &from_addr, const MetaNet::Point *pt, uint64_t loop_count)
{
    int ret = 0;

    // send AdmMsgAssignMMChannelReq and expect a response
    MetaMsgs::AdmMsgAssignMMChannelReq *mm_req = MetaMsgs::AdmMsgAssignMMChannelReq::poolT.get();
    mm_req->pid = getpid();

    ret = mm_req->send(tcp_sock.GetDesc());
    if ( ret < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgAssignMMChannelReq: " << ret << std::endl << std::flush;
        MetaMsgs::AdmMsgAssignMMChannelReq::poolT.put(mm_req);
        return ret;
    }

    MetaMsgs::AdmMsgAssignMMChannelReq::poolT.put(mm_req);

    try {
        for ( uint64_t ii = 0 ; ii < loop_count ; ii++ ) {
            MetaMsgs::AdmMsgBase base_msg;
            MetaMsgs::AdmMsgBuffer buf(true);
            buf.reset();
            int ret = base_msg.recv(tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgBase hdr" << std::endl << std::flush;
                return ret;
            }

            MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
            if ( !recv_msg ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgBase hdr" << std::endl << std::flush;
            }
            recv_msg->hdr = base_msg.hdr;

            switch ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) ) {
            case eMsgAssignMMChannelRsp:
            {
                MetaMsgs::AdmMsgAssignMMChannelRsp *mm_rsp = dynamic_cast<MetaMsgs::AdmMsgAssignMMChannelRsp *>(recv_msg);
                ret = mm_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgAssignMMChannelRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgAssignMMChannelRsp req" <<
                        " pid: " << mm_rsp->pid <<
                        " path: " << mm_rsp->path <<
                        " counter: " << counter++ << std::endl << std::flush;
                }



                MetaMsgs::AdmMsgCreateClusterReq *cc_req = MetaMsgs::AdmMsgCreateClusterReq::poolT.get();
                cc_req->cluster_name = "cluster_one";

                ret = cc_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgCreateClusterReq: " << ret << std::endl << std::flush;
                }
                MetaMsgs::AdmMsgCreateClusterReq::poolT.put(cc_req);
            }
            break;
            case eMsgCreateClusterRsp:
            {
                MetaMsgs::AdmMsgCreateClusterRsp *cc_rsp = dynamic_cast<MetaMsgs::AdmMsgCreateClusterRsp *>(recv_msg);
                ret = cc_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgCreateClusterRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgCreateClusterRsp req" <<
                        " cluster_id: " << cc_rsp->cluster_id <<
                        " counter: " << counter++ << std::endl << std::flush;
                }



                MetaMsgs::AdmMsgCreateSpaceReq *cs_req = MetaMsgs::AdmMsgCreateSpaceReq::poolT.get();
                cs_req->cluster_name = "cluster_one";
                cs_req->space_name = "space_one";

                ret = cs_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgCreateClusterReq: " << ret << std::endl << std::flush;
                }
                MetaMsgs::AdmMsgCreateSpaceReq::poolT.put(cs_req);
            }
            break;
            case eMsgCreateSpaceRsp:
            {
                MetaMsgs::AdmMsgCreateSpaceRsp *cs_rsp = dynamic_cast<MetaMsgs::AdmMsgCreateSpaceRsp *>(recv_msg);
                ret = cs_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgCreateSpaceRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgCreateSpaceRsp req" <<
                        " cluster_id: " << cs_rsp->cluster_id <<
                        " space_id: " << cs_rsp->space_id <<
                        " counter: " << counter++ << std::endl << std::flush;
                }



                MetaMsgs::AdmMsgCreateStoreReq *ct_req = MetaMsgs::AdmMsgCreateStoreReq::poolT.get();
                ct_req->cluster_name = "cluster_one";
                ct_req->space_name = "space_one";
                ct_req->store_name = "store_one";

                ret = ct_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgCreateStoreReq: " << ret << std::endl << std::flush;
                }
                MetaMsgs::AdmMsgCreateStoreReq::poolT.put(ct_req);
            }
            break;
            case eMsgCreateStoreRsp:
            {
                MetaMsgs::AdmMsgCreateStoreRsp *ct_rsp = dynamic_cast<MetaMsgs::AdmMsgCreateStoreRsp *>(recv_msg);
                ret = ct_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgCreateStoreRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgCreateStoreRsp req" <<
                        " cluster_id: " << ct_rsp->cluster_id <<
                        " space_id: " << ct_rsp->space_id <<
                        " store_id: " << ct_rsp->store_id <<
                        " counter: " << counter++ << std::endl << std::flush;
                }



                MetaMsgs::AdmMsgDeleteClusterReq *dc_req = MetaMsgs::AdmMsgDeleteClusterReq::poolT.get();
                dc_req->cluster_name = "cluster_one";

                ret = dc_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgDeleteClusterReq: " << ret << std::endl << std::flush;
                    MetaMsgs::AdmMsgAssignMMChannelReq::poolT.put(mm_req);
                    return -1;
                }

                MetaMsgs::AdmMsgDeleteClusterReq::poolT.put(dc_req);
            }
            break;
            case eMsgDeleteClusterRsp:
            {
                MetaMsgs::AdmMsgDeleteClusterRsp *dc_rsp = dynamic_cast<MetaMsgs::AdmMsgDeleteClusterRsp *>(recv_msg);
                ret = dc_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgDeleteClusterRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgDeleteClusterRsp req" <<
                        " cluster_id: " << dc_rsp->cluster_id <<
                        " counter: " << counter++ << std::endl << std::flush;
                }



                MetaMsgs::AdmMsgDeleteSpaceReq *ds_req = MetaMsgs::AdmMsgDeleteSpaceReq::poolT.get();
                ds_req->cluster_name = "cluster_one";
                ds_req->space_name = "space_one";

                ret = ds_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgDeleteSpaceReq: " << ret << std::endl << std::flush;
                    MetaMsgs::AdmMsgDeleteSpaceReq::poolT.put(ds_req);
                    return -1;
                }

                MetaMsgs::AdmMsgDeleteSpaceReq::poolT.put(ds_req);
            }
            break;
            case eMsgDeleteSpaceRsp:
            {
                MetaMsgs::AdmMsgDeleteSpaceRsp *ds_rsp = dynamic_cast<MetaMsgs::AdmMsgDeleteSpaceRsp *>(recv_msg);
                ret = ds_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgDeleteSpaceRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgDeleteSpaceRsp req" <<
                        " cluster_id: " << ds_rsp->cluster_id <<
                        " space_id: " << ds_rsp->space_id <<
                        " counter: " << counter++ << std::endl << std::flush;
                }



                MetaMsgs::AdmMsgDeleteStoreReq *dt_req = MetaMsgs::AdmMsgDeleteStoreReq::poolT.get();
                dt_req->cluster_name = "cluster_one";
                dt_req->space_name = "space_one";
                dt_req->space_name = "store_one";

                ret = dt_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgDeleteStoreReq: " << ret << std::endl << std::flush;
                    MetaMsgs::AdmMsgDeleteStoreReq::poolT.put(dt_req);
                    return -1;
                }

                MetaMsgs::AdmMsgDeleteStoreReq::poolT.put(dt_req);
            }
            break;
            case eMsgDeleteStoreRsp:
            {
                MetaMsgs::AdmMsgDeleteStoreRsp *dt_rsp = dynamic_cast<MetaMsgs::AdmMsgDeleteStoreRsp *>(recv_msg);
                ret = dt_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgDeleteStoreRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgDeleteStoreRsp req" <<
                        " cluster_id: " << dt_rsp->cluster_id <<
                        " space_id: " << dt_rsp->space_id <<
                        " store_id: " << dt_rsp->store_id <<
                        " counter: " << counter++ << std::endl << std::flush;
                }



                MetaMsgs::AdmMsgCreateBackupReq *cb_req = MetaMsgs::AdmMsgCreateBackupReq::poolT.get();
                cb_req->cluster_name = "cluster_one";
                cb_req->space_name = "space_one";
                cb_req->space_name = "store_one";
                cb_req->cluster_id = 10;
                cb_req->space_id = 1000;
                cb_req->store_id = 100000;
                cb_req->backup_name = "backup_one";

                ret = cb_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgCreateBackupReq: " << ret << std::endl << std::flush;
                    MetaMsgs::AdmMsgCreateBackupReq::poolT.put(cb_req);
                    return -1;
                }

                MetaMsgs::AdmMsgCreateBackupReq::poolT.put(cb_req);
            }
            break;
            case eMsgCreateBackupRsp:
            {
                MetaMsgs::AdmMsgCreateBackupRsp *cb_rsp = dynamic_cast<MetaMsgs::AdmMsgCreateBackupRsp *>(recv_msg);
                ret = cb_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgCreateBackupRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgCreateBackupRsp req" <<
                        " backup_id: " << cb_rsp->backup_id <<
                        " counter: " << counter++ << std::endl << std::flush;
                }



                MetaMsgs::AdmMsgDeleteBackupReq *db_req = MetaMsgs::AdmMsgDeleteBackupReq::poolT.get();
                db_req->cluster_name = "cluster_one";
                db_req->space_name = "space_one";
                db_req->space_name = "store_one";
                db_req->cluster_id = 10;
                db_req->space_id = 1000;
                db_req->store_id = 100000;
                db_req->backup_name = "backup_one";
                db_req->backup_id = 2222;

                ret = db_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgDeleteBackupReq: " << ret << std::endl << std::flush;
                    MetaMsgs::AdmMsgDeleteBackupReq::poolT.put(db_req);
                    return -1;
                }

                MetaMsgs::AdmMsgDeleteBackupReq::poolT.put(db_req);
            }
            break;
            case eMsgDeleteBackupRsp:
            {
                MetaMsgs::AdmMsgDeleteBackupRsp *db_rsp = dynamic_cast<MetaMsgs::AdmMsgDeleteBackupRsp *>(recv_msg);
                ret = db_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgDeleteBackupRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgDeleteBackupRsp req" <<
                        " backup_id: " << db_rsp->backup_id <<
                        " counter: " << counter++ << std::endl << std::flush;
                }



                MetaMsgs::AdmMsgListBackupsReq *lbs_req = MetaMsgs::AdmMsgListBackupsReq::poolT.get();
                lbs_req->cluster_name = "cluster_one";
                lbs_req->space_name = "space_one";
                lbs_req->space_name = "store_one";
                lbs_req->cluster_id = 10;
                lbs_req->space_id = 1000;
                lbs_req->store_id = 100000;

                ret = lbs_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgListBackupsReq: " << ret << std::endl << std::flush;
                    MetaMsgs::AdmMsgListBackupsReq::poolT.put(lbs_req);
                    return -1;
                }

                MetaMsgs::AdmMsgListBackupsReq::poolT.put(lbs_req);
            }
            break;
            case eMsgListBackupsRsp:
            {
                MetaMsgs::AdmMsgListBackupsRsp *lbs_rsp = dynamic_cast<MetaMsgs::AdmMsgListBackupsRsp *>(recv_msg);
                ret = lbs_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgListBackupsRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) {
                        std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgListBackupsRsp req" <<
                        " num_backups: " << lbs_rsp->num_backups <<
                        " counter: " << counter++ << std::endl << std::flush;

                        for ( auto it = lbs_rsp->backups.begin() ; it != lbs_rsp->backups.end() ; it++ ) {
                            std::cout << __PRETTY_FUNCTION__  <<
                                " backup_id: " << it->first <<
                                " backup_name: " << it->second << std::endl << std::flush;
                        }
                    }
                }
                lbs_rsp->backups.erase(lbs_rsp->backups.begin(), lbs_rsp->backups.end());
                lbs_rsp->num_backups = 0;



                MetaMsgs::AdmMsgCreateSnapshotReq *csn_req = MetaMsgs::AdmMsgCreateSnapshotReq::poolT.get();
                csn_req->cluster_name = "cluster_one";
                csn_req->space_name = "space_one";
                csn_req->space_name = "store_one";
                csn_req->cluster_id = 10;
                csn_req->space_id = 1000;
                csn_req->store_id = 100000;
                csn_req->snapshot_name = "snapshot_one";

                ret = csn_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgCreateSnapshotReq: " << ret << std::endl << std::flush;
                    MetaMsgs::AdmMsgCreateSnapshotReq::poolT.put(csn_req);
                    return -1;
                }

                MetaMsgs::AdmMsgCreateSnapshotReq::poolT.put(csn_req);
            }
            break;
            case eMsgCreateSnapshotRsp:
            {
                MetaMsgs::AdmMsgCreateSnapshotRsp *csn_rsp = dynamic_cast<MetaMsgs::AdmMsgCreateSnapshotRsp *>(recv_msg);
                ret = csn_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgCreateSnapshotRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgCreateSnapshotRsp req" <<
                        " snapshot_id: " << csn_rsp->snapshot_id <<
                        " counter: " << counter++ << std::endl << std::flush;
                }



                MetaMsgs::AdmMsgDeleteSnapshotReq *dsn_req = MetaMsgs::AdmMsgDeleteSnapshotReq::poolT.get();
                dsn_req->cluster_name = "cluster_one";
                dsn_req->space_name = "space_one";
                dsn_req->space_name = "store_one";
                dsn_req->cluster_id = 10;
                dsn_req->space_id = 1000;
                dsn_req->store_id = 100000;
                dsn_req->snapshot_name = "snapshot_one";
                dsn_req->snapshot_id = 4444;

                ret = dsn_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgDeleteSnapshotReq: " << ret << std::endl << std::flush;
                    MetaMsgs::AdmMsgDeleteSnapshotReq::poolT.put(dsn_req);
                    return -1;
                }

                MetaMsgs::AdmMsgDeleteSnapshotReq::poolT.put(dsn_req);
            }
            break;
            case eMsgDeleteSnapshotRsp:
            {
                MetaMsgs::AdmMsgDeleteSnapshotRsp *dsn_rsp = dynamic_cast<MetaMsgs::AdmMsgDeleteSnapshotRsp *>(recv_msg);
                ret = dsn_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgDeleteSnapshotRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgDeleteSnapshotRsp req" <<
                        " snapshot_id: " << dsn_rsp->snapshot_id <<
                        " counter: " << counter++ << std::endl << std::flush;
                }



                MetaMsgs::AdmMsgListSnapshotsReq *lsns_req = MetaMsgs::AdmMsgListSnapshotsReq::poolT.get();
                lsns_req->cluster_name = "cluster_one";
                lsns_req->space_name = "space_one";
                lsns_req->space_name = "store_one";
                lsns_req->cluster_id = 10;
                lsns_req->space_id = 1000;
                lsns_req->store_id = 100000;

                ret = lsns_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgListSnapshotsReq: " << ret << std::endl << std::flush;
                    MetaMsgs::AdmMsgListSnapshotsReq::poolT.put(lsns_req);
                    return -1;
                }

                MetaMsgs::AdmMsgListSnapshotsReq::poolT.put(lsns_req);
            }
            break;
            case eMsgListSnapshotsRsp:
            {
                MetaMsgs::AdmMsgListSnapshotsRsp *lsns_rsp = dynamic_cast<MetaMsgs::AdmMsgListSnapshotsRsp *>(recv_msg);
                ret = lsns_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgListSnapshotsRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) {
                        std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgListSnapshotsRsp req" <<
                        " num_snapshots: " << lsns_rsp->num_snapshots <<
                        " counter: " << counter++ << std::endl << std::flush;

                        for ( auto it = lsns_rsp->snapshots.begin() ; it != lsns_rsp->snapshots.end() ; it++ ) {
                            std::cout << __PRETTY_FUNCTION__  <<
                                " snapshot_id: " << it->first <<
                                " snapshot_name: " << it->second << std::endl << std::flush;
                        }
                    }
                }
                lsns_rsp->snapshots.erase(lsns_rsp->snapshots.begin(), lsns_rsp->snapshots.end());
                lsns_rsp->num_snapshots = 0;



                MetaMsgs::AdmMsgOpenStoreReq *os_req = MetaMsgs::AdmMsgOpenStoreReq::poolT.get();
                os_req->cluster_name = "cluster_one";
                os_req->space_name = "space_one";
                os_req->space_name = "store_one";
                os_req->cluster_id = 10;
                os_req->space_id = 1000;
                os_req->store_id = 100000;

                ret = os_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgOpenStoreReq: " << ret << std::endl << std::flush;
                    MetaMsgs::AdmMsgOpenStoreReq::poolT.put(os_req);
                    return -1;
                }

                MetaMsgs::AdmMsgOpenStoreReq::poolT.put(os_req);
            }
            break;
            case eMsgOpenStoreRsp:
            {
                MetaMsgs::AdmMsgOpenStoreRsp *os_rsp = dynamic_cast<MetaMsgs::AdmMsgOpenStoreRsp *>(recv_msg);
                ret = os_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgOpenStoreRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgOpenStoreRsp req" <<
                        " store_id: " << os_rsp->store_id <<
                        " counter: " << counter++ << std::endl << std::flush;
                }



                MetaMsgs::AdmMsgCloseStoreReq *cs_req = MetaMsgs::AdmMsgCloseStoreReq::poolT.get();
                cs_req->cluster_name = "cluster_one";
                cs_req->space_name = "space_one";
                cs_req->space_name = "store_one";
                cs_req->cluster_id = 10;
                cs_req->space_id = 1000;
                cs_req->store_id = 100000;

                ret = cs_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgCloseStoreReq: " << ret << std::endl << std::flush;
                    MetaMsgs::AdmMsgCloseStoreReq::poolT.put(cs_req);
                    return -1;
                }

                MetaMsgs::AdmMsgCloseStoreReq::poolT.put(cs_req);
            }
            break;
            case eMsgCloseStoreRsp:
            {
                MetaMsgs::AdmMsgCloseStoreRsp *cs_rsp = dynamic_cast<MetaMsgs::AdmMsgCloseStoreRsp *>(recv_msg);
                ret = cs_rsp->recv(tcp_sock.GetDesc(), buf);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgCloseStoreRsp" << std::endl << std::flush;
                } else {
                    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgCloseStoreRsp req" <<
                        " store_id: " << cs_rsp->store_id <<
                        " counter: " << counter++ << std::endl << std::flush;
                }



                // send AdmMsgAssignMMChannelReq and expect a response
                MetaMsgs::AdmMsgAssignMMChannelReq *mm_req = MetaMsgs::AdmMsgAssignMMChannelReq::poolT.get();
                mm_req->pid = getpid();

                ret = mm_req->send(tcp_sock.GetDesc());
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgAssignMMChannelReq: " << ret << std::endl << std::flush;
                    MetaMsgs::AdmMsgAssignMMChannelReq::poolT.put(mm_req);
                    return -1;
                }

                MetaMsgs::AdmMsgAssignMMChannelReq::poolT.put(mm_req);
            }
            break;
            default:
                std::cerr << __PRETTY_FUNCTION__ << " Receive invalid AdmMsgBase hdr" << std::endl << std::flush;
                return -1;
            }

            MetaMsgs::AdmMsgFactory::getInstance()->put(recv_msg);
        }
    } catch (const std::runtime_error &ex) {
        std::cerr << "Failed test all loop: " << ex.what() << std::endl << std::flush;
    }
    return ret;
}

//
// Mcast msg Processor
//
struct McastlistCliProcessor: public Processor
{
    int processAdvertisement(MetaMsgs::McastMsgMetaAd *ad, const IPv4Address &from_addr, const MetaNet::Point *pt = 0)
    {
        int ret = 0;

        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got ad msg: " << ad->json << std::endl << std::flush;

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
                sess = new MySession(*ss, test_type, mm_sock_session, debug);
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
                sess = new MySession(*ss, test_type, mm_sock_session, debug);
                sess->setNumSessions(num_io_sessions_per_session);
            } catch (const std::runtime_error &ex) {
                std::cerr << "Fatal std::runtime_error caught: " << ex.what() << " exiting ... " << std::endl << std::flush;
                exit(-1);
            }

            ret = sess->ParseSetAddresses(ad->json);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Invalid mcast ad: " << ret << std::endl << std::flush;
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
            boost::thread sess_thread(&MySession::start, sess);
        }

        return ret;
    }

    // This callback runs session recovery logic as well
    virtual int processDgram(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0)
    {
        if ( !_buf.getSize() ) { // timeout - ignore
            return 0;
        }

        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got dgram req" << std::endl << std::flush;
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
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got tsp req" << std::endl << std::flush;
        return 0;
    }

    virtual int sendDgram(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0)
    {
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Sending dgram rsp" << std::endl << std::flush;
        return 0;
    }

    virtual int sendStream(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0)
    {
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Sending tsp rsp" << std::endl << std::flush;
        return 0;
    }

    McastlistCliProcessor(uint32_t _num_io_sessions_per_session = 1,
            bool _mm_sock_session = false,
            bool _debug = false,
            bool _msg_debug = false,
            bool _allow_remote_srv = false,
            MySession::eTestType _test_type = MySession::eTestType::eAdmCont)
        :counter(0),
         debug(_debug),
         msg_debug(_msg_debug),
         allow_remote_srv(_allow_remote_srv),
         mm_sock_session(_mm_sock_session),
         test_type(_test_type),
         num_io_sessions_per_session(_num_io_sessions_per_session)
    {
        own_addr = IPv4Address::findNodeNoLocalAddr();
    }
    ~McastlistCliProcessor() {}

    IPv4Address            srv_addr;   // for communication
    IPv4Address            own_addr;
    uint64_t               counter;
    bool                   debug;
    bool                   msg_debug;
    bool                   allow_remote_srv;
    bool                   mm_sock_session;
    MySession::eTestType   test_type;
    uint32_t               num_io_sessions_per_session;
};




static void usage(const char *prog)
{
    std::cerr << prog << ": -m (msg debug) -d (debug) -f (allow remote srv) -i (i/o test) -u (mm socket)" << std::endl << std::flush;
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
        McastlistCliProcessor mcli_processor(num_io_sessions, mm_sock_session, debug_mode, msg_debug_mode, allow_remote_srv, test_type);

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
