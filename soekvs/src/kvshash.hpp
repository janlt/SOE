// Simple hash function used for internal data structures

#ifndef SOE_KVS_SRC_KVSHASH_H_
#define SOE_KVS_SRC_KVSHASH_H_

#include <stddef.h>
#include <cstdint>

namespace LzKVStore {

extern uint32_t Hash(const char* data, size_t n, uint32_t seed);

}

#endif  // SOE_KVS_SRC_KVSHASH_H_
