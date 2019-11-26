/**
 * @file    soeaccessors.hpp
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
 * soeaccessors.hpp
 *
 *  Created on: Feb 3, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOE_INCLUDE_SOEACCESSORS_HPP_
#define SOE_SOE_SOE_SOE_INCLUDE_SOEACCESSORS_HPP_

typedef boost::shared_mutex Lock;
typedef boost::unique_lock<Lock> WriteLock;
typedef boost::shared_lock<Lock> ReadLock;

namespace Soe {

typedef uint64_t (CreateStaticFun)(LocalArgsDescriptor &);
typedef void (DestroyStaticFun)(LocalArgsDescriptor &);
typedef uint64_t (GetByNameStaticFun)(LocalArgsDescriptor &);
typedef void (OpObjectFun)(LocalArgsDescriptor &);
typedef void (OpAdmObjectFun)(LocalAdmArgsDescriptor &);
typedef void (ListClustersStaticFun)(LocalArgsDescriptor &);
typedef void (ListSpacesStaticFun)(LocalArgsDescriptor &);
typedef void (ListStoresStaticFun)(LocalArgsDescriptor &);
typedef void (ListSubsetsStaticFun)(LocalArgsDescriptor &);

class SoeAccessors {
public:
    boost::function<CreateStaticFun>         create_fun;
    boost::function<DestroyStaticFun>        destroy_fun;
    boost::function<GetByNameStaticFun>      get_by_name_fun;
    boost::function<OpObjectFun>             open;
    boost::function<OpObjectFun>             close;
    boost::function<OpObjectFun>             repair;
    boost::function<OpObjectFun>             put;
    boost::function<OpObjectFun>             get;
    boost::function<OpObjectFun>             del;
    boost::function<OpObjectFun>             merge;
    boost::function<OpObjectFun>             iterate;
    boost::function<OpObjectFun>             get_range;
    boost::function<OpObjectFun>             put_range;
    boost::function<OpObjectFun>             delete_range;
    boost::function<OpObjectFun>             merge_range;

    boost::function<OpAdmObjectFun>          start_transaction;
    boost::function<OpAdmObjectFun>          commit_transaction;
    boost::function<OpAdmObjectFun>          rollback_transaction;

    boost::function<OpAdmObjectFun>          create_snapshot_engine;
    boost::function<OpAdmObjectFun>          create_snapshot;
    boost::function<OpAdmObjectFun>          destroy_snapshot;
    boost::function<OpAdmObjectFun>          list_snapshots;

    boost::function<OpAdmObjectFun>          create_backup_engine;
    boost::function<OpAdmObjectFun>          create_backup;
    boost::function<OpAdmObjectFun>          restore_backup;
    boost::function<OpAdmObjectFun>          destroy_backup;
    boost::function<OpAdmObjectFun>          verify_backup;
    boost::function<OpAdmObjectFun>          destroy_backup_engine;
    boost::function<OpAdmObjectFun>          list_backups;

    boost::function<OpObjectFun>             create_group;
    boost::function<OpObjectFun>             destroy_group;
    boost::function<OpObjectFun>             write_group;

    boost::function<OpObjectFun>             create_iterator;
    boost::function<OpObjectFun>             destroy_iterator;

    boost::function<ListClustersStaticFun>   list_clusters_fun;
    boost::function<ListSpacesStaticFun>     list_spaces_fun;
    boost::function<ListStoresStaticFun>     list_stores_fun;
    boost::function<ListSubsetsStaticFun>    list_subsets_fun;

public:
    SoeAccessors() {}
    ~SoeAccessors() {}
};

}

#endif /* SOE_SOE_SOE_SOE_INCLUDE_SOEACCESSORS_HPP_ */
