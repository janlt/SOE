#include "kvsbuilder.hpp"
#include "kvsimpl.hpp"
#include "kvconfig.hpp"
#include "fileops.hpp"
#include "logfrontend.hpp"
#include "logbackend.hpp"
#include "kvs_mem.hpp"
#include "kvsmlist.hpp"
#include "kvsmemcon.hpp"
#include "kvsimpl.hpp"
#include "builder.hpp"
#include "kvsaccell.hpp"
#include "kvseditver.hpp"
#include "kvsbatchintrin.hpp"
#include "equalcomp.hpp"
#include "kvstore.hpp"
#include "configur.hpp"
#include "end_rec.hpp"

namespace LzKVStore {

namespace {

class Repairer
{
public:
    Repairer(const std::string& dbname, const OpenOptions& options) :
            dbname_(dbname), env_(options.env), icmp_(options.comparator), ipolicy_(
                    options.filter_policy), options_(
                    SanitizeOptions(dbname, &icmp_, &ipolicy_, options)), owns_info_log_(
                    options_.info_log != options.info_log), owns_cache_(
                    options_.block_cache != options.block_cache), next_file_number_(
                    1)
    {
        // KVStoreAccell can be small since we expect each table to be opened once.
        table_cache_ = new KVStoreAccell(dbname_, &options_, 10);
    }

    ~Repairer()
    {
        delete table_cache_;
        if (owns_info_log_) {
            delete options_.info_log;
        }
        if (owns_cache_) {
            delete options_.block_cache;
        }
    }

    Status Run()
    {
        Status status = FindFiles();
        if (status.ok()) {
            ConvertLogFilesToKVStores();
            ExtractMetaData();
            status = WriteDescriptor();
        }
        if (status.ok()) {
            unsigned long long bytes = 0;
            for (size_t i = 0; i < tables_.size(); i++) {
                bytes += tables_[i].meta.file_size;
            }
            Log(options_.info_log, "**** Repaired LzKVStore %s; "
                    "recovered %d files; %llu bytes. "
                    "Some data may have been lost. "
                    "****", dbname_.c_str(), static_cast<int>(tables_.size()),
                    bytes);
        }
        return status;
    }

private:
    struct KVStoreInfo {
        FileMetaData meta;
        SequenceNumber max_sequence;
    };

    std::string const dbname_;
    Configuration* const env_;
    PrimaryKeyEqualcomp const icmp_;
    InternalFilteringPolicy const ipolicy_;
    OpenOptions const options_;
    bool owns_info_log_;
    bool owns_cache_;
    KVStoreAccell* table_cache_;
    EditingVersion edit_;

    std::vector<std::string> manifests_;
    std::vector<uint64_t> table_numbers_;
    std::vector<uint64_t> logs_;
    std::vector<KVStoreInfo> tables_;
    uint64_t next_file_number_;

    Status FindFiles()
    {
        std::vector < std::string > filenames;
        Status status = env_->GetChildren(dbname_, &filenames);
        if (!status.ok()) {
            return status;
        }
        if (filenames.empty()) {
            return Status::IOError(dbname_, "repair found no files");
        }

        uint64_t number;
        FileType type;
        for (size_t i = 0; i < filenames.size(); i++) {
            if (ParseFileName(filenames[i], &number, &type)) {
                if (type == kDescriptorFile) {
                    manifests_.push_back(filenames[i]);
                } else {
                    if (number + 1 > next_file_number_) {
                        next_file_number_ = number + 1;
                    }
                    if (type == kLogFile) {
                        logs_.push_back(number);
                    } else if (type == kKVStoreFile) {
                        table_numbers_.push_back(number);
                    } else {
                        // Ignore other files
                    }
                }
            }
        }
        return status;
    }

    void ConvertLogFilesToKVStores()
    {
        for (size_t i = 0; i < logs_.size(); i++) {
            std::string logname = LogFileName(dbname_, logs_[i]);
            Status status = ConvertLogToKVStore(logs_[i]);
            if (!status.ok()) {
                Log(options_.info_log,
                        "Log #%llu: ignoring conversion error: %s",
                        (unsigned long long) logs_[i],
                        status.ToString().c_str());
            }
            ArchiveFile(logname);
        }
    }

    Status ConvertLogToKVStore(uint64_t log)
    {
        struct LogReporter: public KVSLogger::Reader::Reporter {
            Configuration* env;
            Messanger* info_log;
            uint64_t lognum;
            virtual void Corruption(size_t bytes, const Status& s) {
                // We print error messages for corruption, but continue repairing.
                Log(info_log, "Log #%llu: dropping %d bytes; %s",
                        (unsigned long long) lognum, static_cast<int>(bytes),
                        s.ToString().c_str());
            }
        };

        // Open the log file
        std::string logname = LogFileName(dbname_, log);
        SeqDatabase* lfile;
        Status status = env_->NewSeqDatabase(logname, &lfile);
        if (!status.ok()) {
            return status;
        }

        // Create the log reader.
        LogReporter reporter;
        reporter.env = env_;
        reporter.info_log = options_.info_log;
        reporter.lognum = log;
        // We intentionally make log::Reader do checksumming so that
        // corruptions cause entire commits to be skipped instead of
        // propagating bad information (like overly large sequence
        // numbers).
        KVSLogger::Reader reader(lfile, &reporter, false/*do not checksum*/,
                0/*initial_offset*/);

        // Read all the records and add to a memtable
        std::string scratch;
        Dissection record;
        SaveBatch batch;
        MemKVStore* mem = new MemKVStore(icmp_);
        mem->Ref();
        int counter = 0;
        while (reader.ReadRecord(&record, &scratch)) {
            if (record.size() < 12) {
                reporter.Corruption(record.size(),
                        Status::Corruption("log record too small"));
                continue;
            }
            SaveBatchInternal::SetContents(&batch, record);
            status = SaveBatchInternal::InsertInto(&batch, mem);
            if (status.ok()) {
                counter += SaveBatchInternal::Count(&batch);
            } else {
                Log(options_.info_log, "Log #%llu: ignoring %s",
                        (unsigned long long) log, status.ToString().c_str());
                status = Status::OK();  // Keep going with rest of file
            }
        }
        delete lfile;

        // Do not record a version edit for this conversion to a KVStore
        // since ExtractMetaData() will also generate edits.
        FileMetaData meta;
        meta.number = next_file_number_++;
        Iterator* iter = mem->NewIterator();
        status = BuildKVStore(dbname_, env_, options_, table_cache_, iter,
                &meta);
        delete iter;
        mem->Unref();
        mem = NULL;
        if (status.ok()) {
            if (meta.file_size > 0) {
                table_numbers_.push_back(meta.number);
            }
        }
        Log(options_.info_log, "Log #%llu: %d ops saved to KVStore #%llu %s",
                (unsigned long long) log, counter,
                (unsigned long long) meta.number, status.ToString().c_str());
        return status;
    }

    void ExtractMetaData()
    {
        for (size_t i = 0; i < table_numbers_.size(); i++) {
            ScanKVStore(table_numbers_[i]);
        }
    }

    Iterator* NewKVStoreIterator(const FileMetaData& meta)
    {
        // Same as compaction iterators: if full_checks are on, turn
        // on checksum verification.
        ReadOptions r;
        r.verify_checksums = options_.full_checks;
        return table_cache_->NewIterator(r, meta.number, meta.file_size);
    }

    void ScanKVStore(uint64_t number)
    {
        KVStoreInfo t;
        t.meta.number = number;
        std::string fname = KVStoreFileName(dbname_, number);
        Status status = env_->GetFileSize(fname, &t.meta.file_size);
        if (!status.ok()) {
            // Try alternate file name.
            fname = MITKVStoreFileName(dbname_, number);
            Status s2 = env_->GetFileSize(fname, &t.meta.file_size);
            if (s2.ok()) {
                status = Status::OK();
            }
        }
        if (!status.ok()) {
            ArchiveFile(KVStoreFileName(dbname_, number));
            ArchiveFile(MITKVStoreFileName(dbname_, number));
            Log(options_.info_log, "KVStore #%llu: dropped: %s",
                    (unsigned long long) t.meta.number,
                    status.ToString().c_str());
            return;
        }

        // Extract metadata by scanning through table.
        int counter = 0;
        Iterator* iter = NewKVStoreIterator(t.meta);
        bool empty = true;
        ParsedPrimaryKey parsed;
        t.max_sequence = 0;
        for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
            Dissection key = iter->key();
            if (!ParsePrimaryKey(key, &parsed)) {
                Log(options_.info_log, "KVStore #%llu: unparsable key %s",
                        (unsigned long long) t.meta.number,
                        EscapeString(key).c_str());
                continue;
            }

            counter++;
            if (empty) {
                empty = false;
                t.meta.smallest.DecodeFrom(key);
            }
            t.meta.largest.DecodeFrom(key);
            if (parsed.sequence > t.max_sequence) {
                t.max_sequence = parsed.sequence;
            }
        }
        if (!iter->status().ok()) {
            status = iter->status();
        }
        delete iter;
        Log(options_.info_log, "KVStore #%llu: %d entries %s",
                (unsigned long long) t.meta.number, counter,
                status.ToString().c_str());

        if (status.ok()) {
            tables_.push_back(t);
        } else {
            RepairKVStore(fname, t);  // RepairKVStore archives input file.
        }
    }

    void RepairKVStore(const std::string& src, KVStoreInfo t)
    {
        // We will copy src contents to a new table and then rename the
        // new table over the source.

        // Create builder.
        std::string copy = KVStoreFileName(dbname_, next_file_number_++);
        ModDatabase* file;
        Status s = env_->NewModDatabase(copy, &file);
        if (!s.ok()) {
            return;
        }
        KVStoreBuilder* builder = new KVStoreBuilder(options_, file);

        // Copy data.
        Iterator* iter = NewKVStoreIterator(t.meta);
        int counter = 0;
        for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
            builder->Add(iter->key(), iter->value());
            counter++;
        }
        delete iter;

        ArchiveFile(src);
        if (counter == 0) {
            builder->Abandon();  // Nothing to save
        } else {
            s = builder->Finish();
            if (s.ok()) {
                t.meta.file_size = builder->FileSize();
            }
        }
        delete builder;
        builder = NULL;

        if (s.ok()) {
            s = file->Close();
        }
        delete file;
        file = NULL;

        if (counter > 0 && s.ok()) {
            std::string orig = KVStoreFileName(dbname_, t.meta.number);
            s = env_->RenameFile(copy, orig);
            if (s.ok()) {
                Log(options_.info_log, "KVStore #%llu: %d entries repaired",
                        (unsigned long long) t.meta.number, counter);
                tables_.push_back(t);
            }
        }
        if (!s.ok()) {
            env_->DeleteFile(copy);
        }
    }

    Status WriteDescriptor()
    {
        std::string tmp = TempFileName(dbname_, 1);
        ModDatabase* file;
        Status status = env_->NewModDatabase(tmp, &file);
        if (!status.ok()) {
            return status;
        }

        SequenceNumber max_sequence = 0;
        for (size_t i = 0; i < tables_.size(); i++) {
            if (max_sequence < tables_[i].max_sequence) {
                max_sequence = tables_[i].max_sequence;
            }
        }

        edit_.SetEqualcompName(icmp_.user_comparator()->Name());
        edit_.SetLogNumber(0);
        edit_.SetNextFile(next_file_number_);
        edit_.SetLastSequence(max_sequence);

        for (size_t i = 0; i < tables_.size(); i++) {
            // TODO(opt): separate out into multiple levels
            const KVStoreInfo& t = tables_[i];
            edit_.AddFile(0, t.meta.number, t.meta.file_size, t.meta.smallest,
                    t.meta.largest);
        }

        //fprintf(stderr, "NewDescriptor:\n%s\n", edit_.DebugString().c_str());
        {
            KVSLogger::Writer log(file);
            std::string record;
            edit_.EncodeTo(&record);
            status = log.AddRecord(record);
        }
        if (status.ok()) {
            status = file->Close();
        }
        delete file;
        file = NULL;

        if (!status.ok()) {
            env_->DeleteFile(tmp);
        } else {
            // Discard older manifests
            for (size_t i = 0; i < manifests_.size(); i++) {
                ArchiveFile(dbname_ + "/" + manifests_[i]);
            }

            // Install new manifest
            status = env_->RenameFile(tmp, DescriptorFileName(dbname_, 1));
            if (status.ok()) {
                status = SetCurrentFile(env_, dbname_, 1);
            } else {
                env_->DeleteFile(tmp);
            }
        }
        return status;
    }

    void ArchiveFile(const std::string& fname)
    {
        // Move into another directory.  E.g., for
        //    dir/foo
        // rename to
        //    dir/lost/foo
        const char* slash = strrchr(fname.c_str(), '/');
        std::string new_dir;
        if (slash != NULL) {
            new_dir.assign(fname.data(), slash - fname.data());
        }
        new_dir.append("/lost");
        env_->CreateDir(new_dir);  // Ignore error
        std::string new_file = new_dir;
        new_file.append("/");
        new_file.append((slash == NULL) ? fname.c_str() : slash + 1);
        Status s = env_->RenameFile(fname, new_file);
        Log(options_.info_log, "Archiving %s: %s\n", fname.c_str(),
                s.ToString().c_str());
    }
};
}  // namespace

Status RepairKVDatabase(const std::string& dbname, const OpenOptions& options)
{
    Repairer repairer(dbname, options);
    return repairer.Run();
}

}  // namespace LzKVStore
