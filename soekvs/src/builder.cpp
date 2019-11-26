#include "kvsbuilder.hpp"

#include "fileops.hpp"
#include "kvconfig.hpp"
#include "kvsaccell.hpp"
#include "kvseditver.hpp"
#include "kvstore.hpp"
#include "sandbox.hpp"
#include "configur.hpp"
#include "iterator.hpp"

namespace LzKVStore {

//
// Build initially KVStore
//
Status BuildKVStore(const std::string& dbname, Configuration* env,
        const OpenOptions& options, KVStoreAccell* table_cache, Iterator* iter,
        FileMetaData* meta) {
    Status s;
    meta->file_size = 0;
    iter->SeekToFirst();

    std::string fname = KVStoreFileName(dbname, meta->number);
    if (iter->Valid()) {
        ModDatabase* file;
        s = env->NewModDatabase(fname, &file);
        if (!s.ok()) {
            return s;
        }

        KVStoreBuilder* builder = new KVStoreBuilder(options, file);
        meta->smallest.DecodeFrom(iter->key());
        for (; iter->Valid(); iter->Next()) {
            Dissection key = iter->key();
            meta->largest.DecodeFrom(key);
            builder->Add(key, iter->value());
        }

        // Finish and check for builder errors
        if (s.ok()) {
            s = builder->Finish();
            if (s.ok()) {
                meta->file_size = builder->FileSize();
                assert(meta->file_size > 0);
            }
        } else {
            builder->Abandon();
        }
        delete builder;

        // Finish and check for file errors
        if (s.ok()) {
            s = file->Sync();
        }
        if (s.ok()) {
            s = file->Close();
        }
        delete file;
        file = NULL;

        if (s.ok()) {
            // Verify that the table is usable
            Iterator* it = table_cache->NewIterator(ReadOptions(), meta->number,
                    meta->file_size);
            s = it->status();
            delete it;
        }
    }

    // Check for input iterator errors
    if (!iter->status().ok()) {
        s = iter->status();
    }

    if (s.ok() && meta->file_size > 0) {
        // Keep it
    } else {
        env->DeleteFile(fname);
    }
    return s;
}

}  // namespace LzKVStore
