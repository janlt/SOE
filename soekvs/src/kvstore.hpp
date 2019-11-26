#ifndef SOE_KVS_KVSTORE_H_
#define SOE_KVS_KVSTORE_H_

#include <cstdint>
#include "iterator.hpp"

namespace LzKVStore {

class Block;
class BlockHandle;
class Endrecord;
struct OpenOptions;
class RADatabase;
struct ReadOptions;
class KVStoreAccell;

// A KVStore implements a sorted map from strings to strings.  KVStores are
// immutable and persistent.  A KVStore may be safely accessed from
// multiple threads without external synchronization.
class KVStore
{
public:
    // Attempt to open the table that is stored in bytes [0..file_size)
    // of "file", and read the metadata entries necessary to allow
    // retrieving data from the table.
    //
    // If successful, returns ok and sets "*table" to the newly opened
    // table.  The client should delete "*table" when no longer needed.
    // If there was an error while initializing the table, sets "*table"
    // to NULL and returns a non-ok status.  Does not take ownership of
    // "*source", but the client must ensure that "source" remains live
    // for the duration of the returned table's lifetime.
    //
    // *file must remain live while this KVStore is in use.
    static Status Open(const OpenOptions& options, RADatabase* file,
            uint64_t file_size, KVStore** table);

    ~KVStore();

    // Returns a new iterator over the table contents.
    // The result of NewIterator() is initially invalid (caller must
    // call one of the Seek methods on the iterator before using it).
    Iterator* NewIterator(const ReadOptions&) const;

    // Given a key, return an approximate byte offset in the file where
    // the data for that key begins (or would begin if the key were
    // present in the file).  The returned value is in terms of file
    // bytes, and so includes effects like compression of the underlying data.
    // E.g., the approximate offset of the last key in the table will
    // be close to the file length.
    uint64_t ApproximateOffsetOf(const Dissection& key) const;

private:
    struct Rep;
    Rep* rep_;

    explicit KVStore(Rep* rep)
    {
        rep_ = rep;
    }
    static Iterator* BlockReader(void*, const ReadOptions&, const Dissection&);

    // Check if the entry exists to Seek(key).
    // May not make such a call if filter policy says
    // that key is not present.
    friend class KVStoreAccell;
    Status InternalCheck(const ReadOptions&, const Dissection& key, void* arg,
            void (*handle_result)(void* arg, const Dissection& k,
                    const Dissection& v));

    // Calls (*handle_result)(arg, ...) with the entry found after a call
    // to Seek(key).  May not make such a call if filter policy says
    // that key is not present.
    Status InternalGet(const ReadOptions&, const Dissection& key, void* arg,
            void (*handle_result)(void* arg, const Dissection& k,
                    const Dissection& v));

    void ReadMeta(const Endrecord& footer);
    void ReadFilter(const Dissection& filter_handle_value);

    // No copying allowed
    KVStore(const KVStore&);
    void operator=(const KVStore&);
};

}  // namespace LzKVStore

#endif  // SOE_KVS_KVSTORE_H_
