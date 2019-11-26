#include "kvs_mem.hpp"
#include "kvsmemcon.hpp"
#include "kvsmlist.hpp"
#include "kvconfig.hpp"
#include "equalcomp.hpp"
#include "configur.hpp"
#include "iterator.hpp"
#include "code_styles.hpp"

namespace LzKVStore {

static Dissection GetLengthPrefixedDissection(const char* data)
{
    uint32_t len;
    const char* p = data;
    p = GetVarint32Ptr(p, p + 5, &len);  // +5: we assume "p" is not corrupted
    return Dissection(p, len);
}

MemKVStore::MemKVStore(const PrimaryKeyEqualcomp& cmp) :
        comparator_(cmp), refs_(0), table_(comparator_, &arena_)
{
}

MemKVStore::~MemKVStore()
{
    assert(refs_ == 0);
}

size_t MemKVStore::ApproximateMemoryUsage()
{
    return arena_.MemoryUsage();
}

int MemKVStore::KeyEqualcomp::operator()(const char* aptr,
        const char* bptr) const
{
    // Internal keys are encoded as length-prefixed strings.
    Dissection a = GetLengthPrefixedDissection(aptr);
    Dissection b = GetLengthPrefixedDissection(bptr);
    return comparator.Compare(a, b);
}

// Encode a suitable internal key target for "target" and return it.
// Uses *scratch as scratch space, and the returned pointer will point
// into this scratch space.
static const char* EncodeKey(std::string* scratch, const Dissection& target)
{
    scratch->clear();
    PutVarint32(scratch, target.size());
    scratch->append(target.data(), target.size());
    return scratch->data();
}

class MemKVStoreIterator: public Iterator
{
public:
    explicit MemKVStoreIterator(MemKVStore::KVStore* table) :
            iter_(table)
    {
    }

    virtual bool Valid() const
    {
        return iter_.Valid();
    }
    virtual void Seek(const Dissection& k)
    {
        iter_.Seek(EncodeKey(&tmp_, k));
    }
    virtual void SeekToFirst()
    {
        iter_.SeekToFirst();
    }
    virtual void SeekToLast()
    {
        iter_.SeekToLast();
    }
    virtual void Next()
    {
        iter_.Next();
    }
    virtual void Prev()
    {
        iter_.Prev();
    }
    virtual Dissection key() const
    {
        return GetLengthPrefixedDissection(iter_.key());
    }
    virtual Dissection value() const
    {
        Dissection key_slice = GetLengthPrefixedDissection(iter_.key());
        return GetLengthPrefixedDissection(key_slice.data() + key_slice.size());
    }

    virtual Status status() const
    {
        return Status::OK();
    }

private:
    MemKVStore::KVStore::Iterator iter_;
    std::string tmp_;       // For passing to EncodeKey

    // No copying allowed
    MemKVStoreIterator(const MemKVStoreIterator&);
    void operator=(const MemKVStoreIterator&);
};

Iterator* MemKVStore::NewIterator()
{
    return new MemKVStoreIterator(&table_);
}

void MemKVStore::Add(SequenceNumber s, ValueType type, const Dissection& key,
        const Dissection& value)
{
    // Format of an entry is concatenation of:
    //  key_size     : varint32 of internal_key.size()
    //  key bytes    : char[internal_key.size()]
    //  value_size   : varint32 of value.size()
    //  value bytes  : char[value.size()]
    size_t key_size = key.size();
    size_t val_size = value.size();
    size_t internal_key_size = key_size + 8;
    const size_t encoded_len = VarintLength(internal_key_size)
            + internal_key_size + VarintLength(val_size) + val_size;
    char* buf = arena_.Allocate(encoded_len);
    char* p = EncodeVarint32(buf, internal_key_size);
    memcpy(p, key.data(), key_size);
    p += key_size;
    EncodeFixed64(p, (s << 8) | type);
    p += 8;
    p = EncodeVarint32(p, val_size);
    memcpy(p, value.data(), val_size);
    assert((size_t )((p + val_size) - buf) == encoded_len);
    table_.Insert(buf);
}

bool MemKVStore::Check(const SearchKey& key, Status* s)
{
    Dissection memkey = key.memtable_key();
    KVStore::Iterator iter(&table_);
    iter.Seek(memkey.data());
    if (iter.Valid()) {
        // entry format is:
        //    klength  varint32
        //    userkey  char[klength]
        //    tag      uint64
        //    vlength  varint32
        //    value    char[vlength]
        // Check that it belongs to same user key.  We do not check the
        // sequence number since the Seek() call above should have skipped
        // all entries with overly large sequence numbers.
        const char* entry = iter.key();
        uint32_t key_length;
        const char* key_ptr = GetVarint32Ptr(entry, entry + 5, &key_length);
        if (comparator_.comparator.user_comparator()->Compare(
                Dissection(key_ptr, key_length - 8), key.user_key()) == 0) {
            // Correct user key
            const uint64_t tag = DecodeFixed64(key_ptr + key_length - 8);
            switch (static_cast<ValueType>(tag & 0xff)) {
            case kTypeValue: {
                return true;
            }
            case kTypeDeletion:
                *s = Status::NotFound(Dissection());
                return true;
            }
        }
    }
    return false;
}

bool MemKVStore::Get(const SearchKey& key, std::string* value, Status* s)
{
    Dissection memkey = key.memtable_key();
    KVStore::Iterator iter(&table_);
    iter.Seek(memkey.data());
    if (iter.Valid()) {
        // entry format is:
        //    klength  varint32
        //    userkey  char[klength]
        //    tag      uint64
        //    vlength  varint32
        //    value    char[vlength]
        // Check that it belongs to same user key.  We do not check the
        // sequence number since the Seek() call above should have skipped
        // all entries with overly large sequence numbers.
        const char* entry = iter.key();
        uint32_t key_length;
        const char* key_ptr = GetVarint32Ptr(entry, entry + 5, &key_length);
        if (comparator_.comparator.user_comparator()->Compare(
                Dissection(key_ptr, key_length - 8), key.user_key()) == 0) {
            // Correct user key
            const uint64_t tag = DecodeFixed64(key_ptr + key_length - 8);
            switch (static_cast<ValueType>(tag & 0xff)) {
            case kTypeValue: {
                Dissection v = GetLengthPrefixedDissection(
                        key_ptr + key_length);
                value->assign(v.data(), v.size());
                return true;
            }
            case kTypeDeletion:
                *s = Status::NotFound(Dissection());
                return true;
            }
        }
    }
    return false;
}

}  // namespace LzKVStore
