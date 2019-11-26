#include <stdio.h>
#include "kvconfig.hpp"
#include "fileops.hpp"
#include "logfrontend.hpp"
#include "kvseditver.hpp"
#include "kvsbatchintrin.hpp"
#include "sandbox.hpp"
#include "configur.hpp"
#include "iterator.hpp"
#include "kvconfig.hpp"
#include "status.hpp"
#include "kvstore.hpp"
#include "kvsbatchops.hpp"
#include "logbackend.hpp"
#include "logger.hpp"

namespace LzKVStore {

namespace {

bool GuessType(const std::string& fname, FileType* type)
{
    size_t pos = fname.rfind('/');
    std::string basename;
    if (pos == std::string::npos) {
        basename = fname;
    } else {
        basename = std::string(fname.data() + pos + 1, fname.size() - pos - 1);
    }
    uint64_t ignored;
    return ParseFileName(basename, &ignored, type);
}

// Notified when KVSLogger reader encounters corruption.
class CorruptionReporter: public KVSLogger::Reader::Reporter
{
public:
    ModDatabase* dst_;
    virtual void Corruption(size_t bytes, const Status& status) {
        std::string r = "corruption: ";
        AppendNumberTo(&r, bytes);
        r += " bytes; ";
        r += status.ToString();
        r.push_back('\n');
        dst_->Append(r);
    }
};

// Print contents of a KVSLogger file. (*func)() is called on every record.
Status PrintLogContents(Configuration* env, const std::string& fname,
        void (*func)(uint64_t, Dissection, ModDatabase*), ModDatabase* dst)
{
    SeqDatabase* file;
    Status s = env->NewSeqDatabase(fname, &file);
    if (!s.ok()) {
        return s;
    }
    CorruptionReporter reporter;
    reporter.dst_ = dst;
    KVSLogger::Reader reader(file, &reporter, true, 0);
    Dissection record;
    std::string scratch;
    while (reader.ReadRecord(&record, &scratch)) {
        (*func)(reader.LastRecordOffset(), record, dst);
    }
    delete file;
    return Status::OK();
}

// Called on every item found in a SaveBatch.
class SaveBatchItemPrinter: public SaveBatch::Handler
{
public:
    ModDatabase* dst_;
    virtual void Put(const Dissection& key, const Dissection& value)
    {
        std::string r = "  put '";
        AppendEscapedStringTo(&r, key);
        r += "' '";
        AppendEscapedStringTo(&r, value);
        r += "'\n";
        dst_->Append(r);
    }
    virtual void Delete(const Dissection& key)
    {
        std::string r = "  del '";
        AppendEscapedStringTo(&r, key);
        r += "'\n";
        dst_->Append(r);
    }
};

// Called on every KVSLogger record (each one of which is a SaveBatch)
// found in a kLogFile.
static void SaveBatchPrinter(uint64_t pos, Dissection record,
        ModDatabase* dst)
{
    std::string r = "--- offset ";
    AppendNumberTo(&r, pos);
    r += "; ";
    if (record.size() < 12) {
        r += "KVSLogger record length ";
        AppendNumberTo(&r, record.size());
        r += " is too small\n";
        dst->Append(r);
        return;
    }
    SaveBatch batch;
    SaveBatchInternal::SetContents(&batch, record);
    r += "sequence ";
    AppendNumberTo(&r, SaveBatchInternal::Sequence(&batch));
    r.push_back('\n');
    dst->Append(r);
    SaveBatchItemPrinter batch_item_printer;
    batch_item_printer.dst_ = dst;
    Status s = batch.Iterate(&batch_item_printer);
    if (!s.ok()) {
        dst->Append("  error: " + s.ToString() + "\n");
    }
}

Status DumpLog(Configuration* env, const std::string& fname, ModDatabase* dst)
{
    return PrintLogContents(env, fname, SaveBatchPrinter, dst);
}

// Called on every KVSLogger record (each one of which is a SaveBatch)
// found in a kDescriptorFile.
static void EditingVersionPrinter(uint64_t pos, Dissection record,
        ModDatabase* dst)
{
    std::string r = "--- offset ";
    AppendNumberTo(&r, pos);
    r += "; ";
    EditingVersion edit;
    Status s = edit.DecodeFrom(record);
    if (!s.ok()) {
        r += s.ToString();
        r.push_back('\n');
    } else {
        r += edit.DebugString();
    }
    dst->Append(r);
}

Status DumpDescriptor(Configuration* env, const std::string& fname,
        ModDatabase* dst)
{
    return PrintLogContents(env, fname, EditingVersionPrinter, dst);
}

Status DumpKVStore(Configuration* env, const std::string& fname,
        ModDatabase* dst)
{
    uint64_t file_size;
    RADatabase* file = NULL;
    KVStore* table = NULL;
    Status s = env->GetFileSize(fname, &file_size);
    if (s.ok()) {
        s = env->NewRADatabase(fname, &file);
    }
    if (s.ok()) {
        // We use the default comparator, which may or may not match the
        // comparator used in this database. However this should not cause
        // problems since we only use KVStore operations that do not require
        // any comparisons.  In particular, we do not call Seek or Prev.
        s = KVStore::Open(OpenOptions(), file, file_size, &table);
    }
    if (!s.ok()) {
        delete table;
        delete file;
        return s;
    }

    ReadOptions ro;
    ro.fill_cache = false;
    Iterator* iter = table->NewIterator(ro);
    std::string r;
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        r.clear();
        ParsedPrimaryKey key;
        if (!ParsePrimaryKey(iter->key(), &key)) {
            r = "badkey '";
            AppendEscapedStringTo(&r, iter->key());
            r += "' => '";
            AppendEscapedStringTo(&r, iter->value());
            r += "'\n";
            dst->Append(r);
        } else {
            r = "'";
            AppendEscapedStringTo(&r, key.user_key);
            r += "' @ ";
            AppendNumberTo(&r, key.sequence);
            r += " : ";
            if (key.type == kTypeDeletion) {
                r += "del";
            } else if (key.type == kTypeValue) {
                r += "val";
            } else {
                AppendNumberTo(&r, key.type);
            }
            r += " => '";
            AppendEscapedStringTo(&r, iter->value());
            r += "'\n";
            dst->Append(r);
        }
    }
    s = iter->status();
    if (!s.ok()) {
        dst->Append("iterator error: " + s.ToString() + "\n");
    }

    delete iter;
    delete table;
    delete file;
    return Status::OK();
}

}  // namespace

Status DumpFile(Configuration* env, const std::string& fname,
        ModDatabase* dst)
{
    FileType ftype;
    if (!GuessType(fname, &ftype)) {
        return Status::InvalidArgument(fname + ": unknown file type");
    }
    switch (ftype) {
    case kLogFile:
        return DumpLog(env, fname, dst);
    case kDescriptorFile:
        return DumpDescriptor(env, fname, dst);
    case kKVStoreFile:
        return DumpKVStore(env, fname, dst);
    default:
        break;
    }
    return Status::InvalidArgument(fname + ": not a dump-able file type");
}

}  // namespace LzKVStore
