#include "filtering_pol.hpp"

#include "dissect.hpp"
#include "kvshash.hpp"

namespace LzKVStore {

//
// Bloom filter is a data structure designed to tell you,
// quickly and memory-efficiently, whether an element is
// present in a set.
// This is implementation of Bloom filtering based on
// double-hashing to generate a sequence of hash values
// providing reduced hashing for the same performance.
//
namespace {
static uint32_t BloomHash(const Dissection& key)
{
    return Hash(key.data(), key.size(), 0xbc9f1d34);
}

class BloomFilteringPolicy: public FilteringPolicy
{
private:
    size_t bits_per_key_;
    size_t k_;

public:
    explicit BloomFilteringPolicy(int bits_per_key) :
            bits_per_key_(bits_per_key)
    {
        // We intentionally round down to reduce probing cost a little bit
        k_ = static_cast<size_t>(bits_per_key * 0.69);  // 0.69 =~ ln(2)
        if (k_ < 1)
            k_ = 1;
        if (k_ > 30)
            k_ = 30;
    }

    virtual const char* Name() const
    {
        return "LzKVStore.BuiltinBloomFilter2";
    }

    virtual void CreateFilter(const Dissection* keys, int n,
            std::string* dst) const
    {
        // Compute bloom filter size (in both bits and bytes)
        size_t bits = n * bits_per_key_;

        // For small n, we can see a very high false positive rate.  Fix it
        // by enforcing a minimum bloom filter length.
        if (bits < 64)
            bits = 64;

        size_t bytes = (bits + 7) / 8;
        bits = bytes * 8;

        const size_t init_size = dst->size();
        dst->resize(init_size + bytes, 0);
        dst->push_back(static_cast<char>(k_)); // Remember # of probes in filter
        char* array = &(*dst)[init_size];
        for (int i = 0; i < n; i++) {
            // Use double-hashing to generate a sequence of hash values.
            uint32_t h = BloomHash(keys[i]);
            const uint32_t delta = (h >> 17) | (h << 15); // Rotate right 17 bits
            for (size_t j = 0; j < k_; j++) {
                const uint32_t bitpos = h % bits;
                array[bitpos / 8] |= (1 << (bitpos % 8));
                h += delta;
            }
        }
    }

    virtual bool KeyMayMatch(const Dissection& key,
            const Dissection& bloom_filter) const
    {
        const size_t len = bloom_filter.size();
        if (len < 2)
            return false;

        const char* array = bloom_filter.data();
        const size_t bits = (len - 1) * 8;

        // Use the encoded k so that we can read filters generated by
        // bloom filters created using different parameters.
        const size_t k = array[len - 1];
        if (k > 30) {
            // Reserved for potentially new encodings for short bloom filters.
            // Consider it a match.
            return true;
        }

        uint32_t h = BloomHash(key);
        const uint32_t delta = (h >> 17) | (h << 15);  // Rotate right 17 bits
        for (size_t j = 0; j < k; j++) {
            const uint32_t bitpos = h % bits;
            if ((array[bitpos / 8] & (1 << (bitpos % 8))) == 0)
                return false;
            h += delta;
        }
        return true;
    }
};
}

const FilteringPolicy* NewBloomFilteringPolicy(int bits_per_key)
{
    return new BloomFilteringPolicy(bits_per_key);
}

}  // namespace LzKVStore
