#include "kvsbatchops.hpp"

#include "fileops.hpp"
#include "end_rec.hpp"
#include "image.hpp"
#include "kvconfig.hpp"
#include "kvsmemcon.hpp"
#include "kvs_mem.hpp"
#include "kvsbatchintrin.hpp"
#include "code_styles.hpp"

namespace LzKVStore {

// SaveBatch header has an 8-byte sequence number followed by a 4-byte count.
static const size_t kHeader = 12;

SaveBatch::SaveBatch()
{
    Clear();
}

SaveBatch::~SaveBatch()
{
}

SaveBatch::Handler::~Handler()
{
}

void SaveBatch::Clear()
{
    rep_.clear();
    rep_.resize(kHeader);
}

Status SaveBatch::Iterate(Handler* handler) const
{
    Dissection input(rep_);
    if (input.size() < kHeader) {
        return Status::Corruption("malformed SaveBatch (too small)");
    }

    input.remove_prefix(kHeader);
    Dissection key, value;
    int found = 0;
    while (!input.empty()) {
        found++;
        char tag = input[0];
        input.remove_prefix(1);
        switch (tag) {
        case kTypeValue:
            if (GetLengthPrefixedDissection(&input, &key)
                    && GetLengthPrefixedDissection(&input, &value)) {
                handler->Put(key, value);
            } else {
                return Status::Corruption("bad SaveBatch Put");
            }
            break;
        case kTypeDeletion:
            if (GetLengthPrefixedDissection(&input, &key)) {
                handler->Delete(key);
            } else {
                return Status::Corruption("bad SaveBatch Delete");
            }
            break;
        default:
            return Status::Corruption("unknown SaveBatch tag");
        }
    }
    if (found != SaveBatchInternal::Count(this)) {
        return Status::Corruption("SaveBatch has wrong count");
    } else {
        return Status::OK();
    }
}

int SaveBatchInternal::Count(const SaveBatch* b)
{
    return DecodeFixed32(b->rep_.data() + 8);
}

void SaveBatchInternal::SetCount(SaveBatch* b, int n)
{
    EncodeFixed32(&b->rep_[8], n);
}

SequenceNumber SaveBatchInternal::Sequence(const SaveBatch* b)
{
    return SequenceNumber(DecodeFixed64(b->rep_.data()));
}

void SaveBatchInternal::SetSequence(SaveBatch* b, SequenceNumber seq)
{
    EncodeFixed64(&b->rep_[0], seq);
}

void SaveBatch::Put(const Dissection& key, const Dissection& value)
{
    SaveBatchInternal::SetCount(this, SaveBatchInternal::Count(this) + 1);
    rep_.push_back(static_cast<char>(kTypeValue));
    PutLengthPrefixedDissection(&rep_, key);
    PutLengthPrefixedDissection(&rep_, value);
}

void SaveBatch::Delete(const Dissection& key) {
    SaveBatchInternal::SetCount(this, SaveBatchInternal::Count(this) + 1);
    rep_.push_back(static_cast<char>(kTypeDeletion));
    PutLengthPrefixedDissection(&rep_, key);
}

namespace {
class MemKVStoreInserter: public SaveBatch::Handler
{
public:
    SequenceNumber sequence_;
    MemKVStore* mem_;

    virtual void Put(const Dissection& key, const Dissection& value)
    {
        mem_->Add(sequence_, kTypeValue, key, value);
        sequence_++;
    }
    virtual void Delete(const Dissection& key)
    {
        mem_->Add(sequence_, kTypeDeletion, key, Dissection());
        sequence_++;
    }
};
}  // namespace

Status SaveBatchInternal::InsertInto(const SaveBatch* b, MemKVStore* memtable)
{
    MemKVStoreInserter inserter;
    inserter.sequence_ = SaveBatchInternal::Sequence(b);
    inserter.mem_ = memtable;
    return b->Iterate(&inserter);
}

void SaveBatchInternal::SetContents(SaveBatch* b, const Dissection& contents)
{
    assert(contents.size() >= kHeader);
    b->rep_.assign(contents.data(), contents.size());
}

void SaveBatchInternal::Append(SaveBatch* dst, const SaveBatch* src)
{
    SetCount(dst, Count(dst) + Count(src));
    assert(src->rep_.size() >= kHeader);
    dst->rep_.append(src->rep_.data() + kHeader, src->rep_.size() - kHeader);
}

}  // namespace LzKVStore
