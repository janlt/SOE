/**
 * @file    metadbsrvmcastprocessor.cpp
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
 * metadbsrvmcastprocessor.cpp
 *
 *  Created on: Jun 29, 2017
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

#include "soelogger.hpp"
using namespace SoeLogger;

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
#include "msgnetprocessor.hpp"
#include "msgnetlistener.hpp"
#include "msgneteventlistener.hpp"
#include "msgnetipv4listener.hpp"
#include "msgnetipv4mcastlistener.hpp"

#include "metadbsrveventprocessor.hpp"
#include "metadbsrvmcastprocessor.hpp"

using namespace std;
using namespace MetaNet;

namespace Metadbsrv {

//
// IPv4McastListener msg Processor
//
int McastlistSrvProcessor::SendAd(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    int ret = 0;

    // get the ip and port of the IPv4Listener and send an advertisement
    MetaNet::IPv4Listener *ipv4lis = MetaNet::IPv4Listener::getInstance();
    if ( !ipv4lis ) {
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "No IPv4Listener req" << SoeLogger::LogError() << endl;
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
        EventLisSrvProcessor::getEventListener()->getIPv4OwnAddr().GetNTOASinAddr(),
        EventLisSrvProcessor::getEventListener()->getSockAddr()->GetNTOHSPort(),
        EventLisSrvProcessor::getEventListener()->getIPv4OwnAddr().GetFamily() == AF_INET ?
             AdvertisementItem::eProtocol::eAll : AdvertisementItem::eProtocol::eAll);

    MsgNetMcastAdvertisement ad_msg;
    ad_msg.AddItem(item_ipv4_lis);
    ad_msg.AddItem(item_event_lis);
    ad->json = ad_msg.AsJson();

    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "sending ... json ad mcast: " << ipv4mlis->getIPv4Addr()->GetNTOASinAddr() <<
        " json: " << ad->json << SoeLogger::LogDebug() << endl;

    ret = ad->sendmsg(ipv4mlis->mcast_sock.GetSoc(), ipv4mlis->getIPv4Addr()->addr/*from_addr.addr*/);
    MetaMsgs::McastMsgMetaAd::poolT.put(ad);

    return ret;
}

int McastlistSrvProcessor::processDgram(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    int ret = 0;

    // If 0 (timeout) or Solicit - send mcast
    if ( !buf.getSize() ) {
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got timeout req" << SoeLogger::LogDebug() << endl;
        return SendAd(lis, buf, from_addr, pt);
    } else {
        MetaMsgs::McastMsgMetaSolicit *sol = MetaMsgs::McastMsgMetaSolicit::poolT.get();
        sol->unmarshall(dynamic_cast<MetaMsgs::McastMsgBuffer &>(buf));

        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got dgram req type: " << sol->hdr.type << " ad_type: " <<
            static_cast<uint32_t>(MetaMsgs::McastMsgMetaSolicit::class_msg_type) << SoeLogger::LogDebug() << endl;

        if ( sol->hdr.type != static_cast<uint32_t>(MetaMsgs::McastMsgMetaSolicit::class_msg_type) ) {
            MetaMsgs::McastMsgMetaSolicit::poolT.put(sol);

            // if it's ad from another srv, also send an ad (not yet .....)
            // TODO: Needs a timer, otehrwise will cause packet storm
            if ( sol->hdr.type == static_cast<uint32_t>(MetaMsgs::McastMsgMetaAd::class_msg_type ) ) {
                //return SendAd(lis, buf, from_addr, pt);
            }
            return 0;
        }
        if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Solicitation from_addr: " << from_addr.GetNTOASinAddr() << SoeLogger::LogDebug() << endl;
        MetaMsgs::McastMsgMetaSolicit::poolT.put(sol);
        return SendAd(lis, buf, from_addr, pt);
    }
    return ret;
}

int McastlistSrvProcessor::processStream(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got tcp req" << SoeLogger::LogDebug() << endl;
    return 0;
}

int McastlistSrvProcessor::sendDgram(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Send dgram rsp" << SoeLogger::LogDebug() << endl;
    return 0;
}

int McastlistSrvProcessor::sendStream(MetaNet::Listener &lis, const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst_addr, const MetaNet::Point *pt)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Sending tsp rsp" << SoeLogger::LogDebug() << endl;
    return 0;
}

int McastlistSrvProcessor::processEvent(MetaNet::Listener &lis, MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got tsp req" << SoeLogger::LogDebug() << endl;
    return 0;
}

McastlistSrvProcessor::McastlistSrvProcessor(bool _debug)
    :debug(_debug)
{}

McastlistSrvProcessor::~McastlistSrvProcessor() {}

}


