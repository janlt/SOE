#include "kvsiter.hpp"

#include "fileops.hpp"
#include "kvsimpl.hpp"
#include "kvconfig.hpp"
#include "configur.hpp"
#include "iterator.hpp"
#include "kvsarch.hpp"
#include "logger.hpp"
#include "kvsmutex.hpp"
#include "random.hpp"

namespace LzKVStore {

#ifdef ITER_INTRIN_DEBUG
static void DumpInternalIter(Iterator* iter)
{
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        ParsedPrimaryKey k;
        if (!ParsePrimaryKey(iter->key(), &k)) {
            fprintf(stderr, "Corrupt '%s'\n", EscapeString(iter->key()).c_str());
        } else {
            fprintf(stderr, "@ '%s'\n", k.DebugString().c_str());
        }
    }
}
#endif

namespace {

class KVDatabaseIter: public Iterator
{
public:
    enum Direction {
        kForward, kReverse
    };

    KVDatabaseIter(KVDatabaseImpl* db, const Equalcomp* cmp, Iterator* iter,
            SequenceNumber s, uint32_t seed) :
            db_(db), user_comparator_(cmp), iter_(iter), sequence_(s), direction_(
                    kForward), valid_(false), rnd_(seed), bytes_counter_(
                    RandomPeriod())
    {
    }
    virtual ~KVDatabaseIter()
    {
        delete iter_;
    }
    virtual bool Valid() const
    {
        return valid_;
    }
    virtual Dissection key() const
    {
        assert(valid_);
        return (direction_ == kForward) ?
                ExtractUserKey(iter_->key()) : saved_key_;
    }
    virtual Dissection value() const
    {
        assert(valid_);
        return (direction_ == kForward) ? iter_->value() : saved_value_;
    }
    virtual Status status() const
    {
        if (status_.ok()) {
            return iter_->status();
        } else {
            return status_;
        }
    }

    virtual void Next();
    virtual void Prev();
    virtual void Seek(const Dissection& target);
    virtual void SeekToFirst();
    virtual void SeekToLast();

private:
    void FindNextUserEntry(bool skipping, std::string* skip);
    void FindPrevUserEntry();
    bool ParseKey(ParsedPrimaryKey* key);

    inline void SaveKey(const Dissection& k, std::string* dst)
    {
        dst->assign(k.data(), k.size());
    }

    inline void ClearSavedValue()
    {
        if (saved_value_.capacity() > 1048576) {
            std::string empty;
            swap(empty, saved_value_);
        } else {
            saved_value_.clear();
        }
    }

    // Pick next gap with average value of config::kReadBytesPeriod.
    ssize_t RandomPeriod()
    {
        return rnd_.Uniform(2 * config::kReadBytesPeriod);
    }

    KVDatabaseImpl* db_;
    const Equalcomp* const user_comparator_;
    Iterator* const iter_;
    SequenceNumber const sequence_;

    Status status_;
    std::string saved_key_;     // == current key when direction_==kReverse
    std::string saved_value_;  // == current raw value when direction_==kReverse
    Direction direction_;
    bool valid_;

    Random rnd_;
    ssize_t bytes_counter_;

    // No copying allowed
    KVDatabaseIter(const KVDatabaseIter&);
    void operator=(const KVDatabaseIter&);
};

inline bool KVDatabaseIter::ParseKey(ParsedPrimaryKey* ikey)
{
    Dissection k = iter_->key();
    ssize_t n = k.size() + iter_->value().size();
    bytes_counter_ -= n;
    while (bytes_counter_ < 0) {
        bytes_counter_ += RandomPeriod();
        db_->RecordReadSample(k);
    }
    if (!ParsePrimaryKey(k, ikey)) {
        status_ = Status::Corruption(
                "corrupted internal key in KVDatabaseIter");
        return false;
    } else {
        return true;
    }
}

void KVDatabaseIter::Next()
{
    assert(valid_);

    if (direction_ == kReverse) {  // Switch directions?
        direction_ = kForward;
        // iter_ is pointing just before the entries for this->key(),
        // so advance into the range of entries for this->key() and then
        // use the normal skipping code below.
        if (!iter_->Valid()) {
            iter_->SeekToFirst();
        } else {
            iter_->Next();
        }
        if (!iter_->Valid()) {
            valid_ = false;
            saved_key_.clear();
            return;
        }
        // saved_key_ already contains the key to skip past.
    } else {
        // Store in saved_key_ the current key so we skip it below.
        SaveKey(ExtractUserKey(iter_->key()), &saved_key_);
    }

    FindNextUserEntry(true, &saved_key_);
}

void KVDatabaseIter::FindNextUserEntry(bool skipping, std::string* skip)
{
    // Loop until we hit an acceptable entry to yield
    assert(iter_->Valid());
    assert(direction_ == kForward);
    do {
        ParsedPrimaryKey ikey;
        if (ParseKey(&ikey) && ikey.sequence <= sequence_) {
            switch (ikey.type) {
            case kTypeDeletion:
                // Arrange to skip all upcoming entries for this key since
                // they are hidden by this deletion.
                SaveKey(ikey.user_key, skip);
                skipping = true;
                break;
            case kTypeValue:
                if (skipping
                        && user_comparator_->Compare(ikey.user_key, *skip)
                                <= 0) {
                    // Entry hidden
                } else {
                    valid_ = true;
                    saved_key_.clear();
                    return;
                }
                break;
            }
        }
        iter_->Next();
    } while (iter_->Valid());
    saved_key_.clear();
    valid_ = false;
}

void KVDatabaseIter::Prev()
{
    assert(valid_);

    if (direction_ == kForward) {  // Switch directions?
        // iter_ is pointing at the current entry.  Scan backwards until
        // the key changes so we can use the normal reverse scanning code.
        assert(iter_->Valid());  // Otherwise valid_ would have been false
        SaveKey(ExtractUserKey(iter_->key()), &saved_key_);
        while (true) {
            iter_->Prev();
            if (!iter_->Valid()) {
                valid_ = false;
                saved_key_.clear();
                ClearSavedValue();
                return;
            }
            if (user_comparator_->Compare(ExtractUserKey(iter_->key()),
                    saved_key_) < 0) {
                break;
            }
        }
        direction_ = kReverse;
    }

    FindPrevUserEntry();
}

void KVDatabaseIter::FindPrevUserEntry()
{
    assert(direction_ == kReverse);

    ValueType value_type = kTypeDeletion;
    if (iter_->Valid()) {
        do {
            ParsedPrimaryKey ikey;
            if (ParseKey(&ikey) && ikey.sequence <= sequence_) {
                if ((value_type != kTypeDeletion)
                        && user_comparator_->Compare(ikey.user_key, saved_key_)
                                < 0) {
                    // We encountered a non-deleted value in entries for previous keys,
                    break;
                }
                value_type = ikey.type;
                if (value_type == kTypeDeletion) {
                    saved_key_.clear();
                    ClearSavedValue();
                } else {
                    Dissection raw_value = iter_->value();
                    if (saved_value_.capacity() > raw_value.size() + 1048576) {
                        std::string empty;
                        swap(empty, saved_value_);
                    }
                    SaveKey(ExtractUserKey(iter_->key()), &saved_key_);
                    saved_value_.assign(raw_value.data(), raw_value.size());
                }
            }
            iter_->Prev();
        } while (iter_->Valid());
    }

    if (value_type == kTypeDeletion) {
        // End
        valid_ = false;
        saved_key_.clear();
        ClearSavedValue();
        direction_ = kForward;
    } else {
        valid_ = true;
    }
}

void KVDatabaseIter::Seek(const Dissection& target)
{
    direction_ = kForward;
    ClearSavedValue();
    saved_key_.clear();
    AppendPrimaryKey(&saved_key_,
            ParsedPrimaryKey(target, sequence_, kValueTypeForSeek));
    iter_->Seek(saved_key_);
    if (iter_->Valid()) {
        FindNextUserEntry(false, &saved_key_ /* temporary storage */);
    } else {
        valid_ = false;
    }
}

void KVDatabaseIter::SeekToFirst()
{
    direction_ = kForward;
    ClearSavedValue();
    iter_->SeekToFirst();
    if (iter_->Valid()) {
        FindNextUserEntry(false, &saved_key_ /* temporary storage */);
    } else {
        valid_ = false;
    }
}

void KVDatabaseIter::SeekToLast()
{
    direction_ = kReverse;
    ClearSavedValue();
    iter_->SeekToLast();
    FindPrevUserEntry();
}

}  // anonymous namespace

Iterator* NewKVDatabaseIterator(KVDatabaseImpl* db,
        const Equalcomp* user_key_comparator, Iterator* internal_iter,
        SequenceNumber sequence, uint32_t seed)
{
    return new KVDatabaseIter(db, user_key_comparator, internal_iter, sequence,
            seed);
}

}  // namespace LzKVStore
