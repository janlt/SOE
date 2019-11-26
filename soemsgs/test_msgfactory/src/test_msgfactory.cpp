/*
 * msgnet_test_cli.cpp
 *
 *  Created on: Jan 13, 2017
 *      Author: Jan Lisowiec
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <stdexcept>
#include <typeinfo>

#include "classalloc.hpp"
#include "generalpool.hpp"
#include "dbsrvmsgbuffer.hpp"
#include "dbsrvmarshallable.hpp"
#include "dbsrvmsgs.hpp"
#include "dbsrviovmsgsbase.hpp"
#include "dbsrviovmsgs.hpp"
#include "dbsrvmsgtypes.hpp"
#include "dbsrvmsgadmtypes.hpp"
#include "dbsrvmcasttypes.hpp"
#include "dbsrvmsgfactory.hpp"
#include "dbsrvmsgadmfactory.hpp"
#include "dbsrvmcastfactory.hpp"
#include "dbsrvmsgiovfactory.hpp"
#include "dbsrvmsgiovtypes.hpp"
#include "dbsrvmsgiovtypesnt.hpp"

using namespace std;
using namespace MetaMsgs;

struct TestInit
{
    int recv(MetaMsgs::IovMsgBase *obj) const;
    int send(MetaMsgs::IovMsgBase *obj);
};

int TestInit::recv(MetaMsgs::IovMsgBase *obj) const
{
    return 0;
}

int TestInit::send(MetaMsgs::IovMsgBase *obj)
{
    return 0;
}

// Test instatiator functions
std::string TestIovType(MetaMsgs::eMsgType type, MetaMsgs::IovMsgBase *msg)
{
    switch ( type ) {
    case MetaMsgs::eMsgGetReq:
    {
        MetaMsgs::GetReq<TestInit> *gr = dynamic_cast<MetaMsgs::GetReq<TestInit> *>(msg);
        if ( !gr ) {
            std::cerr << "Invalid msg type: " << type << std::endl << std::flush;
            return "Error";
        }
        TestInit tester;
        int ret_s = gr->sendMsg(tester);
        int ret_r = gr->recvMsg(tester);
        if ( ret_s < 0 || ret_r < 0 ) {
            std::cerr << "Tester send/recv class failed type: " << type << std::endl << std::flush;
        } else {
            return " s/r " + to_string(ret_s) + "/" + to_string(ret_r);
        }
    }
    break;
    case MetaMsgs::eMsgGetRsp:
    {
        MetaMsgs::GetRsp<TestInit> *gr = dynamic_cast<MetaMsgs::GetRsp<TestInit> *>(msg);
        if ( !gr ) {
            std::cerr << "Invalid msg type: " << type << std::endl << std::flush;
            return "Error";
        }
        TestInit tester;
        int ret_s = gr->sendMsg(tester);
        int ret_r = gr->recvMsg(tester);
        if ( ret_s < 0 || ret_r < 0 ) {
            std::cerr << "Tester send/recv class failed type: " << type << std::endl << std::flush;
        } else {
            return " s/r " + to_string(ret_s) + "/" + to_string(ret_r);
        }
    }
    break;
    case MetaMsgs::eMsgPutReq:
    {
        MetaMsgs::PutReq<TestInit> *gr = dynamic_cast<MetaMsgs::PutReq<TestInit> *>(msg);
        if ( !gr ) {
            std::cerr << "Invalid msg type: " << type << std::endl << std::flush;
            return "Error";
        }
        TestInit tester;
        int ret_s = gr->sendMsg(tester);
        int ret_r = gr->recvMsg(tester);
        if ( ret_s < 0 || ret_r < 0 ) {
            std::cerr << "Tester send/recv class failed type: " << type << std::endl << std::flush;
        } else {
            return " s/r " + to_string(ret_s) + "/" + to_string(ret_r);
        }
    }
    break;
    case MetaMsgs::eMsgPutRsp:
    {
        MetaMsgs::PutRsp<TestInit> *gr = dynamic_cast<MetaMsgs::PutRsp<TestInit> *>(msg);
        if ( !gr ) {
            std::cerr << "Invalid msg type: " << type << std::endl << std::flush;
            return "Error";
        }
        TestInit tester;
        int ret_s = gr->sendMsg(tester);
        int ret_r = gr->recvMsg(tester);
        if ( ret_s < 0 || ret_r < 0 ) {
            std::cerr << "Tester send/recv class failed type: " << type << std::endl << std::flush;
        } else {
            return " s/r " + to_string(ret_s) + "/" + to_string(ret_r);
        }
    }
    break;
    case MetaMsgs::eMsgDeleteReq:
    {
        MetaMsgs::DeleteReq<TestInit> *gr = dynamic_cast<MetaMsgs::DeleteReq<TestInit> *>(msg);
        if ( !gr ) {
            std::cerr << "Invalid msg type: " << type << std::endl << std::flush;
            return "Error";
        }
        TestInit tester;
        int ret_s = gr->sendMsg(tester);
        int ret_r = gr->recvMsg(tester);
        if ( ret_s < 0 || ret_r < 0 ) {
            std::cerr << "Tester send/recv class failed type: " << type << std::endl << std::flush;
        } else {
            return " s/r " + to_string(ret_s) + "/" + to_string(ret_r);
        }
    }
    break;
    case MetaMsgs::eMsgDeleteRsp:
    {
        MetaMsgs::DeleteRsp<TestInit> *gr = dynamic_cast<MetaMsgs::DeleteRsp<TestInit> *>(msg);
        if ( !gr ) {
            std::cerr << "Invalid msg type: " << type << std::endl << std::flush;
            return "Error";
        }
        TestInit tester;
        int ret_s = gr->sendMsg(tester);
        int ret_r = gr->recvMsg(tester);
        if ( ret_s < 0 || ret_r < 0 ) {
            std::cerr << "Tester send/recv class failed type: " << type << std::endl << std::flush;
        } else {
            return " s/r " + to_string(ret_s) + "/" + to_string(ret_r);
        }
    }
    break;
    case MetaMsgs::eMsgGetRangeReq:
    {
        MetaMsgs::GetRangeReq<TestInit> *gr = dynamic_cast<MetaMsgs::GetRangeReq<TestInit> *>(msg);
        if ( !gr ) {
            std::cerr << "Invalid msg type: " << type << std::endl << std::flush;
            return "Error";
        }
        TestInit tester;
        int ret_s = gr->sendMsg(tester);
        int ret_r = gr->recvMsg(tester);
        if ( ret_s < 0 || ret_r < 0 ) {
            std::cerr << "Tester send/recv class failed type: " << type << std::endl << std::flush;
        } else {
            return " s/r " + to_string(ret_s) + "/" + to_string(ret_r);
        }
    }
    break;
    case MetaMsgs::eMsgGetRangeRsp:
    {
        MetaMsgs::GetRangeRsp<TestInit> *gr = dynamic_cast<MetaMsgs::GetRangeRsp<TestInit> *>(msg);
        if ( !gr ) {
            std::cerr << "Invalid msg type: " << type << std::endl << std::flush;
            return "Error";
        }
        TestInit tester;
        int ret_s = gr->sendMsg(tester);
        int ret_r = gr->recvMsg(tester);
        if ( ret_s < 0 || ret_r < 0 ) {
            std::cerr << "Tester send/recv class failed type: " << type << std::endl << std::flush;
        } else {
            return " s/r " + to_string(ret_s) + "/" + to_string(ret_r);
        }
    }
    break;
    case MetaMsgs::eMsgMergeReq:
    {
        MetaMsgs::MergeReq<TestInit> *gr = dynamic_cast<MetaMsgs::MergeReq<TestInit> *>(msg);
        if ( !gr ) {
            std::cerr << "Invalid msg type: " << type << std::endl << std::flush;
            return "Error";
        }
        TestInit tester;
        int ret_s = gr->sendMsg(tester);
        int ret_r = gr->recvMsg(tester);
        if ( ret_s < 0 || ret_r < 0 ) {
            std::cerr << "Tester send/recv class failed type: " << type << std::endl << std::flush;
        } else {
            return " s/r " + to_string(ret_s) + "/" + to_string(ret_r);
        }
    }
    break;
    case MetaMsgs::eMsgMergeRsp:
    {
        MetaMsgs::MergeRsp<TestInit> *gr = dynamic_cast<MetaMsgs::MergeRsp<TestInit> *>(msg);
        if ( !gr ) {
            std::cerr << "Invalid msg type: " << type << std::endl << std::flush;
            return "Error";
        }
        TestInit tester;
        int ret_s = gr->sendMsg(tester);
        int ret_r = gr->recvMsg(tester);
        if ( ret_s < 0 || ret_r < 0 ) {
            std::cerr << "Tester send/recv class failed type: " << type << std::endl << std::flush;
        } else {
            return " s/r " + to_string(ret_s) + "/" + to_string(ret_r);
        }
    }
    break;
    case MetaMsgs::eMsgGetReqNT:
    case MetaMsgs::eMsgGetRspNT:
    case MetaMsgs::eMsgPutReqNT:
    case MetaMsgs::eMsgPutRspNT:
    case MetaMsgs::eMsgDeleteReqNT:
    case MetaMsgs::eMsgDeleteRspNT:
    case MetaMsgs::eMsgGetRangeReqNT:
    case MetaMsgs::eMsgGetRangeRspNT:
    case MetaMsgs::eMsgMergeReqNT:
    case MetaMsgs::eMsgMergeRspNT:
        return " s/r 0/0";
        break;
    default:
        return "Invalid Iov msg";
        break;
    }

    return "Error";
}

int main(int argc, char *argv[])
{
    IovMsgPoolInitializer<TestInit> iov_init;

    for ( int m_typ = eMsgInvalid ; m_typ < eMsgMax ; m_typ++ ) {
        if ( FactoryResolver::isAdm(static_cast<eMsgType>(m_typ)) == true ) {
            MetaMsgs::AdmMsgBase *msg;
            try {
                msg = AdmMsgFactory::getInstance()->get(static_cast<eMsgType>(m_typ));
            } catch (const std::runtime_error &ex) {
                std::cerr << ex.what() << std::endl << std::flush;
                continue;
            }

            if ( msg ) {
                std::cout << "Got msg typ: " << msg->hdr.type << " T:: " << static_cast<eMsgType>(m_typ) <<
                    " type_name: " << MsgTypeInfo::ToString(static_cast<eMsgType>(m_typ)) <<
                    " class_name: " << typeid(*msg).name() << std::endl << std::flush;
                if ( static_cast<eMsgType>(msg->hdr.type) != m_typ ) {
                    std::cout << "TEST FAILED " << typeid(msg).name() << std::endl << std::flush;
                }
            } else {
                std::cout << "Got NULL T:: " << static_cast<eMsgType>(m_typ) << std::endl << std::flush;
            }
        } else if ( FactoryResolver::isIov(static_cast<eMsgType>(m_typ)) == true ) {
            MetaMsgs::IovMsgBase *msg;
            try {
                msg = IovMsgFactory::getInstance()->get(static_cast<eMsgType>(m_typ));
            } catch (const std::runtime_error &ex) {
                std::cerr << ex.what() << std::endl << std::flush;
                continue;
            }

            if ( msg ) {
                std::cout << "Got msg typ: " << msg->iovhdr.hdr.type << " T:: " << static_cast<eMsgType>(m_typ) <<
                    " type_name: " << MsgTypeInfo::ToString(static_cast<eMsgType>(m_typ)) <<
                    " class_name: " << typeid(*msg).name() <<
                    " send/recv: " << TestIovType(static_cast<eMsgType>(m_typ), msg) << std::endl << std::flush;
                if ( static_cast<eMsgType>(msg->iovhdr.hdr.type) != m_typ ) {
                    std::cout << "TEST FAILED " << typeid(msg).name() << std::endl << std::flush;
                }
            } else {
                std::cout << "Got NULL T:: " << static_cast<eMsgType>(m_typ) << std::endl << std::flush;
            }
        } else if ( FactoryResolver::isMcast(static_cast<eMsgType>(m_typ)) == true ) {
            MetaMsgs::McastMsgBase *msg;
            try {
                msg = McastMsgFactory::getInstance()->get(static_cast<eMsgType>(m_typ));
            } catch (const std::runtime_error &ex) {
                std::cerr << ex.what() << std::endl << std::flush;
                continue;
            }

            if ( msg ) {
                std::cout << "Got msg typ: " << msg->hdr.type << " T:: " << static_cast<eMsgType>(m_typ) <<
                    " type_name: " << MsgTypeInfo::ToString(static_cast<eMsgType>(m_typ)) <<
                    " class_name: " << typeid(*msg).name() << std::endl << std::flush;
                if ( static_cast<eMsgType>(msg->hdr.type) != m_typ ) {
                    std::cout << "TEST FAILED " << typeid(msg).name() << std::endl << std::flush;
                }
            } else {
                std::cout << "Got NULL T:: " << static_cast<eMsgType>(m_typ) << std::endl << std::flush;
            }
        } else {
            std::cout << "Invalid T:: " << static_cast<eMsgType>(m_typ) << std::endl << std::flush;
        }
    }

    return 0;
}
