/**
 * @file    metadbcliargdefs.cpp
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
 * metadbcliargdefs.cpp
 *
 *  Created on: Feb 03, 2017
 *      Author: Jan Lisowiec
 */



#include <string>
#include <vector>

#include "rcsdbargdefs.hpp"
#include "rcsdbfacadeapi.hpp"
#include "metadbcliargdefs.hpp"
#include "metadbclifacadeapi.hpp"

//
// API name space defines
//
// The API name space for Metadbcli is defined as a hierarchy of names starting
// from the common root Metadbcli. Then it forks into "local" and "remote" trees.
// From those it splits further into a hierarchy of clusters, spaces and stores,
// like in the example below:
//     METADBCLI.local.europe.denmark.copenhagen
//     METADBCLI.local.europe.switzerland.zurich
//     METADBCLI.local.asia.thailand.kolipe
//
//     METADBCLI.remote.europe.denmark.copenhagen
//     METADBCLI.remote.europe.switzerland.zurich
//     METADBCLI.remote.asia.thailand.kolipe
//
// Functions pointed by the path fall within two categories, per description below:
//
// Global functions, i.e. in C+ terms static function members or functions common
// for all clusters or spaces, for example:
//     METADBCLI.local.Create         -> create a new cluster
//     METADBCLI.local.GetByName      -> query for a cluster by name
//     METADBCLI.local.europe.Create  -> create a new space
//
// Member functions, i.e. in C++ terms object methods, pertain to a given instance on
// the path, for example:
//     METADBCLI.local.europe.Open      -> open "europe" cluster for operations
//     METADBCLI.local.europe.Get       -> get a kv item from "europe"
//     METADBCLI.local.europe.Put       -> put a kv item in "europe"
//     METADBCLI.local.europe.Delete    -> delete a kv item from "europe"
//     METADBCLI.local.europe.GetRange  -> get a range of kv items from "europe", could be specified as
//                                   start -> end with arbitrary criteria
//

using namespace std;

// Navigational names
#define METADBCLI_CLUSTER_API_ROOT        "METADBCLI"

#define METADBCLI_CLUSTER_ROOT_PREFIX   METADBCLI_CLUSTER_API_ROOT CLUSTER_API_DELIMIT
#define METADBCLI_CLUSTER_PREFIX_LOCAL  METADBCLI_CLUSTER_ROOT_PREFIX CLUSTER_API_LOCAL
#define METADBCLI_CLUSTER_PREFIX_REMOTE METADBCLI_CLUSTER_ROOT_PREFIX CLUSTER_API_REMOTE

namespace MetadbcliFacade {

const string CreateName(string &ret,
    const string &cluster,
    const string &space,
    const string &store,
    uint32_t token,
    const string &fun)
{
    if ( cluster != "" ) {
        ret += (CLUSTER_API_DELIMIT + cluster);
    }
    if ( space != "" ) {
        ret += (CLUSTER_API_DELIMIT + space);
    }
    if ( store != "" ) {
        ret += (CLUSTER_API_DELIMIT + store);
    }
    if ( token ) {
        char token_buf[128];
        sprintf(token_buf, "%u", token);
        ret += (CLUSTER_API_DELIMIT + string(token_buf));
    }
    if ( fun != "" ) {
        ret += (CLUSTER_API_DELIMIT + fun);
    }
    return ret;
}

const string CreateLocalName(const string &cluster,
    const string &space,
    const string &store,
    uint32_t token,
    const string &fun)
{
    string ret = METADBCLI_CLUSTER_PREFIX_LOCAL;

    return MetadbcliFacade::CreateName(ret, cluster, space, store, token, fun);
}

const string CreateRemoteName(const string &cluster,
    const string &space,
    const string &store,
    uint32_t token,
    const string &fun)
{
    string ret = METADBCLI_CLUSTER_PREFIX_REMOTE;

    return MetadbcliFacade::CreateName(ret, cluster, space, store, token, fun);
}

string GetProvider()
{
    return METADBCLI_CLUSTER_API_ROOT;
}

}


