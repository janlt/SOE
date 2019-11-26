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

#include "rcsdbstorebackup.hpp"

using namespace std;

namespace RcsdbLocalFacade {

RcsdbBackup::RcsdbBackup(BackupEngine *_be, const string &_name, const string &_path, uint32_t _id)
    :backup_engine(_be),
    name(_name),
    path(_path),
    id(_id)
{}

RcsdbBackup::~RcsdbBackup() {}


}


