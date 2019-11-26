#ifndef SOE_KVS_SRC_KVSBATCHINTRIN_H_
#define SOE_KVS_SRC_KVSBATCHINTRIN_H_

#include "kvconfig.hpp"
#include "kvsbatchops.hpp"

namespace LzKVStore {

class MemKVStore;

class SaveBatchInternal
{
public:
    // Return the number of entries in the batch.
    static int Count(const SaveBatch* batch);

    // Set the count for the number of entries in the batch.
    static void SetCount(SaveBatch* batch, int n);

    // Return the sequence number for the start of this batch.
    static SequenceNumber Sequence(const SaveBatch* batch);

    // Store the specified number as the sequence number for the start of
    // this batch.
    static void SetSequence(SaveBatch* batch, SequenceNumber seq);

    static Dissection Contents(const SaveBatch* batch)
    {
        return Dissection(batch->rep_);
    }

    static size_t ByteSize(const SaveBatch* batch)
    {
        return batch->rep_.size();
    }

    static void SetContents(SaveBatch* batch, const Dissection& contents);

    static Status InsertInto(const SaveBatch* batch, MemKVStore* memtable);

    static void Append(SaveBatch* dst, const SaveBatch* src);
};

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_KVSBATCHINTRIN_H_
