#include "kvsaccell.hpp"

#include "configur.hpp"
#include "kvsarch.hpp"
#include "block.hpp"
#include "code_styles.hpp"
#include "crc32c.hpp"

namespace LzKVStore {

void BlockHandle::EncodeTo(std::string* dst) const
{
    // Sanity check that all fields have been set
    assert(offset_ != ~static_cast<uint64_t>(0));
    assert(size_ != ~static_cast<uint64_t>(0));
    PutVarint64(dst, offset_);
    PutVarint64(dst, size_);
}

Status BlockHandle::DecodeFrom(Dissection* input)
{
    if (GetVarint64(input, &offset_) && GetVarint64(input, &size_)) {
        return Status::OK();
    } else {
        return Status::Corruption("bad block handle");
    }
}

void Endrecord::EncodeTo(std::string* dst) const
{
    const size_t original_size = dst->size();
    metaindex_handle_.EncodeTo(dst);
    index_handle_.EncodeTo(dst);
    dst->resize(2 * BlockHandle::kMaxEncodedLength);  // Padding
    PutFixed32(dst, static_cast<uint32_t>(kKVStoreMagicNumber & 0xffffffffu));
    PutFixed32(dst, static_cast<uint32_t>(kKVStoreMagicNumber >> 32));
    assert(dst->size() == original_size + kEncodedLength);
    (void) original_size;  // Disable unused variable warning.
}

Status Endrecord::DecodeFrom(Dissection* input)
{
    const char* magic_ptr = input->data() + kEncodedLength - 8;
    const uint32_t magic_lo = DecodeFixed32(magic_ptr);
    const uint32_t magic_hi = DecodeFixed32(magic_ptr + 4);
    const uint64_t magic = ((static_cast<uint64_t>(magic_hi) << 32)
            | (static_cast<uint64_t>(magic_lo)));
    if (magic != kKVStoreMagicNumber) {
        return Status::Corruption("not an mitable (bad magic number)");
    }

    Status result = metaindex_handle_.DecodeFrom(input);
    if (result.ok()) {
        result = index_handle_.DecodeFrom(input);
    }
    if (result.ok()) {
        // We skip over any leftover data (just padding for now) in "input"
        const char* end = magic_ptr + 8;
        *input = Dissection(end, input->data() + input->size() - end);
    }
    return result;
}

Status ReadBlock(RADatabase* file, const ReadOptions& options,
        const BlockHandle& handle, BlockContents* result)
{
    result->data = Dissection();
    result->cachable = false;
    result->heap_allocated = false;

    // Read the block contents as well as the type/crc footer.
    // See table_builder.cc for the code that built this structure.
    size_t n = static_cast<size_t>(handle.size());
    char* buf = new char[n + kBlockTrailerSize];
    Dissection contents;
    Status s = file->Read(handle.offset(), n + kBlockTrailerSize, &contents,
            buf);
    if (!s.ok()) {
        delete[] buf;
        return s;
    }
    if (contents.size() != n + kBlockTrailerSize) {
        delete[] buf;
        return Status::Corruption("truncated block read");
    }

    // Check the crc of the type and the block contents
    const char* data = contents.data();    // Pointer to where Read put the data
    if (options.verify_checksums) {
        const uint32_t crc = crc32c::Unmask(DecodeFixed32(data + n + 1));
        const uint32_t actual = crc32c::Value(data, n + 1);
        if (actual != crc) {
            delete[] buf;
            s = Status::Corruption("block checksum mismatch");
            return s;
        }
    }

    switch (data[n]) {
    case kNoCompression:
        if (data != buf) {
            // File implementation gave us pointer to some other data.
            // Use it directly under the assumption that it will be live
            // while the file is open.
            delete[] buf;
            result->data = Dissection(data, n);
            result->heap_allocated = false;
            result->cachable = false;  // Do not double-cache
        } else {
            result->data = Dissection(buf, n);
            result->heap_allocated = true;
            result->cachable = true;
        }

        // Ok
        break;
    case kSnappyCompression: {
        size_t ulength = 0;
        if (!KVSArch::Snappy_GetUncompressedLength(data, n, &ulength)) {
            delete[] buf;
            return Status::Corruption("corrupted compressed block contents");
        }
        char* ubuf = new char[ulength];
        if (!KVSArch::Snappy_Uncompress(data, n, ubuf)) {
            delete[] buf;
            delete[] ubuf;
            return Status::Corruption("corrupted compressed block contents");
        }
        delete[] buf;
        result->data = Dissection(ubuf, ulength);
        result->heap_allocated = true;
        result->cachable = true;
        break;
    }
    default:
        delete[] buf;
        return Status::Corruption("bad block type");
    }

    return Status::OK();
}

}  // namespace LzKVStore
