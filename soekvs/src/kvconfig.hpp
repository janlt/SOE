#ifndef SOE_KVS_SRC_KVCONFIG_H_
#define SOE_KVS_SRC_KVCONFIG_H_

#include <stdio.h>
#include "equalcomp.hpp"
#include "image.hpp"
#include "filtering_pol.hpp"
#include "dissect.hpp"
#include "kvsbuilder.hpp"
#include "code_styles.hpp"
#include "logger.hpp"

namespace LzKVStore {

// Grouping of constants.  We may want to make some of these
// parameters set via options.
namespace config {
static const int kNumLevels = 7;

// Level-0 compaction is started when we hit this many files.
static const int kL0_CompressionTrigger = 4;

// Soft limit on number of level-0 files.  We slow down writes at this point.
static const int kL0_SlowdownWritesTrigger = 8;

// Maximum number of level-0 files.  We stop writes at this point.
static const int kL0_StopWritesTrigger = 12;

// Maximum level to which a new compacted memtable is pushed if it
// does not create overlap.  We try to push to level 2 to avoid the
// relatively expensive level 0=>1 compactions and to avoid some
// expensive manifest file operations.  We do not push all the way to
// the largest level since that can generate a lot of wasted disk
// space if the same key space is being repeatedly overwritten.
static const int kMaxMemCompactLevel = 2;

// Approximate gap in bytes between samples of data read during iteration.
static const int kReadBytesPeriod = 1048576;

}  // namespace config

class PrimaryKey;

// Value types encoded as the last component of internal keys.
// DO NOT CHANGE THESE ENUM VALUES: they are embedded in the on-disk
// data structures.
enum ValueType {
    kTypeDeletion = 0x0, kTypeValue = 0x1
};
// kValueTypeForSeek defines the ValueType that should be passed when
// constructing a ParsedPrimaryKey object for seeking to a particular
// sequence number (since we sort sequence numbers in decreasing order
// and the value type is embedded as the low 8 bits in the sequence
// number in internal keys, we need to use the highest-numbered
// ValueType, not the lowest).
static const ValueType kValueTypeForSeek = kTypeValue;

typedef uint64_t SequenceNumber;

// We leave eight bits empty at the bottom so a type and sequence#
// can be packed together into 64-bits.
static const SequenceNumber kMaxSequenceNumber = ((0x1ull << 56) - 1);

struct ParsedPrimaryKey {
    Dissection user_key;
    SequenceNumber sequence;
    ValueType type;

    ParsedPrimaryKey()
    {
    }  // Intentionally left uninitialized (for speed)
    ParsedPrimaryKey(const Dissection& u, const SequenceNumber& seq,
            ValueType t) :
            user_key(u), sequence(seq), type(t)
    {
    }
    std::string DebugString() const;
};

// Return the length of the encoding of "key".
inline size_t PrimaryKeyEncodingLength(const ParsedPrimaryKey& key)
{
    return key.user_key.size() + 8;
}

// Append the serialization of "key" to *result.
extern void AppendPrimaryKey(std::string* result, const ParsedPrimaryKey& key);

// Attempt to parse an internal key from "internal_key".  On success,
// stores the parsed data in "*result", and returns true.
//
// On error, returns false, leaves "*result" in an undefined state.
extern bool ParsePrimaryKey(const Dissection& internal_key,
        ParsedPrimaryKey* result);

// Returns the user key portion of an internal key.
inline Dissection ExtractUserKey(const Dissection& internal_key)
{
    assert(internal_key.size() >= 8);
    return Dissection(internal_key.data(), internal_key.size() - 8);
}

inline ValueType ExtractValueType(const Dissection& internal_key)
{
    assert(internal_key.size() >= 8);
    const size_t n = internal_key.size();
    uint64_t num = DecodeFixed64(internal_key.data() + n - 8);
    unsigned char c = num & 0xff;
    return static_cast<ValueType>(c);
}

// A comparator for internal keys that uses a specified comparator for
// the user key portion and breaks ties by decreasing sequence number.
class PrimaryKeyEqualcomp: public Equalcomp
{
private:
    const Equalcomp* user_comparator_;
public:
    explicit PrimaryKeyEqualcomp(const Equalcomp* c) :
            user_comparator_(c)
    {
    }
    virtual const char* Name() const;
    virtual int Compare(const Dissection& a, const Dissection& b) const;
    virtual void FindShortestSeparator(std::string* start,
            const Dissection& limit) const;
    virtual void FindShortSuccessor(std::string* key) const;

    const Equalcomp* user_comparator() const
    {
        return user_comparator_;
    }

    int Compare(const PrimaryKey& a, const PrimaryKey& b) const;
};

// Filter policy wrapper that converts from internal keys to user keys
class InternalFilteringPolicy: public FilteringPolicy
{
private:
    const FilteringPolicy* const user_policy_;
public:
    explicit InternalFilteringPolicy(const FilteringPolicy* p) :
            user_policy_(p)
    {
    }
    virtual const char* Name() const;
    virtual void CreateFilter(const Dissection* keys, int n,
            std::string* dst) const;
    virtual bool KeyMayMatch(const Dissection& key,
            const Dissection& filter) const;
};

// Modules in this directory should keep internal keys wrapped inside
// the following class instead of plain strings so that we do not
// incorrectly use string comparisons instead of an PrimaryKeyEqualcomp.
class PrimaryKey
{
private:
    std::string rep_;
public:
    PrimaryKey()
    {
    }   // Leave rep_ as empty to indicate it is invalid
    PrimaryKey(const Dissection& user_key, SequenceNumber s, ValueType t)
    {
        AppendPrimaryKey(&rep_, ParsedPrimaryKey(user_key, s, t));
    }

    void DecodeFrom(const Dissection& s)
    {
        rep_.assign(s.data(), s.size());
    }
    Dissection Encode() const
    {
        assert(!rep_.empty());
        return rep_;
    }

    Dissection user_key() const
    {
        return ExtractUserKey(rep_);
    }

    void SetFrom(const ParsedPrimaryKey& p)
    {
        rep_.clear();
        AppendPrimaryKey(&rep_, p);
    }

    void Clear()
    {
        rep_.clear();
    }

    std::string DebugString() const;
};

inline int PrimaryKeyEqualcomp::Compare(const PrimaryKey& a,
        const PrimaryKey& b) const
{
    return Compare(a.Encode(), b.Encode());
}

inline bool ParsePrimaryKey(const Dissection& internal_key,
        ParsedPrimaryKey* result)
{
    const size_t n = internal_key.size();
    if (n < 8)
        return false;
    uint64_t num = DecodeFixed64(internal_key.data() + n - 8);
    unsigned char c = num & 0xff;
    result->sequence = num >> 8;
    result->type = static_cast<ValueType>(c);
    result->user_key = Dissection(internal_key.data(), n - 8);
    return (c <= static_cast<unsigned char>(kTypeValue));
}

// A helper class useful for KVDatabaseImpl::Get()
class SearchKey
{
public:
    // Initialize *this for looking up user_key at a snapshot with
    // the specified sequence number.
    SearchKey(const Dissection& user_key, SequenceNumber sequence);

    ~SearchKey();

    // Return a key suitable for lookup in a MemKVStore.
    Dissection memtable_key() const
    {
        return Dissection(start_, end_ - start_);
    }

    // Return an internal key (suitable for passing to an internal iterator)
    Dissection internal_key() const
    {
        return Dissection(kstart_, end_ - kstart_);
    }

    // Return the user key
    Dissection user_key() const
    {
        return Dissection(kstart_, end_ - kstart_ - 8);
    }

private:
    // We construct a char array of the form:
    //    klength  varint32               <-- start_
    //    userkey  char[klength]          <-- kstart_
    //    tag      uint64
    //                                    <-- end_
    // The array is a suitable MemKVStore key.
    // The suffix starting with "userkey" can be used as an PrimaryKey.
    const char* start_;
    const char* kstart_;
    const char* end_;
    char space_[200];      // Avoid allocation for short keys

    // No copying allowed
    SearchKey(const SearchKey&);
    void operator=(const SearchKey&);
};

inline SearchKey::~SearchKey()
{
    if (start_ != space_)
        delete[] start_;
}

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_KVCONFIG_H_
