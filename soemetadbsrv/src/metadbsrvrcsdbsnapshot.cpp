/**
 * @file    rcsdbsnapshot.cpp
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
 * rcsdbsnapshot.cpp
 *
 *  Created on: Jan 26, 2017
 *      Author: Jan Lisowiec
 */

#include <rocksdb/db.h>
#include <rocksdb/utilities/checkpoint.h>

#include "metadbsrvrcsdbsnapshot.hpp"

using namespace std;

namespace Metadbsrv {

RcsdbSnapshot::RcsdbSnapshot(Checkpoint *_ch, const string &_name, const string &_path, uint32_t _id)
    :checkpoint(_ch),
    name(_name),
    path(_path),
    id(_id)
{}

RcsdbSnapshot::~RcsdbSnapshot() {}

}

