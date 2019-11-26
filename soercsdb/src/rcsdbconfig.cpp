/*
 * rcsdbconfig.cpp
 *
 *  Created on: Dec 20, 2016
 *      Author: Jan Lisowiec
 */

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>

extern "C" {
#include "soeutil.h"
}

#include "soelogger.hpp"
using namespace SoeLogger;

#include "rcsdbargdefs.hpp"
#include "rcsdbfacadeapi.hpp"
#include "rcsdbconfigbase.hpp"
#include "rcsdbconfig.hpp"

using namespace std;

namespace RcsdbFacade {

static int ProcessJsonString(const JS_VAL_SOE *node,
    char *buf,
    size_t buf_size)
{
    int ret = JS_RET_SOE_OK;

    if ( !jsoeGetValueStringUTF8(node, buf, buf_size) ) {
        return 1;
    }
    return ret;
}

static int ProcessJsonNumber(const JS_VAL_SOE *node,
    uint64_t &id)
{
    int ret = JS_RET_SOE_OK;

    ret = jsoeGetU64(node, &id);

    return ret;
}



uint32_t StoreConfig::bloom_bits_default = 4;
bool StoreConfig::overdups_default = false;
bool StoreConfig::transaction_db_default = false;

StoreConfig::~StoreConfig()
{}

StoreConfig::StoreConfig(bool _debug)
    :ConfigBase(_debug),
    bloom_bits(StoreConfig::bloom_bits_default),
    overwrite_duplicates(StoreConfig::overdups_default),
    transaction_db(StoreConfig::transaction_db_default)
{}

void StoreConfig::ParseSnapshots(const JS_VAL_SOE *val) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " val: " << val->eType << SoeLogger::LogDebug() << endl;
    }

    for ( const JS_ELEM_SOE *it = jsoeGetFirstItem(val) ; it ; it = jsoeGetNextItem(it) ) {
        char buf_name[128];
        uint64_t val_id;
        const JS_DUPLE_SOE *pair;
        char buf[1024];

        uint32_t s_id;
        std::string s_name;

        int i_ret = 0;
        if ( jsoeIsList(&it->value) ) {
            for ( pair = jsoeGetFirstPair(&it->value) ; pair ; pair = jsoeGetNextPair(pair) ) {
                (void )jsoeGetPairNameUTF8(pair, buf, sizeof(buf));
                const JS_VAL_SOE *pair_val = jsoeGetPairValue(pair);

                if ( debug == true ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " key: " << pair_val->eType << SoeLogger::LogDebug() << endl;
                }

                if ( jsoeIsString(pair_val) ) {
                    i_ret = ProcessJsonString(pair_val, buf_name, sizeof(buf_name));
                } else if ( jsoeIsNumber(pair_val) ) {
                    i_ret = ProcessJsonNumber(pair_val, val_id);
                } else {
                    throw RcsdbFacade::Exception("Json invalid snapshots entry: " + std::string(__FUNCTION__));
                }

                if ( i_ret ) {
                    throw RcsdbFacade::Exception("Json invalid snapshots entry error: " + to_string(i_ret) + " " + std::string(__FUNCTION__));
                }

                if ( std::string(buf) == "name" ) {
                    s_name = std::string(buf_name);
                } else if ( std::string(buf) == "id" ) {
                    s_id = static_cast<uint32_t>(val_id);
                } else {
                    throw RcsdbFacade::Exception("Json invalid snapshots entry error: " + to_string(i_ret) + " " + std::string(__FUNCTION__));
                }
            }
        }

        // add entry in snapshots, if it's not there
        bool found_snap = false;
        for ( auto it = snapshots.begin() ; it != snapshots.end() ; it++ ) {
            if ( it->first == s_name && it->second == s_id ) {
                found_snap = true;
            }
        }

        if ( found_snap == false ) {
            snapshots.push_back(std::pair<std::string, uint32_t>(s_name, s_id));
            if ( debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << s_name << " id: " << s_id << SoeLogger::LogDebug() << endl;
            }
        }
    }
}

void StoreConfig::ParseBackups(const JS_VAL_SOE *val) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " val: " << val->eType << SoeLogger::LogDebug() << endl;
    }

    for ( const JS_ELEM_SOE *it = jsoeGetFirstItem(val) ; it ; it = jsoeGetNextItem(it) ) {
        char buf_name[128];
        uint64_t val_id;
        const JS_DUPLE_SOE *pair;
        char buf[1024];

        uint32_t s_id;
        std::string s_name;

        int i_ret = 0;
        if ( jsoeIsList(&it->value) ) {
            for ( pair = jsoeGetFirstPair(&it->value) ; pair ; pair = jsoeGetNextPair(pair) ) {
                (void )jsoeGetPairNameUTF8(pair, buf, sizeof(buf));
                const JS_VAL_SOE *pair_val = jsoeGetPairValue(pair);

                if ( debug == true ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " key: " << pair_val->eType << SoeLogger::LogDebug() << endl;
                }

                if ( jsoeIsString(pair_val) ) {
                    i_ret = ProcessJsonString(pair_val, buf_name, sizeof(buf_name));
                } else if ( jsoeIsNumber(pair_val) ) {
                    i_ret = ProcessJsonNumber(pair_val, val_id);
                } else {
                    throw RcsdbFacade::Exception("Json invalid backpus entry: " + std::string(__FUNCTION__));
                }

                if ( i_ret ) {
                    throw RcsdbFacade::Exception("Json invalid backpus entry error: " + to_string(i_ret) + " " + std::string(__FUNCTION__));
                }

                if ( std::string(buf) == "name" ) {
                    s_name = std::string(buf_name);
                } else if ( std::string(buf) == "id" ) {
                    s_id = static_cast<uint32_t>(val_id);
                } else {
                    throw RcsdbFacade::Exception("Json invalid backpus entry error: " + to_string(i_ret) + " " + std::string(__FUNCTION__));
                }
            }
        }

        // add entry in backups, if it's not there
        bool found_back = false;
        for ( auto it = backups.begin() ; it != backups.end() ; it++ ) {
            if ( it->first == s_name && it->second == s_id ) {
                found_back = true;
            }
        }

        if ( found_back == false ) {
            backups.push_back(std::pair<std::string, uint32_t>(s_name, s_id));
            if ( debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " name: " << s_name << " id: " << s_id << SoeLogger::LogDebug() << endl;
            }
        }
    }
}

void StoreConfig::Parse(const std::string &cfg, bool no_exception) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) <<  " Config " << cfg << SoeLogger::LogDebug() << endl;
    }

    JS_DAT_SOE *jsoe = jsoeGetJsonData();
    JS_RET_SOE ret = jsoeParse(jsoe, reinterpret_cast<const uint8_t *>(cfg.c_str()), static_cast<size_t>(cfg.length()));
    if ( ret != JS_RET_SOE_OK ) {
        jsoeFree(jsoe, 1);
        if ( no_exception == false ) {
            throw RcsdbFacade::Exception("Json parse error, jsoe: " + cfg + " error: " +
                to_string(ret) + " " + std::string(__FUNCTION__));
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json parse error, jsoe: " << cfg << " error: " <<
                to_string(ret) << " " << SoeLogger::LogError() << endl;
        }
        return;
    }

    const JS_VAL_SOE *jnode = jsoeGetTopValue(jsoe);

    char buf_name[1024];
    uint64_t val_id;
    int sync_lvl = 0;
    uint64_t con_id = -1;
    uint32_t b_bits = StoreConfig::bloom_bits_default;
    bool overwr_dups = StoreConfig::overdups_default;
    bool tra_db = StoreConfig::transaction_db_default;
    int i_ret = 0;
    if ( jsoeIsList(jnode) ) {
        const JS_DUPLE_SOE *pair;
        const JS_VAL_SOE *val;
        char buf[1024];

        void *ptr = jsoeGetValueStringUTF8(jnode, buf, sizeof(buf));

        for ( pair = jsoeGetFirstPair(jnode) ; pair ; pair = jsoeGetNextPair(pair) ) {
            ptr = jsoeGetPairNameUTF8(pair, buf, sizeof(buf));
            val = jsoeGetPairValue(pair);
            overwr_dups = StoreConfig::overdups_default;

            if ( jsoeIsString(val) ) {
                i_ret = ProcessJsonString(val, buf_name, sizeof(buf_name));
            } else if ( jsoeIsNumber(val) ) {
                i_ret = ProcessJsonNumber(val, val_id);
            } else if ( jsoeIsArray(val) == true ) {
                if ( std::string(buf) == "snapshots" ) {
                    try {
                        ParseSnapshots(val);
                    } catch (const RcsdbFacade::Exception &ex) {
                        if ( no_exception == false ) {
                            throw RcsdbFacade::Exception("Json invalid snapshots error, jsoe: " + cfg + " error: " +
                                ex.what() + " " + std::string(__FUNCTION__));
                        } else {
                            if ( debug == true ) {
                                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json invalid snapshots error, jsoe: " << cfg << " error: " <<
                                    ex.what() << " " << SoeLogger::LogDebug() << endl;
                            }
                        }
                    }
                } else if ( std::string(buf) == "backups" ) {
                    try {
                        ParseBackups(val);
                    } catch (const RcsdbFacade::Exception &ex) {
                        if ( no_exception == false ) {
                            throw RcsdbFacade::Exception("Json invalid backups error, jsoe: " + cfg + " error: " +
                                ex.what() + " " + std::string(__FUNCTION__));
                        } else {
                            if ( debug == true ) {
                                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json invalid backups error, jsoe: " << cfg << " error: " <<
                                    ex.what() << " " << SoeLogger::LogDebug() << endl;
                            }
                        }
                    }
                } else {
                    throw RcsdbFacade::Exception("Json invalid value error, jsoe: " + cfg + " error: " +
                        to_string(i_ret) + " " + std::string(__FUNCTION__));
                }
            } else {
                i_ret = -5;
                if ( no_exception == false ) {
                    throw RcsdbFacade::Exception("Json invalid value error, jsoe: " + cfg + " error: " +
                        to_string(i_ret) + " " + std::string(__FUNCTION__));
                } else {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json invalid value error, jsoe: " << cfg << " error: " <<
                        to_string(i_ret) << " " << SoeLogger::LogError() << endl;
                }
                break;
            }
            if ( debug == true ) {
                if ( no_exception == false ) {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json node: " << buf << " buf_name: " << buf_name <<
                        " val_id: " << val_id << " " << SoeLogger::LogDebug() << endl;
                } else{
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json node: " << buf << " buf_name: " << buf_name <<
                        " val_id: " << val_id << " " << SoeLogger::LogError() << endl;
                }
            }

            if ( i_ret ) {
                break;
            }

            if ( std::string(buf) == "bloom_bits" ) {
                b_bits = static_cast<uint32_t>(atoi(buf_name));
            }
            if ( std::string(buf) == "overwrite_duplicates" ) {
                overwr_dups = static_cast<bool>(val_id);
            }
            if ( std::string(buf) == "transaction_db" ) {
                tra_db = static_cast<bool>(val_id);
            }
            if ( std::string(buf) == "id" ) {
                con_id = static_cast<uint64_t>(val_id);
            }
            if ( std::string(buf) == "sync_level" ) {
                sync_lvl = static_cast<int>(val_id);
            }
        }
    }

    if ( i_ret ) {
        jsoeFree(jsoe, 1);
        if ( no_exception == false ) {
            throw RcsdbFacade::Exception("Json value error, jsoe: " + cfg + " error: " +
                to_string(i_ret) + " " + std::string(__FUNCTION__));
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json value error, jsoe: " <<  cfg + " error: " <<
                to_string(i_ret) << " " << SoeLogger::LogError() << endl;
        }
        return;
    }

    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Config jsoe.name: " << buf_name <<
            " id: " << con_id << " b_bits: " << b_bits <<
            " overwr_dups: " << overwr_dups << SoeLogger::LogDebug() << endl;
    }
    name = buf_name;
    id = con_id;
    bloom_bits = b_bits;
    overwrite_duplicates = overwr_dups;
    transaction_db = tra_db;
    sync_level = sync_lvl;
    jsoeFree(jsoe, 1);
}

void StoreConfig::CreateSnapshots(std::string &json_config) const
{
    json_config += std::string("\"snapshots\": [") + "\n";
    for ( auto it = snapshots.begin() ; it != snapshots.end() ; it++ ) {
        json_config += std::string("{ \"name\": ") + "\"" + it->first + "\", ";
        json_config += "\"id\": " + to_string(it->second) + " }";
        if ( it + 1 != snapshots.end() ) {
            json_config += ",\n";
        } else {
            json_config += "\n";
        }
    }
    json_config += "]";
}

void StoreConfig::CreateBackups(std::string &json_config) const
{
    json_config += std::string("\"backups\": [") + "\n";
    for ( auto it = backups.begin() ; it != backups.end() ; it++ ) {
        json_config += std::string("{ \"name\": ") + "\"" + it->first + "\", ";
        json_config += "\"id\": " + to_string(it->second) + " }";
        if ( it + 1 != backups.end() ) {
            json_config += ",\n";
        } else {
            json_config += "\n";
        }
    }
    json_config += "]";
}

std::string StoreConfig::Create()
{
    json_config = "{ \"name\": \"" + name + "\"" +
        ", \"id\": " + to_string(id) +
        ", \"bloom_bits\": " + to_string(bloom_bits) +
        ", \"overwrite_duplicates\": " + to_string(overwrite_duplicates) +
        ", \"transaction_db\": " + to_string(transaction_db) +
        ", \"sync_level\": " + to_string(sync_level) + ",\n";

    // Add snapshots and backups
    CreateSnapshots(json_config);
    json_config += ",\n";
    CreateBackups(json_config);

    json_config += "\n}";
    return json_config;
}




SpaceConfig::~SpaceConfig()
{}

SpaceConfig::SpaceConfig(bool _debug)
    :ConfigBase(_debug)
{}

void SpaceConfig::Parse(const std::string &cfg, bool no_exception) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Config " << cfg << SoeLogger::LogDebug() << endl;
    }

     JS_DAT_SOE *jsoe = jsoeGetJsonData();
     JS_RET_SOE ret = jsoeParse(jsoe, reinterpret_cast<const uint8_t *>(cfg.c_str()), static_cast<size_t>(cfg.length()));
     if ( ret != JS_RET_SOE_OK ) {
         jsoeFree(jsoe, 1);
         if ( no_exception == false ) {
             throw RcsdbFacade::Exception("Json parse error, jsoe: " + cfg + " error: " +
                 to_string(ret) + " " + std::string(__FUNCTION__));
         } else {
             SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json parse error, jsoe: " << cfg << " error: " <<
                 to_string(ret) << " " << SoeLogger::LogError() << endl;
         }
         return;
     }

     const JS_VAL_SOE *jnode = jsoeGetTopValue(jsoe);

     char buf_name[1024];
     uint64_t val_id;
     int i_ret = 0;
     if ( jsoeIsList(jnode) ) {
         const JS_DUPLE_SOE *pair;
         const JS_VAL_SOE *val;
         char buf[1024];

         void *ptr = jsoeGetValueStringUTF8(jnode, buf, sizeof(buf));

         for ( pair = jsoeGetFirstPair(jnode) ; pair ; pair = jsoeGetNextPair(pair) ) {
             ptr = jsoeGetPairNameUTF8(pair, buf, sizeof(buf));
             val = jsoeGetPairValue(pair);

             if ( debug == true ) {
                 SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json node: " << buf << " " << SoeLogger::LogDebug() << endl;
             }

             if ( jsoeIsString(val) ) {
                 i_ret = ProcessJsonString(val, buf_name, sizeof(buf_name));
             } else if ( jsoeIsNumber(val) ) {
                 i_ret = ProcessJsonNumber(val, val_id);
             } else {
                 i_ret = -5;
                 if ( no_exception == false ) {
                     throw RcsdbFacade::Exception("Json invalid value error, jsoe: " + cfg + " error: " +
                         to_string(i_ret) + " " + std::string(__FUNCTION__));
                 } else {
                     SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json invalid value error, jsoe: " << cfg << " error: " <<
                         to_string(i_ret) << " " << SoeLogger::LogError() << endl;
                 }
                 break;
             }

             if ( i_ret ) {
                 break;
             }
         }
     }

     if ( i_ret ) {
         jsoeFree(jsoe, 1);
         if ( no_exception == false ) {
             throw RcsdbFacade::Exception("Json value error, jsoe: " + cfg + " error: " +
                 to_string(i_ret) + " " + std::string(__FUNCTION__));
         } else {
             SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json value error, jsoe: " <<  cfg + " error: " <<
                 to_string(i_ret) << " " << SoeLogger::LogError() << endl;
         }
         return;
     }

     if ( debug == true ) {
         SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Config jsoe.name: " << buf_name << " id: " << val_id << SoeLogger::LogDebug() << endl;
     }
     name = buf_name;
     id = val_id;
     jsoeFree(jsoe, 1);
}

std::string SpaceConfig::Create() throw(RcsdbFacade::Exception)
{
    json_config = "{ \"name\": \"" + name + "\"" + ", \"id\": " + to_string(id) + "}";
    return json_config;
}



ClusterConfig::~ClusterConfig()
{}

ClusterConfig::ClusterConfig(bool _debug)
    :ConfigBase(_debug)
{}

void ClusterConfig::Parse(const std::string &cfg, bool no_exception) throw(RcsdbFacade::Exception)
{
    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Config " << cfg << SoeLogger::LogDebug() << endl;
    }

    JS_DAT_SOE *jsoe = jsoeGetJsonData();
    JS_RET_SOE ret = jsoeParse(jsoe, reinterpret_cast<const uint8_t *>(cfg.c_str()), static_cast<size_t>(cfg.length()));
    if ( ret != JS_RET_SOE_OK ) {
        jsoeFree(jsoe, 1);
        if ( no_exception == false ) {
            throw RcsdbFacade::Exception("Json parse error, jsoe: " + cfg + " error: " +
                to_string(ret) + " " + std::string(__FUNCTION__));
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json parse error, jsoe: " << cfg << " error: " <<
                to_string(ret) << " " << SoeLogger::LogDebug() << endl;
        }
        return;
    }

    const JS_VAL_SOE *jnode = jsoeGetTopValue(jsoe);

    char buf_name[1024];
    uint64_t val_id;
    int i_ret = 0;
    if ( jsoeIsList(jnode) ) {
        const JS_DUPLE_SOE *pair;
        const JS_VAL_SOE *val;
        char buf[1024];

        void *ptr = jsoeGetValueStringUTF8(jnode, buf, sizeof(buf));

        for ( pair = jsoeGetFirstPair(jnode) ; pair ; pair = jsoeGetNextPair(pair) ) {
            ptr = jsoeGetPairNameUTF8(pair, buf, sizeof(buf));
            val = jsoeGetPairValue(pair);

            if ( debug == true ) {
                SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json node: " << buf << " " << SoeLogger::LogDebug() << endl;
            }

            if ( jsoeIsString(val) ) {
                i_ret = ProcessJsonString(val, buf_name, sizeof(buf_name));
            } else if ( jsoeIsNumber(val) ) {
                i_ret = ProcessJsonNumber(val, val_id);
            } else {
                i_ret = -5;
                if ( no_exception == false ) {
                    throw RcsdbFacade::Exception("Json invalid value error, jsoe: " + cfg + " error: " +
                        to_string(i_ret) + " " + std::string(__FUNCTION__));
                } else {
                    SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json invalid value error, jsoe: " << cfg << " error: " <<
                        to_string(i_ret) << " " << SoeLogger::LogError() << endl;
                }
                break;
            }

            if ( i_ret ) {
                break;
            }
        }
    }

    if ( i_ret ) {
        jsoeFree(jsoe, 1);
        if ( no_exception == false ) {
            throw RcsdbFacade::Exception("Json value error, jsoe: " + cfg + " error: " +
                to_string(i_ret) + " " + std::string(__FUNCTION__));
        } else {
            SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << "Json value error, jsoe: " <<  cfg + " error: " <<
                to_string(i_ret) << " " << SoeLogger::LogError() << endl;
        }
        return;
    }

    if ( debug == true ) {
        SoeLogger::Logger().Clear(CLEAR_DEFAULT_ARGS) << " Config jsoe.name: " << buf_name << " id: " << val_id << SoeLogger::LogDebug() << endl;
    }
    name = buf_name;
    id = val_id;
    jsoeFree(jsoe, 1);
}

std::string ClusterConfig::Create() throw(RcsdbFacade::Exception)
{
    json_config = "{ \"name\": \"" + name + "\"" + ", \"id\": " + to_string(id) + "}";
    return json_config;
}

}



