/**
 * @file    metadbsrvaccessors.hpp
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
 * metadbsrvaccessors.hpp
 *
 *  Created on: Jun 30, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_METADBSRV_SRC_METADBSRVACCESSORS_HPP_
#define SOE_SOE_SOE_METADBSRV_SRC_METADBSRVACCESSORS_HPP_

namespace Metadbsrv {

typedef uint64_t (CreateStaticFun)(LocalArgsDescriptor &);
typedef void (DestroyStaticFun)(LocalArgsDescriptor &);
typedef uint64_t (GetByNameStaticFun)(LocalArgsDescriptor &);
typedef void (OpObjectFun)(LocalArgsDescriptor &);
typedef void (OpAdmObjectFun)(LocalAdmArgsDescriptor &);

class RcsdbAccessors {
public:
    boost::function<CreateStaticFun>      create_fun;
    boost::function<DestroyStaticFun>     destroy_fun;
    boost::function<GetByNameStaticFun>   get_by_name_fun;
    boost::function<OpObjectFun>          open;
    boost::function<OpObjectFun>          close;
    boost::function<OpObjectFun>          repair;
    boost::function<OpObjectFun>          put;
    boost::function<OpObjectFun>          get;
    boost::function<OpObjectFun>          del;
    boost::function<OpObjectFun>          merge;
    boost::function<OpObjectFun>          iterate;
    boost::function<OpObjectFun>          get_range;
    boost::function<OpObjectFun>          put_range;
    boost::function<OpObjectFun>          delete_range;
    boost::function<OpObjectFun>          merge_range;

    boost::function<OpAdmObjectFun>       start_transaction;
    boost::function<OpAdmObjectFun>       commit_transaction;
    boost::function<OpAdmObjectFun>       rollback_transaction;

    boost::function<OpAdmObjectFun>       create_snapshot_engine;
    boost::function<OpAdmObjectFun>       create_snapshot;
    boost::function<OpAdmObjectFun>       destroy_snapshot;
    boost::function<OpAdmObjectFun>       list_snapshots;

    boost::function<OpAdmObjectFun>       create_backup_engine;
    boost::function<OpAdmObjectFun>       create_backup;
    boost::function<OpAdmObjectFun>       restore_backup;
    boost::function<OpAdmObjectFun>       destroy_backup;
    boost::function<OpAdmObjectFun>       destroy_backup_engine;
    boost::function<OpAdmObjectFun>       list_backups;

    boost::function<OpObjectFun>          create_group;
    boost::function<OpObjectFun>          destroy_group;
    boost::function<OpObjectFun>          write_group;

    boost::function<OpObjectFun>          create_iterator;
    boost::function<OpObjectFun>          destroy_iterator;

public:
    RcsdbAccessors() {}
    ~RcsdbAccessors() {}
};

}

#endif /* SOE_SOE_SOE_METADBSRV_SRC_METADBSRVACCESSORS_HPP_ */
