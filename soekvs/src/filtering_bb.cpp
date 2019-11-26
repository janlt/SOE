#include "filtering_bb.hpp"

#include "filtering_pol.hpp"
#include "code_styles.hpp"

namespace LzKVStore {

// See doc/table_format.txt for an explanation of the filter block format.

// Generate new filter every 2KB of data
static const size_t kFilterBaseLg = 11;
static const size_t kFilterBase = 1 << kFilterBaseLg;

FilteringBlockBuilder::FilteringBlockBuilder(const FilteringPolicy* policy) :
        policy_(policy)
{
}

void FilteringBlockBuilder::StartBlock(uint64_t block_offset)
{
    uint64_t filter_index = (block_offset / kFilterBase);
    assert(filter_index >= filter_offsets_.size());
    while (filter_index > filter_offsets_.size()) {
        GenerateFilter();
    }
}

void FilteringBlockBuilder::AddKey(const Dissection& key)
{
    Dissection k = key;
    start_.push_back(keys_.size());
    keys_.append(k.data(), k.size());
}

Dissection FilteringBlockBuilder::Finish()
{
    if (!start_.empty()) {
        GenerateFilter();
    }

    // Append array of per-filter offsets
    const uint32_t array_offset = result_.size();
    for (size_t i = 0; i < filter_offsets_.size(); i++) {
        PutFixed32(&result_, filter_offsets_[i]);
    }

    PutFixed32(&result_, array_offset);
    result_.push_back(kFilterBaseLg);  // Save encoding parameter in result
    return Dissection(result_);
}

void FilteringBlockBuilder::GenerateFilter()
{
    const size_t num_keys = start_.size();
    if (num_keys == 0) {
        // Fast path if there are no keys for this filter
        filter_offsets_.push_back(result_.size());
        return;
    }

    // Make list of keys from flattened key structure
    start_.push_back(keys_.size());  // Simplify length computation
    tmp_keys_.resize(num_keys);
    for (size_t i = 0; i < num_keys; i++) {
        const char* base = keys_.data() + start_[i];
        size_t length = start_[i + 1] - start_[i];
        tmp_keys_[i] = Dissection(base, length);
    }

    // Generate filter for current set of keys and append to result_.
    filter_offsets_.push_back(result_.size());
    policy_->CreateFilter(&tmp_keys_[0], static_cast<int>(num_keys), &result_);

    tmp_keys_.clear();
    keys_.clear();
    start_.clear();
}

FilteringBlockReader::FilteringBlockReader(const FilteringPolicy* policy,
        const Dissection& contents) :
        policy_(policy), data_(NULL), offset_(NULL), num_(0), base_lg_(0)
{
    size_t n = contents.size();
    if (n < 5)
        return;  // 1 byte for base_lg_ and 4 for start of offset array
    base_lg_ = contents[n - 1];
    uint32_t last_word = DecodeFixed32(contents.data() + n - 5);
    if (last_word > n - 5)
        return;
    data_ = contents.data();
    offset_ = data_ + last_word;
    num_ = (n - 5 - last_word) / 4;
}

bool FilteringBlockReader::KeyMayMatch(uint64_t block_offset,
        const Dissection& key)
{
    uint64_t index = block_offset >> base_lg_;
    if (index < num_) {
        uint32_t start = DecodeFixed32(offset_ + index * 4);
        uint32_t limit = DecodeFixed32(offset_ + index * 4 + 4);
        if (start <= limit && limit <= static_cast<size_t>(offset_ - data_)) {
            Dissection filter = Dissection(data_ + start, limit - start);
            return policy_->KeyMayMatch(key, filter);
        } else if (start == limit) {
            // Empty filters do not match any keys
            return false;
        }
    }
    return true;  // Errors are treated as potential matches
}

}
