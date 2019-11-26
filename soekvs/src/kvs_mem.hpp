#ifndef SOE_KVS_SRC_KVS_MEM_H_
#define SOE_KVS_SRC_KVS_MEM_H_

namespace LzKVStore {

class Configuration;

// Returns a new environment that stores its data in memory and delegates
// all non-file-storage tasks to base_env. The caller must delete the result
// when it is no longer needed.
// *base_env must remain live while the result is in use.
Configuration* NewMemConfiguration(Configuration* base_env);

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_KVS_MEM_H_
