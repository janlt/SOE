#ifndef SOE_KVS_SRC_IMAGE_H_
#define SOE_KVS_SRC_IMAGE_H_

#include <cstdint>
#include <stdio.h>
#include "iterator.hpp"
#include "kvsoptions.hpp"

namespace LzKVStore {

// Update Makefile if you change these
static const int kMajorVersion = 1;
static const int kMinorVersion = 19;

struct OpenOptions;
struct ReadOptions;
struct WriteOptions;
class SaveBatch;

// Abstract handle to particular state of a KVDatabase.
// A Image is an immutable object and can therefore be safely
// accessed from multiple threads without any external synchronization.
class Image
{
protected:
    virtual ~Image();
};

// A range of keys
struct Range {
    Dissection start;          // Included in the range
    Dissection limit;          // Not included in the range

    Range()
    {
    }
    Range(const Dissection& s, const Dissection& l) :
            start(s), limit(l)
    {
    }
};

// A KVDatabase is a persistent ordered map from keys to values.
// A KVDatabase is safe for concurrent access from multiple threads without
// any external synchronization.
class KVDatabase
{
public:
    // Open the database with the specified "name".
    // Stores a pointer to a heap-allocated database in *dbptr and returns
    // OK on success.
    // Stores NULL in *dbptr and returns a non-OK status on error.
    // Caller should delete *dbptr when it is no longer needed.
    static Status Open(const OpenOptions& options, const std::string& name,
            KVDatabase** dbptr);

    KVDatabase()
    {
    }
    virtual ~KVDatabase();

    // Set the database entry for "key" to "value".  Returns OK on success,
    // and a non-OK status on error.
    // Note: consider setting options.sync = true.
    virtual Status Put(const WriteOptions& options, const Dissection& key,
            const Dissection& value) = 0;

    // Remove the database entry (if any) for "key".  Returns OK on
    // success, and a non-OK status on error.  It is not an error if "key"
    // did not exist in the database.
    // Note: consider setting options.sync = true.
    virtual Status Delete(const WriteOptions& options,
            const Dissection& key) = 0;

    // Apply the specified updates to the database.
    // Returns OK on success, non-OK on failure.
    // Note: consider setting options.sync = true.
    virtual Status Write(const WriteOptions& options, SaveBatch* updates) = 0;

    // If the database contains an entry for "key" return OK.
    //
    // If there is no entry for "key" return
    // a status for which Status::IsNotFound() returns true.
    //
    // May return some other Status on an error.
    virtual Status Check(const ReadOptions& options, const Dissection& key) = 0;

    // If the database contains an entry for "key" store the
    // corresponding value in *value and return OK.
    //
    // If there is no entry for "key" leave *value unchanged and return
    // a status for which Status::IsNotFound() returns true.
    //
    // May return some other Status on an error.
    virtual Status Get(const ReadOptions& options, const Dissection& key,
            std::string* value) = 0;

    // Return a heap-allocated iterator over the contents of the database.
    // The result of NewIterator() is initially invalid (caller must
    // call one of the Seek methods on the iterator before using it).
    //
    // Caller should delete the iterator when it is no longer needed.
    // The returned iterator should be deleted before this db is deleted.
    virtual Iterator* NewIterator(const ReadOptions& options) = 0;

    // Return a handle to the current KVDatabase state.  Iterators created with
    // this handle will all observe a stable snapshot of the current KVDatabase
    // state.  The caller must call ReleaseImage(result) when the
    // snapshot is no longer needed.
    virtual const Image* GetImage() = 0;

    // Release a previously acquired snapshot.  The caller must not
    // use "snapshot" after this call.
    virtual void ReleaseImage(const Image* snapshot) = 0;

    // KVDatabase implementations can export properties about their state
    // via this method.  If "property" is a valid property understood by this
    // KVDatabase implementation, fills "*value" with its current value and returns
    // true.  Otherwise returns false.
    //
    //
    // Valid property names include:
    //
    //  "LzKVStore.num-files-at-level<N>" - return the number of files at level <N>,
    //     where <N> is an ASCII representation of a level number (e.g. "0").
    //  "LzKVStore.stats" - returns a multi-line string that describes statistics
    //     about the internal operation of the KVDatabase.
    //  "LzKVStore.sstables" - returns a multi-line string that describes all
    //     of the sstables that make up the db contents.
    //  "LzKVStore.approximate-memory-usage" - returns the approximate number of
    //     bytes of memory in use by the KVDatabase.
    virtual bool GetProperty(const Dissection& property,
            std::string* value) = 0;

    // For each i in [0,n-1], store in "sizes[i]", the approximate
    // file system space used by keys in "[range[i].start .. range[i].limit)".
    //
    // Note that the returned sizes measure file system space usage, so
    // if the user data compresses by a factor of ten, the returned
    // sizes will be one-tenth the size of the corresponding user data size.
    //
    // The results may not include the sizes of recently written data.
    virtual void GetApproximateSizes(const Range* range, int n,
            uint64_t* sizes) = 0;

    // Compact the underlying storage for the key range [*begin,*end].
    // In particular, deleted and overwritten versions are discarded,
    // and the data is rearranged to reduce the cost of operations
    // needed to access the data.  This operation should typically only
    // be invoked by users who understand the underlying implementation.
    //
    // begin==NULL is treated as a key before all keys in the database.
    // end==NULL is treated as a key after all keys in the database.
    // Therefore the following call will compact the entire database:
    //    db->CompactRange(NULL, NULL);
    virtual void CompactRange(const Dissection* begin,
            const Dissection* end) = 0;

private:
    // No copying allowed
    KVDatabase(const KVDatabase&);
    void operator=(const KVDatabase&);
};

// Destroy the contents of the specified database.
// Be very careful using this method.
Status DestroyKVDatabase(const std::string& name, const OpenOptions& options);

// If a KVDatabase cannot be opened, you may attempt to call this method to
// resurrect as much of the contents of the database as possible.
// Some data may be lost, so be careful when calling this function
// on a database that contains important information.
Status RepairKVDatabase(const std::string& dbname, const OpenOptions& options);

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_IMAGE_H_
