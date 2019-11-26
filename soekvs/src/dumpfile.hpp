#ifndef SOE_KVS_SRC_DUMPFILE_H_
#define SOE_KVS_SRC_DUMPFILE_H_

#include <string>
#include "configur.hpp"
#include "status.hpp"

namespace LzKVStore {

// Dump the contents of the file named by fname in text format to
// *dst.  Makes a sequence of dst->Append() calls; each call is passed
// the newline-terminated text corresponding to a single item found
// in the file.
//
// Returns a non-OK result if fname does not name a LzKVStore storage
// file, or if the file cannot be read.
Status DumpFile(Configuration* env, const std::string& fname, ModDatabase* dst);

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_DUMPFILE_H_
