/*
 * rcsdbconfigbase.hpp
 *
 *  Created on: Dec 20, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_RCSDB_SRC_RCSDBCONFIGBASE_HPP_
#define SOE_SOE_SOE_RCSDB_SRC_RCSDBCONFIGBASE_HPP_

namespace RcsdbFacade {

class ConfigBase
{
public:
    std::string json_config;
    std::string name;
    uint64_t    id;
    bool        debug;

public:
    virtual ~ConfigBase() {}

    ConfigBase(bool _debug = true): name(), id(-1), debug(_debug) {}

    virtual void Parse(const std::string &cfg, bool no_exception = false) throw(RcsdbFacade::Exception) = 0;
    virtual std::string Create() = 0;
};

}

#endif /* SOE_SOE_SOE_RCSDB_SRC_RCSDBCONFIGBASE_HPP_ */
