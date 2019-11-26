#ifndef SOE_KVS_SRC_KVSBATCHOPS_H_
#define SOE_KVS_SRC_KVSBATCHOPS_H_

#include <string>
#include "status.hpp"

namespace LzKVStore {

class Dissection;

class SaveBatch
{
public:
    SaveBatch();
    ~SaveBatch();

    // Store the mapping "key->value" in the database.
    void Put(const Dissection& key, const Dissection& value);

    // If the database contains a mapping for "key", erase it.  Else do nothing.
    void Delete(const Dissection& key);

    // Clear all updates buffered in this batch.
    void Clear();

    // Support for iterating over the contents of a batch.
    class Handler
    {
    public:
        virtual ~Handler();
        virtual void Put(const Dissection& key, const Dissection& value) = 0;
        virtual void Delete(const Dissection& key) = 0;
    };
    Status Iterate(Handler* handler) const;

private:
    friend class SaveBatchInternal;

    std::string rep_;  // See comment in write_batch.cc for the format of rep_

    // Intentionally copyable
};

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_KVSBATCHOPS_H_
