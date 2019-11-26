/**
 * @file    soesessiongroup.hpp
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
 * soesessiongroup.hpp
 *
 *  Created on: Feb 3, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOE_INCLUDE_SOESESSIONGROUP_HPP_
#define SOE_SOE_SOE_SOE_INCLUDE_SOESESSIONGROUP_HPP_

namespace Soe {

/**
 * @brief Group class
 *
 * This class allows grouping operations pertaining to the same store and same session.
 *
 * A group can be obtained from session by calling CreateGroup() method. Subsequently, Set(), Merge()
 * or Delete() can be called on the group repetitively. Finally, a group is written to Store by invoking
 * Write() method on a session to which the group belongs to. There might be multiple instances of Group
 * present at any point in time in the application. Soe doesn't provide any synchronization or checking
 * of conflicts arising from multiple groups manipulating the same keys. Users have to provide any
 * synchronization mechanisms of their own.
 *
 */
class Group
{
friend class Session;

private:
    uint32_t                   group_no;
    const static uint32_t      invalid_group = -1;
    std::shared_ptr<Session>   sess;
    LocalArgsDescriptor        args;

    Group(std::shared_ptr<Session> _sess, uint32_t _gr = Group::invalid_group);
    ~Group();

public:
    Session::Status Put(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error);
    Session::Status Merge(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error);
    Session::Status Delete(const char *key, size_t key_size) throw(std::runtime_error);

    Session *GetSession() { return sess.get(); }
};

}

#endif /* SOE_SOE_SOE_SOE_INCLUDE_SOESESSIONGROUP_HPP_ */
