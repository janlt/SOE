#ifndef SOE_KVS_SRC_FILEOPS_H_
#define SOE_KVS_SRC_FILEOPS_H_

// File ops to be used by KVDatabase

#include <cstdint>
#include <string>
#include "dissect.hpp"
#include "status.hpp"
#include "kvsarch.hpp"

namespace LzKVStore {

class Configuration;

enum FileType {
  kLogFile,
  kKVDatabaseLockFile,
  kKVStoreFile,
  kDescriptorFile,
  kCurrentFile,
  kTempFile,
  kInfoLogFile  // Either the current one, or an old one
};

// Return the name of the log file with the specified number
// in the db named by "dbname".  The result will be prefixed with
// "dbname".
extern std::string LogFileName(const std::string& dbname, uint64_t number);

// Return the name of the mitable with the specified number
// in the db named by "dbname".  The result will be prefixed with
// "dbname".
extern std::string KVStoreFileName(const std::string& dbname, uint64_t number);

// Return the legacy file name for an mitable with the specified number
// in the db named by "dbname". The result will be prefixed with
// "dbname".
extern std::string MITKVStoreFileName(const std::string& dbname, uint64_t number);

// Return the name of the descriptor file for the db named by
// "dbname" and the specified incarnation number.  The result will be
// prefixed with "dbname".
extern std::string DescriptorFileName(const std::string& dbname,
                                      uint64_t number);

// Return the name of the current file.  This file contains the name
// of the current manifest file.  The result will be prefixed with
// "dbname".
extern std::string CurrentFileName(const std::string& dbname);

// Return the name of the lock file for the db named by
// "dbname".  The result will be prefixed with "dbname".
extern std::string LockFileName(const std::string& dbname);

// Return the name of a temporary file owned by the db named "dbname".
// The result will be prefixed with "dbname".
extern std::string TempFileName(const std::string& dbname, uint64_t number);

// Return the name of the info log file for "dbname".
extern std::string InfoLogFileName(const std::string& dbname);

// Return the name of the old info log file for "dbname".
extern std::string OldInfoLogFileName(const std::string& dbname);

// If filename is a LzKVStore file, store the type of the file in *type.
// The number encoded in the filename is stored in *number.  If the
// filename was successfully parsed, returns true.  Else return false.
extern bool ParseFileName(const std::string& filename,
                          uint64_t* number,
                          FileType* type);

// Make the CURRENT file point to the descriptor file with the
// specified number.
extern Status SetCurrentFile(Configuration* env, const std::string& dbname,
                             uint64_t descriptor_number);


}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_FILEOPS_H_
