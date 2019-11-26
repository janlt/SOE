/*
 * msgnet_test_srv.cpp
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
#include "msgnetlistener.hpp"
#include "msgneteventlistener.hpp"
#include "msgnetprocessor.hpp"
#include "msgnetipv4listener.hpp"
#include "msgnetipv4mcastlistener.hpp"
#include "msgnetsession.hpp"
#include "msgnetipv4tcpsession.hpp"
#include "msgnetipv4tcpsrvsession.hpp"
#include "msgneteventsession.hpp"
#include "msgneteventsrvsession.hpp"

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
class MyIOSession: public EventSrvSession<T>
{
public:
    IPv4Address my_addr;
    uint64_t    counter;
    uint64_t    bytes_received;
    uint64_t    bytes_sent;
    bool        sess_debug;

    uint64_t    msg_recv_count;
    uint64_t    msg_sent_count;
    uint64_t    bytes_recv_count;
    uint64_t    bytes_sent_count;
    static const uint64_t print_divider = 0x10000;

    const static uint32_t                   max_sessions = 1024;
    static std::shared_ptr<MyIOSession<T> > sessions[max_sessions];
    static uint32_t                         session_counter;

    uint32_t    session_id;

    Session *parent_session;

    MyIOSession(Point &point, Session *_parent, bool _test = false)
        :EventSrvSession<T>(point, _test),
         counter(0),
         bytes_received(0),
         bytes_sent(0),
         sess_debug(false),
         msg_recv_count(0),
         msg_sent_count(0),
         bytes_recv_count(0),
         bytes_sent_count(0),
         session_id(session_counter++),
         parent_session(_parent)
    {
        my_addr = IPv4Address::findNodeNoLocalAddr();
    }

    virtual ~MyIOSession() {}
    virtual int processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);


    int ParseSetAddresses(const std::string &ad_json);
    int Connect();

    void setSessionDebug(bool _sess_debug)
    {
        sess_debug = _sess_debug;
    }
};

template <class T>
std::shared_ptr<MyIOSession<T> > MyIOSession<T>::sessions[max_sessions];

template <class T>
uint32_t MyIOSession<T>::session_counter = 0;

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

    Point *c_pt = const_cast<Point *>(pt);

    GetReq<TestIov> *gr = MetaMsgs::GetReq<TestIov>::poolT.get();

    int ret = c_pt->RecvIov(*gr);
    if ( ret < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " recv failed type:" << gr->iovhdr.hdr.type << " ret: " << ret << std::endl << std::flush;

        // Check if it's connection reset so we can remove the session from the listener
        if ( ret == -ECONNRESET ) {
            MetaMsgs::GetReq<TestIov>::poolT.put(gr);
            return ret;
        }
    }
    msg_recv_count++;
    bytes_recv_count += static_cast<uint64_t>(gr->iovhdr.hdr.total_length);

    if ( sess_debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got GetReq<TestIov> " <<
        " cluster: " << gr->meta_desc.desc.cluster_name <<
        " space: " << gr->meta_desc.desc.space_name <<
        " store: " << gr->meta_desc.desc.store_name <<
        std::endl << std::flush;

    MetaMsgs::GetReq<TestIov>::poolT.put(gr);

    GetRsp<TestIov> *gp = MetaMsgs::GetRsp<TestIov>::poolT.get();

    ::strcpy(gp->meta_desc.desc.cluster_name, "SRV_DUPEK_CLUSTER");
    gp->meta_desc.desc.cluster_id = 1234;
    if ( gp->SetIovSize(GetReq<TestIov>::cluster_name_idx, static_cast<uint32_t>(::strlen(gp->meta_desc.desc.cluster_name))) ) {
        std::cerr << __PRETTY_FUNCTION__ << " Data too big" << std::endl << std::flush;
    }

    ::strcpy(gp->meta_desc.desc.space_name, "SRV_DUPEK_SPACE");
    gp->meta_desc.desc.space_id = 5678;
    if ( gp->SetIovSize(GetReq<TestIov>::space_name_idx, static_cast<uint32_t>(::strlen(gp->meta_desc.desc.space_name))) ) {
        std::cerr << __PRETTY_FUNCTION__ << " Data too big" << std::endl << std::flush;
    }

    ::strcpy(gp->meta_desc.desc.store_name, "SRV_DUPAK_STORE");
    gp->meta_desc.desc.store_id = 9012;
    if ( gp->SetIovSize(GetReq<TestIov>::store_name_idx, static_cast<uint32_t>(::strlen(gp->meta_desc.desc.store_name))) ) {
        std::cerr << __PRETTY_FUNCTION__ << " Data too big" << std::endl << std::flush;
    }

    ::strcpy(gp->meta_desc.desc.key, "SRV_TESTKEY");
    gp->meta_desc.desc.key_len = strlen(gp->meta_desc.desc.key);
    if ( gp->SetIovSize(GetReq<TestIov>::key_idx, gp->meta_desc.desc.key_len) ) {
        std::cerr << __PRETTY_FUNCTION__ << " Data too big" << std::endl << std::flush;
    }

    ::strcpy(gp->meta_desc.desc.end_key, "SRV_TESTENDKEY");
    if ( gp->SetIovSize(GetReq<TestIov>::end_key_idx, gp->meta_desc.desc.key_len) ) {
        std::cerr << __PRETTY_FUNCTION__ << " Data too big" << std::endl << std::flush;
    }

    gp->meta_desc.desc.data_len = 0;
    if ( gp->SetIovSize(GetReq<TestIov>::data_idx, gp->meta_desc.desc.data_len) ) {
        std::cerr << __PRETTY_FUNCTION__ << " Data too big" << std::endl << std::flush;
    }

    ::memset(gp->meta_desc.desc.buf, 'B', 2024);
    gp->meta_desc.desc.buf_len = 1024;
    if ( gp->SetIovSize(GetReq<TestIov>::buf_idx, gp->meta_desc.desc.buf_len) ) {
        std::cerr << __PRETTY_FUNCTION__ << " Data too big" << std::endl << std::flush;
    }

    ret = c_pt->SendIov(*gp);
    if ( ret < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " send failed type: " << gp->iovhdr.hdr.type << " ret: " << ret << std::endl << std::flush;
    }
    msg_sent_count++;
    bytes_sent_count += static_cast<uint64_t>(gp->iovhdr.hdr.total_length);

    if ( !(msg_sent_count%print_divider) ) {
        std::cout << "s_id: " << session_id << " msg_sent_count: " << msg_sent_count <<
            " msg_recv_count: " << msg_recv_count <<
            " bytes_sent_count: " << bytes_sent_count <<
            " bytes_recv_count: " << bytes_recv_count << std::endl << std::flush;
    }

    MetaMsgs::GetRsp<TestIov>::poolT.put(gp);

    return ret;
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


//
// Adm Session
//
class MySession: public IPv4TCPSrvSession
{
public:
    IPv4Address my_addr;
    uint64_t    counter;
    uint64_t    bytes_received;
    uint64_t    bytes_sent;
    bool        sess_debug;

    const static uint32_t                  max_sessions = 1024;
    static std::shared_ptr<MySession>      sessions[max_sessions];

    MySession(TcpSocket &soc, bool _test = false)
        :IPv4TCPSrvSession(soc, _test),
         counter(0),
         bytes_received(0),
         bytes_sent(0),
         sess_debug(false)
    {
        my_addr = IPv4Address::findNodeNoLocalAddr();
    }

    virtual ~MySession() {}
    virtual int processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0);
    virtual int sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);
    virtual int sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt = 0);


    int ParseSetAddresses(const std::string &ad_json);
    int Connect();
};

std::shared_ptr<MySession> MySession::sessions[max_sessions];

int MySession::processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Invalid invocation" << std::endl << std::flush;
    return -1;
}

int MySession::processStreamMessage(MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( sess_debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got msg" << std::endl << std::flush;

    uint64_t cluster_id = 10;
    uint64_t space_id = 1000;
    uint64_t store_id = 100000;
    uint64_t backup_id = 2222;
    uint64_t snapshot_id = 4444;

    MetaMsgs::AdmMsgBase base_msg;
    MetaMsgs::AdmMsgBuffer buf(true);
    int ret = base_msg.recv(this->tcp_sock.GetDesc(), buf);
    if ( ret < 0 ) {
        std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgBase hdr" << std::endl << std::flush;
        return ret;
    }
    bytes_received += ret;

    if ( !(counter%0x1000) ) {
        std::cout << " counter: " << counter <<
            " received: " << bytes_received <<
            " sent: " << bytes_sent <<
            " sess->soc: " << this->tcp_sock.GetDesc() << std::endl << std::flush;
    }
    try {
        MetaMsgs::AdmMsgBase *recv_msg = MetaMsgs::AdmMsgFactory::getInstance()->get(static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type));
        if ( !recv_msg ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgBase hdr" << std::endl << std::flush;
        }
        recv_msg->hdr = base_msg.hdr;

        switch ( static_cast<MetaMsgs::eMsgType>(base_msg.hdr.type) ) {
        case eMsgAssignMMChannelReq:
        {
            MetaMsgs::AdmMsgAssignMMChannelReq *mm_req = dynamic_cast<MetaMsgs::AdmMsgAssignMMChannelReq *>(recv_msg);
            ret = mm_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgAssignMMChannelReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgAssignMMChannelReq req" <<
                    " pid: " << mm_req->pid <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgAssignMMChannelRsp *mm_rsp = MetaMsgs::AdmMsgAssignMMChannelRsp::poolT.get();
            mm_rsp->path = "/dupek/kupek/laaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaDUPA";
            mm_rsp->pid = mm_req->pid;
            ret = mm_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgAssignMMChannelRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgAssignMMChannelRsp::poolT.put(mm_rsp);
        }
        break;
        case eMsgCreateClusterReq:
        {
            MetaMsgs::AdmMsgCreateClusterReq *cc_req = dynamic_cast<MetaMsgs::AdmMsgCreateClusterReq *>(recv_msg);
            ret = cc_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgCreateClusterReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgCreateClusterReq req" <<
                    " cluster_name: " << cc_req->cluster_name <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgCreateClusterRsp *cc_rsp = MetaMsgs::AdmMsgCreateClusterRsp::poolT.get();
            cc_rsp->cluster_id = cluster_id++;
            ret = cc_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgCreateClusterRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgCreateClusterRsp::poolT.put(cc_rsp);
        }
        break;
        case eMsgCreateSpaceReq:
        {
            MetaMsgs::AdmMsgCreateSpaceReq *cs_req = dynamic_cast<MetaMsgs::AdmMsgCreateSpaceReq *>(recv_msg);
            ret = cs_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgCreateSpaceReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgCreateSpaceReq req" <<
                    " cluster_name: " << cs_req->cluster_name <<
                    " space_name: " << cs_req->space_name <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgCreateSpaceRsp *cs_rsp = MetaMsgs::AdmMsgCreateSpaceRsp::poolT.get();
            cs_rsp->cluster_id = cluster_id++;
            cs_rsp->space_id = space_id++;
            ret = cs_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgCreateSpaceRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgCreateSpaceRsp::poolT.put(cs_rsp);
        }
        break;
        case eMsgCreateStoreReq:
        {
            MetaMsgs::AdmMsgCreateStoreReq *ct_req = dynamic_cast<MetaMsgs::AdmMsgCreateStoreReq *>(recv_msg);
            ret = ct_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgCreateStoreReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgCreateStoreReq req" <<
                    " cluster_name: " << ct_req->cluster_name <<
                    " space_name: " << ct_req->space_name <<
                    " store_name: " << ct_req->store_name <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgCreateStoreRsp *ct_rsp = MetaMsgs::AdmMsgCreateStoreRsp::poolT.get();
            ct_rsp->cluster_id = cluster_id++;
            ct_rsp->space_id = space_id++;
            ct_rsp->store_id = store_id++;
            ret = ct_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgCreateStoreRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgCreateStoreRsp::poolT.put(ct_rsp);
        }
        break;
        case eMsgDeleteClusterReq:
        {
            MetaMsgs::AdmMsgDeleteClusterReq *dc_req = dynamic_cast<MetaMsgs::AdmMsgDeleteClusterReq *>(recv_msg);
            ret = dc_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgDeleteClusterReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgDeleteClusterReq req" <<
                    " cluster_name: " << dc_req->cluster_name <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgDeleteClusterRsp *dc_rsp = MetaMsgs::AdmMsgDeleteClusterRsp::poolT.get();
            dc_rsp->cluster_id = cluster_id;
            ret = dc_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgDeleteClusterRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgDeleteClusterRsp::poolT.put(dc_rsp);
        }
        break;
        case eMsgDeleteSpaceReq:
        {
            MetaMsgs::AdmMsgDeleteSpaceReq *ds_req = dynamic_cast<MetaMsgs::AdmMsgDeleteSpaceReq *>(recv_msg);
            ret = ds_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgDeleteSpaceReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgDeleteSpaceReq req" <<
                    " cluster_name: " << ds_req->cluster_name <<
                    " space_name: " << ds_req->space_name <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgDeleteSpaceRsp *ds_rsp = MetaMsgs::AdmMsgDeleteSpaceRsp::poolT.get();
            ds_rsp->cluster_id = cluster_id;
            ds_rsp->space_id = space_id;
            ret = ds_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgDeleteSpaceRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgDeleteSpaceRsp::poolT.put(ds_rsp);
        }
        break;
        case eMsgDeleteStoreReq:
        {
            MetaMsgs::AdmMsgDeleteStoreReq *dt_req = dynamic_cast<MetaMsgs::AdmMsgDeleteStoreReq *>(recv_msg);
            ret = dt_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgDeleteStoreReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgDeleteStoreReq req" <<
                    " cluster_name: " << dt_req->cluster_name <<
                    " space_name: " << dt_req->space_name <<
                    " store_name: " << dt_req->store_name <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgDeleteStoreRsp *dt_rsp = MetaMsgs::AdmMsgDeleteStoreRsp::poolT.get();
            dt_rsp->cluster_id = cluster_id;
            dt_rsp->space_id = space_id;
            dt_rsp->store_id = store_id;
            ret = dt_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgDeleteStoreRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgDeleteStoreRsp::poolT.put(dt_rsp);
        }
        break;
        case eMsgCreateBackupReq:
        {
            MetaMsgs::AdmMsgCreateBackupReq *cb_req = dynamic_cast<MetaMsgs::AdmMsgCreateBackupReq *>(recv_msg);
            ret = cb_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgCreateBackupReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgCreateBackupReq req" <<
                    " cluster_name: " << cb_req->cluster_name <<
                    " space_name: " << cb_req->space_name <<
                    " store_name: " << cb_req->store_name <<
                    " backup_name: " << cb_req->backup_name <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgCreateBackupRsp *cb_rsp = MetaMsgs::AdmMsgCreateBackupRsp::poolT.get();
            cb_rsp->backup_id = backup_id;
            ret = cb_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgCreateBackupRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgCreateBackupRsp::poolT.put(cb_rsp);
        }
        break;
        case eMsgDeleteBackupReq:
        {
            MetaMsgs::AdmMsgDeleteBackupReq *db_req = dynamic_cast<MetaMsgs::AdmMsgDeleteBackupReq *>(recv_msg);
            ret = db_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgDeleteBackupReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgDeleteBackupReq req" <<
                    " cluster_name: " << db_req->cluster_name <<
                    " space_name: " << db_req->space_name <<
                    " store_name: " << db_req->store_name <<
                    " backup_name: " << db_req->backup_name <<
                    " backup_id: " << db_req->backup_id <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgDeleteBackupRsp *db_rsp = MetaMsgs::AdmMsgDeleteBackupRsp::poolT.get();
            db_rsp->backup_id = backup_id;
            ret = db_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgDeleteBackupRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgDeleteBackupRsp::poolT.put(db_rsp);
        }
        break;
        case eMsgListBackupsReq:
        {
            MetaMsgs::AdmMsgListBackupsReq *lbks_req = dynamic_cast<MetaMsgs::AdmMsgListBackupsReq *>(recv_msg);
            ret = lbks_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgListBackupsReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgListBackupsReq req" <<
                    " cluster_name: " << lbks_req->cluster_name <<
                    " space_name: " << lbks_req->space_name <<
                    " store_name: " << lbks_req->store_name <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgListBackupsRsp *lbks_rsp = MetaMsgs::AdmMsgListBackupsRsp::poolT.get();
            std::string backup_name = "bbbbeeeeckup_name_";
            lbks_rsp->num_backups = 128;
            for ( uint32_t i = 0 ; i < lbks_rsp->num_backups ; i++ ) {
                lbks_rsp->backups.push_back(std::pair<uint64_t, std::string>(backup_id + i, backup_name + to_string(i)));
            }
            ret = lbks_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgListBackupsRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            lbks_rsp->backups.erase(lbks_rsp->backups.begin(), lbks_rsp->backups.end());
            lbks_rsp->num_backups = 0;
            MetaMsgs::AdmMsgListBackupsRsp::poolT.put(lbks_rsp);
        }
        break;
        case eMsgCreateSnapshotReq:
        {
            MetaMsgs::AdmMsgCreateSnapshotReq *csn_req = dynamic_cast<MetaMsgs::AdmMsgCreateSnapshotReq *>(recv_msg);
            ret = csn_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgCreateSnapshotReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgCreateSnapshotReq req" <<
                    " cluster_name: " << csn_req->cluster_name <<
                    " space_name: " << csn_req->space_name <<
                    " store_name: " << csn_req->store_name <<
                    " snapshot_name: " << csn_req->snapshot_name <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgCreateSnapshotRsp *csn_rsp = MetaMsgs::AdmMsgCreateSnapshotRsp::poolT.get();
            csn_rsp->snapshot_id = snapshot_id;
            ret = csn_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgCreateSnapshotRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgCreateSnapshotRsp::poolT.put(csn_rsp);
        }
        break;
        case eMsgDeleteSnapshotReq:
        {
            MetaMsgs::AdmMsgDeleteSnapshotReq *dsn_req = dynamic_cast<MetaMsgs::AdmMsgDeleteSnapshotReq *>(recv_msg);
            ret = dsn_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgDeleteSnapshotReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgDeleteSnapshotReq req" <<
                    " cluster_name: " << dsn_req->cluster_name <<
                    " space_name: " << dsn_req->space_name <<
                    " store_name: " << dsn_req->store_name <<
                    " snapshot_name: " << dsn_req->snapshot_name <<
                    " snapshot_id: " << dsn_req->snapshot_id <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgDeleteSnapshotRsp *dsn_rsp = MetaMsgs::AdmMsgDeleteSnapshotRsp::poolT.get();
            dsn_rsp->snapshot_id = snapshot_id;
            ret = dsn_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgDeleteSnapshotRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgDeleteSnapshotRsp::poolT.put(dsn_rsp);
        }
        break;
        case eMsgListSnapshotsReq:
        {
            MetaMsgs::AdmMsgListSnapshotsReq *lsns_req = dynamic_cast<MetaMsgs::AdmMsgListSnapshotsReq *>(recv_msg);
            ret = lsns_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgListSnapshotsReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgListSnapshotsReq req" <<
                    " cluster_name: " << lsns_req->cluster_name <<
                    " space_name: " << lsns_req->space_name <<
                    " store_name: " << lsns_req->store_name <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgListSnapshotsRsp *lsns_rsp = MetaMsgs::AdmMsgListSnapshotsRsp::poolT.get();
            std::string snapshot_name = "snapppshot_name_";
            lsns_rsp->num_snapshots = 128;
            for ( uint32_t i = 0 ; i < lsns_rsp->num_snapshots ; i++ ) {
                lsns_rsp->snapshots.push_back(std::pair<uint64_t, std::string>(snapshot_id + i, snapshot_name + to_string(i)));
            }
            ret = lsns_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgListSnapshotsRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            lsns_rsp->snapshots.erase(lsns_rsp->snapshots.begin(), lsns_rsp->snapshots.end());
            lsns_rsp->num_snapshots = 0;
            MetaMsgs::AdmMsgListSnapshotsRsp::poolT.put(lsns_rsp);
        }
        break;
        case eMsgOpenStoreReq:
        {
            MetaMsgs::AdmMsgOpenStoreReq *os_req = dynamic_cast<MetaMsgs::AdmMsgOpenStoreReq *>(recv_msg);
            ret = os_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgOpenStoreReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgOpenStoreReq req" <<
                    " cluster_name: " << os_req->cluster_name <<
                    " space_name: " << os_req->space_name <<
                    " store_name: " << os_req->store_name <<
                    " store_id: " << os_req->store_id <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgOpenStoreRsp *os_rsp = MetaMsgs::AdmMsgOpenStoreRsp::poolT.get();
            os_rsp->store_id = store_id;
            ret = os_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgOpenStoreRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgOpenStoreRsp::poolT.put(os_rsp);
        }
        break;
        case eMsgCloseStoreReq:
        {
            MetaMsgs::AdmMsgCloseStoreReq *cs_req = dynamic_cast<MetaMsgs::AdmMsgCloseStoreReq *>(recv_msg);
            ret = cs_req->recv(this->tcp_sock.GetDesc(), buf);
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Can't receive AdmMsgCloseStoreReq" << std::endl << std::flush;
            } else {
                counter++;
                bytes_received += ret;
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got AdmMsgCloseStoreReq req" <<
                    " cluster_name: " << cs_req->cluster_name <<
                    " space_name: " << cs_req->space_name <<
                    " store_name: " << cs_req->store_name <<
                    " store_id: " << cs_req->store_id <<
                    " counter: " << counter << std::endl << std::flush;
            }



            MetaMsgs::AdmMsgCloseStoreRsp *cs_rsp = MetaMsgs::AdmMsgCloseStoreRsp::poolT.get();
            cs_rsp->store_id = store_id;
            ret = cs_rsp->send(this->tcp_sock.GetDesc());
            if ( ret < 0 ) {
                std::cerr << __PRETTY_FUNCTION__ << " Failed sending AdmMsgCloseStoreRsp: " << ret << std::endl << std::flush;
            }
            bytes_sent += ret;
            MetaMsgs::AdmMsgCloseStoreRsp::poolT.put(cs_rsp);
        }
        break;
        default:
            std::cerr << __PRETTY_FUNCTION__ << " Receive invalid AdmMsgBase hdr" << std::endl << std::flush;
            return -1;
        }

        MetaMsgs::AdmMsgFactory::getInstance()->put(recv_msg);

    } catch (const std::runtime_error &ex) {
        std::cerr << "Failed getting msg from factory: " << ex.what() << std::endl << std::flush;
    }

    return ret;
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



//
// EventListener msg Processor
//
struct EventListSrvProcessor: public Processor
{
    int CreateLocalUnixSockIOSession(const IPv4Address &_io_peer_addr, EventListener &_event_lis, const Point *_lis_pt)
    {
        UnixSocket *ss = new UnixSocket();
        MyIOSession<TestIov> *sss = new MyIOSession<TestIov>(*ss, 0);
        sss->initialise(const_cast<Point *>(_lis_pt));
        sss->setSessionDebug(debug);
        if ( ss->SetNoDelay() < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't SetNoDelay() on sock errno: " << errno << std::endl << std::flush;
            return -1;
        }
        if ( ss->SetNonBlocking(true) < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't SetNonBlocking(true) on Unix sock errno: " << errno << std::endl << std::flush;
            return -1;
        }
        if ( ss->SetRecvTmout(200) ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't set recv tmout on Unix socket errno: " << errno << std::endl << std::flush;
            return -1;
        }
        if ( ss->SetSndTmout(200) ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't set snd tmout on Unix socket errno: " << errno << std::endl << std::flush;
            return -1;
        }
        if ( ss->SetNoLinger() < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't set reuse address on Unix socket errno: " << errno << std::endl << std::flush;
            return -1;
        }
        MyIOSession<TestIov>::sessions[sss->event_point.GetDesc()].reset(sss);

        int ret = _event_lis.AddPoint(&sss->event_point);
        if ( ret < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't add cli session" << std::endl << std::flush;
            sss->deinitialise();
            return -1;
        }
        return 0;
    }

    int CreateLocalMMSockIOSession(const IPv4Address &_io_peer_addr, EventListener &_event_lis, const Point *_lis_pt)
    {
        MmPoint *ss = new MmPoint();
        if ( ss->Create() < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't Create MM sock errno: " << errno << std::endl << std::flush;
            return -1;
        }
        MyIOSession<TestIov> *sss = new MyIOSession<TestIov>(*ss, 0);
        sss->initialise(const_cast<Point *>(_lis_pt));
        sss->setSessionDebug(debug);
        if ( ss->SetNoDelay() < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't SetNoDelay() on MM sock errno: " << errno << std::endl << std::flush;
            return -1;
        }
        if ( ss->SetNonBlocking(true) < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't SetNonBlocking(true) on MM sock errno: " << errno << std::endl << std::flush;
            return -1;
        }
        if ( ss->SetRecvTmout(200) ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't set recv tmout on MM socket errno: " << errno << std::endl << std::flush;
            return -1;
        }
        if ( ss->SetSndTmout(200) ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't set snd tmout on MM socket errno: " << errno << std::endl << std::flush;
            return -1;
        }
        MyIOSession<TestIov>::sessions[sss->event_point.GetDesc()].reset(sss);

        int ret = _event_lis.AddPoint(&sss->event_point);
        if ( ret < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't add cli session" << std::endl << std::flush;
            sss->deinitialise();
            return -1;
        }
        return 0;
    }

    int CreateRemoteIOSession(const IPv4Address &_io_peer_addr, EventListener &_event_lis, const Point *_lis_pt)
    {
        TcpSocket *ss = new TcpSocket();
        MyIOSession<TestIov> *sss = new MyIOSession<TestIov>(*ss, 0);
        sss->initialise(const_cast<Point *>(_lis_pt));
        sss->setSessionDebug(debug);
        if ( ss->SetNoDelay() < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't SetNoDelay() on TCP sock errno: " << errno << std::endl << std::flush;
            return -1;
        }
        if ( ss->SetNonBlocking(true) < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't SetNonBlocking(true) on TCP sock errno: " << errno << std::endl << std::flush;
            return -1;
        }
        if ( ss->SetRecvTmout(200) ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't set recv tmout on TCP socket errno: " << errno << std::endl << std::flush;
            return -1;
        }
        if ( ss->SetSndTmout(200) ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't set snd tmout on TCP socket errno: " << errno << std::endl << std::flush;
            return -1;
        }
        if ( ss->SetNoLinger() < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't set reuse address on TCP socket errno: " << errno << std::endl << std::flush;
            return -1;
        }
        MyIOSession<TestIov>::sessions[sss->event_point.GetDesc()].reset(sss);

        int ret = _event_lis.AddPoint(&sss->event_point);
        if ( ret < 0 ) {
            std::cerr << __PRETTY_FUNCTION__ << " Can't add cli session" << std::endl << std::flush;
            sss->deinitialise();
            return -1;
        }
        return 0;
    }

    int processDgram(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0)
    {
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got dgram req" << std::endl << std::flush;
        return 0;
    }

    int processStream(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0)
    {
        int ret = 0;
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got tcp/unix/mm req" << std::endl << std::flush;

        EventListener &event_lis = dynamic_cast<EventListener &>(lis);

        if ( event_lis.getSock() == pt || event_lis.getUnixSock() == pt || event_lis.getMmSock() == pt ) { // connect request
            try {
                int ret = 0;
                if ( event_lis.getUnixSock() == pt ) {
                    ret = CreateLocalUnixSockIOSession(from_addr, event_lis, pt);
                } else if ( event_lis.getSock() == pt ){
                    ret = CreateRemoteIOSession(from_addr, event_lis, pt);
                } else if ( event_lis.getMmSock() == pt ){
                    ret = CreateLocalMMSockIOSession(from_addr, event_lis, pt);
                } else {
                    std::cerr << "Invalid Point requesting connection connecting " << std::endl << std::flush;
                    return -1;
                }

                if ( ret < 0 ) {
                    std::cerr << "Failed connecting cli io session from: " << from_addr.GetNTOASinAddr() <<
                        " to: " << event_lis.getIPv4OwnAddr().GetNTOASinAddr() << std::endl << std::flush;
                    return ret;
                }
            } catch (const std::runtime_error &ex) {
                std::cerr << "Failed connecting cli session: " << ex.what() << std::endl << std::flush;
            }
        } else { // msg
            if ( MyIOSession<TestIov>::sessions[const_cast<Point*>(pt)->GetDesc()] ) {
                ret = MyIOSession<TestIov>::sessions[const_cast<Point*>(pt)->GetDesc()]->processStreamMessage(_buf, from_addr, pt);
            }

            // Check if it's connection reset so we can remove the session from the listener
            if ( ret == -ECONNRESET ) {
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ <<
                    " Disconnecting client: " << const_cast<Point*>(pt)->GetDesc() << std::endl << std::flush;

                event_lis.RemovePoint(&MyIOSession<TestIov>::sessions[const_cast<Point*>(pt)->GetDesc()]->event_point);
                MyIOSession<TestIov>::sessions[const_cast<Point*>(pt)->GetDesc()]->deinitialise();
                MyIOSession<TestIov>::sessions[const_cast<Point*>(pt)->GetDesc()] = 0;
            }
        }

        return ret;
    }

    int sendDgram(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0)
    {
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Sending dgram rsp" << std::endl << std::flush;
        return 0;
    }

    int sendStream(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0)
    {
        //IPv4Listener &ipv4lis = dynamic_cast<IPv4Listener &>(lis);
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Sending tcp rsp" << std::endl << std::flush;
        return 0;
    }

    EventListSrvProcessor(bool _debug = false)
        :debug(_debug)
    {}
    ~EventListSrvProcessor() {}

    static MetaNet::EventListener *getEventListener();

    bool                    debug;
    static MetaNet::EventListener *eventlis;
};

MetaNet::EventListener *EventListSrvProcessor::eventlis = 0;
MetaNet::EventListener *EventListSrvProcessor::getEventListener()
{
    return EventListSrvProcessor::eventlis;
}


//
// IPv4Listener msg Processor
//
struct ListSrvProcessor: public Processor
{
    int processDgram(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0)
    {
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got dgram req" << std::endl << std::flush;
        return 0;
    }

    int processStream(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& _buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0)
    {
        int ret = 0;
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got tcp req" << std::endl << std::flush;

        IPv4Listener &ipv4_lis = dynamic_cast<IPv4Listener &>(lis);
        const TcpSocket *lis_soc = dynamic_cast<const TcpSocket *>(pt);
        if ( !lis_soc ) {
            std::cerr << __PRETTY_FUNCTION__ << " Wrong listening socket type" << std::endl << std::flush;
            return -1;
        }

        if ( ipv4_lis.getSock() == lis_soc ) { // connect request
            try {
                bool clean_up = false;
                TcpSocket *ss = new TcpSocket();
                MySession *sss = new MySession(*ss);
                if ( sss->initialise(const_cast<TcpSocket *>(lis_soc), true) < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't initialize session errno: " << errno << std::endl << std::flush;
                    delete ss;
                    delete sss;
                    return -1;
                }
                if ( ss->SetNoDelay() < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't SetNoDelay() on sock errno: " << errno << std::endl << std::flush;
                    clean_up = true;
                }
                if ( ss->SetNoLinger() < 0 ) {
                    std::cerr << "Can't set no linger address on tcp socket: " << errno << " exiting ..." << std::flush;
                    clean_up = true;
                }
                if ( ss->SetReuseAddr() < 0 ) {
                    std::cerr << "Can't set reuse address on tcp socket: " << errno << " exiting ..." << std::flush;
                    clean_up = true;
                }
                if ( ss->SetRecvTmout(200) ) {
                    std::cerr << "Can't set recv tmout on tcp socket: " << errno << " exiting ..." << std::flush;
                    clean_up = true;
                }
                if ( ss->SetSndTmout(200) ) {
                    std::cerr << "Can't set snd tmout on tcp socket: " << errno << " exiting ..." << std::flush;
                    clean_up = true;
                }
                if ( clean_up == true ) {
                    delete ss;
                    delete sss;
                    return -1;
                }
                MySession::sessions[sss->tcp_sock.GetDesc()].reset(sss);

                int ret = ipv4_lis.AddPoint(&sss->tcp_sock);
                if ( ret < 0 ) {
                    std::cerr << __PRETTY_FUNCTION__ << " Can't add cli session" << std::endl << std::flush;
                    (void )sss->deinitialise();
                    delete sss;
                    delete ss;
                    return -1;
                }
            } catch (const std::runtime_error &ex) {
                std::cerr << "Failed connecting cli session: " << ex.what() << std::endl << std::flush;
            }
        } else { // msg
            if ( MySession::sessions[const_cast<Point*>(pt)->GetDesc()] ) {
                ret = MySession::sessions[const_cast<Point*>(pt)->GetDesc()]->processStreamMessage(_buf, from_addr, pt);
            }


            // Check if it's connection reset so we can remove the session from the listener
            if ( ret == -ECONNRESET ) {
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ <<
                    " Disconnecting client: " << const_cast<Point*>(pt)->GetDesc() << std::endl << std::flush;

                ipv4_lis.RemovePoint(&MySession::sessions[const_cast<Point*>(pt)->GetDesc()]->tcp_sock);
                MySession::sessions[const_cast<Point*>(pt)->GetDesc()]->deinitialise();
                MySession::sessions[const_cast<Point*>(pt)->GetDesc()].reset();
            }
        }

        return ret;
    }

    int sendDgram(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0)
    {
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Sending dgram rsp" << std::endl << std::flush;
        return 0;
    }

    int sendStream(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0)
    {
        //IPv4Listener &ipv4lis = dynamic_cast<IPv4Listener &>(lis);
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Sending tcp rsp" << std::endl << std::flush;
        return 0;
    }

    ListSrvProcessor(bool _debug = false)
        :debug(_debug)
    {}
    ~ListSrvProcessor() {}

    bool          debug;
};

//
// IPv4McastListener msg Processor
//
struct McastlistSrvProcessor: public Processor
{
    int processDgram(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0)
    {
        int ret = 0;

        // If 0 - timeout - send mcast
        if ( !buf.getSize() ) {
            if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got timeout req" << std::endl << std::flush;

            // get the ip and port of the IPv4Listener and send an advertisement
            MetaNet::IPv4Listener *ipv4lis = MetaNet::IPv4Listener::getInstance();
            if ( !ipv4lis ) {
                if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " No IPv4Listener req" << std::endl << std::flush;
                return -1;
            }
            MetaNet::IPv4MulticastListener *ipv4mlis = MetaNet::IPv4MulticastListener::getInstance();
            srv_addr = from_addr;

            MetaMsgs::McastMsgMetaAd *ad = MetaMsgs::McastMsgMetaAd::poolT.get();

            AdvertisementItem item_ipv4_lis("meta_srv_adm",
                "Ipv4 admin listener service",
                ipv4lis->getIPv4OwnAddr().GetNTOASinAddr(),
                ipv4lis->getIPv4Addr()->GetNTOHSPort(),
                ipv4lis->getIPv4Addr()->GetFamily() == AF_INET ? AdvertisementItem::eProtocol::eIPv4Tcp : AdvertisementItem::eProtocol::eNone);
            AdvertisementItem item_event_lis("meta_srv_io",
                "Event i/o listener service",
                EventListSrvProcessor::getEventListener()->getIPv4OwnAddr().GetNTOASinAddr(),
                EventListSrvProcessor::getEventListener()->getSockAddr()->GetNTOHSPort(),
                EventListSrvProcessor::getEventListener()->getIPv4OwnAddr().GetFamily() == AF_INET ?
                     AdvertisementItem::eProtocol::eAll : AdvertisementItem::eProtocol::eAll);

            MsgNetMcastAdvertisement ad_msg;
            ad_msg.AddItem(item_ipv4_lis);
            ad_msg.AddItem(item_event_lis);
            ad->json = ad_msg.AsJson();

            ret = ad->sendmsg(ipv4mlis->mcast_sock.GetSoc(), from_addr.addr);
            MetaMsgs::McastMsgMetaAd::poolT.put(ad);
        } else {
            if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got empty dgram req" << std::endl << std::flush;
        }
        return ret;
    }

    int processStream(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt = 0)
    {
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Got tsp req" << std::endl << std::flush;
        return 0;
    }

    int sendDgram(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0)
    {
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Sending dgram rsp" << std::endl << std::flush;
        return 0;
    }

    int sendStream(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt = 0)
    {
        if ( debug == true ) std::cout << __PRETTY_FUNCTION__ << " Sending tsp rsp" << std::endl << std::flush;
        return 0;
    }

    McastlistSrvProcessor(bool _debug = false)
        :debug(_debug)
    {}
    ~McastlistSrvProcessor() {}

    IPv4Address   srv_addr;   // for sending advertisements
    bool          debug;
};



static void usage(const char *prog)
{
    std::cerr << prog << ": -m (msg debug) -d (debug)" << std::endl << std::flush;
}

//
// main
//
int main(int argc, char *argv[])
{
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

    if ( argc > 1 ) {
        for ( int i = 1 ; i < argc ; i++ ) {
            if ( std::string(argv[i]) == "-m") {
                msg_debug_mode = true;
            } else if ( std::string(argv[i]) == "-d") {
                debug_mode = true;
                msg_debug_mode = true;
            } else {
                usage(argv[0]);
                exit(-1);
            }
        }
    }

    try {
        MetaNet::IPv4MulticastListener *ipv4mcastlis = MetaNet::IPv4MulticastListener::getInstance(false, true);
        McastlistSrvProcessor msrv_processor;
        boost::function<CallbackIPv4ProcessFun> m_p1 = boost::bind(&McastlistSrvProcessor::processStream, &msrv_processor, _1, _2, _3, _4);
        ipv4mcastlis->setProcessStreamCB(m_p1);
        boost::function<CallbackIPv4ProcessFun> m_p2 = boost::bind(&McastlistSrvProcessor::processDgram, &msrv_processor, _1, _2, _3, _4);
        ipv4mcastlis->setProcessDgramCB(m_p2);
        boost::function<CallbackIPv4SendFun> m_s1 = boost::bind(&McastlistSrvProcessor::sendStream, &msrv_processor, _1, _2, _3, _4, _5);
        ipv4mcastlis->setSendStreamCB(m_s1);
        boost::function<CallbackIPv4SendFun> m_s2 = boost::bind(&McastlistSrvProcessor::sendDgram, &msrv_processor, _1, _2, _3, _4, _5);
        ipv4mcastlis->setSendDgramCB(m_s2);


        TcpSocket ipv4_sock;
        ipv4_sock.SetNoSigPipe();
        MetaNet::IPv4Listener *ipv4lis = MetaNet::IPv4Listener::setInstance(&ipv4_sock);
        ipv4lis->setDebug(msg_debug_mode);
        ListSrvProcessor srv_processor(msg_debug_mode);
        boost::function<CallbackIPv4ProcessFun> l_p1 = boost::bind(&ListSrvProcessor::processStream, &srv_processor, _1, _2, _3, _4);
        ipv4lis->setProcessStreamCB(l_p1);
        boost::function<CallbackIPv4ProcessFun> l_p2 = boost::bind(&ListSrvProcessor::processDgram, &srv_processor, _1, _2, _3, _4);
        ipv4lis->setProcessDgramCB(l_p2);
        boost::function<CallbackIPv4SendFun> l_s1 = boost::bind(&ListSrvProcessor::sendStream, &srv_processor, _1, _2, _3, _4, _5);
        ipv4lis->setSendStreamCB(l_s1);
        boost::function<CallbackIPv4SendFun> l_s2 = boost::bind(&ListSrvProcessor::sendDgram, &srv_processor, _1, _2, _3, _4, _5);
        ipv4lis->setSendDgramCB(l_s2);

        ipv4lis->setAssignedAddress(addr);
        TcpSocket *tcp_sock = dynamic_cast<TcpSocket *>(ipv4lis->getSock());
        if ( !tcp_sock ) {
            throw std::runtime_error(std::string(std::string(__PRETTY_FUNCTION__) + " Wrong socket type").c_str());
        }
        int ret = tcp_sock->Listen(10);
        if ( ret < 0 ) {
            throw std::runtime_error((std::string(__PRETTY_FUNCTION__) +
                " Listen failed addr: " + addr.GetNTOASinAddr() + " errno: " + to_string(errno)).c_str());
        }


        TcpSocket event_sock;
        event_sock.SetNoSigPipe();
        event_sock.SetNonBlocking(true);
        EventListSrvProcessor eventsrv_processor(debug_mode);
        eventsrv_processor.eventlis = new MetaNet::EventListener(event_sock);
        eventsrv_processor.eventlis->setDebug(debug_mode);
        boost::function<CallbackEventProcessFun> el_p1 = boost::bind(&EventListSrvProcessor::processStream, &eventsrv_processor, _1, _2, _3, _4);
        eventsrv_processor.eventlis->setProcessStreamCB(el_p1);
        boost::function<CallbackEventProcessFun> el_p2 = boost::bind(&EventListSrvProcessor::processDgram, &eventsrv_processor, _1, _2, _3, _4);
        eventsrv_processor.eventlis->setProcessDgramCB(el_p2);
        boost::function<CallbackEventSendFun> el_s1 = boost::bind(&EventListSrvProcessor::sendStream, &eventsrv_processor, _1, _2, _3, _4, _5);
        eventsrv_processor.eventlis->setSendStreamCB(el_s1);
        boost::function<CallbackEventSendFun> el_s2 = boost::bind(&EventListSrvProcessor::sendDgram, &eventsrv_processor, _1, _2, _3, _4, _5);
        eventsrv_processor.eventlis->setSendDgramCB(el_s2);

        eventsrv_processor.eventlis->setAssignedAddress(event_addr);
        tcp_sock = dynamic_cast<TcpSocket *>(eventsrv_processor.eventlis->getSock());
        if ( !tcp_sock ) {
            throw std::runtime_error(std::string(std::string(__PRETTY_FUNCTION__) + " Wrong socket type").c_str());
        }
        ret = tcp_sock->Listen(10);
        if ( ret < 0 ) {
            throw std::runtime_error((std::string(__PRETTY_FUNCTION__) +
                " Listen on TCP socket failed addr: " + addr.GetNTOASinAddr() + " errno: " + to_string(errno)).c_str());
        }
        UnixSocket *unix_sock = dynamic_cast<UnixSocket *>(eventsrv_processor.eventlis->getUnixSock());
        if ( !unix_sock ) {
            throw std::runtime_error(std::string(std::string(__PRETTY_FUNCTION__) + " Wrong socket type").c_str());
        }
        ret = unix_sock->Listen(10);
        if ( ret < 0 ) {
            throw std::runtime_error((std::string(__PRETTY_FUNCTION__) +
                " Listen on Unix socket failed addr: " + addr.GetNTOASinAddr() + " errno: " + to_string(errno)).c_str());
        }
        MmPoint *mm_point = dynamic_cast<MmPoint *>(eventsrv_processor.eventlis->getMmSock());
        if ( !mm_point ) {
            throw std::runtime_error(std::string(std::string(__PRETTY_FUNCTION__) + " Wrong socket type").c_str());
        }
        ret = mm_point->Listen(10);
        if ( ret < 0 ) {
            throw std::runtime_error((std::string(__PRETTY_FUNCTION__) +
                " Listen on MM socket failed addr: " + addr.GetNTOASinAddr() + " errno: " + to_string(errno)).c_str());
        }

        boost::thread lis_thread(&MetaNet::IPv4Listener::start, ipv4lis);
        boost::thread eventlis_thread(&MetaNet::EventListener::start, eventsrv_processor.eventlis);
        boost::thread mcastlis_thread(&MetaNet::IPv4MulticastListener::start, ipv4mcastlis);

        lis_thread.join();
        eventlis_thread.join();
        mcastlis_thread.join();
    } catch (const std::runtime_error &ex) {
        std::cerr << "std::runtime_error caught: " << ex.what() << std::endl << std::flush;
    }

    return 0;
}


