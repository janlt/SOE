#ifndef SOE_KVS_SRC_KVSITERH_
#define SOE_KVS_SRC_KVSITERH_

#include <cstdint>
#include "kvstore.hpp"
#include "kvconfig.hpp"

namespace LzKVStore {

class KVDatabaseImpl;

extern Iterator* NewKVDatabaseIterator(KVDatabaseImpl* db,
        const Equalcomp* user_key_comparator, Iterator* internal_iter,
        SequenceNumber sequence, uint32_t seed);

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_KVSITERH_
