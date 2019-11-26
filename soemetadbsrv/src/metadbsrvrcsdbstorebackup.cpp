/**
 * @file    rcsdbstorebackup.cpp
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
 * rcsdbstorebackup.cpp
 *
 *  Created on: Jan 30, 2017
 *      Author: Jan Lisowiec
 */

#include <rocksdb/db.h>
#include <rocksdb/utilities/backupable_db.h>

#include "metadbsrvrcsdbstorebackup.hpp"

using namespace std;

namespace Metadbsrv {

RcsdbBackup::RcsdbBackup(BackupEngine *_be, BackupEngineReadOnly *_be_ro, const string &_name, const string &_path, uint32_t _id)
    :backup_engine(_be),
    backup_engine_ro(_be_ro),
    name(_name),
    path(_path),
    id(_id),
    type(eBackupEngineType::eInvalid)
{
    if ( !_be && _be_ro ) {
        type = eBackupEngineType::eReadOnly;
    } else if ( !_be_ro && _be ) {
        type = eBackupEngineType::eReadWrite;
    } else {
        type = eBackupEngineType::eInvalid;
    }
}

RcsdbBackup::~RcsdbBackup()
{
    if ( backup_engine.get() ) {
        backup_engine.reset();
    }
    if ( backup_engine_ro.get() ) {
        backup_engine_ro.reset();
    }
}


}


