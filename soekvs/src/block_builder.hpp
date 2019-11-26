#ifndef SOE_KVS_SRC_BLOCK_BUILDER_H_
#define SOE_KVS_SRC_BLOCK_BUILDER_H_

#include <vector>

#include <cstdint>
#include "dissect.hpp"

namespace LzKVStore {

struct OpenOptions;

//
// Block builder kye'ing off buffers
//
class BlockBuilder
{
public:
    explicit BlockBuilder(const OpenOptions* options);

    // Reset the contents as if the BlockBuilder was just constructed.
    void Reset();

    // This implementation requires that the following conditions are met:
    // Finish() has not been called since the last call to Reset().
    // key is larger than any previously added key
    void Add(const Dissection& key, const Dissection& value);

    // Finish building the block and return a slice that refers to the
    // block contents.  The returned slice will remain valid for the
    // lifetime of this builder or until Reset() is called.
    Dissection Finish();

    // Returns an estimate of the current (uncompressed) size of the block
    // we are building.
    size_t CurrentSizeEstimate() const;

    // Return true iff no entries have been added since the last Reset()
    bool empty() const
    {
        return buffer_.empty();
    }

private:
    const OpenOptions* options_;
    std::string buffer_;      // Destination buffer
    std::vector<uint32_t> restarts_;    // Restart points
    int counter_;     // Number of entries emitted since restart
    bool finished_;    // Has Finish() been called?
    std::string last_key_;

    // No copying allowed
    BlockBuilder(const BlockBuilder&);
    void operator=(const BlockBuilder&);
};

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_BLOCK_BUILDER_H_
