#include "c_api.hpp"

#include <stdlib.h>
#include <unistd.h>
#include "accell.hpp"
#include "image.hpp"
#include "equalcomp.hpp"
#include "kvstore.hpp"
#include "sandbox.hpp"
#include "configur.hpp"
#include "filtering_pol.hpp"
#include "iterator.hpp"
#include "configur.hpp"
#include "status.hpp"
#include "kvsbatchops.hpp"
#include "kvsoptions.hpp"

using LzKVStore::Accell;
using LzKVStore::Equalcomp;
using LzKVStore::CompressionType;
using LzKVStore::KVDatabase;
using LzKVStore::Configuration;
using LzKVStore::DatabaseLock;
using LzKVStore::FilteringPolicy;
using LzKVStore::Iterator;
using LzKVStore::kMajorVersion;
using LzKVStore::kMinorVersion;
using LzKVStore::Messanger;
using LzKVStore::NewBloomFilteringPolicy;
using LzKVStore::NewLRUAccell;
using LzKVStore::OpenOptions;
using LzKVStore::RADatabase;
using LzKVStore::Range;
using LzKVStore::ReadOptions;
using LzKVStore::SeqDatabase;
using LzKVStore::Dissection;
using LzKVStore::Image;
using LzKVStore::Status;
using LzKVStore::ModDatabase;
using LzKVStore::SaveBatch;
using LzKVStore::WriteOptions;

extern "C" {

struct LzKVStore_t {
    KVDatabase* rep;
};
struct LzKVStore_iterator_t {
    Iterator* rep;
};
struct LzKVStore_writebatch_t {
    SaveBatch rep;
};
struct LzKVStore_snapshot_t {
    const Image* rep;
};
struct LzKVStore_readoptions_t {
    ReadOptions rep;
};
struct LzKVStore_writeoptions_t {
    WriteOptions rep;
};
struct LzKVStore_options_t {
    OpenOptions rep;
};
struct LzKVStore_cache_t {
    Accell* rep;
};
struct LzKVStore_seqfile_t {
    SeqDatabase* rep;
};
struct LzKVStore_randomfile_t {
    RADatabase* rep;
};
struct LzKVStore_writablefile_t {
    ModDatabase* rep;
};
struct LzKVStore_logger_t {
    Messanger* rep;
};
struct LzKVStore_filelock_t {
    DatabaseLock* rep;
};

struct LzKVStore_comparator_t: public Equalcomp
{
    void* state_;
    void (*destructor_)(void*);
    int (*compare_)(void*, const char* a, size_t alen, const char* b,
            size_t blen);
    const char* (*name_)(void*);

    virtual ~LzKVStore_comparator_t() {
        (*destructor_)(state_);
    }

    virtual int Compare(const Dissection& a, const Dissection& b) const
    {
        return (*compare_)(state_, a.data(), a.size(), b.data(), b.size());
    }

    virtual const char* Name() const
    {
        return (*name_)(state_);
    }

    // No-ops since the C binding does not support key shortening methods.
    virtual void FindShortestSeparator(std::string*, const Dissection&) const
    {
    }
    virtual void FindShortSuccessor(std::string* key) const
    {
    }
};

struct LzKVStore_filterpolicy_t: public FilteringPolicy
{
    void* state_;
    void (*destructor_)(void*);
    const char* (*name_)(void*);
    char* (*create_)(void*, const char* const * key_array,
            const size_t* key_length_array, int num_keys,
            size_t* filter_length);
    unsigned char (*key_match_)(void*, const char* key, size_t length,
            const char* filter, size_t filter_length);

    virtual ~LzKVStore_filterpolicy_t()
    {
        (*destructor_)(state_);
    }

    virtual const char* Name() const
    {
        return (*name_)(state_);
    }

    virtual void CreateFilter(const Dissection* keys, int n,
            std::string* dst) const
    {
        std::vector<const char*> key_pointers(n);
        std::vector<size_t> key_sizes(n);
        for (int i = 0; i < n; i++) {
            key_pointers[i] = keys[i].data();
            key_sizes[i] = keys[i].size();
        }
        size_t len;
        char* filter = (*create_)(state_, &key_pointers[0], &key_sizes[0], n,
                &len);
        dst->append(filter, len);
        free(filter);
    }

    virtual bool KeyMayMatch(const Dissection& key,
            const Dissection& filter) const
    {
        return (*key_match_)(state_, key.data(), key.size(), filter.data(),
                filter.size());
    }
};

struct LzKVStore_env_t {
    Configuration* rep;
    bool is_default;
};

static bool SaveError(char** errptr, const Status& s)
{
    assert(errptr != NULL);
    if (s.ok()) {
        return false;
    } else if (*errptr == NULL) {
        *errptr = strdup(s.ToString().c_str());
    } else {
        // TODO(sanjay): Merge with existing error?
        free(*errptr);
        *errptr = strdup(s.ToString().c_str());
    }
    return true;
}

static char* CopyString(const std::string& str)
{
    char* result = reinterpret_cast<char*>(malloc(sizeof(char) * str.size()));
    memcpy(result, str.data(), sizeof(char) * str.size());
    return result;
}

LzKVStore_t* LzKVStore_open(const LzKVStore_options_t* options,
        const char* name, char** errptr)
{
    KVDatabase* db;
    if (SaveError(errptr,
            KVDatabase::Open(options->rep, std::string(name), &db))) {
        return NULL;
    }
    LzKVStore_t* result = new LzKVStore_t;
    result->rep = db;
    return result;
}

void LzKVStore_close(LzKVStore_t* db)
{
    delete db->rep;
    delete db;
}

void LzKVStore_put(LzKVStore_t* db, const LzKVStore_writeoptions_t* options,
        const char* key, size_t keylen, const char* val, size_t vallen,
        char** errptr)
{
    SaveError(errptr,
            db->rep->Put(options->rep, Dissection(key, keylen),
                    Dissection(val, vallen)));
}

void LzKVStore_delete(LzKVStore_t* db, const LzKVStore_writeoptions_t* options,
        const char* key, size_t keylen, char** errptr)
{
    SaveError(errptr, db->rep->Delete(options->rep, Dissection(key, keylen)));
}

void LzKVStore_write(LzKVStore_t* db, const LzKVStore_writeoptions_t* options,
        LzKVStore_writebatch_t* batch, char** errptr)
{
    SaveError(errptr, db->rep->Write(options->rep, &batch->rep));
}

char* LzKVStore_get(LzKVStore_t* db, const LzKVStore_readoptions_t* options,
        const char* key, size_t keylen, size_t* vallen, char** errptr)
{
    char* result = NULL;
    std::string tmp;
    Status s = db->rep->Get(options->rep, Dissection(key, keylen), &tmp);
    if (s.ok()) {
        *vallen = tmp.size();
        result = CopyString(tmp);
    } else {
        *vallen = 0;
        if (!s.IsNotFound()) {
            SaveError(errptr, s);
        }
    }
    return result;
}

LzKVStore_iterator_t* LzKVStore_create_iterator(LzKVStore_t* db,
        const LzKVStore_readoptions_t* options)
{
    LzKVStore_iterator_t* result = new LzKVStore_iterator_t;
    result->rep = db->rep->NewIterator(options->rep);
    return result;
}

const LzKVStore_snapshot_t* LzKVStore_create_snapshot(LzKVStore_t* db)
{
    LzKVStore_snapshot_t* result = new LzKVStore_snapshot_t;
    result->rep = db->rep->GetImage();
    return result;
}

void LzKVStore_release_snapshot(LzKVStore_t* db,
        const LzKVStore_snapshot_t* snapshot)
{
    db->rep->ReleaseImage(snapshot->rep);
    delete snapshot;
}

char* LzKVStore_property_value(LzKVStore_t* db, const char* propname)
{
    std::string tmp;
    if (db->rep->GetProperty(Dissection(propname), &tmp)) {
        // We use strdup() since we expect human readable output.
        return strdup(tmp.c_str());
    } else {
        return NULL;
    }
}

void LzKVStore_approximate_sizes(LzKVStore_t* db, int num_ranges,
        const char* const * range_start_key, const size_t* range_start_key_len,
        const char* const * range_limit_key, const size_t* range_limit_key_len,
        uint64_t* sizes)
{
    Range* ranges = new Range[num_ranges];
    for (int i = 0; i < num_ranges; i++) {
        ranges[i].start = Dissection(range_start_key[i],
                range_start_key_len[i]);
        ranges[i].limit = Dissection(range_limit_key[i],
                range_limit_key_len[i]);
    }
    db->rep->GetApproximateSizes(ranges, num_ranges, sizes);
    delete[] ranges;
}

void LzKVStore_compact_range(LzKVStore_t* db, const char* start_key,
        size_t start_key_len, const char* limit_key, size_t limit_key_len)
{
    Dissection a, b;
    db->rep->CompactRange(
            // Pass NULL Dissection if corresponding "const char*" is NULL
            (start_key ? (a = Dissection(start_key, start_key_len), &a) : NULL),
            (limit_key ? (b = Dissection(limit_key, limit_key_len), &b) : NULL));
}

void LzKVStore_destroy_db(const LzKVStore_options_t* options, const char* name,
        char** errptr)
{
    SaveError(errptr, DestroyKVDatabase(name, options->rep));
}

void LzKVStore_repair_db(const LzKVStore_options_t* options, const char* name,
        char** errptr)
{
    SaveError(errptr, RepairKVDatabase(name, options->rep));
}

void LzKVStore_iter_destroy(LzKVStore_iterator_t* iter)
{
    delete iter->rep;
    delete iter;
}

unsigned char LzKVStore_iter_valid(const LzKVStore_iterator_t* iter)
{
    return iter->rep->Valid();
}

void LzKVStore_iter_seek_to_first(LzKVStore_iterator_t* iter)
{
    iter->rep->SeekToFirst();
}

void LzKVStore_iter_seek_to_last(LzKVStore_iterator_t* iter)
{
    iter->rep->SeekToLast();
}

void LzKVStore_iter_seek(LzKVStore_iterator_t* iter, const char* k,
        size_t klen)
{
    iter->rep->Seek(Dissection(k, klen));
}

void LzKVStore_iter_next(LzKVStore_iterator_t* iter)
{
    iter->rep->Next();
}

void LzKVStore_iter_prev(LzKVStore_iterator_t* iter)
{
    iter->rep->Prev();
}

const char* LzKVStore_iter_key(const LzKVStore_iterator_t* iter, size_t* klen)
{
    Dissection s = iter->rep->key();
    *klen = s.size();
    return s.data();
}

const char* LzKVStore_iter_value(const LzKVStore_iterator_t* iter,
        size_t* vlen)
{
    Dissection s = iter->rep->value();
    *vlen = s.size();
    return s.data();
}

void LzKVStore_iter_get_error(const LzKVStore_iterator_t* iter, char** errptr)
{
    SaveError(errptr, iter->rep->status());
}

LzKVStore_writebatch_t* LzKVStore_writebatch_create()
{
    return new LzKVStore_writebatch_t;
}

void LzKVStore_writebatch_destroy(LzKVStore_writebatch_t* b)
{
    delete b;
}

void LzKVStore_writebatch_clear(LzKVStore_writebatch_t* b)
{
    b->rep.Clear();
}

void LzKVStore_writebatch_put(LzKVStore_writebatch_t* b, const char* key,
        size_t klen, const char* val, size_t vlen)
{
    b->rep.Put(Dissection(key, klen), Dissection(val, vlen));
}

void LzKVStore_writebatch_delete(LzKVStore_writebatch_t* b, const char* key,
        size_t klen)
{
    b->rep.Delete(Dissection(key, klen));
}

void LzKVStore_writebatch_iterate(LzKVStore_writebatch_t* b, void* state,
        void (*put)(void*, const char* k, size_t klen, const char* v,
                size_t vlen),
        void (*deleted)(void*, const char* k, size_t klen))
{
    class H: public SaveBatch::Handler {
    public:
        void* state_;
        void (*put_)(void*, const char* k, size_t klen, const char* v,
                size_t vlen);
        void (*deleted_)(void*, const char* k, size_t klen);
        virtual void Put(const Dissection& key, const Dissection& value) {
            (*put_)(state_, key.data(), key.size(), value.data(), value.size());
        }
        virtual void Delete(const Dissection& key) {
            (*deleted_)(state_, key.data(), key.size());
        }
    };
    H handler;
    handler.state_ = state;
    handler.put_ = put;
    handler.deleted_ = deleted;
    b->rep.Iterate(&handler);
}

LzKVStore_options_t* LzKVStore_options_create()
{
    return new LzKVStore_options_t;
}

void LzKVStore_options_destroy(LzKVStore_options_t* options)
{
    delete options;
}

void LzKVStore_options_set_comparator(LzKVStore_options_t* opt,
        LzKVStore_comparator_t* cmp)
{
    opt->rep.comparator = cmp;
}

void LzKVStore_options_set_filter_policy(LzKVStore_options_t* opt,
        LzKVStore_filterpolicy_t* policy)
{
    opt->rep.filter_policy = policy;
}

void LzKVStore_options_set_create_if_missing(LzKVStore_options_t* opt,
        unsigned char v)
{
    opt->rep.create_if_missing = v;
}

void LzKVStore_options_set_error_if_exists(LzKVStore_options_t* opt,
        unsigned char v)
{
    opt->rep.error_if_exists = v;
}

void LzKVStore_options_set_full_checks(LzKVStore_options_t* opt,
        unsigned char v)
{
    opt->rep.full_checks = v;
}

void LzKVStore_options_set_env(LzKVStore_options_t* opt, LzKVStore_env_t* env)
{
    opt->rep.env = (env ? env->rep : NULL);
}

void LzKVStore_options_set_info_log(LzKVStore_options_t* opt,
        LzKVStore_logger_t* l)
{
    opt->rep.info_log = (l ? l->rep : NULL);
}

void LzKVStore_options_set_write_buffer_size(LzKVStore_options_t* opt,
        size_t s)
{
    opt->rep.write_buffer_size = s;
}

void LzKVStore_options_set_max_open_files(LzKVStore_options_t* opt, int n)
{
    opt->rep.max_open_files = n;
}

void LzKVStore_options_set_cache(LzKVStore_options_t* opt,
        LzKVStore_cache_t* c)
{
    opt->rep.block_cache = c->rep;
}

void LzKVStore_options_set_block_size(LzKVStore_options_t* opt, size_t s)
{
    opt->rep.block_size = s;
}

void LzKVStore_options_set_block_restart_interval(LzKVStore_options_t* opt,
        int n)
{
    opt->rep.block_restart_interval = n;
}

void LzKVStore_options_set_compression(LzKVStore_options_t* opt, int t)
{
    opt->rep.compression = static_cast<CompressionType>(t);
}

LzKVStore_comparator_t* LzKVStore_comparator_create(void* state,
        void (*destructor)(void*),
        int (*compare)(void*, const char* a, size_t alen, const char* b,
                size_t blen), const char* (*name)(void*))
{
    LzKVStore_comparator_t* result = new LzKVStore_comparator_t;
    result->state_ = state;
    result->destructor_ = destructor;
    result->compare_ = compare;
    result->name_ = name;
    return result;
}

void LzKVStore_comparator_destroy(LzKVStore_comparator_t* cmp)
{
    delete cmp;
}

LzKVStore_filterpolicy_t* LzKVStore_filterpolicy_create(void* state,
        void (*destructor)(void*),
        char* (*create_filter)(void*, const char* const * key_array,
                const size_t* key_length_array, int num_keys,
                size_t* filter_length),
        unsigned char (*key_may_match)(void*, const char* key, size_t length,
                const char* filter, size_t filter_length),
        const char* (*name)(void*))
{
    LzKVStore_filterpolicy_t* result = new LzKVStore_filterpolicy_t;
    result->state_ = state;
    result->destructor_ = destructor;
    result->create_ = create_filter;
    result->key_match_ = key_may_match;
    result->name_ = name;
    return result;
}

void LzKVStore_filterpolicy_destroy(LzKVStore_filterpolicy_t* filter)
{
    delete filter;
}

LzKVStore_filterpolicy_t* LzKVStore_filterpolicy_create_bloom(
        int bits_per_key)
{
    // Make a LzKVStore_filterpolicy_t, but override all of its methods so
    // they delegate to a NewBloomFilteringPolicy() instead of user
    // supplied C functions.
    struct Wrapper: public LzKVStore_filterpolicy_t {
        const FilteringPolicy* rep_;
        ~Wrapper() {
            delete rep_;
        }
        const char* Name() const {
            return rep_->Name();
        }
        void CreateFilter(const Dissection* keys, int n,
                std::string* dst) const {
            return rep_->CreateFilter(keys, n, dst);
        }
        bool KeyMayMatch(const Dissection& key,
                const Dissection& filter) const {
            return rep_->KeyMayMatch(key, filter);
        }
        static void DoNothing(void*) {
        }
    };
    Wrapper* wrapper = new Wrapper;
    wrapper->rep_ = NewBloomFilteringPolicy(bits_per_key);
    wrapper->state_ = NULL;
    wrapper->destructor_ = &Wrapper::DoNothing;
    return wrapper;
}

LzKVStore_readoptions_t* LzKVStore_readoptions_create()
{
    return new LzKVStore_readoptions_t;
}

void LzKVStore_readoptions_destroy(LzKVStore_readoptions_t* opt)
{
    delete opt;
}

void LzKVStore_readoptions_set_verify_checksums(LzKVStore_readoptions_t* opt,
        unsigned char v)
{
    opt->rep.verify_checksums = v;
}

void LzKVStore_readoptions_set_fill_cache(LzKVStore_readoptions_t* opt,
        unsigned char v)
{
    opt->rep.fill_cache = v;
}

void LzKVStore_readoptions_set_snapshot(LzKVStore_readoptions_t* opt,
        const LzKVStore_snapshot_t* snap)
{
    opt->rep.snapshot = (snap ? snap->rep : NULL);
}

LzKVStore_writeoptions_t* LzKVStore_writeoptions_create()
{
    return new LzKVStore_writeoptions_t;
}

void LzKVStore_writeoptions_destroy(LzKVStore_writeoptions_t* opt)
{
    delete opt;
}

void LzKVStore_writeoptions_set_sync(LzKVStore_writeoptions_t* opt,
        unsigned char v)
{
    opt->rep.sync = v;
}

LzKVStore_cache_t* LzKVStore_cache_create_lru(size_t capacity)
{
    LzKVStore_cache_t* c = new LzKVStore_cache_t;
    c->rep = NewLRUAccell(capacity);
    return c;
}

void LzKVStore_cache_destroy(LzKVStore_cache_t* cache)
{
    delete cache->rep;
    delete cache;
}

LzKVStore_env_t* LzKVStore_create_default_env()
{
    LzKVStore_env_t* result = new LzKVStore_env_t;
    result->rep = Configuration::Default();
    result->is_default = true;
    return result;
}

void LzKVStore_env_destroy(LzKVStore_env_t* env)
{
    if (!env->is_default)
        delete env->rep;
    delete env;
}

void LzKVStore_free(void* ptr)
{
    free(ptr);
}

int LzKVStore_major_version()
{
    return kMajorVersion;
}

int LzKVStore_minor_version()
{
    return kMinorVersion;
}

}  // end extern "C"
