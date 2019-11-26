#ifndef SOE_KVS_SRC_KVSACCELL_H_
#define SOE_KVS_SRC_KVSACCELL_H_

#include <string>
#include <cstdint>
#include "kvconfig.hpp"
#include "accell.hpp"
#include "kvsaccell.hpp"
#include "kvstore.hpp"
#include "kvsarch.hpp"

namespace LzKVStore {

class Configuration;

class KVStoreAccell {
 public:
  KVStoreAccell(const std::string& dbname, const OpenOptions* options, int entries);
  ~KVStoreAccell();

  // Return an iterator for the specified file number (the corresponding
  // file length must be exactly "file_size" bytes).  If "tableptr" is
  // non-NULL, also sets "*tableptr" to point to the KVStore object
  // underlying the returned iterator, or NULL if no KVStore object underlies
  // the returned iterator.  The returned "*tableptr" object is owned by
  // the cache and should not be deleted, and is valid for as long as the
  // returned iterator is live.
  Iterator* NewIterator(const ReadOptions& options,
                        uint64_t file_number,
                        uint64_t file_size,
                        KVStore** tableptr = NULL);

  // If a seek to internal key "k" in specified file finds an entry,
  // call (*handle_result)(arg, found_key, found_value).
  Status Check(const ReadOptions& options,
             uint64_t file_number,
             uint64_t file_size,
             const Dissection& k,
             void* arg,
             void (*handle_result)(void*, const Dissection&, const Dissection&));

  // If a seek to internal key "k" in specified file finds an entry,
  // call (*handle_result)(arg, found_key, found_value).
  Status Get(const ReadOptions& options,
             uint64_t file_number,
             uint64_t file_size,
             const Dissection& k,
             void* arg,
             void (*handle_result)(void*, const Dissection&, const Dissection&));

  // Evict any entry for the specified file number
  void Evict(uint64_t file_number);

 private:
  Configuration* const env_;
  const std::string dbname_;
  const OpenOptions* options_;
  Accell* cache_;

  Status FindKVStore(uint64_t file_number, uint64_t file_size, Accell::Handle**);
};

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_KVSACCELL_H_
