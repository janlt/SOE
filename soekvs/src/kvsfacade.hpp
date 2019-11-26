/*
 * kvsfacade.hpp
 *
 *  Created on: Nov 29, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_KVS_SRC_KVSFACADE_HPP_
#define SOE_SOE_SOE_KVS_SRC_KVSFACADE_HPP_

#include "kvsfacadeapi.hpp"

using namespace LzKVStore;

typedef boost::shared_mutex Lock;
typedef boost::unique_lock<Lock> WriteLock;
typedef boost::shared_lock<Lock> ReadLock;

namespace KvsFacade {

void StaticInitializerEnter(void);
void StaticInitializerExit(void);

class Store;
class Space;
class Cluster;

class NameBase
{
friend class Cluster;
friend class Space;
friend class Store;

public:
    uint32_t next_available;

public:
    virtual ~NameBase() {}

protected:
    NameBase() {}
    virtual void Create() = 0;
    virtual void SetParent(NameBase *par) = 0;
    virtual void SetPath() = 0;
    virtual const std::string &GetPath() = 0;
    virtual void ParseConfig(const std::string &cfg, bool no_exc = false) = 0;
    virtual std::string CreateConfig(std::string &nm,
        uint64_t id,
        uint32_t b_bits = 4,
        bool overwr_dups = false) = 0;
    virtual bool StopAndArchiveName(LocalArgsDescriptor &args) = 0;

    virtual void RegisterApiFunctions() throw(Exception) = 0;

    virtual void Open(LocalArgsDescriptor &args) throw(Exception) = 0;
    virtual void Close(LocalArgsDescriptor &args) throw(Exception) = 0;
    virtual void Put(LocalArgsDescriptor &args) throw(Exception) = 0;
    virtual void Get(LocalArgsDescriptor &args) throw(Exception) = 0;
    virtual void Delete(LocalArgsDescriptor &args) throw(Exception) = 0;
    virtual void GetRange(LocalArgsDescriptor &args) throw(Exception) = 0;

    virtual void SetNextAvailable() = 0;
};


//
// Store
//
class Store: public NameBase
{
    friend class Space;
    friend class Cluster;

public:
    typedef enum _eStoreState {
        eStoreInit        = 0,
        eStoreOpened      = 1,
        eStoreClosing     = 2,
        eStoreDestroying  = 3,
        eStoreDestroyed   = 4,
        eStoreClosed      = 5
    } eStoreState;

private:
    Accell                *cache;
    const FilteringPolicy *filter_policy;
    KVDatabase            *kvsd;
    OpenOptions            open_options;
    ReadOptions            read_options;
    WriteOptions           write_options;
    eStoreState            state;
    bool                   batch_mode;
    uint32_t               bloom_bits;
    bool                   overwrite_duplicates;
    bool                   debug;

public:
    std::string            name;
    NameBase              *parent;
    uint64_t               id;
    std::string            path;

    static uint64_t        store_null_id;
    static uint32_t        bloom_bits_default;
    static bool            overdups_default;

    static RcsdbFacade::FunctorBase static_ftors;

protected:
    void SetOpenOptions();
    void SetReadOptions();
    void SetWriteOptions();

    Store(const std::string &nm,
        uint64_t _id,
        uint32_t b_bits = Store::bloom_bits_default,
        bool over_dup = false,
        bool db = false);
    virtual void Create();
    virtual void SetPath();
    virtual const std::string &GetPath();
    virtual void ParseConfig(const std::string &cfg, bool no_exc = false);
    virtual std::string CreateConfig(std::string &nm,
        uint64_t id,
        uint32_t b_bits = Store::bloom_bits_default,
        bool overwr_dups = Store::overdups_default);
    virtual bool StopAndArchiveName(LocalArgsDescriptor &args);
    virtual void SetParent(NameBase *par) { parent = par; }

public:
    virtual ~Store();

    static uint64_t CreateStore(LocalArgsDescriptor &args) throw(Exception);
    static void DestroyStore(LocalArgsDescriptor &args) throw(Exception);
    static uint64_t GetStoreByName(LocalArgsDescriptor &args) throw(Exception);

    virtual void Open(LocalArgsDescriptor &args) throw(Exception);
    virtual void Close(LocalArgsDescriptor &args) throw(Exception);
    virtual void Put(LocalArgsDescriptor &args) throw(Exception);
    virtual void Get(LocalArgsDescriptor &args) throw(Exception);
    virtual void Delete(LocalArgsDescriptor &args) throw(Exception);
    virtual void GetRange(LocalArgsDescriptor &args) throw(Exception);

    bool IsOpen();

protected:
    void OpenPr() throw(Exception);
    void ClosePr() throw(Exception);
    void Crc32cPr() throw(Exception);
    void PutPr(const Dissection& key, const Dissection& value) throw(Exception);
    void GetPr(const Dissection& key, std::string *&value) throw(Exception);
    void DeletePr(const Dissection& key) throw(Exception);
    void GetRangePr(const Dissection& start_key, const Dissection& end_key, RangeCallback range_callback, char *arg) throw(Exception);

    virtual void RegisterApiFunctions() throw(Exception);
    static void RegisterApiStaticFunctions() throw(Exception);

    virtual void SetNextAvailable();
};


//
// Space
//
class Space: public NameBase
{
    friend class Cluster;
    friend class Store;

public:
    typedef enum _eSpaceState {
        eSpaceOpened       = 0,
        eSpaceDestroying   = 1,
        eSpaceDestroyed    = 2,
        eSpaceClosing      = 3,
        eSpaceClosed       = 4
    } eSpaceState;

private:
    std::vector<KvsFacade::Store *>  stores;
    static uint32_t                  max_stores;

public:
    std::string                      name;
    NameBase                        *parent;
    uint64_t                         id;
    bool                             checking_names;
    bool                             debug;
    std::string                      path;
    eSpaceState                      state;

    static uint64_t                  space_null_id;
    static RcsdbFacade::FunctorBase  static_ftors;

protected:
    Space(const std::string &nm, uint64_t _id = 0, bool ch_names = true, bool db = false);
    virtual void Create();
    virtual void SetParent(NameBase *par) { parent = par; }
    virtual void SetPath();
    virtual const std::string &GetPath();
    virtual void ParseConfig(const std::string &cfg, bool no_exc = false);
    virtual std::string CreateConfig(std::string &nm,
        uint64_t id,
        uint32_t b_bits = Store::bloom_bits_default,
        bool overwr_dups = Store::overdups_default);
    virtual bool StopAndArchiveName(LocalArgsDescriptor &args);

    void DiscoverStores(const std::string &clu);

public:
    virtual ~Space();

    static uint64_t CreateSpace(LocalArgsDescriptor &args) throw(Exception);
    static void DestroySpace(LocalArgsDescriptor &args) throw(Exception);
    static uint64_t GetSpaceByName(LocalArgsDescriptor &args) throw(Exception);

    virtual void Open(LocalArgsDescriptor &args) throw(Exception);
    virtual void Close(LocalArgsDescriptor &args) throw(Exception);
    virtual void Put(LocalArgsDescriptor &args) throw(Exception);
    virtual void Get(LocalArgsDescriptor &args) throw(Exception);
    virtual void Delete(LocalArgsDescriptor &args) throw(Exception);
    virtual void GetRange(LocalArgsDescriptor &args) throw(Exception);

protected:
    void PutPr(uint32_t store_idx, uint64_t store_id, const std::string &store_name,
        const char *data, size_t data_len,
        const char *key, size_t key_len) throw(Exception);

    void GetPr(uint32_t store_idx, uint64_t store_id, const std::string &store_name,
        const char *key, size_t key_len,
        char *buf, size_t &buf_len) throw(Exception);

    void DeletePr(uint32_t store_idx, uint64_t store_id, const std::string &store_name,
        const char *key, size_t key_len) throw(Exception);

    Store *GetStore(LocalArgsDescriptor &args,
            bool ch_names = true,
            bool db = false)  throw(Exception);

    Store *FindStore(LocalArgsDescriptor &args) throw(Exception);
    bool StopAndArchiveStore(LocalArgsDescriptor &args);

    virtual void RegisterApiFunctions() throw(Exception);
    static void RegisterApiStaticFunctions() throw(Exception);

    virtual void SetNextAvailable();
};




//
// Cluster is N-leton
//
class Cluster: public NameBase
{
    friend class Space;
    friend class Store;

public:
    typedef enum _eClusterState {
        eClusterOpened       = 0,
        eClusterDestroying   = 1,
        eClusterDestroyed    = 2,
        eClusterClosing      = 3,
        eClusterClosed       = 4
    } eClusterState;

private:
    std::vector<KvsFacade::Space *>  spaces;
    static uint32_t                  max_spaces;

    static uint64_t                  cluster_null_id;

    static uint32_t                  max_clusters;
    static Cluster                  *clusters[];
    static Lock                      global_lock;

    static bool                      global_debug;
    eClusterState                    state;

    static std::vector<boost::shared_ptr<NameBase>>  archive;

public:
    static const char               *db_root_path;
    static const char               *db_elem_filename;

public:
    std::string                      name;
    NameBase                        *parent;
    uint64_t                         id;
    bool                             checking_names;
    bool                             debug;
    std::string                      path;
    static RcsdbFacade::FunctorBase  static_ftors;

    class Initializer {
    public:
        static bool initialized;
        Initializer();
    };
    static Initializer initalized;

public:
    virtual ~Cluster();
    virtual void SetParent(NameBase *par) { parent = this; }

    static uint64_t CreateCluster(LocalArgsDescriptor &args) throw(Exception);
    static void DestroyCluster(LocalArgsDescriptor &args) throw(Exception);
    static uint64_t GetClusterByName(LocalArgsDescriptor &args) throw(Exception);

    virtual void Open(LocalArgsDescriptor &args) throw(Exception);
    virtual void Close(LocalArgsDescriptor &args) throw(Exception);
    virtual void Put(LocalArgsDescriptor &args) throw(Exception);
    virtual void Get(LocalArgsDescriptor &args) throw(Exception);
    virtual void Delete(LocalArgsDescriptor &args) throw(Exception);
    virtual void GetRange(LocalArgsDescriptor &args) throw(Exception);

protected:
    Cluster(const std::string &nm,
            uint64_t _id = 0,
            bool ch_names = true,
            bool db = false) throw(Exception);
    virtual void SetNextAvailable();
    virtual void Create();
    virtual void SetPath();
    virtual const std::string &GetPath();
    virtual void ParseConfig(const std::string &cfg, bool no_exc = false);
    virtual std::string CreateConfig(std::string &nm,
        uint64_t id,
        uint32_t b_bits = Store::bloom_bits_default,
        bool overwr_dups = Store::overdups_default);
    virtual bool StopAndArchiveName(LocalArgsDescriptor &args);

    static void ArchiveName(const NameBase *name);
    static void Discover();
    void DiscoverSpaces();
    static Cluster *GetCluster(LocalArgsDescriptor &args,
            bool ch_names = true,
            bool db = false) throw(Exception);
    Space *GetSpace(LocalArgsDescriptor &args,
            bool ch_names = true,
            bool db = false) throw(Exception);
    Store *GetStore(LocalArgsDescriptor &args,
            bool ch_names = true,
            bool db = false) throw(Exception);

    Space *FindSpace(LocalArgsDescriptor &args) throw(Exception);
    Store *FindStore(LocalArgsDescriptor &args) throw(Exception);
    bool StopAndArchiveSpace(LocalArgsDescriptor &args);
    bool StopAndArchiveStore(LocalArgsDescriptor &args);

    virtual void RegisterApiFunctions() throw(Exception);
    static void RegisterApiStaticFunctions() throw(Exception);

    static uint64_t CreateSpace(LocalArgsDescriptor &args) throw(Exception);
    static uint64_t CreateStore(LocalArgsDescriptor &args) throw(Exception);
    static void DestroySpace(LocalArgsDescriptor &args) throw(Exception);
    static void DestroyStore(LocalArgsDescriptor &args) throw(Exception);
    static uint64_t GetSpaceByName(LocalArgsDescriptor &args) throw(Exception);
    static uint64_t GetStoreByName(LocalArgsDescriptor &args) throw(Exception);
};

}


#endif /* SOE_SOE_SOE_KVS_SRC_KVSFACADE_HPP_ */
