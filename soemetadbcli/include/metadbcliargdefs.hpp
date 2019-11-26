/**
 * @file    rcsdbargdefs.hpp
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
 * rcsdbargdefs.hpp
 *
 *  Created on: Feb 03, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBCLI_INCLUDE_METADBCLIARGDEFS_HPP_
#define SOE_SOE_SOE_METADBCLI_INCLUDE_METADBCLIARGDEFS_HPP_

//
// API name space defines
//
// The API name space for METADBCLI is defined as a hierarchy of names starting
// from the common root METADBCLI. Then it forks into "local" and "remote" trees.
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

namespace MetadbcliFacade {

const std::string CreateName(std::string &ret,
    const std::string &cluster,
    const std::string &space,
    const std::string &store,
    uint32_t token,
    const std::string &fun);

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

#endif /* SOE_SOE_SOE_METADBCLI_INCLUDE_METADBCLIARGDEFS_HPP_ */
