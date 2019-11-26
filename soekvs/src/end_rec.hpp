#ifndef SOE_KVS_SRC_END_REC_H_
#define SOE_KVS_SRC_END_REC_H_

#include <string>
#include <cstdint>
#include "accell.hpp"
#include "dissect.hpp"
#include "status.hpp"
#include "kvsaccell.hpp"
#include "kvsbuilder.hpp"
#include "end_rec.hpp"

namespace LzKVStore {

class Block;
class RADatabase;
struct ReadOptions;

class BlockHandle
{
public:
    BlockHandle();

    // The offset of the block in the file.
    uint64_t offset() const
    {
        return offset_;
    }
    void set_offset(uint64_t offset)
    {
        offset_ = offset;
    }

    // The size of the stored block
    uint64_t size() const
    {
        return size_;
    }
    void set_size(uint64_t size)
    {
        size_ = size;
    }

    void EncodeTo(std::string* dst) const;
    Status DecodeFrom(Dissection* input);

    // Maximum encoding length of a BlockHandle
    enum {
        kMaxEncodedLength = 10 + 10
    };

private:
    uint64_t offset_;
    uint64_t size_;
};

// Endrecord encapsulates the fixed information stored at the tail
// end of every table file.
class Endrecord
{
public:
    Endrecord()
    {
    }

    // The block handle for the metaindex block of the table
    const BlockHandle& metaindex_handle() const
    {
        return metaindex_handle_;
    }
    void set_metaindex_handle(const BlockHandle& h)
    {
        metaindex_handle_ = h;
    }

    // The block handle for the index block of the table
    const BlockHandle& index_handle() const
    {
        return index_handle_;
    }
    void set_index_handle(const BlockHandle& h)
    {
        index_handle_ = h;
    }

    void EncodeTo(std::string* dst) const;
    Status DecodeFrom(Dissection* input);

    // Encoded length of a Endrecord.  Note that the serialization of a
    // Endrecord will always occupy exactly this many bytes.  It consists
    // of two block handles and a magic number.
    enum {
        kEncodedLength = 2 * BlockHandle::kMaxEncodedLength + 8
    };

private:
    BlockHandle metaindex_handle_;
    BlockHandle index_handle_;
};

// kKVStoreMagicNumber chosen arbitrarily
// and taking the leading 64 bits.
static const uint64_t kKVStoreMagicNumber = 0xdb4775248b80fb57ull;

// 1-byte type + 32-bit crc
static const size_t kBlockTrailerSize = 5;

struct BlockContents {
    Dissection data;           // Actual contents of data
    bool cachable;        // True iff data can be cached
    bool heap_allocated;  // True iff caller should delete[] data.data()
};

// Read the block identified by "handle" from "file".  On failure
// return non-OK.  On success fill *result and return OK.
extern Status ReadBlock(RADatabase* file, const ReadOptions& options,
        const BlockHandle& handle, BlockContents* result);

// Implementation details follow.  Clients should ignore,

inline BlockHandle::BlockHandle() :
        offset_(~static_cast<uint64_t>(0)), size_(~static_cast<uint64_t>(0))
{
}

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_END_REC_H_
