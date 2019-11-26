#ifndef SOE_KVS_SRC_BUILDER_H_
#define SOE_KVS_SRC_BUILDER_H_

#include "status.hpp"

namespace LzKVStore {

struct OpenOptions;
struct FileMetaData;

class Configuration;
class Iterator;
class KVStoreAccell;
class EditingVersion;

extern Status BuildKVStore(const std::string& dbname,
     Configuration* env,
     const OpenOptions& options,
     KVStoreAccell* table_cache,
     Iterator* iter,
     FileMetaData* meta);

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_BUILDER_H_
