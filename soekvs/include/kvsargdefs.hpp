/*
 * kvsargdefs.hpp
 *
 *  Created on: Dec 1, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_KVS_INCLUDE_KVSARGDEFS_HPP_
#define SOE_SOE_SOE_KVS_INCLUDE_KVSARGDEFS_HPP_

//
// API name space defines
//
// The API name space for KVS is defined as a hierarchy of names starting
// from the common root KVS. Then it forks into "local" and "remote" trees.
// From those it splits further into a hierarchy of clusters, spaces and stores,
// like in the example below:
//     KVS.local.europe.denmark.copenhagen
//     KVS.local.europe.switzerland.zurich
//     KVS.local.asia.thailand.kolipe
//
//     KVS.remote.europe.denmark.copenhagen
//     KVS.remote.europe.switzerland.zurich
//     KVS.remote.asia.thailand.kolipe
//
// Functions pointed by the path fall within two categories, per description below:
//
// Global functions, i.e. in C+ terms static function members or functions common
// for all clusters or spaces, for example:
//     KVS.local.Create         -> create a new cluster
//     KVS.local.GetByName      -> query for a cluster by name
//     KVS.local.europe.Create  -> create a new space
//
// Member functions, i.e. in C++ terms object methods, pertain to a given instance on
// the path, for example:
//     KVS.local.europe.Open      -> open "europe" cluster for operations
//     KVS.local.europe.Get       -> get a kv item from "europe"
//     KVS.local.europe.Put       -> put a kv item in "europe"
//     KVS.local.europe.Delete    -> delete a kv item from "europe"
//     KVS.local.europe.GetRange  -> get a range of kv items from "europe", could be specified as
//                                   start -> end with arbitrary criteria
//

namespace KvsFacade {
const std::string CreateLocalName(const std::string &cluster,
    const std::string &space,
    const std::string &store,
    uint32_t token,
    const std::string &fun);

const std::string CreateRemoteName(const std::string &cluster,
    const std::string &space,
    const std::string &store,
    uint32_t token,
    const std::string &fun);

std::string GetProvider();

}

#endif /* SOE_SOE_SOE_KVS_INCLUDE_KVSARGDEFS_HPP_ */
