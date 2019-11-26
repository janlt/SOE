/**
 * @file    metadbclisessionmgr.hpp
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
 * metadbclisessionmgr.hpp
 *
 *  Created on: Jun 1, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBCLI_SRC_METADBCLISESSIONMGR_HPP_
#define SOE_SOE_SOE_METADBCLI_SRC_METADBCLISESSIONMGR_HPP_

namespace MetadbcliFacade {

void SessionMgrStaticInitializerEnter(void);
void SessionMgrStaticInitializerExit(void);

class SessionMgr {
public:
    static Lock             global_lock;
    static SessionMgr      *sess_mgr;
    McastlistCliProcessor  *mcast_proc;
    static bool             started;

    static SessionMgr *GetInstance();
    static void PutInstance();

    void McastSetDebug(bool _debug)
    {
        if ( mcast_proc ) {
            mcast_proc->setDebug(_debug);
        }
    }
    static void StartMgr();

protected:
    SessionMgr();
    virtual ~SessionMgr();

    virtual void start();

public:
    class Initializer {
    public:
        static bool initialized;
        Initializer();
        ~Initializer();
    };
    static Initializer sess_mgr_initalizer;

friend class Initializer;
};

}

#endif /* SOE_SOE_SOE_METADBCLI_SRC_METADBCLISESSIONMGR_HPP_ */
