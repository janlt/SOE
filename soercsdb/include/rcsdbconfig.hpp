/*
 * rcsdbconfig.hpp
 *
 *  Created on: Dec 20, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_RCSDB_SRC_RCSDBCONFIG_HPP_
#define SOE_SOE_SOE_RCSDB_SRC_RCSDBCONFIG_HPP_

namespace RcsdbFacade {

class StoreConfig: public ConfigBase
{
public:
    static uint32_t          bloom_bits_default;
    static bool              overdups_default;
    static bool              transaction_db_default;

    uint32_t                 bloom_bits;
    bool                     overwrite_duplicates;
    bool                     transaction_db;
    int                      sync_level;

    std::vector<std::pair<std::string, uint32_t> > snapshots;

    std::vector<std::pair<std::string, uint32_t> > backups;

    virtual ~StoreConfig();

    StoreConfig(bool _debug = false);
    void ParseSnapshots(const JS_VAL_SOE *val) throw(RcsdbFacade::Exception);
    void ParseBackups(const JS_VAL_SOE *val) throw(RcsdbFacade::Exception);
    void CreateSnapshots(std::string &json) const;
    void CreateBackups(std::string &json) const ;
    virtual void Parse(const std::string &cfg, bool no_exception = false) throw(RcsdbFacade::Exception);
    virtual std::string Create();
};

class SpaceConfig: public ConfigBase
{
public:
    virtual ~SpaceConfig();

    SpaceConfig(bool _debug = false);
    virtual void Parse(const std::string &cfg, bool no_exception = false) throw(RcsdbFacade::Exception);
    virtual std::string Create() throw(RcsdbFacade::Exception);
};

class ClusterConfig: public ConfigBase
{
public:
    virtual ~ClusterConfig();

    ClusterConfig(bool _debug = false);
    virtual void Parse(const std::string &cfg, bool no_exception = false) throw(RcsdbFacade::Exception);
    virtual std::string Create() throw(RcsdbFacade::Exception);
};

}


#endif /* SOE_SOE_SOE_RCSDB_SRC_RCSDBCONFIG_HPP_ */
