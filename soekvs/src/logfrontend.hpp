#ifndef SOE_KVS_SRC_LOGFRONTEND_H_
#define SOE_KVS_SRC_LOGFRONTEND_H_

#include <cstdint>
#include "log_types.hpp"
#include "dissect.hpp"
#include "status.hpp"

namespace LzKVStore {

class ModDatabase;

namespace KVSLogger {

class Writer
{
public:
    // Create a writer that will append data to "*dest".
    // "*dest" must be initially empty.
    // "*dest" must remain live while this Writer is in use.
    explicit Writer(ModDatabase* dest);

    // Create a writer that will append data to "*dest".
    // "*dest" must have initial length "dest_length".
    // "*dest" must remain live while this Writer is in use.
    Writer(ModDatabase* dest, uint64_t dest_length);

    ~Writer();

    Status AddRecord(const Dissection& slice);

private:
    ModDatabase* dest_;
    int block_offset_;       // Current offset in block

    // crc32c values for all supported record types.  These are
    // pre-computed to reduce the overhead of computing the crc of the
    // record type stored in the header.
    uint32_t type_crc_[kMaxRecordType + 1];

    Status EmitPhysicalRecord(RecordType type, const char* ptr, size_t length);

    // No copying allowed
    Writer(const Writer&);
    void operator=(const Writer&);
};

}  // namespace KVSLogger
}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_LOGFRONTEND_H_
