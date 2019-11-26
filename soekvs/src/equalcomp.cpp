#include <algorithm>
#include <cstdint>
#include "equalcomp.hpp"
#include "dissect.hpp"
#include "kvsarch.hpp"
#include "logger.hpp"

namespace LzKVStore {

Equalcomp::~Equalcomp() {
}

namespace {
class BytewiseEqualcompImpl: public Equalcomp
{
public:
    BytewiseEqualcompImpl()
    {
    }

    virtual const char* Name() const
    {
        return "LzKVStore.BytewiseEqualcomp";
    }

    virtual int Compare(const Dissection& a, const Dissection& b) const
    {
        return a.compare(b);
    }

    virtual void FindShortestSeparator(std::string* start,
            const Dissection& limit) const
    {
        // Find length of common prefix
        size_t min_length = std::min(start->size(), limit.size());
        size_t diff_index = 0;
        while ((diff_index < min_length)
                && ((*start)[diff_index] == limit[diff_index])) {
            diff_index++;
        }

        if (diff_index >= min_length) {
            // Do not shorten if one string is a prefix of the other
        } else {
            uint8_t diff_byte = static_cast<uint8_t>((*start)[diff_index]);
            if (diff_byte < static_cast<uint8_t>(0xff)
                    && diff_byte + 1
                            < static_cast<uint8_t>(limit[diff_index])) {
                (*start)[diff_index]++;
                start->resize(diff_index + 1);
                assert(Compare(*start, limit) < 0);
            }
        }
    }

    virtual void FindShortSuccessor(std::string* key) const
    {
        // Find first character that can be incremented
        size_t n = key->size();
        for (size_t i = 0; i < n; i++) {
            const uint8_t byte = (*key)[i];
            if (byte != static_cast<uint8_t>(0xff)) {
                (*key)[i] = byte + 1;
                key->resize(i + 1);
                return;
            }
        }
        // *key is a run of 0xffs.  Leave it alone.
    }
};
}  // namespace

static KVSArch::OnceType once = LZKVSTORE_ONCE_INIT;
static const Equalcomp* bytewise;

static void InitModule()
{
    bytewise = new BytewiseEqualcompImpl;
}

const Equalcomp* BytewiseEqualcomp()
{
    KVSArch::InitOnce(&once, InitModule);
    return bytewise;
}

}  // namespace LzKVStore
