#ifndef SOE_KVS_SRC_KVSMEMCON_H_
#define SOE_KVS_SRC_KVSMEMCON_H_

#include <string>
#include "kvstore.hpp"
#include "kvconfig.hpp"
#include "kvsmlist.hpp"
#include "sandbox.hpp"

namespace LzKVStore {

class PrimaryKeyEqualcomp;
class Mutex;
class MemKVStoreIterator;

class MemKVStore
{
public:
    // MemKVStores are reference counted.  The initial reference count
    // is zero and the caller must call Ref() at least once.
    explicit MemKVStore(const PrimaryKeyEqualcomp& comparator);

    // Increase reference count.
    void Ref()
    {
        ++refs_;
    }

    // Drop reference count.  Delete if no more references exist.
    void Unref()
    {
        --refs_;
        assert(refs_ >= 0);
        if (refs_ <= 0) {
            delete this;
        }
    }

    // Returns an estimate of the number of bytes of data in use by this
    // data structure. It is safe to call when MemKVStore is being modified.
    size_t ApproximateMemoryUsage();

    // Return an iterator that yields the contents of the memtable.
    //
    // The caller must ensure that the underlying MemKVStore remains live
    // while the returned iterator is live.  The keys returned by this
    // iterator are internal keys encoded by AppendPrimaryKey in the
    // db/format.{h,cc} module.
    Iterator* NewIterator();

    // Add an entry into memtable that maps key to value at the
    // specified sequence number and with the specified type.
    // Typically value will be empty if type==kTypeDeletion.
    void Add(SequenceNumber seq, ValueType type, const Dissection& key,
            const Dissection& value);

    // If memtable contains a value for key, return true, without copying the value.
    // If memtable contains a deletion for key, store a NotFound() error
    // in *status and return true.
    // Else, return false.
    bool Check(const SearchKey& key, Status* s);

    // If memtable contains a value for key, store it in *value and return true.
    // If memtable contains a deletion for key, store a NotFound() error
    // in *status and return true.
    // Else, return false.
    bool Get(const SearchKey& key, std::string* value, Status* s);

private:
    ~MemKVStore();  // Private since only Unref() should be used to delete it

    struct KeyEqualcomp
    {
        const PrimaryKeyEqualcomp comparator;
        explicit KeyEqualcomp(const PrimaryKeyEqualcomp& c) :
                comparator(c) {
        }
        int operator()(const char* a, const char* b) const;
    };
    friend class MemKVStoreIterator;
    friend class MemKVStoreBackwardIterator;

    typedef MultiLinkList<const char*, KeyEqualcomp> KVStore;

    KeyEqualcomp comparator_;
    int refs_;
    Sandbox arena_;
    KVStore table_;

    // No copying allowed
    MemKVStore(const MemKVStore&);
    void operator=(const MemKVStore&);
};

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_KVSMEMCON_H_
