/**
 * @file    soesnapshotengine.hpp
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
 * soesnapshotengine.hpp
 *
 *  Created on: Feb 3, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOE_INCLUDE_SOESNAPSHOTENGINE_HPP_
#define SOE_SOE_SOE_SOE_INCLUDE_SOESNAPSHOTENGINE_HPP_

namespace Soe {

/**
 * @brief SnapshotEngine class
 *
 * This class allows creating snapshot engines and store snapshots.
 *
 * Snapshot engines are created by invoking SnapshotEngine constructor.
 *
 * Snapshot engines have their scope within the scope of a session used to create them.
 *
 * Once a snapshot engine has been created, a store snapshot can be created. Multiple snapshots
 * can be created using the same backup engine. When snapshot engine is no longer needed it can
 * be deleted. The snapshots created using that engine will still reside as part of the store.
 * Subsequently, when more snapshots are needed, a new snapshot engine and new snapshots can be created,
 * using new session. When an entire store is destroyed, all the snapshots residing within that
 * store will be discarded as well.
 *
 */
class SnapshotEngine
{
friend class Session;

private:
    uint32_t                       snapshot_engine_no;
    uint32_t                       snapshot_engine_id;
    std::string                    snapshot_engine_name;
    const static uint32_t          invalid_snapshot_engine = -1;
    std::shared_ptr<Session>       sess;

    SnapshotEngine(std::shared_ptr<Session> _sess,
        const std::string _nm = "snasphot_1",
        uint32_t _no = SnapshotEngine::invalid_snapshot_engine,
        uint32_t _id = SnapshotEngine::invalid_snapshot_engine);
    ~SnapshotEngine();

public:
    Session::Status CreateSnapshot() throw(std::runtime_error);
    std::string GetName()
    {
        return snapshot_engine_name;
    }

    uint32_t GetNo()
    {
        return snapshot_engine_no;
    }

    uint32_t GetId()
    {
        return snapshot_engine_id;
    }
};

}

#endif /* SOE_SOE_SOE_SOE_INCLUDE_SOESNAPSHOTENGINE_HPP_ */
