#ifndef SOE_KVS_SRC_BLOCK_H_
#define SOE_KVS_SRC_BLOCK_H_

#include <stddef.h>
#include <cstdint>
#include "iterator.hpp"

namespace LzKVStore {

struct BlockContents;
class Equalcomp;

class Block
{
public:
    // Initialize the block with the specified contents.
    explicit Block(const BlockContents& contents);

    ~Block();

    size_t size() const
    {
        return size_;
    }
    Iterator* NewIterator(const Equalcomp* comparator);

private:
    uint32_t NumRestarts() const;

    const char* data_;
    size_t size_;
    uint32_t restart_offset_;     // Offset in data_ of restart array
    bool owned_;                  // Block owns data_[]

    // No copying allowed
    Block(const Block&);
    void operator=(const Block&);

    class Iter;
};

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_BLOCK_H_
