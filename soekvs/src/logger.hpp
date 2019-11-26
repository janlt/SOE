// Must not be included from any .h files to prevent polluting the namespace
// with macros.

#ifndef SOE_KVS_SRC_LOGGERH_
#define SOE_KVS_SRC_LOGGERH_

#include <stdio.h>
#include <cstdint>
#include <string>
#include "kvsarch.hpp"

namespace LzKVStore {

class Dissection;
class ModDatabase;

// Append a human-readable printout of "num" to *str
extern void AppendNumberTo(std::string* str, uint64_t num);

// Append a human-readable printout of "value" to *str.
// Escapes any non-printable characters found in "value".
extern void AppendEscapedStringTo(std::string* str, const Dissection& value);

// Return a human-readable printout of "num"
extern std::string NumberToString(uint64_t num);

// Return a human-readable version of "value".
// Escapes any non-printable characters found in "value".
extern std::string EscapeString(const Dissection& value);

// Parse a human-readable number from "*in" into *value.  On success,
// advances "*in" past the consumed number and sets "*val" to the
// numeric value.  Otherwise, returns false and leaves *in in an
// unspecified state.
extern bool ConsumeDecimalNumber(Dissection* in, uint64_t* val);

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_LOGGERH_
