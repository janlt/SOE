#include "fileops.hpp"
#include "end_rec.hpp"

#include "configur.hpp"
#include "kvstore.hpp"
#include "code_styles.hpp"

namespace LzKVStore {

struct KVStoreAndFile {
    RADatabase* file;
    KVStore* table;
};

static void DeleteEntry(const Dissection& key, void* value)
{
    KVStoreAndFile* tf = reinterpret_cast<KVStoreAndFile*>(value);
    delete tf->table;
    delete tf->file;
    delete tf;
}

static void UnrefEntry(void* arg1, void* arg2)
{
    Accell* cache = reinterpret_cast<Accell*>(arg1);
    Accell::Handle* h = reinterpret_cast<Accell::Handle*>(arg2);
    cache->Release(h);
}

KVStoreAccell::KVStoreAccell(const std::string& dbname, const OpenOptions* options,
        int entries) :
        env_(options->env), dbname_(dbname), options_(options), cache_(
                NewLRUAccell(entries))
{
}

KVStoreAccell::~KVStoreAccell()
{
    delete cache_;
}

Status KVStoreAccell::FindKVStore(uint64_t file_number, uint64_t file_size,
        Accell::Handle** handle)
{
    Status s;
    char buf[sizeof(file_number)];
    EncodeFixed64(buf, file_number);
    Dissection key(buf, sizeof(buf));
    *handle = cache_->Lookup(key);
    if (*handle == NULL) {
        std::string fname = KVStoreFileName(dbname_, file_number);
        RADatabase* file = NULL;
        KVStore* table = NULL;
        s = env_->NewRADatabase(fname, &file);
        if (!s.ok()) {
            std::string old_fname = MITKVStoreFileName(dbname_, file_number);
            if (env_->NewRADatabase(old_fname, &file).ok()) {
                s = Status::OK();
            }
        }
        if (s.ok()) {
            s = KVStore::Open(*options_, file, file_size, &table);
        }

        if (!s.ok()) {
            assert(table == NULL);
            delete file;
            // We do not cache error results so that if the error is transient,
            // or somebody repairs the file, we recover automatically.
        } else {
            KVStoreAndFile* tf = new KVStoreAndFile;
            tf->file = file;
            tf->table = table;
            *handle = cache_->Insert(key, tf, 1, &DeleteEntry);
        }
    }
    return s;
}

Iterator* KVStoreAccell::NewIterator(const ReadOptions& options,
        uint64_t file_number, uint64_t file_size, KVStore** tableptr)
{
    if (tableptr != NULL) {
        *tableptr = NULL;
    }

    Accell::Handle* handle = NULL;
    Status s = FindKVStore(file_number, file_size, &handle);
    if (!s.ok()) {
        return NewErrorIterator(s);
    }

    KVStore* table =
            reinterpret_cast<KVStoreAndFile*>(cache_->Value(handle))->table;
    Iterator* result = table->NewIterator(options);
    result->RegisterCleanup(&UnrefEntry, cache_, handle);
    if (tableptr != NULL) {
        *tableptr = table;
    }
    return result;
}

Status KVStoreAccell::Check(const ReadOptions& options, uint64_t file_number,
        uint64_t file_size, const Dissection& k, void* arg,
        void (*saver)(void*, const Dissection&, const Dissection&))
{
    Accell::Handle* handle = NULL;
    Status s = FindKVStore(file_number, file_size, &handle);
    if (s.ok()) {
        KVStore* t =
                reinterpret_cast<KVStoreAndFile*>(cache_->Value(handle))->table;
        s = t->InternalCheck(options, k, arg, saver);
        cache_->Release(handle);
    }
    return s;
}

Status KVStoreAccell::Get(const ReadOptions& options, uint64_t file_number,
        uint64_t file_size, const Dissection& k, void* arg,
        void (*saver)(void*, const Dissection&, const Dissection&))
{
    Accell::Handle* handle = NULL;
    Status s = FindKVStore(file_number, file_size, &handle);
    if (s.ok()) {
        KVStore* t =
                reinterpret_cast<KVStoreAndFile*>(cache_->Value(handle))->table;
        s = t->InternalGet(options, k, arg, saver);
        cache_->Release(handle);
    }
    return s;
}

void KVStoreAccell::Evict(uint64_t file_number)
{
    char buf[sizeof(file_number)];
    EncodeFixed64(buf, file_number);
    cache_->Erase(Dissection(buf, sizeof(buf)));
}

}  // namespace LzKVStore
