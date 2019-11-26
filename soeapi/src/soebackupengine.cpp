/**
 * @file    soebackupengine.cpp
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
 * soebackupengine.cpp
 *
 *  Created on: Feb 3, 2017
 *      Author: Jan Lisowiec
 */

#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <new>

#include "soe_soe.hpp"

using namespace std;
using namespace KvsFacade;
using namespace RcsdbFacade;

namespace Soe {

//
// Transaction
//
BackupEngine::BackupEngine(shared_ptr<Session> _sess, const string _nm, const string _brd, uint32_t _no, uint32_t _id)
    :backup_engine_no(_no),
     backup_engine_id(_id),
     backup_engine_name(_nm),
     backup_root_dir(_brd),
     sess(_sess)
{}

BackupEngine::~BackupEngine()
{
    backup_engine_no = BackupEngine::invalid_backup_engine;
}

Session::Status BackupEngine::CreateBackup() throw(runtime_error)
{
    return sess->CreateBackup(this);
}

Session::Status BackupEngine::RestoreBackup(const std::string &cl_n,
    const std::string &sp_n,
    const std::string &st_n,
    bool _overwrite) throw(std::runtime_error)
{
    return sess->RestoreBackup(this, cl_n, sp_n, st_n, _overwrite);
}

Session::Status BackupEngine::DestroyBackup() throw(std::runtime_error)
{
    return sess->DestroyBackup(this);
}

Session::Status BackupEngine::VerifyBackup(const std::string &cl_n,
    const std::string &sp_n,
    const std::string &st_n) throw(std::runtime_error)
{
    return sess->VerifyBackup(this, cl_n, sp_n, st_n);
}

}
