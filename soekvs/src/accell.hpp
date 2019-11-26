#ifndef SOE_KVS_SRC_ACCELL_H_
#define SOE_KVS_SRC_ACCELL_H_

#include <cstdint>
#include "dissect.hpp"

namespace LzKVStore {

class Accell;

// This class provides cache-like capability
//
// Create a new cache with a fixed size capacity.  This implementation
// of Accell uses a least-recently-used eviction policy.
extern Accell* NewLRUAccell(size_t capacity);

class Accell
{
public:
    Accell()
    {
    }

    // Destroys all existing entries by calling the "deleter"
    // function that was passed to the constructor.
    virtual ~Accell();

    // Opaque handle to an entry stored in the cache.
    struct Handle {
    };

    // Insert a mapping from key->value into the cache and assign it
    // the specified charge against the total cache capacity.
    //
    // Returns a handle that corresponds to the mapping.  The caller
    // must call this->Release(handle) when the returned mapping is no
    // longer needed.
    //
    // When the inserted entry is no longer needed, the key and
    // value will be passed to "deleter".
    virtual Handle* Insert(const Dissection& key, void* value, size_t charge,
            void (*deleter)(const Dissection& key, void* value)) = 0;

    // If the cache has no mapping for "key", returns NULL.
    //
    // Else return a handle that corresponds to the mapping.  The caller
    // must call this->Release(handle) when the returned mapping is no
    // longer needed.
    virtual Handle* Lookup(const Dissection& key) = 0;

    // Release a mapping returned by a previous Lookup().
    // REQUIRES: handle must not have been released yet.
    // REQUIRES: handle must have been returned by a method on *this.
    virtual void Release(Handle* handle) = 0;

    // Return the value encapsulated in a handle returned by a
    // successful Lookup().
    // REQUIRES: handle must not have been released yet.
    // REQUIRES: handle must have been returned by a method on *this.
    virtual void* Value(Handle* handle) = 0;

    // If the cache contains entry for key, erase it.  Note that the
    // underlying entry will be kept around until all existing handles
    // to it have been released.
    virtual void Erase(const Dissection& key) = 0;

    // Return a new numeric id.  May be used by multiple clients who are
    // sharing the same cache to partition the key space.  Typically the
    // client will allocate a new id at startup and prepend the id to
    // its cache keys.
    virtual uint64_t NewId() = 0;

    // Remove all cache entries that are not actively in use.  Memory-constrained
    // applications may wish to call this method to reduce memory usage.
    // Default implementation of Prune() does nothing.  Subclasses are strongly
    // encouraged to override the default implementation.  A future release of
    // LzKVStore may change Prune() to a pure abstract method.
    virtual void Prune() {
    }

    // Return an estimate of the combined charges of all elements stored in the
    // cache.
    virtual size_t TotalCharge() const = 0;

private:
    void LRU_Remove(Handle* e);
    void LRU_Append(Handle* e);
    void Unref(Handle* e);

    struct Rep;
    Rep* rep_;

    // No copying allowed
    Accell(const Accell&);
    void operator=(const Accell&);
};

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_ACCELL_H_
