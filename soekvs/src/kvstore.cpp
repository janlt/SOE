#include "kvstore.hpp"

#include "accell.hpp"
#include "equalcomp.hpp"
#include "configur.hpp"
#include "filtering_pol.hpp"
#include "kvconfig.hpp"
#include "block.hpp"
#include "filtering_bb.hpp"
#include "image.hpp"
#include "iterator2l.hpp"
#include "code_styles.hpp"

namespace LzKVStore {

struct KVStore::Rep {
    ~Rep()
    {
        delete filter;
        delete[] filter_data;
        delete index_block;
    }

    OpenOptions options;
    Status status;
    RADatabase* file;
    uint64_t cache_id;
    FilteringBlockReader* filter;
    const char* filter_data;

    BlockHandle metaindex_handle; // Handle to metaindex_block: saved from footer
    Block* index_block;
};

Status KVStore::Open(const OpenOptions& options, RADatabase* file, uint64_t size,
        KVStore** table)
{
    *table = NULL;
    if (size < Endrecord::kEncodedLength) {
        return Status::Corruption("file is too short to be an mitable");
    }

    char footer_space[Endrecord::kEncodedLength];
    Dissection footer_input;
    Status s = file->Read(size - Endrecord::kEncodedLength,
            Endrecord::kEncodedLength, &footer_input, footer_space);
    if (!s.ok())
        return s;

    Endrecord footer;
    s = footer.DecodeFrom(&footer_input);
    if (!s.ok())
        return s;

    // Read the index block
    BlockContents contents;
    Block* index_block = NULL;
    if (s.ok()) {
        ReadOptions opt;
        if (options.full_checks) {
            opt.verify_checksums = true;
        }
        s = ReadBlock(file, opt, footer.index_handle(), &contents);
        if (s.ok()) {
            index_block = new Block(contents);
        }
    }

    if (s.ok()) {
        // We've successfully read the footer and the index block: we're
        // ready to serve requests.
        Rep* rep = new KVStore::Rep;
        rep->options = options;
        rep->file = file;
        rep->metaindex_handle = footer.metaindex_handle();
        rep->index_block = index_block;
        rep->cache_id =
                (options.block_cache ? options.block_cache->NewId() : 0);
        rep->filter_data = NULL;
        rep->filter = NULL;
        *table = new KVStore(rep);
        (*table)->ReadMeta(footer);
    } else {
        delete index_block;
    }

    return s;
}

void KVStore::ReadMeta(const Endrecord& footer)
{
    if (rep_->options.filter_policy == NULL) {
        return;  // Do not need any metadata
    }

    // TODO(sanjay): Skip this if footer.metaindex_handle() size indicates
    // it is an empty block.
    ReadOptions opt;
    if (rep_->options.full_checks) {
        opt.verify_checksums = true;
    }
    BlockContents contents;
    if (!ReadBlock(rep_->file, opt, footer.metaindex_handle(), &contents).ok()) {
        // Do not propagate errors since meta info is not needed for operation
        return;
    }
    Block* meta = new Block(contents);

    Iterator* iter = meta->NewIterator(BytewiseEqualcomp());
    std::string key = "filter.";
    key.append(rep_->options.filter_policy->Name());
    iter->Seek(key);
    if (iter->Valid() && iter->key() == Dissection(key)) {
        ReadFilter(iter->value());
    }
    delete iter;
    delete meta;
}

void KVStore::ReadFilter(const Dissection& filter_handle_value)
{
    Dissection v = filter_handle_value;
    BlockHandle filter_handle;
    if (!filter_handle.DecodeFrom(&v).ok()) {
        return;
    }

    // We might want to unify with ReadBlock() if we start
    // requiring checksum verification in KVStore::Open.
    ReadOptions opt;
    if (rep_->options.full_checks) {
        opt.verify_checksums = true;
    }
    BlockContents block;
    if (!ReadBlock(rep_->file, opt, filter_handle, &block).ok()) {
        return;
    }
    if (block.heap_allocated) {
        rep_->filter_data = block.data.data();     // Will need to delete later
    }
    rep_->filter = new FilteringBlockReader(rep_->options.filter_policy,
            block.data);
}

KVStore::~KVStore()
{
    delete rep_;
}

static void DeleteBlock(void* arg, void* ignored)
{
    delete reinterpret_cast<Block*>(arg);
}

static void DeleteAccelldBlock(const Dissection& key, void* value)
{
    Block* block = reinterpret_cast<Block*>(value);
    delete block;
}

static void ReleaseBlock(void* arg, void* h)
{
    Accell* cache = reinterpret_cast<Accell*>(arg);
    Accell::Handle* handle = reinterpret_cast<Accell::Handle*>(h);
    cache->Release(handle);
}

// Convert an index iterator value (i.e., an encoded BlockHandle)
// into an iterator over the contents of the corresponding block.
Iterator* KVStore::BlockReader(void* arg, const ReadOptions& options,
        const Dissection& index_value)
{
    KVStore* table = reinterpret_cast<KVStore*>(arg);
    Accell* block_cache = table->rep_->options.block_cache;
    Block* block = NULL;
    Accell::Handle* cache_handle = NULL;

    BlockHandle handle;
    Dissection input = index_value;
    Status s = handle.DecodeFrom(&input);
    // We intentionally allow extra stuff in index_value so that we
    // can add more features in the future.

    if (s.ok()) {
        BlockContents contents;
        if (block_cache != NULL) {
            char cache_key_buffer[16];
            EncodeFixed64(cache_key_buffer, table->rep_->cache_id);
            EncodeFixed64(cache_key_buffer + 8, handle.offset());
            Dissection key(cache_key_buffer, sizeof(cache_key_buffer));
            cache_handle = block_cache->Lookup(key);
            if (cache_handle != NULL) {
                block = reinterpret_cast<Block*>(block_cache->Value(
                        cache_handle));
            } else {
                s = ReadBlock(table->rep_->file, options, handle, &contents);
                if (s.ok()) {
                    block = new Block(contents);
                    if (contents.cachable && options.fill_cache) {
                        cache_handle = block_cache->Insert(key, block,
                                block->size(), &DeleteAccelldBlock);
                    }
                }
            }
        } else {
            s = ReadBlock(table->rep_->file, options, handle, &contents);
            if (s.ok()) {
                block = new Block(contents);
            }
        }
    }

    Iterator* iter;
    if (block != NULL) {
        iter = block->NewIterator(table->rep_->options.comparator);
        if (cache_handle == NULL) {
            iter->RegisterCleanup(&DeleteBlock, block, NULL);
        } else {
            iter->RegisterCleanup(&ReleaseBlock, block_cache, cache_handle);
        }
    } else {
        iter = NewErrorIterator(s);
    }
    return iter;
}

Iterator* KVStore::NewIterator(const ReadOptions& options) const
{
    return NewTwoLevelIterator(
            rep_->index_block->NewIterator(rep_->options.comparator),
            &KVStore::BlockReader, const_cast<KVStore*>(this), options);
}

Status KVStore::InternalCheck(const ReadOptions& options, const Dissection& k,
        void* arg, void (*saver)(void*, const Dissection&, const Dissection&))
{
    Status s;
    Iterator* iiter = rep_->index_block->NewIterator(rep_->options.comparator);
    iiter->Seek(k);
    if (iiter->Valid()) {
        Dissection handle_value = iiter->value();
        FilteringBlockReader* filter = rep_->filter;
        BlockHandle handle;
        if (filter != NULL && handle.DecodeFrom(&handle_value).ok()
                && !filter->KeyMayMatch(handle.offset(), k)) {
            // Not found
        } else {
            Iterator* block_iter = BlockReader(this, options, iiter->value());
            block_iter->Seek(k);
            if (block_iter->Valid()) {
                (*saver)(arg, block_iter->key(), block_iter->value());
            }
            s = block_iter->status();
            delete block_iter;
        }
    }
    if (s.ok()) {
        s = iiter->status();
    }
    delete iiter;
    return s;
}

Status KVStore::InternalGet(const ReadOptions& options, const Dissection& k,
        void* arg, void (*saver)(void*, const Dissection&, const Dissection&))
{
    Status s;
    Iterator* iiter = rep_->index_block->NewIterator(rep_->options.comparator);
    iiter->Seek(k);
    if (iiter->Valid()) {
        Dissection handle_value = iiter->value();
        FilteringBlockReader* filter = rep_->filter;
        BlockHandle handle;
        if (filter != NULL && handle.DecodeFrom(&handle_value).ok()
                && !filter->KeyMayMatch(handle.offset(), k)) {
            // Not found
        } else {
            Iterator* block_iter = BlockReader(this, options, iiter->value());
            block_iter->Seek(k);
            if (block_iter->Valid()) {
                (*saver)(arg, block_iter->key(), block_iter->value());
            }
            s = block_iter->status();
            delete block_iter;
        }
    }
    if (s.ok()) {
        s = iiter->status();
    }
    delete iiter;
    return s;
}

uint64_t KVStore::ApproximateOffsetOf(const Dissection& key) const
{
    Iterator* index_iter = rep_->index_block->NewIterator(
            rep_->options.comparator);
    index_iter->Seek(key);
    uint64_t result;
    if (index_iter->Valid()) {
        BlockHandle handle;
        Dissection input = index_iter->value();
        Status s = handle.DecodeFrom(&input);
        if (s.ok()) {
            result = handle.offset();
        } else {
            // Strange: we can't decode the block handle in the index block.
            // We'll just return the offset of the metaindex block, which is
            // close to the whole file size for this case.
            result = rep_->metaindex_handle.offset();
        }
    } else {
        // key is past the last key in the file.  Approximate the offset
        // by returning the offset of the metaindex block (which is
        // right near the end of the file).
        result = rep_->metaindex_handle.offset();
    }
    delete index_iter;
    return result;
}

}  // namespace LzKVStore
