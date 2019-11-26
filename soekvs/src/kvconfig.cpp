#include <stdio.h>
#include "kvconfig.hpp"
#include "kvsarch.hpp"
#include "code_styles.hpp"

namespace LzKVStore {

static uint64_t PackSequenceAndType(uint64_t seq, ValueType t)
{
    assert(seq <= kMaxSequenceNumber);
    assert(t <= kValueTypeForSeek);
    return (seq << 8) | t;
}

void AppendPrimaryKey(std::string* result, const ParsedPrimaryKey& key)
{
    result->append(key.user_key.data(), key.user_key.size());
    PutFixed64(result, PackSequenceAndType(key.sequence, key.type));
}

std::string ParsedPrimaryKey::DebugString() const
{
    char buf[50];
    snprintf(buf, sizeof(buf), "' @ %llu : %d", (unsigned long long) sequence,
            int(type));
    std::string result = "'";
    result += EscapeString(user_key.ToString());
    result += buf;
    return result;
}

std::string PrimaryKey::DebugString() const
{
    std::string result;
    ParsedPrimaryKey parsed;
    if (ParsePrimaryKey(rep_, &parsed)) {
        result = parsed.DebugString();
    } else {
        result = "(bad)";
        result.append(EscapeString(rep_));
    }
    return result;
}

const char* PrimaryKeyEqualcomp::Name() const
{
    return "LzKVStore.PrimaryKeyEqualcomp";
}

int PrimaryKeyEqualcomp::Compare(const Dissection& akey,
        const Dissection& bkey) const
{
    // Order by:
    //    increasing user key (according to user-supplied comparator)
    //    decreasing sequence number
    //    decreasing type (though sequence# should be enough to disambiguate)
    int r = user_comparator_->Compare(ExtractUserKey(akey),
            ExtractUserKey(bkey));
    if (r == 0) {
        const uint64_t anum = DecodeFixed64(akey.data() + akey.size() - 8);
        const uint64_t bnum = DecodeFixed64(bkey.data() + bkey.size() - 8);
        if (anum > bnum) {
            r = -1;
        } else if (anum < bnum) {
            r = +1;
        }
    }
    return r;
}

void PrimaryKeyEqualcomp::FindShortestSeparator(std::string* start,
        const Dissection& limit) const
{
    // Attempt to shorten the user portion of the key
    Dissection user_start = ExtractUserKey(*start);
    Dissection user_limit = ExtractUserKey(limit);
    std::string tmp(user_start.data(), user_start.size());
    user_comparator_->FindShortestSeparator(&tmp, user_limit);
    if (tmp.size() < user_start.size()
            && user_comparator_->Compare(user_start, tmp) < 0) {
        // User key has become shorter physically, but larger logically.
        // Tack on the earliest possible number to the shortened user key.
        PutFixed64(&tmp,
                PackSequenceAndType(kMaxSequenceNumber, kValueTypeForSeek));
        assert(this->Compare(*start, tmp) < 0);
        assert(this->Compare(tmp, limit) < 0);
        start->swap(tmp);
    }
}

void PrimaryKeyEqualcomp::FindShortSuccessor(std::string* key) const
{
    Dissection user_key = ExtractUserKey(*key);
    std::string tmp(user_key.data(), user_key.size());
    user_comparator_->FindShortSuccessor(&tmp);
    if (tmp.size() < user_key.size()
            && user_comparator_->Compare(user_key, tmp) < 0) {
        // User key has become shorter physically, but larger logically.
        // Tack on the earliest possible number to the shortened user key.
        PutFixed64(&tmp,
                PackSequenceAndType(kMaxSequenceNumber, kValueTypeForSeek));
        assert(this->Compare(*key, tmp) < 0);
        key->swap(tmp);
    }
}

const char* InternalFilteringPolicy::Name() const
{
    return user_policy_->Name();
}

void InternalFilteringPolicy::CreateFilter(const Dissection* keys, int n,
        std::string* dst) const
{
    // We rely on the fact that the code in table.cc does not mind us
    // adjusting keys[].
    Dissection* mkey = const_cast<Dissection*>(keys);
    for (int i = 0; i < n; i++) {
        mkey[i] = ExtractUserKey(keys[i]);
        // TODO(sanjay): Suppress dups?
    }
    user_policy_->CreateFilter(keys, n, dst);
}

bool InternalFilteringPolicy::KeyMayMatch(const Dissection& key,
        const Dissection& f) const
{
    return user_policy_->KeyMayMatch(ExtractUserKey(key), f);
}

SearchKey::SearchKey(const Dissection& user_key, SequenceNumber s)
{
    size_t usize = user_key.size();
    size_t needed = usize + 13;  // A conservative estimate
    char* dst;
    if (needed <= sizeof(space_)) {
        dst = space_;
    } else {
        dst = new char[needed];
    }
    start_ = dst;
    dst = EncodeVarint32(dst, usize + 8);
    kstart_ = dst;
    memcpy(dst, user_key.data(), usize);
    dst += usize;
    EncodeFixed64(dst, PackSequenceAndType(s, kValueTypeForSeek));
    dst += 8;
    end_ = dst;
}

}  // namespace LzKVStore
