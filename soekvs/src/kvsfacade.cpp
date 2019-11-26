/*
 * kvsfacade.cpp
 *
 *  Created on: Nov 29, 2016
 *      Author: Jan Lisowiec
 */

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_index.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>

extern "C" {
#include "soeutil.h"
}

#include "kvsimpl.hpp"
#include "kvssetver.hpp"
#include "accell.hpp"
#include "kvstore.hpp"
#include "configur.hpp"
#include "kvsbatchops.hpp"
#include "kvsarch.hpp"
#include "crc32c.hpp"
#include "plots.hpp"
#include "kvsmutex.hpp"
#include "random.hpp"

#include "soe_bind_api.hpp"
#include "rcsdbargdefs.hpp"
#include "rcsdbfacadeapi.hpp"
#include "rcsdbfacadebase.hpp"
#include "rcsdbfunctorbase.hpp"
#include "kvsargdefs.hpp"
#include "kvsfacade.hpp"

using namespace LzKVStore;
using namespace RcsdbFacade;

void __attribute__ ((constructor)) KvsFacade::StaticInitializerEnter(void);
void __attribute__ ((destructor)) KvsFacade::StaticInitializerExit(void);

namespace KvsFacade {

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

    int res = jsoeGetValueNumber(node);
    id = static_cast<uint64_t>(res);

    return ret;
}

uint64_t Store::store_null_id = -1;  // max 64 bit number
uint32_t Store::bloom_bits_default = 4;
bool Store::overdups_default = false;

RcsdbFacade::FunctorBase   Store::static_ftors;
RcsdbFacade::FunctorBase   Space::static_ftors;
RcsdbFacade::FunctorBase   Cluster::static_ftors;

void Store::SetOpenOptions()
{
    open_options.env = Configuration::Default();
    open_options.create_if_missing = true;
    open_options.block_cache = cache;
    open_options.write_buffer_size = 0;
    open_options.max_file_size = 0;
    open_options.block_size = 0;
    open_options.max_open_files = 0;
    open_options.filter_policy = filter_policy;
    open_options.reuse_logs = false;
    open_options.full_checks = true;
    open_options.block_restart_interval = true;
}

void Store::SetReadOptions()
{
}

void Store::SetWriteOptions()
{
    write_options = WriteOptions();
}

Store::Store(const std::string &nm, uint64_t _id, uint32_t b_bits, bool over_dups, bool db):
    cache(NewLRUAccell(-1)),
    filter_policy(NULL),
    kvsd(0),
    state(eStoreInit),
    batch_mode(false),
    bloom_bits(b_bits),
    overwrite_duplicates(over_dups),
    debug(db),
    name(nm),
    id(_id)
{
    NameBase::next_available = 0;

    filter_policy = bloom_bits ? NewBloomFilteringPolicy(bloom_bits) : NULL;

    SetOpenOptions();
    SetReadOptions();
    SetWriteOptions();
}

Store::~Store()
{
    // no throwing from destructor
    if ( kvsd ) {
        delete kvsd;
        kvsd = 0;
    }
    if ( cache ) {
        delete cache;
        cache = 0;
    }
    if ( filter_policy ) {
        delete filter_policy;
        filter_policy = 0;
    }
}

void Store::SetPath()
{
    assert(parent);
    path = parent->GetPath() + "/" + name;
    if ( debug ) {
        std::cout << "path: " << path << " " << std::string(__FUNCTION__) << std::endl << std::flush;
    }
}

const std::string &Store::GetPath()
{
    return path;
}

void Store::ParseConfig(const std::string &cfg, bool no_exc)
{
    JS_DAT_SOE *json = jsoeGetJsonData();
     JS_RET_SOE ret = jsoeParse(json, reinterpret_cast<const uint8_t *>(cfg.c_str()), static_cast<size_t>(cfg.length()));
     if ( ret != JS_RET_SOE_OK ) {
         jsoeFree(json, 1);
         if ( no_exc == false ) {
             throw Exception("Json parse error, json: " + cfg + " error: " +
                 to_string(ret) + " " + std::string(__FUNCTION__));
         } else {
             std::cout << "Json parse error, json: " << cfg << " error: " <<
                 to_string(ret) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
         }
         return;
     }

     const JS_VAL_SOE *jnode = jsoeGetTopValue(json);

     char buf_name[1024];
     uint64_t val_id;
     uint32_t b_bits = Store::bloom_bits_default;
     bool overwr_dups = Store::overdups_default;
     int i_ret = 0;
     if ( jsoeIsList(jnode) ) {
         const JS_DUPLE_SOE *pair;
         const JS_VAL_SOE *val;
         char buf[1024];

         void *ptr = jsoeGetValueStringUTF8(jnode, buf, sizeof(buf));

         for ( pair = jsoeGetFirstPair(jnode) ; pair ; pair = jsoeGetNextPair(pair) ) {
             ptr = jsoeGetPairNameUTF8(pair, buf, sizeof(buf));
             val = jsoeGetPairValue(pair);
             overwr_dups = Store::overdups_default;

             if ( jsoeIsString(val) ) {
                 i_ret = ProcessJsonString(val, buf_name, sizeof(buf_name));
             } else if ( jsoeIsNumber(val) ) {
                 i_ret = ProcessJsonNumber(val, val_id);
             } else {
                 jsoeFree(json, 1);
                 if ( no_exc == false ) {
                     throw Exception("Json invalid value error, json: " + cfg + " error: " +
                         to_string(i_ret) + " " + std::string(__FUNCTION__));
                 } else {
                     std::cout << "Json invalid value error, json: " << cfg << " error: " <<
                         to_string(i_ret) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
                 }
                 break;
             }
             std::cout << "Json node: " << buf << " buf_name: " << buf_name <<
                 " val_id: " << val_id << " " << std::string(__FUNCTION__) << std::endl << std::flush;

             if ( std::string(buf) == "bloom_bits" ) {
                 b_bits = static_cast<uint32_t>(atoi(buf_name));
             }
             if ( std::string(buf) == "overwrite_dups" ) {
                 overwr_dups = static_cast<bool>(val_id);
             }

             if ( i_ret ) {
                 break;
             }
         }
     }

     if ( i_ret ) {
         jsoeFree(json, 1);
         if ( no_exc == false ) {
             throw Exception("Json value error, json: " + cfg + " error: " +
                 to_string(i_ret) + " " + std::string(__FUNCTION__));
         } else {
             std::cout << "Json value error, json: " <<  cfg + " error: " <<
                 to_string(i_ret) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
         }
         return;
     }

     std::cout << "Config json.name: " << buf_name <<
         " id: " << val_id << " b_bits: " << b_bits <<
         " overwr_dups: " << overwr_dups << std::endl << std::flush;
     name = buf_name;
     id = val_id;
     if ( b_bits && b_bits != bloom_bits ) {
         bloom_bits = b_bits;
         if ( filter_policy ) {
             delete filter_policy;
         }
         filter_policy = NewBloomFilteringPolicy(bloom_bits);
     }
     overwrite_duplicates = overwr_dups;
     jsoeFree(json, 1);
}

std::string Store::CreateConfig(std::string &nm, uint64_t id, uint32_t b_bits, bool overwr_dups)
{
    return "{ \"name\": \"" + nm + "\"" +
        ", \"id\": " + to_string(id) +
        ", \"bloom_bits\": " + to_string(b_bits) +
        ", \"overwrite_dups\": " + to_string(overwr_dups) + "}";
}

void Store::SetNextAvailable()
{}

uint64_t Store::CreateStore(LocalArgsDescriptor &args) throw(Exception)
{
    return Cluster::CreateStore(args);
}

void Store::DestroyStore(LocalArgsDescriptor &args) throw(Exception)
{
    Cluster::DestroyStore(args);
}

uint64_t Store::GetStoreByName(LocalArgsDescriptor &args) throw(Exception)
{
    return Cluster::GetStoreByName(args);
}

void Store::OpenPr() throw(Exception)
{
    if ( state != eStoreInit && state != eStoreClosed ) {
        throw Exception("Store in wrong state: " + to_string(state));
    }
    Status s = KVDatabase::Open(open_options, path, &kvsd);
    if ( !s.ok() ) {
        if ( debug == true ) {
            std::cout << __FUNCTION__ << " error: " << s.ToString() << std::endl << std::flush;
        }
        throw Exception("Open failed: " + s.ToString() + " " + std::string(__FUNCTION__));
    }
    state = eStoreOpened;
}

void Store::ClosePr() throw(Exception)
{
    if ( state != eStoreOpened ) {
        throw Exception("Store in wrong state: " + to_string(state));
    }
    if ( kvsd ) {
        delete kvsd;
    }
    state = eStoreClosed;
}

void Store::Crc32cPr() throw(Exception)
{
    throw Exception("Don't call me just yet" + std::string(__FUNCTION__));
}

void Store::PutPr(const Dissection& key, const Dissection& value) throw(Exception)
{
    if ( state != eStoreOpened ) {
        throw Exception("Store in wrong state: " + to_string(state));
    }

    if ( overwrite_duplicates == false ) {
        Status check_s = kvsd->Check(read_options, key);
        if ( check_s.ok() ) {
            throw Exception(std::string("Duplicate found: ") + key.ToString());
        }
    }

    Status s;
    if ( batch_mode == true ) {
        SaveBatch batch;
        batch.Put(key, value);
        s = kvsd->Write(write_options, &batch);
    } else {
        s = kvsd->Put(write_options, key, value);
    }
    if ( !s.ok() ) {
        if ( debug == true ) {
            std::cout << __FUNCTION__ << " error: " << s.ToString() << std::endl << std::flush;
        }
        throw Exception("Put failed: " + s.ToString() + " " + std::string(__FUNCTION__));
    }
}

void Store::GetPr(const Dissection& key, std::string *&value) throw(Exception)
{
    if ( state != eStoreOpened ) {
        throw Exception("Store in wrong state: " + to_string(state));
    }
    Status s = kvsd->Get(read_options, key, value);
    if ( !s.ok() ) {
        if ( debug == true ) {
            std::cout << __FUNCTION__ << " error: " << s.ToString() << std::endl << std::flush;
        }
        if ( s.IsNotFound() != true ) {
            throw Exception("Get failed: " + s.ToString() + " " + std::string(__FUNCTION__));
        }
        value = 0;
    }
}

void Store::DeletePr(const Dissection& key) throw(Exception)
{
    if ( state != eStoreOpened ) {
        throw Exception("Store in wrong state: " + to_string(state));
    }
    Status s = kvsd->Delete(write_options, key);
    if ( !s.ok() ) {
        if ( debug == true ) {
            std::cout << __FUNCTION__ << " error: " << s.ToString() << std::endl << std::flush;
        }
        throw Exception("Delete failed: " + s.ToString() + " " + std::string(__FUNCTION__));
    }
}

void Store::GetRangePr(const Dissection& start_key, const Dissection& end_key, RangeCallback range_callback, char *arg) throw(Exception)
{
    if ( state != eStoreOpened ) {
        throw Exception("Store in wrong state: " + to_string(state));
    }

    // Get a store iterator
    boost::shared_ptr<Iterator> iter(kvsd->NewIterator(read_options));

    if ( !start_key.data() ) {
        iter->SeekToFirst();
    } else {
        iter->Seek(start_key);
    }

    for ( iter->Seek(start_key) ; iter->Valid() && (!end_key.data() || iter->key().ToString() <= end_key.ToString()) ; iter->Next() ) {
        if ( iter->value().empty() != true ) {
            Dissection key_d(iter->key());
            Dissection data_d(iter->value());

            if ( range_callback ) {
                bool ret = range_callback(key_d.data(), key_d.size(), data_d.data(), data_d.size(), arg);
                if ( ret != true ) {
                    break;
                }
            }
        }
    }
}

bool Store::IsOpen()
{
    return state == eStoreOpened ? true : false;
}

void Store::RegisterApiStaticFunctions() throw(Exception)
{
    // bind functions to boost functors
    boost::function<uint64_t(LocalArgsDescriptor &)> StoreCreate =
        boost::bind(&KvsFacade::Cluster::CreateStore, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreDestroy =
        boost::bind(&KvsFacade::Cluster::DestroyStore, _1);

    boost::function<uint64_t(LocalArgsDescriptor &)> StoreGetByName =
        boost::bind(&KvsFacade::Cluster::GetStoreByName, _1);


    // register via SoeApi
    static_ftors.Register(new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
        StoreCreate, KvsFacade::CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_CREATE), Cluster::global_debug));

    static_ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreDestroy, KvsFacade::CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_DESTROY), Cluster::global_debug));

    static_ftors.Register(new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
            StoreGetByName, KvsFacade::CreateLocalName(STATIC_CLUSTER, STATIC_SPACE, "", 0, FUNCTION_GET_BY_NAME), Cluster::global_debug));
}

void Store::RegisterApiFunctions() throw(Exception)
{
    // bind functions to boost functors
    boost::function<void(LocalArgsDescriptor &)> StoreOpen =
        boost::bind(&KvsFacade::Store::Open, this, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreClose =
        boost::bind(&KvsFacade::Store::Close, this, _1);

    boost::function<void(LocalArgsDescriptor &)> StorePut =
        boost::bind(&KvsFacade::Store::Put, this, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreGet =
        boost::bind(&KvsFacade::Store::Get, this, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreDelete =
        boost::bind(&KvsFacade::Store::Delete, this, _1);

    boost::function<void(LocalArgsDescriptor &)> StoreGetRange =
        boost::bind(&KvsFacade::Store::GetRange, this, _1);

    // register via SoeApi
    assert(parent);
    assert(dynamic_cast<Space *>(parent)->parent);
    Space *sp = dynamic_cast<Space *>(parent);

    if ( debug ) {
        std::cout << "store: " << name << " space: " << sp->name << std::flush;
    }

    std::string &space = sp->name;
    Cluster *clu = dynamic_cast<Cluster *>(sp->parent);

    if ( debug ) {
        std::cout << " cluster: " << clu->name << std::flush;
    }

    std::string &cluster = clu->name;

    if ( debug ) {
        std::cout << " " << std::string(__FUNCTION__) << std::endl << std::flush;
    }

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreOpen, KvsFacade::CreateLocalName(cluster, space, name, 0, FUNCTION_OPEN), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
         StoreClose, KvsFacade::CreateLocalName(cluster, space, name, 0, FUNCTION_CLOSE), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StorePut, KvsFacade::CreateLocalName(cluster, space, name, 0, FUNCTION_PUT), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreGet, KvsFacade::CreateLocalName(cluster, space, name, 0, FUNCTION_GET), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreDelete, KvsFacade::CreateLocalName(cluster, space, name, 0, FUNCTION_DELETE), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        StoreGetRange, KvsFacade::CreateLocalName(cluster, space, name, 0, FUNCTION_GET_RANGE), Cluster::global_debug);
}

void Store::Open(LocalArgsDescriptor &args) throw(Exception)
{
    DIR *dir = ::opendir(path.c_str());
    if ( !dir ) {
        ::closedir(dir);
        throw Exception("Can't open store name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + std::string(__FUNCTION__));
    }

    std::string config = path + "/" + Cluster::db_elem_filename;
    std::string line;
    std::string all_lines;
    ifstream cfile(config.c_str());
    if ( cfile.is_open() != true ) {
        ::closedir(dir);
        throw Exception("Can't open config, name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + std::string(__FUNCTION__));
    }
    while ( cfile.eof() != true ) {
        cfile >> line;
        std::cout << "line: " << line;
        all_lines += line;
    }

    ParseConfig(all_lines);
    cfile.close();
    ::closedir(dir);

    this->OpenPr();
}

void Store::Close(LocalArgsDescriptor &args) throw(Exception)
{
    this->ClosePr();
}

void Store::Create()
{
    if ( debug ) {
        std::cout << "path: " << path << std::endl << std::flush;
    }

    DIR *dir = ::opendir(path.c_str());
    if ( dir ) {
        ::closedir(dir);
        std::cout << "Path already exists, store name: " << name << " path: " <<
            path << " errno: " << to_string(errno) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
        return;
    }

    int ret = ::mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if ( ret ) {
        throw Exception("Can't create path, store name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + std::string(__FUNCTION__));
    }

    std::string all_lines = CreateConfig(name, id, bloom_bits, overwrite_duplicates);

    std::string config = path + "/" + Cluster::db_elem_filename;
    ofstream cfile(config.c_str());
    cfile << all_lines;
    cfile.close();
}

void Store::Put(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.key || !args.data ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    PutPr(Dissection(args.key, args.key_len), Dissection(args.data, args.data_len));
}

void Store::Get(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.key ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    std::string value;
    std::string *str_ptr = &value;
    GetPr(Dissection(args.key, args.key_len), str_ptr);
    if ( str_ptr ) {
        size_t value_l = str_ptr->length();
        if ( value_l > args.buf_len ) {
            throw Exception("Buffer too small: " + to_string(args.buf_len) + " value.length: " +
                to_string(value_l) + " " + std::string(__FUNCTION__));
        }

        args.buf_len = value_l;
        memcpy(args.buf, str_ptr->c_str(), args.buf_len);
        args.buf[args.buf_len] = '\0';
    } else {
        args.buf_len = 0;
    }
}

void Store::Delete(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.key ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    DeletePr(Dissection(args.key, args.key_len));
}

void Store::GetRange(LocalArgsDescriptor &args) throw(Exception)
{
    // if key = 0 set the iter to the begin(), if end_key = 0 iterate to end()
    GetRangePr(Dissection(args.key, args.key_len), Dissection(args.end_key, args.key_len), args.range_callback, args.range_callback_arg);
}



//
// Miscellaneous functions
//
bool Store::StopAndArchiveName(LocalArgsDescriptor &args)
{
    state = eStoreDestroying;
    Cluster::ArchiveName(this);
    return true;
}



//
// Space
//
uint32_t Space::max_stores = 128;
uint64_t Space::space_null_id = -1;  // max 64 bit number

Space::Space(const std::string &nm, uint64_t _id, bool ch_names, bool db):
    name(nm),
    id(_id),
    checking_names(ch_names),
    debug(db)
{
    NameBase::next_available = 0;

    stores.resize(static_cast<size_t>(Space::max_stores), 0);
    for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
        Space::stores[i] = 0;
    }
}

Space::~Space()
{
    for ( uint32_t i = 0 ; i < stores.size() ; i++ ) {
        if ( stores[i] ) {
            delete stores[i];
        }
    }
}

void Space::SetPath()
{
    assert(parent);
    path = parent->GetPath() + "/" + name;
    if ( debug ) {
        std::cout << "path: " << path << " " << std::string(__FUNCTION__) << std::endl << std::flush;
    }
}

const std::string &Space::GetPath()
{
    return path;
}

void Space::SetNextAvailable()
{
    for ( uint32_t i = 0 ; i < Space::stores.size() ; i++ ) {
        if ( !Space::stores[i] ) {
            NameBase::next_available = i;
            break;
        }
    }
}

Store *Space::GetStore(LocalArgsDescriptor &args,
        bool ch_names,
        bool db)  throw(Exception)
{
    for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
        if ( Space::stores[i] && Space::stores[i]->name == *args.store_name ) {
            return Space::stores[i];
        }
    }

    for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
        if ( !Space::stores[i] ) {
            Space::stores[i] = new Store(*args.store_name, args.store_id, args.bloom_bits, args.overwrite_dups, db);
            Space::stores[i]->SetParent(this);
            Space::stores[i]->SetPath();
            Space::stores[i]->Create();
            Space::stores[i]->RegisterApiFunctions();
            return stores[i];
        }
    }

    throw Exception("No more space slots available");
}

void Space::DiscoverStores(const std::string &clu)
{
    std::string path = std::string(Cluster::db_root_path) + "/" + clu + "/" + name;
    DIR *dir = ::opendir(path.c_str());
    if ( !dir ) {
        std::cout << std::string("Can't open path: ") << path.c_str() <<
            to_string(errno) << " "<< std::string(__FUNCTION__) << std::endl << std::flush;
            return;
    }

    struct dirent *dir_entry;
    uint32_t cnt = 0;
    while ( (dir_entry = readdir(dir)) ) {
        if ( std::string(dir_entry->d_name) == "." ||
            std::string(dir_entry->d_name) == ".." ||
            std::string(dir_entry->d_name) == Cluster::db_elem_filename ) {
            continue;
        }
        std::cout << "Dir file: " << dir_entry->d_name << " " << std::string(__FUNCTION__) << std::endl << std::flush;

        std::string config = path + "/" + dir_entry->d_name + "/" + Cluster::db_elem_filename;
        std::string line;
        std::string all_lines;
        ifstream cfile(config.c_str());
        if ( cfile.is_open() != true ) {
            ::closedir(dir);
            std::cout << "Can't open config: " << config + " errno: " <<
                to_string(errno) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
                return;
        }
        while ( cfile.eof() != true ) {
            cfile >> line;
            //std::cout << "line: " << line;
            all_lines += line;
        }

        Space::stores[cnt] = new Store("zzzz", Store::store_null_id, Store::bloom_bits_default, Store::overdups_default, true);
        Space::stores[cnt]->SetParent(this);
        std::cout << std::string(__FUNCTION__) << " config: " << config << " json: " << all_lines << std::endl << std::flush;
        Space::stores[cnt]->ParseConfig(all_lines, true);
        Space::stores[cnt]->SetPath();
        Space::stores[cnt]->RegisterApiFunctions();
        cfile.close();
        if ( cnt++ >= Space::max_stores ) {
            break;
        }
    }
    ::closedir(dir);
}

//
// Find functions
//
Store *Space::FindStore(LocalArgsDescriptor &args) throw(Exception)
{
    for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
        if ( Space::stores[i] && Space::stores[i]->name == *args.store_name ) {
            return Space::stores[i];
        }
    }

    return 0;
}

//
// Create
//
uint64_t Space::CreateSpace(LocalArgsDescriptor &args) throw(Exception)
{
    return Cluster::CreateSpace(args);
}

void Space::DestroySpace(LocalArgsDescriptor &args) throw(Exception)
{
    return Cluster::DestroySpace(args);
}

uint64_t Space::GetSpaceByName(LocalArgsDescriptor &args) throw(Exception)
{
    return Cluster::GetSpaceByName(args);
}

void Space::ParseConfig(const std::string &cfg, bool no_exc)
{
    JS_DAT_SOE *json = jsoeGetJsonData();
     JS_RET_SOE ret = jsoeParse(json, reinterpret_cast<const uint8_t *>(cfg.c_str()), static_cast<size_t>(cfg.length()));
     if ( ret != JS_RET_SOE_OK ) {
         jsoeFree(json, 1);
         if ( no_exc == false ) {
             throw Exception("Json parse error, json: " + cfg + " error: " +
                 to_string(ret) + " " + std::string(__FUNCTION__));
         } else {
             std::cout << "Json parse error, json: " << cfg << " error: " <<
                 to_string(ret) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
         }
         return;
     }

     const JS_VAL_SOE *jnode = jsoeGetTopValue(json);

     char buf_name[1024];
     uint64_t val_id = 0;
     int i_ret = 0;
     if ( jsoeIsList(jnode) ) {
         const JS_DUPLE_SOE *pair;
         const JS_VAL_SOE *val;
         char buf[1024];

         void *ptr = jsoeGetValueStringUTF8(jnode, buf, sizeof(buf));

         for ( pair = jsoeGetFirstPair(jnode) ; pair ; pair = jsoeGetNextPair(pair) ) {
             ptr = jsoeGetPairNameUTF8(pair, buf, sizeof(buf));
             val = jsoeGetPairValue(pair);

             std::cout << "Json node: " << buf << " " << std::string(__FUNCTION__) << std::endl << std::flush;

             if ( jsoeIsString(val) ) {
                 i_ret = ProcessJsonString(val, buf_name, sizeof(buf_name));
             } else if ( jsoeIsNumber(val) ) {
                 i_ret = ProcessJsonNumber(val, val_id);
             } else {
                 jsoeFree(json, 1);
                 if ( no_exc == false ) {
                     throw Exception("Json invalid value error, json: " + cfg + " error: " +
                         to_string(i_ret) + " " + std::string(__FUNCTION__));
                 } else {
                     std::cout << "Json invalid value error, json: " << cfg << " error: " <<
                         to_string(i_ret) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
                 }
                 break;
             }

             if ( i_ret ) {
                 break;
             }
         }
     }

     if ( i_ret ) {
         jsoeFree(json, 1);
         if ( no_exc == false ) {
             throw Exception("Json value error, json: " + cfg + " error: " +
                 to_string(i_ret) + " " + std::string(__FUNCTION__));
         } else {
             std::cout << "Json value error, json: " <<  cfg + " error: " <<
                 to_string(i_ret) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
         }
         return;
     }

     std::cout << "Config json.name: " << buf_name << " id: " << val_id << std::endl << std::flush;
     name = buf_name;
     id = val_id;
     jsoeFree(json, 1);
}

std::string Space::CreateConfig(std::string &nm, uint64_t id, uint32_t b_bits, bool not_used)
{
    return "{ \"name\": \"" + nm + "\"" + ", \"id\": " + to_string(id) + "}";
}

void Space::PutPr(uint32_t store_idx, uint64_t store_id, const std::string &store_name,
    const char *data, size_t data_len,
    const char *key, size_t key_len) throw(Exception)
{
    if ( !key || !data ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    if ( store_idx >= Space::max_stores ) {
        throw Exception("Out-of-bounds store idx: " + to_string(store_idx) + " " + std::string(__FUNCTION__));
    }
    if ( !stores[store_idx] ) {
        throw Exception("Wrong store: " + to_string(store_idx) + " " + std::string(__FUNCTION__));
    }
    if ( stores[store_idx]->id == store_id &&
            (checking_names && store_name == stores[store_idx]->name) ) {
        stores[store_idx]->PutPr(Dissection(key, key_len), Dissection(data, data_len));
        return;
    }
    throw Exception("Store in wrong state idx: " + to_string(store_idx) + " " + std::string(__FUNCTION__));
}

void Space::GetPr(uint32_t store_idx, uint64_t store_id, const std::string &store_name,
    const char *key, size_t key_len,
    char *buf, size_t &buf_len) throw(Exception)
{
    if ( !key ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    if ( store_idx >= Space::max_stores ) {
        throw Exception("Out-of-bounds store idx: " + to_string(store_idx) + " " + std::string(__FUNCTION__));
    }
    if ( !stores[store_idx] ) {
        throw Exception("Wrong store: " + to_string(store_idx) + " " + std::string(__FUNCTION__));
    }
    if ( stores[store_idx]->id == store_id &&
            (checking_names && store_name == stores[store_idx]->name) ) {
        std::string value;
        std::string *str_ptr = &value;
        stores[store_idx]->GetPr(Dissection(key, key_len), str_ptr);
        if ( str_ptr ) {
            size_t value_l = str_ptr->length();
            if ( value_l > buf_len ) {
                throw Exception("Buffer too small: " + to_string(buf_len) + " value.length: " +
                    to_string(value_l) + " " + std::string(__FUNCTION__));
            }

            buf_len = value_l;
            memcpy(buf, str_ptr->c_str(), buf_len);
        } else {
            buf_len = 0;
        }
        return;
    }
    throw Exception("Store in wrong state idx: " + to_string(store_idx) + " " + std::string(__FUNCTION__));
}

void Space::DeletePr(uint32_t store_idx, uint64_t store_id,
    const std::string &store_name, const char *key, size_t key_len) throw(Exception)
{
    if ( !key ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    if ( store_idx >= Space::max_stores ) {
        throw Exception("Out-of-bounds store idx: " + to_string(store_idx) + " " + std::string(__FUNCTION__));
    }
    if ( !stores[store_idx] ) {
        throw Exception("Wrong store: " + to_string(store_idx) + " " + std::string(__FUNCTION__));
    }
    if (  stores[store_idx]->id == store_id &&
            (checking_names && store_name == stores[store_idx]->name) ) {
        stores[store_idx]->DeletePr(Dissection(key, key_len));
        return;
    }
    throw Exception("Store in wrong state idx: " + to_string(store_idx) + " " + std::string(__FUNCTION__));
}

//
// Registration functions
//
void Space::RegisterApiStaticFunctions() throw(Exception)
{
    // bind functions to boost functors
    boost::function<uint64_t(LocalArgsDescriptor &)> SpaceCreate =
        boost::bind(&KvsFacade::Cluster::CreateSpace, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceDestroy =
        boost::bind(&KvsFacade::Cluster::DestroySpace, _1);

    boost::function<uint64_t(LocalArgsDescriptor &)> SpaceGetByName =
        boost::bind(&KvsFacade::Cluster::GetSpaceByName, _1);


    // register via SoeApi
    static_ftors.Register(new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
        SpaceCreate, KvsFacade::CreateLocalName(STATIC_CLUSTER, "", "", 0, FUNCTION_CREATE), Cluster::global_debug));

    static_ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceDestroy, KvsFacade::CreateLocalName(STATIC_CLUSTER, "", "", 0, FUNCTION_DESTROY), Cluster::global_debug));

    static_ftors.Register(new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
            SpaceGetByName, KvsFacade::CreateLocalName(STATIC_CLUSTER, "", "", 0, FUNCTION_GET_BY_NAME), Cluster::global_debug));
}

void Space::RegisterApiFunctions() throw(Exception)
{
    // bind functions to boost functors
    boost::function<void(LocalArgsDescriptor &)> SpaceOpen =
        boost::bind(&KvsFacade::Space::Open, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceClose =
        boost::bind(&KvsFacade::Space::Close, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpacePut =
        boost::bind(&KvsFacade::Space::Put, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceGet =
        boost::bind(&KvsFacade::Space::Get, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceDelete =
        boost::bind(&KvsFacade::Space::Delete, this, _1);

    boost::function<void(LocalArgsDescriptor &)> SpaceGetRange =
        boost::bind(&KvsFacade::Space::GetRange, this, _1);


    // register via SoeApi
    assert(parent);
    std::string &cluster = dynamic_cast<Cluster *>(parent)->name;

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceOpen, KvsFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_OPEN), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
         SpaceClose, KvsFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_CLOSE), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpacePut, KvsFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_PUT), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceGet, KvsFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_GET), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceDelete, KvsFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_DELETE), Cluster::global_debug);

    new SoeApi::Api<void(LocalArgsDescriptor &)>(
        SpaceGetRange, KvsFacade::CreateLocalName(cluster, name, "", 0, FUNCTION_GET_RANGE), Cluster::global_debug);
}

//
// Dynamic API functions
//
void Space::Open(LocalArgsDescriptor &args) throw(Exception)
{
    DIR *dir = ::opendir(path.c_str());
    if ( !dir ) {
        ::closedir(dir);
        throw Exception("Can't open space name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + std::string(__FUNCTION__));
    }

    std::string config = path + "/" + Cluster::db_elem_filename;
    std::string line;
    std::string all_lines;
    ifstream cfile(config.c_str());
    if ( cfile.is_open() != true ) {
        ::closedir(dir);
        throw Exception("Can't open config, name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + std::string(__FUNCTION__));
    }
    while ( cfile.eof() != true ) {
        cfile >> line;
        std::cout << "line: " << line;
        all_lines += line;
    }

    ParseConfig(all_lines);
    cfile.close();
    ::closedir(dir);
}

void Space::Close(LocalArgsDescriptor &args) throw(Exception)
{
    throw Exception("NOP " + std::string(__FUNCTION__));
}

void Space::Create()
{
    if ( debug ) {
        std::cout << "path: " << path << std::endl << std::flush;
    }

    DIR *dir = ::opendir(path.c_str());
    if ( dir ) {
        ::closedir(dir);
        std::cout << "Path already exists, space name: " << name << " path: " <<
            path << " errno: " << to_string(errno) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
        return;
    }

    int ret = ::mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if ( ret ) {
        throw Exception("Can't create path, space name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + std::string(__FUNCTION__));
    }

    std::string all_lines = CreateConfig(name, id);

    std::string config = path + "/" + Cluster::db_elem_filename;
    ofstream cfile(config.c_str());
    cfile << all_lines;
    cfile.close();
}

void Space::Put(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.store_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    if ( args.store_idx >= Space::max_stores ) {
        throw Exception("Out-of-bounds store idx: " + to_string(args.store_idx) + " " + std::string(__FUNCTION__));
    }
    if ( !stores[args.store_idx] ) {
        throw Exception("Wrong store: " + to_string(args.store_idx) + " " + std::string(__FUNCTION__));
    }
    if ( stores[args.store_idx]->id == args.store_id &&
            (checking_names && *args.store_name == stores[args.store_idx]->name) ) {
        stores[args.store_idx]->Put(args);
        return;
    }
    throw Exception("Store in wrong state idx: " + to_string(args.store_idx) + " " + std::string(__FUNCTION__));

}

void Space::Get(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.store_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    if ( args.store_idx >= Space::max_stores ) {
        throw Exception("Out-of-bounds store idx: " + to_string(args.store_idx) + " " + std::string(__FUNCTION__));
    }
    if ( !stores[args.store_idx] ) {
        throw Exception("Wrong store: " + to_string(args.store_idx) + " " + std::string(__FUNCTION__));
    }
    if ( stores[args.store_idx]->id == args.store_id &&
            (checking_names && *args.store_name == stores[args.store_idx]->name) ) {
        std::string value;
        stores[args.store_idx]->Get(args);
        return;
    }
    throw Exception("Store in wrong state idx: " + to_string(args.store_idx) + " " + std::string(__FUNCTION__));

}

void Space::Delete(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.store_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    if ( args.store_idx >= Space::max_stores ) {
        throw Exception("Out-of-bounds store idx: " + to_string(args.store_idx) + " " + std::string(__FUNCTION__));
    }
    if ( !stores[args.store_idx] ) {
        throw Exception("Wrong store: " + to_string(args.store_idx) + " " + std::string(__FUNCTION__));
    }
    if (  stores[args.store_idx]->id == args.store_id &&
            (checking_names && *args.store_name == stores[args.store_idx]->name) ) {
        stores[args.store_idx]->Delete(args);
        return;
    }
    throw Exception("Store in wrong state idx: " + to_string(args.store_idx) + " " + std::string(__FUNCTION__));

}

void Space::GetRange(LocalArgsDescriptor &args) throw(Exception)
{
    throw Exception("Call GetRange on individual Store instead " + std::string(__FUNCTION__));
}



//
// Miscellaneous functions
//
bool Space::StopAndArchiveName(LocalArgsDescriptor &args)
{
    for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
        if ( Space::stores[i] && Space::stores[i]->name == *args.store_name &&
                Space::stores[i] && Space::stores[i]->id == args.store_id ) {
            bool ret = Space::stores[i]->StopAndArchiveName(args);
            if ( ret != true ) {
                return ret;
            }
            Space::stores[i] = 0;
        }
    }
    SetNextAvailable();
    state = eSpaceDestroying;

    Cluster::ArchiveName(this);
    return true;

}

bool Space::StopAndArchiveStore(LocalArgsDescriptor &args)
{
    bool ret = false;
    for ( uint32_t i = 0 ; i < Space::max_stores ; i++ ) {
        if ( Space::stores[i] && Space::stores[i]->name == *args.store_name &&
                Space::stores[i] && Space::stores[i]->id == args.store_id ) {
            bool ret = Space::stores[i]->StopAndArchiveName(args);
            if ( ret == true ) {
                Space::stores[i] = 0;
            }
            break;
        }
    }

    return ret;
}



//
// Cluster
//
uint32_t Cluster::max_spaces = 32;
uint32_t Cluster::max_clusters = 8;

uint64_t Cluster::cluster_null_id = -1;  // max 64 bit number

Cluster *Cluster::clusters[] = {0, 0, 0, 0, 0, 0, 0, 0};
Lock     Cluster::global_lock;

bool Cluster::global_debug = false;

std::vector<boost::shared_ptr<NameBase>>  Cluster::archive;

//
// Static initalizer
//
Cluster::Initializer *static_initializer_ptr = 0;
Cluster::Initializer static_initializer;

void StaticInitializerEnter(void)
{
    static_initializer_ptr = &static_initializer;
}

void StaticInitializerExit(void)
{
    static_initializer_ptr = 0;
}


bool Cluster::Initializer::initialized = false;
Cluster::Initializer::Initializer()
{
    WriteLock w_lock(Cluster::global_lock);

    if ( Initializer::initialized == false ) {
        try {
            Cluster::RegisterApiStaticFunctions();
            Space::RegisterApiStaticFunctions();
            Store::RegisterApiStaticFunctions();

            //std::cout << "Dir: " << Cluster::db_root_path << std::endl << std::flush;
            //std::cout << "config: " << Cluster::db_elem_filename << std::endl << std::flush;
            Cluster::Discover();
        } catch (const SoeApi::Exception &ex) {
            std::cout << "Register static functions failed: " << endl;
        } catch (...) {
            std::cout << "Register static functions failed: " << endl;
        }
    }
    Initializer::initialized = true;
}

void Cluster::Discover()
{
    DIR *dir = ::opendir(Cluster::db_root_path);
    if ( !dir ) {
        std::cout << std::string("Can't open root: ") << db_root_path <<
            to_string(errno) << " "<< std::string(__FUNCTION__) << std::endl << std::flush;
        return;
    }

    struct dirent *dir_entry;
    uint32_t cnt = 0;
    while ( (dir_entry = readdir(dir)) ) {
        if ( std::string(dir_entry->d_name) == "." || std::string(dir_entry->d_name) == ".." ) {
            continue;
        }
        std::cout << "Dir file: " << dir_entry->d_name << " " << std::string(__FUNCTION__) << std::endl << std::flush;

        std::string config = std::string(Cluster::db_root_path) + "/" + dir_entry->d_name + "/" + Cluster::db_elem_filename;
        std::string line;
        std::string all_lines;
        ifstream cfile(config.c_str());
        if ( cfile.is_open() != true ) {
            ::closedir(dir);
            std::cout << "Can't open config: " << config + " errno: " <<
                to_string(errno) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
                return;
        }
        while ( cfile.eof() != true ) {
            cfile >> line;
            //std::cout << "line: " << line;
            all_lines += line;
        }

        Cluster::clusters[cnt] = new Cluster("xxxx", Cluster::cluster_null_id, true, true);
        Cluster::clusters[cnt]->SetParent(Cluster::clusters[cnt]);
        std::cout << std::string(__FUNCTION__) << " config: " << config << " json: " << all_lines << std::endl << std::flush;
        Cluster::clusters[cnt]->ParseConfig(all_lines, true);
        Cluster::clusters[cnt]->SetPath();
        Cluster::clusters[cnt]->RegisterApiFunctions();
        Cluster::clusters[cnt]->state = Cluster::eClusterOpened;
        cfile.close();
        if ( cnt++ >= Cluster::max_clusters ) {
            break;
        }
    }
    ::closedir(dir);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] ) {
            Cluster::clusters[i]->DiscoverSpaces();
        }
    }
}

void Cluster::DiscoverSpaces()
{
    std::string path = std::string(Cluster::db_root_path) + "/" + name;
    DIR *dir = ::opendir(path.c_str());
    if ( !dir ) {
        std::cout << std::string("Can't open path: ") << path.c_str() <<
            to_string(errno) << " "<< std::string(__FUNCTION__) << std::endl << std::flush;
            return;
    }

    struct dirent *dir_entry;
    uint32_t cnt = 0;
    while ( (dir_entry = readdir(dir)) ) {
        if ( std::string(dir_entry->d_name) == "." ||
            std::string(dir_entry->d_name) == ".." ||
            std::string(dir_entry->d_name) == Cluster::db_elem_filename ) {
            continue;
        }
        std::cout << "Dir file: " << dir_entry->d_name << " " << std::string(__FUNCTION__) << std::endl << std::flush;

        std::string config = path + "/" + dir_entry->d_name + "/" + Cluster::db_elem_filename;
        std::string line;
        std::string all_lines;
        ifstream cfile(config.c_str());
        if ( cfile.is_open() != true ) {
            ::closedir(dir);
            std::cout << "Can't open config: " << config + " errno: " <<
                to_string(errno) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
                return;
        }
        while ( cfile.eof() != true ) {
            cfile >> line;
            //std::cout << "line: " << line;
            all_lines += line;
        }

        Cluster::spaces[cnt] = new Space("yyyy", Space::space_null_id, true, true);
        Cluster::spaces[cnt]->SetParent(this);
        std::cout << std::string(__FUNCTION__) << " config: " << config << " json: " << all_lines << std::endl << std::flush;
        Cluster::spaces[cnt]->ParseConfig(all_lines, true);
        Cluster::spaces[cnt]->SetPath();
        Cluster::spaces[cnt]->RegisterApiFunctions();
        Cluster::spaces[cnt]->state = Space::eSpaceOpened;
        cfile.close();
        if ( cnt++ >= Cluster::max_spaces ) {
            break;
        }
    }
    ::closedir(dir);

    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] ) {
            Cluster::spaces[i]->DiscoverStores(name);
        }
    }
}


//
// Non static iterator like functions
//
Cluster *Cluster::GetCluster(LocalArgsDescriptor &args,
        bool ch_names,
        bool db) throw(Exception)
{
    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] && Cluster::clusters[i]->name == *args.cluster_name ) {
            return Cluster::clusters[i];
        }
    }

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( !Cluster::clusters[i] ) {
            Cluster::clusters[i] = new Cluster(*args.cluster_name, args.cluster_id, ch_names, db);
            Cluster::clusters[i]->SetParent(Cluster::clusters[i]);
            Cluster::clusters[i]->SetPath();
            Cluster::clusters[i]->Create();
            Cluster::clusters[i]->RegisterApiFunctions();
            Cluster::clusters[i]->state = Cluster::eClusterOpened;
            return Cluster::clusters[i];
        }
    }

    throw Exception("No more cluster slots available");
}

Space *Cluster::GetSpace(LocalArgsDescriptor &args,
        bool ch_names,
        bool db) throw(Exception)
{
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] && Cluster::spaces[i]->name == *args.space_name ) {
            return Cluster::spaces[i];
        }
    }

    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( !Cluster::spaces[i] ) {
            Cluster::spaces[i] = new Space(*args.space_name, args.space_id, ch_names, db);
            Cluster::spaces[i]->SetParent(this);
            Cluster::spaces[i]->SetPath();
            Cluster::spaces[i]->Create();
            Cluster::spaces[i]->RegisterApiFunctions();
            Cluster::spaces[i]->state = Space::eSpaceOpened;
            return Cluster::spaces[i];
        }
    }

    throw Exception("No more cluster slots available");
}

Store *Cluster::GetStore(LocalArgsDescriptor &args,
        bool ch_names,
        bool db) throw(Exception)
{
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] && Cluster::spaces[i]->name == *args.space_name &&
            Cluster::spaces[i]->id == args.space_id ) {
            return Cluster::spaces[i]->GetStore(args, ch_names, db);
        }
    }

    throw Exception("Space name: " + *args.space_name + " not found " + std::string(__FUNCTION__));
}


//
// Create functions
//
uint64_t Cluster::CreateCluster(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.cluster_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    WriteLock w_lock(Cluster::global_lock);

    Cluster *clu = Cluster::GetCluster(args, true, true);
    if ( clu ) {
        return clu->id;
    }
    return Cluster::cluster_null_id;
}

uint64_t Cluster::CreateSpace(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.cluster_name || !args.space_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    WriteLock w_lock(Cluster::global_lock);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] &&
            Cluster::clusters[i]->name == *args.cluster_name &&
            Cluster::clusters[i]->id == args.cluster_id ) {
            Space *sp = Cluster::clusters[i]->GetSpace(args, true, true);
            if ( sp ) {
                return sp->id;
            }
            break;
        }
    }

    throw Exception("Cluster name: " + *args.cluster_name + " not found " + std::string(__FUNCTION__));
}

uint64_t Cluster::CreateStore(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.cluster_name || !args.space_name || !args.store_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    WriteLock w_lock(Cluster::global_lock);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] &&
            Cluster::clusters[i]->name == *args.cluster_name &&
            Cluster::clusters[i]->id == args.cluster_id ) {
            Store *st = Cluster::clusters[i]->GetStore(args, true, true);
            if ( st ) {
                return st->id;
            }
            break;
        }
    }

    throw Exception("Cluster name: " + *args.cluster_name + " not found " + std::string(__FUNCTION__));
}

//
// Destroy functions
// The general approach is that because operations may be in progress we need to:
//   1. Put the object on the list to be destroyed
//   2. Together, tint it so that new i/os won't get started (throw exception)
//   3. Quiesce all the i/os
//   4. Once there are no i/os and clients hold no references destroy it, i.e.
//      remove the contents on disk and delete the object in memory
//
void Cluster::DestroyCluster(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.cluster_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    WriteLock w_lock(Cluster::global_lock);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] && Cluster::clusters[i]->name == *args.cluster_name &&
            Cluster::clusters[i]->id == args.cluster_id ) {
            bool ret = Cluster::clusters[i]->StopAndArchiveName(args);
            if ( ret != true ) {
                throw Exception("Can't destroy cluster: " + *args.cluster_name + " id: " +
                    to_string(args.cluster_id) + " " + std::string(__FUNCTION__));
            }
            Cluster::clusters[i] = 0;
        }
    }

    throw Exception("Cluster not found: " + *args.cluster_name + " id: " +
        to_string(args.cluster_id) + " " + std::string(__FUNCTION__));
}

void Cluster::DestroySpace(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.cluster_name || !args.space_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    WriteLock w_lock(Cluster::global_lock);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] && Cluster::clusters[i]->name == *args.cluster_name &&
            Cluster::clusters[i]->id == args.cluster_id ) {
            bool ret = Cluster::clusters[i]->StopAndArchiveSpace(args);
            if ( ret != true ) {
                throw Exception("Can't destroy space, cluster: " + *args.cluster_name + " id: " +
                    to_string(args.cluster_id) +
                    " space: " + *args.space_name + " space_id: " + to_string(args.space_id) +
                    " " + std::string(__FUNCTION__));
            }
            return;
        }
    }

    throw Exception("Space not found, cluster: " + *args.cluster_name + " id: " +
        to_string(args.cluster_id) +
        " space: " + *args.space_name + " space_id: " + to_string(args.space_id) +
        " " + std::string(__FUNCTION__));
}

void Cluster::DestroyStore(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.cluster_name || !args.space_name || !args.store_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    WriteLock w_lock(Cluster::global_lock);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] && Cluster::clusters[i]->name == *args.cluster_name &&
            Cluster::clusters[i]->id == args.cluster_id ) {
            bool ret = Cluster::clusters[i]->StopAndArchiveStore(args);
            if ( ret != true ) {
                throw Exception("Can't destroy store, cluster: " + *args.cluster_name + " id: " +
                    to_string(args.cluster_id) +
                    " space: " + *args.space_name + " space_id: " + to_string(args.space_id) +
                    " store: " + *args.store_name + " store_id: " + to_string(args.store_id) +
                    " " + std::string(__FUNCTION__));
            }
            return;
        }
    }

    throw Exception("Store not found, cluster: " + *args.cluster_name + " id: " +
        to_string(args.cluster_id) +
        " space: " + *args.space_name + " space_id: " + to_string(args.space_id) +
        " store: " + *args.store_name + " store_id: " + to_string(args.store_id) +
        " " + std::string(__FUNCTION__));
}

//
// GetByName functions
//
uint64_t Cluster::GetClusterByName(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.cluster_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    ReadLock r_lock(Cluster::global_lock);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] && Cluster::clusters[i]->name == *args.cluster_name ) {
            return Cluster::clusters[i]->id;
        }
    }
    return Cluster::cluster_null_id;
}

uint64_t Cluster::GetSpaceByName(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.cluster_name || !args.space_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    ReadLock r_lock(Cluster::global_lock);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] && Cluster::clusters[i]->name == *args.cluster_name &&
            Cluster::clusters[i]->id == args.cluster_id ) {
            Space *sp = Cluster::clusters[i]->FindSpace(args);
            if ( sp ) {
                return sp->id;
            }
            break;
        }
    }
    return Space::space_null_id;
}

uint64_t Cluster::GetStoreByName(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.cluster_name || !args.space_name || !args.store_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    ReadLock r_lock(Cluster::global_lock);

    for ( uint32_t i = 0 ; i < Cluster::max_clusters ; i++ ) {
        if ( Cluster::clusters[i] && Cluster::clusters[i]->name == *args.cluster_name &&
            Cluster::clusters[i]->id == args.cluster_id ) {
            Store *st = Cluster::clusters[i]->FindStore(args);
            if ( st ) {
                return st->id;
            }
            break;
        }
    }
    return Store::store_null_id;
}

//
// Find functions
//
Space *Cluster::FindSpace(LocalArgsDescriptor &args) throw(Exception)
{
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] && Cluster::spaces[i]->name == *args.space_name ) {
            return Cluster::spaces[i];
        }
    }
    return 0;
}

Store *Cluster::FindStore(LocalArgsDescriptor &args) throw(Exception)
{
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] && Cluster::spaces[i]->name == *args.space_name &&
                Cluster::spaces[i]->id == args.space_id ) {
            return Cluster::spaces[i]->FindStore(args);
        }
    }
    return 0;
}

//
// Register API functions
//
void Cluster::RegisterApiStaticFunctions() throw(Exception)
{
    try {
        // bind functions to boost functors
        boost::function<uint64_t(LocalArgsDescriptor &)> ClusterLocalCreate =
            boost::bind(&KvsFacade::Cluster::CreateCluster, _1);

        boost::function<void(LocalArgsDescriptor &)> ClusterLocalDestroy =
            boost::bind(&KvsFacade::Cluster::DestroyCluster, _1);

        boost::function<uint64_t(LocalArgsDescriptor &)> ClusterLocalGetByName =
            boost::bind(&KvsFacade::Cluster::GetClusterByName, _1);

        // register via SoeApi
        static_ftors.Register(new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
            ClusterLocalCreate, KvsFacade::CreateLocalName("", "", "", 0, FUNCTION_CREATE), Cluster::global_debug));

        static_ftors.Register(new SoeApi::Api<void(LocalArgsDescriptor &)>(
             ClusterLocalDestroy, KvsFacade::CreateLocalName("", "", "", 0, FUNCTION_DESTROY), Cluster::global_debug));

        static_ftors.Register(new SoeApi::Api<uint64_t(LocalArgsDescriptor &)>(
                ClusterLocalGetByName, KvsFacade::CreateLocalName("", "", "", 0, FUNCTION_GET_BY_NAME), Cluster::global_debug));
    } catch (const SoeApi::Exception &ex) {
        throw ex;
    } catch(...) {
        cout << "Unknow exception caught" << endl;
    }
}

void Cluster::RegisterApiFunctions() throw(Exception)
{
    try {
        // bind functions to boost functors
        boost::function<void(LocalArgsDescriptor &)> ClusterLocalOpen =
            boost::bind(&KvsFacade::Cluster::Open, this, _1);

        boost::function<void(LocalArgsDescriptor &)> ClusterLocalClose =
            boost::bind(&KvsFacade::Cluster::Close, this, _1);

        boost::function<void(LocalArgsDescriptor &)> ClusterLocalPut =
            boost::bind(&KvsFacade::Cluster::Put, this, _1);

        boost::function<void(LocalArgsDescriptor &)> ClusterLocalGet =
            boost::bind(&KvsFacade::Cluster::Get, this, _1);

        boost::function<void(LocalArgsDescriptor &)> ClusterLocalDelete =
            boost::bind(&KvsFacade::Cluster::Delete, this, _1);

        boost::function<void(LocalArgsDescriptor &)> ClusterLocalGetRange =
            boost::bind(&KvsFacade::Cluster::GetRange, this, _1);


        // register via SoeApi
        new SoeApi::Api<void(LocalArgsDescriptor &)>(
            ClusterLocalOpen, KvsFacade::CreateLocalName(name, "", "", 0, FUNCTION_OPEN), Cluster::global_debug);

        new SoeApi::Api<void(LocalArgsDescriptor &)>(
            ClusterLocalClose, KvsFacade::CreateLocalName(name, "", "", 0, FUNCTION_CLOSE), Cluster::global_debug);

        new SoeApi::Api<void(LocalArgsDescriptor &)>(
            ClusterLocalPut, KvsFacade::CreateLocalName(name, "", "", 0, FUNCTION_PUT), Cluster::global_debug);

        new SoeApi::Api<void(LocalArgsDescriptor &)>(
            ClusterLocalGet, KvsFacade::CreateLocalName(name, "", "", 0, FUNCTION_GET), Cluster::global_debug);

        new SoeApi::Api<void(LocalArgsDescriptor &)>(
            ClusterLocalDelete, KvsFacade::CreateLocalName(name, "", "", 0, FUNCTION_DELETE), Cluster::global_debug);

        new SoeApi::Api<void(LocalArgsDescriptor &)>(
            ClusterLocalGetRange, KvsFacade::CreateLocalName(name, "", "", 0, FUNCTION_GET_RANGE), Cluster::global_debug);
    } catch (const SoeApi::Exception &ex) {
        throw ex;
    } catch(...) {
        cout << "Unknow exception caught" << endl;
    }
}


const char *Cluster::db_root_path = "/var/KVS_cluster";
const char *Cluster::db_elem_filename = ".config.json";

//
// Ctor/Dest functions
//
Cluster::Cluster(const std::string &nm,
        uint64_t _id,
        bool ch_names,
        bool db) throw(Exception):
    name(nm),
    id(_id),
    checking_names(ch_names),
    debug(db)
{
    NameBase::next_available = 0;

    spaces.resize(static_cast<size_t>(Cluster::max_spaces), 0);
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        Cluster::spaces[i] = 0;
    }
}

Cluster::~Cluster()
{
    for ( uint32_t i = 0 ; i < spaces.size() ; i++ ) {
        if ( spaces[i] ) {
            delete spaces[i];
        }
    }
}

void Cluster::SetNextAvailable()
{
    for ( uint32_t i = 0 ; i < spaces.size() ; i++ ) {
        if ( !spaces[i] ) {
            NameBase::next_available = i;
            break;
        }
    }
}

void Cluster::SetPath()
{
    path = std::string(db_root_path) + "/" + name;
    if ( debug ) {
        std::cout << "path: " << path << " " << std::string(__FUNCTION__) << std::endl << std::flush;
    }
}

const std::string &Cluster::GetPath()
{
    return path;
}

void Cluster::ParseConfig(const std::string &cfg, bool no_exc)
{
    JS_DAT_SOE *json = jsoeGetJsonData();
    JS_RET_SOE ret = jsoeParse(json, reinterpret_cast<const uint8_t *>(cfg.c_str()), static_cast<size_t>(cfg.length()));
    if ( ret != JS_RET_SOE_OK ) {
        jsoeFree(json, 1);
        if ( no_exc == false ) {
            throw Exception("Json parse error, json: " + cfg + " error: " +
                to_string(ret) + " " + std::string(__FUNCTION__));
        } else {
            std::cout << "Json parse error, json: " << cfg << " error: " <<
                to_string(ret) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
        }
        return;
    }

    const JS_VAL_SOE *jnode = jsoeGetTopValue(json);

    char buf_name[1024];
    uint64_t val_id = 0;
    int i_ret = 0;
    if ( jsoeIsList(jnode) ) {
        const JS_DUPLE_SOE *pair = NULL;
        const JS_VAL_SOE *val = NULL;
        char buf[1024];

        void *ptr = jsoeGetValueStringUTF8(jnode, buf, sizeof(buf));

        for ( pair = jsoeGetFirstPair(jnode) ; pair ; pair = jsoeGetNextPair(pair) ) {
            ptr = jsoeGetPairNameUTF8(pair, buf, sizeof(buf));
            val = jsoeGetPairValue(pair);

            std::cout << "Json node: " << buf << " " << std::string(__FUNCTION__) << std::endl << std::flush;

            if ( jsoeIsString(val) ) {
                i_ret = ProcessJsonString(val, buf_name, sizeof(buf_name));
            } else if ( jsoeIsNumber(val) ) {
                i_ret = ProcessJsonNumber(val, val_id);
            } else {
                jsoeFree(json, 1);
                if ( no_exc == false ) {
                    throw Exception("Json invalid value error, json: " + cfg + " error: " +
                        to_string(i_ret) + " " + std::string(__FUNCTION__));
                } else {
                    std::cout << "Json invalid value error, json: " << cfg << " error: " <<
                        to_string(i_ret) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
                }
                break;
            }

            if ( i_ret ) {
                break;
            }
        }
    }

    if ( i_ret ) {
        jsoeFree(json, 1);
        if ( no_exc == false ) {
            throw Exception("Json value error, json: " + cfg + " error: " +
                to_string(i_ret) + " " + std::string(__FUNCTION__));
        } else {
            std::cout << "Json value error, json: " <<  cfg + " error: " <<
                to_string(i_ret) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
        }
        return;
    }

    std::cout << "Config json.name: " << buf_name << " id: " << val_id << std::endl << std::flush;
    name = buf_name;
    id = val_id;
    jsoeFree(json, 1);
}

std::string Cluster::CreateConfig(std::string &nm, uint64_t id, uint32_t b_bits, bool not_used)
{
    return "{ \"name\": \"" + nm + "\"" + ", \"id\": " + to_string(id) + "}";
}

void Cluster::Create()
{
    if ( debug ) {
        std::cout << "path: " << path << std::endl << std::flush;
    }

    DIR *dir = ::opendir(path.c_str());
    if ( dir ) {
        ::closedir(dir);
        std::cout << "Path already exists, cluster name: " << name << " path: " <<
            path << " errno: " << to_string(errno) << " " << std::string(__FUNCTION__) << std::endl << std::flush;
        return;
    }

    int ret = ::mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if ( ret ) {
        throw Exception("Can't create path, cluster name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + std::string(__FUNCTION__));
    }

    std::string all_lines = CreateConfig(name, id);

    std::string config = path + "/" + Cluster::db_elem_filename;
    ofstream cfile(config.c_str());
    cfile << all_lines;
    cfile.close();
}


//
// Miscellaneous functions
// WARNING: This operation destroys the entire cluster
//
bool Cluster::StopAndArchiveName(LocalArgsDescriptor &args)
{
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] && Cluster::spaces[i]->name == *args.space_name &&
                Cluster::spaces[i] && Cluster::spaces[i]->id == args.space_id ) {
            bool ret = Cluster::spaces[i]->StopAndArchiveName(args);
            if ( ret != true ) {
                return ret;
            }
            Cluster::spaces[i] = 0;
        }
    }
    SetNextAvailable();
    state = eClusterDestroying;
    Cluster::ArchiveName(this);

    return true;
}

bool Cluster::StopAndArchiveSpace(LocalArgsDescriptor &args)
{
    bool ret = false;
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] && Cluster::spaces[i]->name == *args.space_name &&
                Cluster::spaces[i] && Cluster::spaces[i]->id == args.space_id ) {
            ret = Cluster::spaces[i]->StopAndArchiveName(args);
            if ( ret == true ) {
                Cluster::spaces[i] = 0;
            }
            break;
        }
    }

    return ret;
}

bool Cluster::StopAndArchiveStore(LocalArgsDescriptor &args)
{
    for ( uint32_t i = 0 ; i < Cluster::max_spaces ; i++ ) {
        if ( Cluster::spaces[i] && Cluster::spaces[i]->name == *args.space_name &&
                Cluster::spaces[i] && Cluster::spaces[i]->id == args.space_id ) {
            return Cluster::spaces[i]->StopAndArchiveStore(args);
        }
    }

    return false;
}

void Cluster::ArchiveName(const NameBase *name)
{
    Cluster::archive.push_back(boost::shared_ptr<NameBase>(const_cast<NameBase *>(name)));
}

//
// Dynamic API functions
//
void Cluster::Open(LocalArgsDescriptor &args) throw(Exception)
{
    DIR *dir = ::opendir(path.c_str());
    if ( !dir ) {
        ::closedir(dir);
        throw Exception("Can't open cluster name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + std::string(__FUNCTION__));
    }

    std::string config = path + "/" + Cluster::db_elem_filename;
    std::string line;
    std::string all_lines;
    ifstream cfile(config.c_str());
    if ( cfile.is_open() != true ) {
        ::closedir(dir);
        throw Exception("Can't open config, name: " + name + " path: " +
            path + " errno: " + to_string(errno) + " " + std::string(__FUNCTION__));
    }
    while ( cfile.eof() != true ) {
        cfile >> line;
        std::cout << "line: " << line;
        all_lines += line;
    }

    ParseConfig(all_lines);
    cfile.close();
    ::closedir(dir);
}

void Cluster::Close(LocalArgsDescriptor &args) throw(Exception)
{
    throw Exception("NOP " + std::string(__FUNCTION__));
}

void Cluster::Put(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.space_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    ReadLock r_lock(Cluster::global_lock);

    if ( args.space_idx >= Cluster::max_spaces  || !spaces[args.space_idx] ) {
        throw Exception("Wrong space_idx: " + to_string(args.space_idx) + " " + std::string(__FUNCTION__));
    }
    if ( spaces[args.space_idx]->id != args.space_id ||
            (checking_names == true && spaces[args.space_idx]->name != *args.space_name) ) {
        throw Exception("Wrong space: " + *args.space_name + " id: " + to_string(args.space_id) +
            " []->name: " + spaces[args.space_idx]->name + " []->id: " +
            to_string(spaces[args.space_idx]->id) + " " + std::string(__FUNCTION__));
    }
    spaces[args.space_idx]->Put(args);
}

void Cluster::Get(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.space_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    ReadLock r_lock(Cluster::global_lock);

    if ( args.space_idx >= Cluster::max_spaces  || !spaces[args.space_idx] ) {
        throw Exception("Wrong space_idx: " + to_string(args.space_idx) + " " + std::string(__FUNCTION__));
    }
    if ( spaces[args.space_idx]->id != args.space_id ||
            (checking_names == true && spaces[args.space_idx]->name != *args.space_name) ) {
        throw Exception("Wrong space: " + *args.space_name + " id: " + to_string(args.space_id) +
            " []->name: " + spaces[args.space_idx]->name + " []->id: " +
            to_string(spaces[args.space_idx]->id) + " " + std::string(__FUNCTION__));
    }
    spaces[args.space_idx]->Get(args);
}

void Cluster::Delete(LocalArgsDescriptor &args) throw(Exception)
{
    if ( !args.space_name ) {
        throw Exception("Bad argument " + std::string(__FUNCTION__));
    }

    ReadLock r_lock(Cluster::global_lock);

    if ( args.space_idx >= Cluster::max_spaces  || !spaces[args.space_idx] ) {
        throw Exception("Wrong space_idx: " + to_string(args.space_idx) + " " + std::string(__FUNCTION__));
    }
    if ( spaces[args.space_idx]->id != args.space_id ||
            (checking_names == true && spaces[args.space_idx]->name != *args.space_name) ) {
        throw Exception("Wrong space: " + *args.space_name + " id: " + to_string(args.space_id) +
            " []->name: " + spaces[args.space_idx]->name + " []->id: " +
            to_string(spaces[args.space_idx]->id) + " " + std::string(__FUNCTION__));
    }
    spaces[args.space_idx]->Delete(args);
}

void Cluster::GetRange(LocalArgsDescriptor &args) throw(Exception)
{
    throw Exception("Call GetRange on individual Store instead " + std::string(__FUNCTION__));
}


}


