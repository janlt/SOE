#ifndef SOE_KVS_SRC_FILTERING_BB_H_
#define SOE_KVS_SRC_FILTERING_BB_H_

#include <stddef.h>
#include <cstdint>
#include <string>
#include <vector>
#include "dissect.hpp"
#include "kvshash.hpp"

namespace LzKVStore {

class FilteringPolicy;

// A FilteringBlockBuilder is used to construct all of the filters for a
// particular KVStore.  It generates a single string which is stored as
// a special block in the KVStore.
//
// The sequence of calls to FilteringBlockBuilder must match the regexp:
//      (StartBlock AddKey*)* Finish
class FilteringBlockBuilder
{
public:
    explicit FilteringBlockBuilder(const FilteringPolicy*);

    void StartBlock(uint64_t block_offset);
    void AddKey(const Dissection& key);
    Dissection Finish();

private:
    void GenerateFilter();

    const FilteringPolicy* policy_;
    std::string keys_;              // Flattened key contents
    std::vector<size_t> start_;     // Starting index in keys_ of each key
    std::string result_;            // Filter data computed so far
    std::vector<Dissection> tmp_keys_;   // policy_->CreateFilter() argument
    std::vector<uint32_t> filter_offsets_;

    // No copying allowed
    FilteringBlockBuilder(const FilteringBlockBuilder&);
    void operator=(const FilteringBlockBuilder&);
};

class FilteringBlockReader
{
public:
    // REQUIRES: "contents" and *policy must stay live while *this is live.
    FilteringBlockReader(const FilteringPolicy* policy,
            const Dissection& contents);
    bool KeyMayMatch(uint64_t block_offset, const Dissection& key);

private:
    const FilteringPolicy* policy_;
    const char* data_;    // Pointer to filter data (at block-start)
    const char* offset_;  // Pointer to beginning of offset array (at block-end)
    size_t num_;          // Number of entries in offset array
    size_t base_lg_;      // Encoding parameter (see kFilterBaseLg in .cc file)
};

}

#endif  // SOE_KVS_SRC_FILTERING_BB_H_
