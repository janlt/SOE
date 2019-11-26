/**
 * @file    rcsdbsnapshot.hpp
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
 * rcsdbsnapshot.hpp
 *
 *  Created on: Jan 26, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_RCSDBEM_SRC_RCSDBSNAPSHOT_HPP_
#define SOE_SOE_SOE_RCSDBEM_SRC_RCSDBSNAPSHOT_HPP_


using namespace rocksdb;

namespace RcsdbLocalFacade {

class RcsdbSnapshot
{
    std::shared_ptr<Checkpoint>   checkpoint;
    std::string                   name;
    std::string                   path;
    const static uint32_t         invalid_id = -1;
    uint32_t                      id;

public:
    RcsdbSnapshot(Checkpoint *_ch, const std::string &_name = "", const std::string &_path = "", uint32_t _id = RcsdbSnapshot::invalid_id);
    virtual ~RcsdbSnapshot();
    std::string GetName() const { return name; }
    std::string GetPath() const { return path; }
    uint32_t GetId() const { return id; }
    void SetName(const std::string &_name) { name = _name; }
    void SetPath(const std::string &_path) { path = _path; }
    void SetId(uint32_t _id) { id = _id; }
    Checkpoint *GetCheckpoint() const { return checkpoint.get(); }
};

}


#endif /* SOE_SOE_SOE_RCSDBEM_SRC_RCSDBSNAPSHOT_HPP_ */
