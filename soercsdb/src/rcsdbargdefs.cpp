/*
 * Rcsdbargdefs.cpp
 *
 *  Created on: Dec 16, 2016
 *      Author: Jan Lisowiec
 */

#include <string>
#include <vector>

#include "rcsdbargdefs.hpp"


//
// API name space defines
//
// The API name space for Rcsdb is defined as a hierarchy of names starting
// from the common root Rcsdb. Then it forks into "local" and "remote" trees.
// From those it splits further into a hierarchy of clusters, spaces and stores,
// like in the example below:
//     Rcsdb.local.europe.denmark.copenhagen
//     Rcsdb.local.europe.switzerland.zurich
//     Rcsdb.local.asia.thailand.kolipe
//
//     Rcsdb.remote.europe.denmark.copenhagen
//     Rcsdb.remote.europe.switzerland.zurich
//     Rcsdb.remote.asia.thailand.kolipe
//
// Functions pointed by the path fall within two categories, per description below:
//
// Global functions, i.e. in C+ terms static function members or functions common
// for all clusters or spaces, for example:
//     Rcsdb.local.Create         -> create a new cluster
//     Rcsdb.local.GetByName      -> query for a cluster by name
//     Rcsdb.local.europe.Create  -> create a new space
//
// Member functions, i.e. in C++ terms object methods, pertain to a given instance on
// the path, for example:
//     Rcsdb.local.europe.Open      -> open "europe" cluster for operations
//     Rcsdb.local.europe.Get       -> get a kv item from "europe"
//     Rcsdb.local.europe.Put       -> put a kv item in "europe"
//     Rcsdb.local.europe.Delete    -> delete a kv item from "europe"
//     Rcsdb.local.europe.GetRange  -> get a range of kv items from "europe", could be specified as
//                                   start -> end with arbitrary criteria
//

// Navigational names
#define RCSDB_CLUSTER_API_ROOT        "RCSDB"

#define RCSDB_CLUSTER_ROOT_PREFIX   RCSDB_CLUSTER_API_ROOT CLUSTER_API_DELIMIT
#define RCSDB_CLUSTER_PREFIX_LOCAL  RCSDB_CLUSTER_ROOT_PREFIX CLUSTER_API_LOCAL
#define RCSDB_CLUSTER_PREFIX_REMOTE RCSDB_CLUSTER_ROOT_PREFIX CLUSTER_API_REMOTE

namespace RcsdbFacade {

const std::string CreateName(std::string &ret,
    const std::string &cluster,
    const std::string &space,
    const std::string &store,
    uint32_t token,
    const std::string &fun)
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
    if ( fun != "" ) {
        ret += (CLUSTER_API_DELIMIT + fun);
    }
    return ret;
}

const std::string CreateLocalName(const std::string &cluster,
    const std::string &space,
    const std::string &store,
    uint32_t token,
    const std::string &fun)
{
    std::string ret = RCSDB_CLUSTER_PREFIX_LOCAL;

    return RcsdbFacade::CreateName(ret, cluster, space, store, token, fun);
}

const std::string CreateRemoteName(const std::string &cluster,
    const std::string &space,
    const std::string &store,
    uint32_t token,
    const std::string &fun)
{
    std::string ret = RCSDB_CLUSTER_PREFIX_REMOTE;

    return RcsdbFacade::CreateName(ret, cluster, space, store, token, fun);
}

std::string GetProvider()
{
    return RCSDB_CLUSTER_API_ROOT;
}

}


