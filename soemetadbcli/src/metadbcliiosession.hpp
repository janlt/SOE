/**
 * @file    metadbcliiosession.hpp
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
 * metadbcliiosession.hpp
 *
 *  Created on: Jun 1, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBCLI_SRC_METADBCLIIOSESSION_HPP_
#define SOE_SOE_SOE_METADBCLI_SRC_METADBCLIIOSESSION_HPP_

namespace MetadbcliFacade {

template <class T>
class MetaIOSession: public EventCliSession<T>
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

    MetaIOSession(Point &point, Session *_parent, bool _test = false)
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

    virtual ~MetaIOSession() {}
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
};

template <class T>
inline int MetaIOSession<T>::processDgramMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( sess_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got msg" << SoeLogger::LogDebug() << endl;

    return 0;
}

template <class T>
inline int MetaIOSession<T>::processStreamMessage(MetaMsgs::MsgBufferBase& buf, const IPv4Address &from_addr, const MetaNet::Point *pt)
{
    if ( sess_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got msg" << SoeLogger::LogDebug() << endl;

    return 0;
}

template <class T>
inline int MetaIOSession<T>::sendDgramMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    if ( sess_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got msg" << SoeLogger::LogDebug() << endl;

    return 0;
}

template <class T>
inline int MetaIOSession<T>::sendStreamMessage(const uint8_t *msg, uint32_t msg_size, const IPv4Address *dst, const MetaNet::Point *pt)
{
    if ( sess_debug == true ) SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Got msg" << SoeLogger::LogDebug() << endl;

    return 0;
}

}

#endif /* SOE_SOE_SOE_METADBCLI_SRC_METADBCLIIOSESSION_HPP_ */
