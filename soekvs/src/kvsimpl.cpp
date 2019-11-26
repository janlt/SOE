#include "kvsimpl.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <cstdint>
#include <stdio.h>
#include <vector>
#include "builder.hpp"
#include "iterator_con.hpp"
#include "kvsiter.hpp"
#include "kvconfig.hpp"
#include "fileops.hpp"
#include "logfrontend.hpp"
#include "logbackend.hpp"
#include "kvsmemcon.hpp"
#include "kvs_mem.hpp"
#include "kvsaccell.hpp"
#include "kvssetver.hpp"
#include "kvsbatchintrin.hpp"
#include "kvstore.hpp"
#include "configur.hpp"
#include "status.hpp"
#include "kvstore.hpp"
#include "kvsbuilder.hpp"
#include "kvsarch.hpp"
#include "block.hpp"
#include "kvsconcat.hpp"
#include "iterator2l.hpp"
#include "code_styles.hpp"
#include "logger.hpp"
#include "kvsmutex.hpp"

namespace LzKVStore {

const int kNumNonKVStoreAccellFiles = 10;

// Information kept for every waiting writer
struct KVDatabaseImpl::Writer {
    Status status;
    SaveBatch* batch;
    bool sync;
    bool done;
    KVSArch::CondVar cv;

    explicit Writer(KVSArch::Mutex* mu) :
            cv(mu)
    {
    }
};

struct KVDatabaseImpl::CompressionState {
    Compression* const compaction;

    // Sequence numbers < smallest_snapshot are not significant since we
    // will never have to service a snapshot below smallest_snapshot.
    // Therefore if we have seen a sequence number S <= smallest_snapshot,
    // we can drop all entries for the same key with sequence numbers < S.
    SequenceNumber smallest_snapshot;

    // Files produced by compaction
    struct Output {
        uint64_t number;
        uint64_t file_size;
        PrimaryKey smallest, largest;
    };
    std::vector<Output> outputs;

    // State kept for output being generated
    ModDatabase* outfile;
    KVStoreBuilder* builder;

    uint64_t total_bytes;

    Output* current_output()
    {
        return &outputs[outputs.size() - 1];
    }

    explicit CompressionState(Compression* c) :
            compaction(c), outfile(NULL), builder(NULL), total_bytes(0)
    {
    }
};

// Fix user-supplied options to be reasonable
template<class T, class V>
static void ClipToRange(T* ptr, V minvalue, V maxvalue)
{
    if (static_cast<V>(*ptr) > maxvalue)
        *ptr = maxvalue;
    if (static_cast<V>(*ptr) < minvalue)
        *ptr = minvalue;
}

OpenOptions SanitizeOptions(const std::string& dbname,
        const PrimaryKeyEqualcomp* icmp, const InternalFilteringPolicy* ipolicy,
        const OpenOptions& src)
{
    OpenOptions result = src;
    result.comparator = icmp;
    result.filter_policy = (src.filter_policy != NULL) ? ipolicy : NULL;
    ClipToRange(&result.max_open_files, 64 + kNumNonKVStoreAccellFiles, 50000);
    ClipToRange(&result.write_buffer_size, 64 << 10, 1 << 30);
    ClipToRange(&result.max_file_size, 1 << 20, 1 << 30);
    ClipToRange(&result.block_size, 1 << 10, 4 << 20);
    if (result.info_log == NULL) {
        // Open a log file in the same directory as the db
        src.env->CreateDir(dbname);  // In case it does not exist
        src.env->RenameFile(InfoLogFileName(dbname),
                OldInfoLogFileName(dbname));
        Status s = src.env->NewMessanger(InfoLogFileName(dbname),
                &result.info_log);
        if (!s.ok()) {
            // No place suitable for logging
            result.info_log = NULL;
        }
    }
    if (result.block_cache == NULL) {
        result.block_cache = NewLRUAccell(8 << 20);
    }
    return result;
}

KVDatabaseImpl::KVDatabaseImpl(const OpenOptions& raw_options,
        const std::string& dbname) :
        env_(raw_options.env), internal_comparator_(raw_options.comparator), internal_filter_policy_(
                raw_options.filter_policy), options_(
                SanitizeOptions(dbname, &internal_comparator_,
                        &internal_filter_policy_, raw_options)), owns_info_log_(
                options_.info_log != raw_options.info_log), owns_cache_(
                options_.block_cache != raw_options.block_cache), dbname_(
                dbname), db_lock_(NULL), shutting_down_(NULL), bg_cv_(&mutex_), mem_(
                NULL), imm_(NULL), logfile_(NULL), logfile_number_(0), log_(
                NULL), seed_(0), tmp_batch_(new SaveBatch), bg_compaction_scheduled_(
                false), manual_compaction_(NULL)
{
    has_imm_.Release_Store(NULL);

    // Reserve ten files or so for other uses and give the rest to KVStoreAccell.
    const int table_cache_size = options_.max_open_files
            - kNumNonKVStoreAccellFiles;
    table_cache_ = new KVStoreAccell(dbname_, &options_, table_cache_size);

    versions_ = new VersionSet(dbname_, &options_, table_cache_,
            &internal_comparator_);
}

KVDatabaseImpl::~KVDatabaseImpl()
{
    // Wait for background work to finish
    mutex_.Lock();
    shutting_down_.Release_Store(this);  // Any non-NULL value is ok
    while (bg_compaction_scheduled_) {
        bg_cv_.Wait();
    }
    mutex_.Unlock();

    if (db_lock_ != NULL) {
        env_->UnlockFile(db_lock_);
    }

    delete versions_;
    if (mem_ != NULL)
        mem_->Unref();
    if (imm_ != NULL)
        imm_->Unref();
    delete tmp_batch_;
    delete log_;
    delete logfile_;
    delete table_cache_;

    if (owns_info_log_) {
        delete options_.info_log;
    }
    if (owns_cache_) {
        delete options_.block_cache;
    }
}

Status KVDatabaseImpl::NewKVDatabase()
{
    EditingVersion new_db;
    new_db.SetEqualcompName(user_comparator()->Name());
    new_db.SetLogNumber(0);
    new_db.SetNextFile(2);
    new_db.SetLastSequence(0);

    const std::string manifest = DescriptorFileName(dbname_, 1);
    ModDatabase* file;
    Status s = env_->NewModDatabase(manifest, &file);
    if (!s.ok()) {
        return s;
    }
    {
        KVSLogger::Writer log(file);
        std::string record;
        new_db.EncodeTo(&record);
        s = log.AddRecord(record);
        if (s.ok()) {
            s = file->Close();
        }
    }
    delete file;
    if (s.ok()) {
        // Make "CURRENT" file that points to the new manifest file.
        s = SetCurrentFile(env_, dbname_, 1);
    } else {
        env_->DeleteFile(manifest);
    }
    return s;
}

void KVDatabaseImpl::MaybeIgnoreError(Status* s) const
{
    if (s->ok() || options_.full_checks) {
        // No change needed
    } else {
        Log(options_.info_log, "Ignoring error %s", s->ToString().c_str());
        *s = Status::OK();
    }
}

void KVDatabaseImpl::DeleteObsoleteFiles()
{
    if (!bg_error_.ok()) {
        // After a background error, we don't know whether a new version may
        // or may not have been committed, so we cannot safely garbage collect.
        return;
    }

    // Make a set of all of the live files
    std::set<uint64_t> live = pending_outputs_;
    versions_->AddLiveFiles(&live);

    std::vector < std::string > filenames;
    env_->GetChildren(dbname_, &filenames); // Ignoring errors on purpose
    uint64_t number;
    FileType type;
    for (size_t i = 0; i < filenames.size(); i++) {
        if (ParseFileName(filenames[i], &number, &type)) {
            bool keep = true;
            switch (type) {
            case kLogFile:
                keep = ((number >= versions_->LogNumber())
                        || (number == versions_->PrevLogNumber()));
                break;
            case kDescriptorFile:
                // Keep my manifest file, and any newer incarnations'
                // (in case there is a race that allows other incarnations)
                keep = (number >= versions_->ManifestFileNumber());
                break;
            case kKVStoreFile:
                keep = (live.find(number) != live.end());
                break;
            case kTempFile:
                // Any temp files that are currently being written to must
                // be recorded in pending_outputs_, which is inserted into "live"
                keep = (live.find(number) != live.end());
                break;
            case kCurrentFile:
            case kKVDatabaseLockFile:
            case kInfoLogFile:
                keep = true;
                break;
            }

            if (!keep) {
                if (type == kKVStoreFile) {
                    table_cache_->Evict(number);
                }
                Log(options_.info_log, "Delete type=%d #%lld\n", int(type),
                        static_cast<unsigned long long>(number));
                env_->DeleteFile(dbname_ + "/" + filenames[i]);
            }
        }
    }
}

Status KVDatabaseImpl::Recover(EditingVersion* edit, bool *save_manifest)
{
    mutex_.AssertHeld();

    // Ignore error from CreateDir since the creation of the KVDatabase is
    // committed only when the descriptor is created, and this directory
    // may already exist from a previous failed creation attempt.
    env_->CreateDir(dbname_);
    assert(db_lock_ == NULL);
    Status s = env_->LockFile(LockFileName(dbname_), &db_lock_);
    if (!s.ok()) {
        return s;
    }

    if (!env_->FileExists(CurrentFileName(dbname_))) {
        if (options_.create_if_missing) {
            s = NewKVDatabase();
            if (!s.ok()) {
                return s;
            }
        } else {
            return Status::InvalidArgument(dbname_,
                    "does not exist (create_if_missing is false)");
        }
    } else {
        if (options_.error_if_exists) {
            return Status::InvalidArgument(dbname_,
                    "exists (error_if_exists is true)");
        }
    }

    s = versions_->Recover(save_manifest);
    if (!s.ok()) {
        return s;
    }
    SequenceNumber max_sequence(0);

    // Recover from all newer log files than the ones named in the
    // descriptor (new log files may have been added by the previous
    // incarnation without registering them in the descriptor).
    //
    // Note that PrevLogNumber() is no longer used, but we pay
    // attention to it in case we are recovering a database
    // produced by an older version of LzKVStore.
    const uint64_t min_log = versions_->LogNumber();
    const uint64_t prev_log = versions_->PrevLogNumber();
    std::vector < std::string > filenames;
    s = env_->GetChildren(dbname_, &filenames);
    if (!s.ok()) {
        return s;
    }
    std::set<uint64_t> expected;
    versions_->AddLiveFiles(&expected);
    uint64_t number;
    FileType type;
    std::vector<uint64_t> logs;
    for (size_t i = 0; i < filenames.size(); i++) {
        if (ParseFileName(filenames[i], &number, &type)) {
            expected.erase(number);
            if (type == kLogFile
                    && ((number >= min_log) || (number == prev_log)))
                logs.push_back(number);
        }
    }
    if (!expected.empty()) {
        char buf[50];
        snprintf(buf, sizeof(buf), "%d missing files; e.g.",
                static_cast<int>(expected.size()));
        return Status::Corruption(buf,
                KVStoreFileName(dbname_, *(expected.begin())));
    }

    // Recover in the order in which the logs were generated
    std::sort(logs.begin(), logs.end());
    for (size_t i = 0; i < logs.size(); i++) {
        s = RecoverLogFile(logs[i], (i == logs.size() - 1), save_manifest, edit,
                &max_sequence);
        if (!s.ok()) {
            return s;
        }

        // The previous incarnation may not have written any MANIFEST
        // records after allocating this log number.  So we manually
        // update the file number allocation counter in VersionSet.
        versions_->MarkFileNumberUsed(logs[i]);
    }

    if (versions_->LastSequence() < max_sequence) {
        versions_->SetLastSequence(max_sequence);
    }

    return Status::OK();
}

Status KVDatabaseImpl::RecoverLogFile(uint64_t log_number, bool last_log,
        bool* save_manifest, EditingVersion* edit,
        SequenceNumber* max_sequence)
{
    struct LogReporter: public KVSLogger::Reader::Reporter {
        Configuration* env;
        Messanger* info_log;
        const char* fname;
        Status* status;  // NULL if options_.full_checks==false
        virtual void Corruption(size_t bytes, const Status& s) {
            Log(info_log, "%s%s: dropping %d bytes; %s",
                    (this->status == NULL ? "(ignoring error) " : ""), fname,
                    static_cast<int>(bytes), s.ToString().c_str());
            if (this->status != NULL && this->status->ok())
                *this->status = s;
        }
    };

    mutex_.AssertHeld();

    // Open the log file
    std::string fname = LogFileName(dbname_, log_number);
    SeqDatabase* file;
    Status status = env_->NewSeqDatabase(fname, &file);
    if (!status.ok()) {
        MaybeIgnoreError(&status);
        return status;
    }

    // Create the log reader.
    LogReporter reporter;
    reporter.env = env_;
    reporter.info_log = options_.info_log;
    reporter.fname = fname.c_str();
    reporter.status = (options_.full_checks ? &status : NULL);
    // We intentionally make log::Reader do checksumming even if
    // full_checks==false so that corruptions cause entire commits
    // to be skipped instead of propagating bad information (like overly
    // large sequence numbers).
    KVSLogger::Reader reader(file, &reporter, true/*checksum*/,
            0/*initial_offset*/);
    Log(options_.info_log, "Recovering log #%llu",
            (unsigned long long) log_number);

    // Read all the records and add to a memtable
    std::string scratch;
    Dissection record;
    SaveBatch batch;
    int compactions = 0;
    MemKVStore* mem = NULL;
    while (reader.ReadRecord(&record, &scratch) && status.ok()) {
        if (record.size() < 12) {
            reporter.Corruption(record.size(),
                    Status::Corruption("log record too small"));
            continue;
        }
        SaveBatchInternal::SetContents(&batch, record);

        if (mem == NULL) {
            mem = new MemKVStore(internal_comparator_);
            mem->Ref();
        }
        status = SaveBatchInternal::InsertInto(&batch, mem);
        MaybeIgnoreError(&status);
        if (!status.ok()) {
            break;
        }
        const SequenceNumber last_seq = SaveBatchInternal::Sequence(&batch)
                + SaveBatchInternal::Count(&batch) - 1;
        if (last_seq > *max_sequence) {
            *max_sequence = last_seq;
        }

        if (mem->ApproximateMemoryUsage() > options_.write_buffer_size) {
            compactions++;
            *save_manifest = true;
            status = WriteLevel0KVStore(mem, edit, NULL);
            mem->Unref();
            mem = NULL;
            if (!status.ok()) {
                // Reflect errors immediately so that conditions like full
                // file-systems cause the KVDatabase::Open() to fail.
                break;
            }
        }
    }

    delete file;

    // See if we should keep reusing the last log file.
    if (status.ok() && options_.reuse_logs && last_log && compactions == 0) {
        assert(logfile_ == NULL);
        assert(log_ == NULL);
        assert(mem_ == NULL);
        uint64_t lfile_size;
        if (env_->GetFileSize(fname, &lfile_size).ok()
                && env_->NewAppendableFile(fname, &logfile_).ok()) {
            Log(options_.info_log, "Reusing old log %s \n", fname.c_str());
            log_ = new KVSLogger::Writer(logfile_, lfile_size);
            logfile_number_ = log_number;
            if (mem != NULL) {
                mem_ = mem;
                mem = NULL;
            } else {
                // mem can be NULL if lognum exists but was empty.
                mem_ = new MemKVStore(internal_comparator_);
                mem_->Ref();
            }
        }
    }

    if (mem != NULL) {
        // mem did not get reused; compact it.
        if (status.ok()) {
            *save_manifest = true;
            status = WriteLevel0KVStore(mem, edit, NULL);
        }
        mem->Unref();
    }

    return status;
}

Status KVDatabaseImpl::WriteLevel0KVStore(MemKVStore* mem, EditingVersion* edit,
        Version* base)
{
    mutex_.AssertHeld();
    const uint64_t start_micros = env_->NowMicros();
    FileMetaData meta;
    meta.number = versions_->NewFileNumber();
    pending_outputs_.insert(meta.number);
    Iterator* iter = mem->NewIterator();
    Log(options_.info_log, "Level-0 table #%llu: started",
            (unsigned long long) meta.number);

    Status s;
    {
        mutex_.Unlock();
        s = BuildKVStore(dbname_, env_, options_, table_cache_, iter, &meta);
        mutex_.Lock();
    }

    Log(options_.info_log, "Level-0 table #%llu: %lld bytes %s",
            (unsigned long long) meta.number,
            (unsigned long long) meta.file_size, s.ToString().c_str());
    delete iter;
    pending_outputs_.erase(meta.number);

    // Note that if file_size is zero, the file has been deleted and
    // should not be added to the manifest.
    int level = 0;
    if (s.ok() && meta.file_size > 0) {
        const Dissection min_user_key = meta.smallest.user_key();
        const Dissection max_user_key = meta.largest.user_key();
        if (base != NULL) {
            level = base->PickLevelForMemKVStoreOutput(min_user_key,
                    max_user_key);
        }
        edit->AddFile(level, meta.number, meta.file_size, meta.smallest,
                meta.largest);
    }

    CompressionStats stats;
    stats.micros = env_->NowMicros() - start_micros;
    stats.bytes_written = meta.file_size;
    stats_[level].Add(stats);
    return s;
}

void KVDatabaseImpl::CompactMemKVStore()
{
    mutex_.AssertHeld();
    assert(imm_ != NULL);

    // Save the contents of the memtable as a new KVStore
    EditingVersion edit;
    Version* base = versions_->current();
    base->Ref();
    Status s = WriteLevel0KVStore(imm_, &edit, base);
    base->Unref();

    if (s.ok() && shutting_down_.Acquire_Load()) {
        s = Status::IOError("Deleting KVDatabase during memtable compaction");
    }

    // Replace immutable memtable with the generated KVStore
    if (s.ok()) {
        edit.SetPrevLogNumber(0);
        edit.SetLogNumber(logfile_number_);  // Earlier logs no longer needed
        s = versions_->LogAndApply(&edit, &mutex_);
    }

    if (s.ok()) {
        // Commit to the new state
        imm_->Unref();
        imm_ = NULL;
        has_imm_.Release_Store(NULL);
        DeleteObsoleteFiles();
    } else {
        RecordBackgroundError(s);
    }
}

void KVDatabaseImpl::CompactRange(const Dissection* begin,
        const Dissection* end)
{
    int max_level_with_files = 1;
    {
        MutexLock l(&mutex_);
        Version* base = versions_->current();
        for (int level = 1; level < config::kNumLevels; level++) {
            if (base->OverlapInLevel(level, begin, end)) {
                max_level_with_files = level;
            }
        }
    }
    TEST_CompactMemKVStore(); // TODO(sanjay): Skip if memtable does not overlap
    for (int level = 0; level < max_level_with_files; level++) {
        TEST_CompactRange(level, begin, end);
    }
}

void KVDatabaseImpl::TEST_CompactRange(int level, const Dissection* begin,
        const Dissection* end)
{
    assert(level >= 0);
    assert(level + 1 < config::kNumLevels);

    PrimaryKey begin_storage, end_storage;

    ManualCompression manual;
    manual.level = level;
    manual.done = false;
    if (begin == NULL) {
        manual.begin = NULL;
    } else {
        begin_storage = PrimaryKey(*begin, kMaxSequenceNumber,
                kValueTypeForSeek);
        manual.begin = &begin_storage;
    }
    if (end == NULL) {
        manual.end = NULL;
    } else {
        end_storage = PrimaryKey(*end, 0, static_cast<ValueType>(0));
        manual.end = &end_storage;
    }

    MutexLock l(&mutex_);
    while (!manual.done && !shutting_down_.Acquire_Load() && bg_error_.ok()) {
        if (manual_compaction_ == NULL) {  // Idle
            manual_compaction_ = &manual;
            MaybeScheduleCompression();
        } else {  // Running either my compaction or another compaction.
            bg_cv_.Wait();
        }
    }
    if (manual_compaction_ == &manual) {
        // Cancel my manual compaction since we aborted early for some reason.
        manual_compaction_ = NULL;
    }
}

Status KVDatabaseImpl::TEST_CompactMemKVStore()
{
    // NULL batch means just wait for earlier writes to be done
    Status s = Write(WriteOptions(), NULL);
    if (s.ok()) {
        // Wait until the compaction completes
        MutexLock l(&mutex_);
        while (imm_ != NULL && bg_error_.ok()) {
            bg_cv_.Wait();
        }
        if (imm_ != NULL) {
            s = bg_error_;
        }
    }
    return s;
}

void KVDatabaseImpl::RecordBackgroundError(const Status& s)
{
    mutex_.AssertHeld();
    if (bg_error_.ok()) {
        bg_error_ = s;
        bg_cv_.SignalAll();
    }
}

void KVDatabaseImpl::MaybeScheduleCompression()
{
    mutex_.AssertHeld();
    if (bg_compaction_scheduled_) {
        // Already scheduled
    } else if (shutting_down_.Acquire_Load()) {
        // KVDatabase is being deleted; no more background compactions
    } else if (!bg_error_.ok()) {
        // Already got an error; no more changes
    } else if (imm_ == NULL && manual_compaction_ == NULL
            && !versions_->NeedsCompression()) {
        // No work to be done
    } else {
        bg_compaction_scheduled_ = true;
        env_->Schedule(&KVDatabaseImpl::BGWork, this);
    }
}

void KVDatabaseImpl::BGWork(void* db)
{
    reinterpret_cast<KVDatabaseImpl*>(db)->BackgroundCall();
}

void KVDatabaseImpl::BackgroundCall()
{
    MutexLock l(&mutex_);
    assert(bg_compaction_scheduled_);
    if (shutting_down_.Acquire_Load()) {
        // No more background work when shutting down.
    } else if (!bg_error_.ok()) {
        // No more background work after a background error.
    } else {
        BackgroundCompression();
    }

    bg_compaction_scheduled_ = false;

    // Previous compaction may have produced too many files in a level,
    // so reschedule another compaction if needed.
    MaybeScheduleCompression();
    bg_cv_.SignalAll();
}

void KVDatabaseImpl::BackgroundCompression()
{
    mutex_.AssertHeld();

    if (imm_ != NULL) {
        CompactMemKVStore();
        return;
    }

    Compression* c;
    bool is_manual = (manual_compaction_ != NULL);
    PrimaryKey manual_end;
    if (is_manual) {
        ManualCompression* m = manual_compaction_;
        c = versions_->CompactRange(m->level, m->begin, m->end);
        m->done = (c == NULL);
        if (c != NULL) {
            manual_end = c->input(0, c->num_input_files(0) - 1)->largest;
        }
        Log(options_.info_log,
                "Manual compaction at level-%d from %s .. %s; will stop at %s\n",
                m->level,
                (m->begin ? m->begin->DebugString().c_str() : "(begin)"),
                (m->end ? m->end->DebugString().c_str() : "(end)"),
                (m->done ? "(end)" : manual_end.DebugString().c_str()));
    } else {
        c = versions_->PickCompression();
    }

    Status status;
    if (c == NULL) {
        // Nothing to do
    } else if (!is_manual && c->IsTrivialMove()) {
        // Move file to next level
        assert(c->num_input_files(0) == 1);
        FileMetaData* f = c->input(0, 0);
        c->edit()->DeleteFile(c->level(), f->number);
        c->edit()->AddFile(c->level() + 1, f->number, f->file_size, f->smallest,
                f->largest);
        status = versions_->LogAndApply(c->edit(), &mutex_);
        if (!status.ok()) {
            RecordBackgroundError(status);
        }
        VersionSet::LevelSummaryStorage tmp;
        Log(options_.info_log, "Moved #%lld to level-%d %lld bytes %s: %s\n",
                static_cast<unsigned long long>(f->number), c->level() + 1,
                static_cast<unsigned long long>(f->file_size),
                status.ToString().c_str(), versions_->LevelSummary(&tmp));
    } else {
        CompressionState* compact = new CompressionState(c);
        status = DoCompressionWork(compact);
        if (!status.ok()) {
            RecordBackgroundError(status);
        }
        CleanupCompression(compact);
        c->ReleaseInputs();
        DeleteObsoleteFiles();
    }
    delete c;

    if (status.ok()) {
        // Done
    } else if (shutting_down_.Acquire_Load()) {
        // Ignore compaction errors found during shutting down
    } else {
        Log(options_.info_log, "Compression error: %s",
                status.ToString().c_str());
    }

    if (is_manual) {
        ManualCompression* m = manual_compaction_;
        if (!status.ok()) {
            m->done = true;
        }
        if (!m->done) {
            // We only compacted part of the requested range.  Update *m
            // to the range that is left to be compacted.
            m->tmp_storage = manual_end;
            m->begin = &m->tmp_storage;
        }
        manual_compaction_ = NULL;
    }
}

void KVDatabaseImpl::CleanupCompression(CompressionState* compact)
{
    mutex_.AssertHeld();
    if (compact->builder != NULL) {
        // May happen if we get a shutdown call in the middle of compaction
        compact->builder->Abandon();
        delete compact->builder;
    } else {
        assert(compact->outfile == NULL);
    }
    delete compact->outfile;
    for (size_t i = 0; i < compact->outputs.size(); i++) {
        const CompressionState::Output& out = compact->outputs[i];
        pending_outputs_.erase(out.number);
    }
    delete compact;
}

Status KVDatabaseImpl::OpenCompressionOutputFile(CompressionState* compact)
{
    assert(compact != NULL);
    assert(compact->builder == NULL);
    uint64_t file_number;
    {
        mutex_.Lock();
        file_number = versions_->NewFileNumber();
        pending_outputs_.insert(file_number);
        CompressionState::Output out;
        out.number = file_number;
        out.smallest.Clear();
        out.largest.Clear();
        compact->outputs.push_back(out);
        mutex_.Unlock();
    }

    // Make the output file
    std::string fname = KVStoreFileName(dbname_, file_number);
    Status s = env_->NewModDatabase(fname, &compact->outfile);
    if (s.ok()) {
        compact->builder = new KVStoreBuilder(options_, compact->outfile);
    }
    return s;
}

Status KVDatabaseImpl::FinishCompressionOutputFile(CompressionState* compact,
        Iterator* input)
{
    assert(compact != NULL);
    assert(compact->outfile != NULL);
    assert(compact->builder != NULL);

    const uint64_t output_number = compact->current_output()->number;
    assert(output_number != 0);

    // Check for iterator errors
    Status s = input->status();
    const uint64_t current_entries = compact->builder->NumEntries();
    if (s.ok()) {
        s = compact->builder->Finish();
    } else {
        compact->builder->Abandon();
    }
    const uint64_t current_bytes = compact->builder->FileSize();
    compact->current_output()->file_size = current_bytes;
    compact->total_bytes += current_bytes;
    delete compact->builder;
    compact->builder = NULL;

    // Finish and check for file errors
    if (s.ok()) {
        s = compact->outfile->Sync();
    }
    if (s.ok()) {
        s = compact->outfile->Close();
    }
    delete compact->outfile;
    compact->outfile = NULL;

    if (s.ok() && current_entries > 0) {
        // Verify that the table is usable
        Iterator* iter = table_cache_->NewIterator(ReadOptions(), output_number,
                current_bytes);
        s = iter->status();
        delete iter;
        if (s.ok()) {
            Log(options_.info_log,
                    "Generated table #%llu@%d: %lld keys, %lld bytes",
                    (unsigned long long) output_number,
                    compact->compaction->level(),
                    (unsigned long long) current_entries,
                    (unsigned long long) current_bytes);
        }
    }
    return s;
}

Status KVDatabaseImpl::InstallCompressionResults(CompressionState* compact)
{
    mutex_.AssertHeld();
    Log(options_.info_log, "Compacted %d@%d + %d@%d files => %lld bytes",
            compact->compaction->num_input_files(0),
            compact->compaction->level(),
            compact->compaction->num_input_files(1),
            compact->compaction->level() + 1,
            static_cast<long long>(compact->total_bytes));

    // Add compaction outputs
    compact->compaction->AddInputDeletions(compact->compaction->edit());
    const int level = compact->compaction->level();
    for (size_t i = 0; i < compact->outputs.size(); i++) {
        const CompressionState::Output& out = compact->outputs[i];
        compact->compaction->edit()->AddFile(level + 1, out.number,
                out.file_size, out.smallest, out.largest);
    }
    return versions_->LogAndApply(compact->compaction->edit(), &mutex_);
}

Status KVDatabaseImpl::DoCompressionWork(CompressionState* compact)
{
    const uint64_t start_micros = env_->NowMicros();
    int64_t imm_micros = 0;  // Micros spent doing imm_ compactions

    Log(options_.info_log, "Compacting %d@%d + %d@%d files",
            compact->compaction->num_input_files(0),
            compact->compaction->level(),
            compact->compaction->num_input_files(1),
            compact->compaction->level() + 1);

    assert(versions_->NumLevelFiles(compact->compaction->level()) > 0);
    assert(compact->builder == NULL);
    assert(compact->outfile == NULL);
    if (snapshots_.empty()) {
        compact->smallest_snapshot = versions_->LastSequence();
    } else {
        compact->smallest_snapshot = snapshots_.oldest()->number_;
    }

    // Release mutex while we're actually doing the compaction work
    mutex_.Unlock();

    Iterator* input = versions_->MakeInputIterator(compact->compaction);
    input->SeekToFirst();
    Status status;
    ParsedPrimaryKey ikey;
    std::string current_user_key;
    bool has_current_user_key = false;
    SequenceNumber last_sequence_for_key = kMaxSequenceNumber;
    for (; input->Valid() && !shutting_down_.Acquire_Load();) {
        // Prioritize immutable compaction work
        if (has_imm_.NoBarrier_Load() != NULL) {
            const uint64_t imm_start = env_->NowMicros();
            mutex_.Lock();
            if (imm_ != NULL) {
                CompactMemKVStore();
                bg_cv_.SignalAll();  // Wakeup MakeRoomForWrite() if necessary
            }
            mutex_.Unlock();
            imm_micros += (env_->NowMicros() - imm_start);
        }

        Dissection key = input->key();
        if (compact->compaction->ShouldStopBefore(key) &&
        compact->builder != NULL) {
            status = FinishCompressionOutputFile(compact, input);
            if (!status.ok()) {
                break;
            }
        }

        // Handle key/value, add to state, etc.
        bool drop = false;
        if (!ParsePrimaryKey(key, &ikey)) {
            // Do not hide error keys
            current_user_key.clear();
            has_current_user_key = false;
            last_sequence_for_key = kMaxSequenceNumber;
        } else {
            if (!has_current_user_key
                    || user_comparator()->Compare(ikey.user_key,
                            Dissection(current_user_key)) != 0) {
                // First occurrence of this user key
                current_user_key.assign(ikey.user_key.data(),
                        ikey.user_key.size());
                has_current_user_key = true;
                last_sequence_for_key = kMaxSequenceNumber;
            }

            if (last_sequence_for_key <= compact->smallest_snapshot) {
                // Hidden by an newer entry for same user key
                drop = true;    // (A)
            } else if (ikey.type == kTypeDeletion
                    && ikey.sequence <= compact->smallest_snapshot
                    && compact->compaction->IsBaseLevelForKey(ikey.user_key)) {
                // For this user key:
                // (1) there is no data in higher levels
                // (2) data in lower levels will have larger sequence numbers
                // (3) data in layers that are being compacted here and have
                //     smaller sequence numbers will be dropped in the next
                //     few iterations of this loop (by rule (A) above).
                // Therefore this deletion marker is obsolete and can be dropped.
                drop = true;
            }

            last_sequence_for_key = ikey.sequence;
        }
#if 0
        Log(options_.info_log,
                "  Compact: %s, seq %d, type: %d %d, drop: %d, is_base: %d, "
                "%d smallest_snapshot: %d",
                ikey.user_key.ToString().c_str(),
                (int)ikey.sequence, ikey.type, kTypeValue, drop,
                compact->compaction->IsBaseLevelForKey(ikey.user_key),
                (int)last_sequence_for_key, (int)compact->smallest_snapshot);
#endif

        if (!drop) {
            // Open output file if necessary
            if (compact->builder == NULL) {
                status = OpenCompressionOutputFile(compact);
                if (!status.ok()) {
                    break;
                }
            }
            if (compact->builder->NumEntries() == 0) {
                compact->current_output()->smallest.DecodeFrom(key);
            }
            compact->current_output()->largest.DecodeFrom(key);
            compact->builder->Add(key, input->value());

            // Close output file if it is big enough
            if (compact->builder->FileSize()
                    >= compact->compaction->MaxOutputFileSize()) {
                status = FinishCompressionOutputFile(compact, input);
                if (!status.ok()) {
                    break;
                }
            }
        }

        input->Next();
    }

    if (status.ok() && shutting_down_.Acquire_Load()) {
        status = Status::IOError("Deleting KVDatabase during compaction");
    }
    if (status.ok() && compact->builder != NULL) {
        status = FinishCompressionOutputFile(compact, input);
    }
    if (status.ok()) {
        status = input->status();
    }
    delete input;
    input = NULL;

    CompressionStats stats;
    stats.micros = env_->NowMicros() - start_micros - imm_micros;
    for (int which = 0; which < 2; which++) {
        for (int i = 0; i < compact->compaction->num_input_files(which); i++) {
            stats.bytes_read += compact->compaction->input(which, i)->file_size;
        }
    }
    for (size_t i = 0; i < compact->outputs.size(); i++) {
        stats.bytes_written += compact->outputs[i].file_size;
    }

    mutex_.Lock();
    stats_[compact->compaction->level() + 1].Add(stats);

    if (status.ok()) {
        status = InstallCompressionResults(compact);
    }
    if (!status.ok()) {
        RecordBackgroundError(status);
    }
    VersionSet::LevelSummaryStorage tmp;
    Log(options_.info_log, "compacted to: %s", versions_->LevelSummary(&tmp));
    return status;
}

namespace {
struct IterState {
    KVSArch::Mutex* mu;
    Version* version;
    MemKVStore* mem;
    MemKVStore* imm;
};

static void CleanupIteratorState(void* arg1, void* arg2)
{
    IterState* state = reinterpret_cast<IterState*>(arg1);
    state->mu->Lock();
    state->mem->Unref();
    if (state->imm != NULL)
        state->imm->Unref();
    state->version->Unref();
    state->mu->Unlock();
    delete state;
}
}  // namespace

Iterator* KVDatabaseImpl::NewInternalIterator(const ReadOptions& options,
        SequenceNumber* latest_snapshot, uint32_t* seed)
{
    IterState* cleanup = new IterState;
    mutex_.Lock();
    *latest_snapshot = versions_->LastSequence();

    // Collect together all needed child iterators
    std::vector<Iterator*> list;
    list.push_back(mem_->NewIterator());
    mem_->Ref();
    if (imm_ != NULL) {
        list.push_back(imm_->NewIterator());
        imm_->Ref();
    }
    versions_->current()->AddIterators(options, &list);
    Iterator* internal_iter = NewMergingIterator(&internal_comparator_,
            &list[0], list.size());
    versions_->current()->Ref();

    cleanup->mu = &mutex_;
    cleanup->mem = mem_;
    cleanup->imm = imm_;
    cleanup->version = versions_->current();
    internal_iter->RegisterCleanup(CleanupIteratorState, cleanup, NULL);

    *seed = ++seed_;
    mutex_.Unlock();
    return internal_iter;
}

Iterator* KVDatabaseImpl::TEST_NewInternalIterator()
{
    SequenceNumber ignored;
    uint32_t ignored_seed;
    return NewInternalIterator(ReadOptions(), &ignored, &ignored_seed);
}

int64_t KVDatabaseImpl::TEST_MaxNextLevelOverlappingBytes()
{
    MutexLock l(&mutex_);
    return versions_->MaxNextLevelOverlappingBytes();
}

Status KVDatabaseImpl::Check(const ReadOptions& options, const Dissection& key)
{
    Status s;
    MutexLock l(&mutex_);
    SequenceNumber snapshot;
    if (options.snapshot != NULL) {
        snapshot =
                reinterpret_cast<const ImageImpl*>(options.snapshot)->number_;
    } else {
        snapshot = versions_->LastSequence();
    }

    MemKVStore* mem = mem_;
    MemKVStore* imm = imm_;
    Version* current = versions_->current();
    mem->Ref();
    if (imm != NULL)
        imm->Ref();
    current->Ref();

    bool have_stat_update = false;
    Version::GetStats stats;

    // Unlock while reading from files and memtables
    {
        mutex_.Unlock();
        // First look in the memtable, then in the immutable memtable (if any).
        SearchKey lkey(key, snapshot);
        if (mem->Check(lkey, &s)) {
            // Done
        } else if (imm != NULL && imm->Check(lkey, &s)) {
            // Done
        } else {
            s = current->Check(options, lkey, &stats);
            have_stat_update = true;
        }
        mutex_.Lock();
    }

    if (have_stat_update && current->UpdateStats(stats)) {
        MaybeScheduleCompression();
    }
    mem->Unref();
    if (imm != NULL)
        imm->Unref();
    current->Unref();
    return s;
}

Status KVDatabaseImpl::Get(const ReadOptions& options, const Dissection& key,
        std::string* value)
{
    Status s;
    MutexLock l(&mutex_);
    SequenceNumber snapshot;
    if (options.snapshot != NULL) {
        snapshot =
                reinterpret_cast<const ImageImpl*>(options.snapshot)->number_;
    } else {
        snapshot = versions_->LastSequence();
    }

    MemKVStore* mem = mem_;
    MemKVStore* imm = imm_;
    Version* current = versions_->current();
    mem->Ref();
    if (imm != NULL)
        imm->Ref();
    current->Ref();

    bool have_stat_update = false;
    Version::GetStats stats;

    // Unlock while reading from files and memtables
    {
        mutex_.Unlock();
        // First look in the memtable, then in the immutable memtable (if any).
        SearchKey lkey(key, snapshot);
        if (mem->Get(lkey, value, &s)) {
            // Done
        } else if (imm != NULL && imm->Get(lkey, value, &s)) {
            // Done
        } else {
            s = current->Get(options, lkey, value, &stats);
            have_stat_update = true;
        }
        mutex_.Lock();
    }

    if (have_stat_update && current->UpdateStats(stats)) {
        MaybeScheduleCompression();
    }
    mem->Unref();
    if (imm != NULL)
        imm->Unref();
    current->Unref();
    return s;
}

Iterator* KVDatabaseImpl::NewIterator(const ReadOptions& options)
{
    SequenceNumber latest_snapshot;
    uint32_t seed;
    Iterator* iter = NewInternalIterator(options, &latest_snapshot, &seed);
    return NewKVDatabaseIterator(this, user_comparator(), iter,
            (options.snapshot != NULL ?
                    reinterpret_cast<const ImageImpl*>(options.snapshot)->number_ :
                    latest_snapshot), seed);
}

void KVDatabaseImpl::RecordReadSample(Dissection key)
{
    MutexLock l(&mutex_);
    if (versions_->current()->RecordReadSample(key)) {
        MaybeScheduleCompression();
    }
}

const Image* KVDatabaseImpl::GetImage()
{
    MutexLock l(&mutex_);
    return snapshots_.New(versions_->LastSequence());
}

void KVDatabaseImpl::ReleaseImage(const Image* s)
{
    MutexLock l(&mutex_);
    snapshots_.Delete(reinterpret_cast<const ImageImpl*>(s));
}

// Convenience methods
Status KVDatabaseImpl::Put(const WriteOptions& o, const Dissection& key,
        const Dissection& val)
{
    return KVDatabase::Put(o, key, val);
}

Status KVDatabaseImpl::Delete(const WriteOptions& options,
        const Dissection& key) {
    return KVDatabase::Delete(options, key);
}

Status KVDatabaseImpl::Write(const WriteOptions& options, SaveBatch* my_batch)
{
    Writer w(&mutex_);
    w.batch = my_batch;
    w.sync = options.sync;
    w.done = false;

    MutexLock l(&mutex_);
    writers_.push_back(&w);
    while (!w.done && &w != writers_.front()) {
        w.cv.Wait();
    }
    if (w.done) {
        return w.status;
    }

    // May temporarily unlock and wait.
    Status status = MakeRoomForWrite(my_batch == NULL);
    uint64_t last_sequence = versions_->LastSequence();
    Writer* last_writer = &w;
    if (status.ok() && my_batch != NULL) {  // NULL batch is for compactions
        SaveBatch* updates = BuildBatchGroup(&last_writer);
        SaveBatchInternal::SetSequence(updates, last_sequence + 1);
        last_sequence += SaveBatchInternal::Count(updates);

        // Add to log and apply to memtable.  We can release the lock
        // during this phase since &w is currently responsible for logging
        // and protects against concurrent loggers and concurrent writes
        // into mem_.
        {
            mutex_.Unlock();
            status = log_->AddRecord(SaveBatchInternal::Contents(updates));
            bool sync_error = false;
            if (status.ok() && options.sync) {
                status = logfile_->Sync();
                if (!status.ok()) {
                    sync_error = true;
                }
            }
            if (status.ok()) {
                status = SaveBatchInternal::InsertInto(updates, mem_);
            }
            mutex_.Lock();
            if (sync_error) {
                // The state of the log file is indeterminate: the log record we
                // just added may or may not show up when the KVDatabase is re-opened.
                // So we force the KVDatabase into a mode where all future writes fail.
                RecordBackgroundError(status);
            }
        }
        if (updates == tmp_batch_)
            tmp_batch_->Clear();

        versions_->SetLastSequence(last_sequence);
    }

    while (true) {
        Writer* ready = writers_.front();
        writers_.pop_front();
        if (ready != &w) {
            ready->status = status;
            ready->done = true;
            ready->cv.Signal();
        }
        if (ready == last_writer)
            break;
    }

    // Notify new head of write queue
    if (!writers_.empty()) {
        writers_.front()->cv.Signal();
    }

    return status;
}

// REQUIRES: Writer list must be non-empty
// REQUIRES: First writer must have a non-NULL batch
SaveBatch* KVDatabaseImpl::BuildBatchGroup(Writer** last_writer)
{
    assert(!writers_.empty());
    Writer* first = writers_.front();
    SaveBatch* result = first->batch;
    assert(result != NULL);

    size_t size = SaveBatchInternal::ByteSize(first->batch);

    // Allow the group to grow up to a maximum size, but if the
    // original write is small, limit the growth so we do not slow
    // down the small write too much.
    size_t max_size = 1 << 20;
    if (size <= (128 << 10)) {
        max_size = size + (128 << 10);
    }

    *last_writer = first;
    std::deque<Writer*>::iterator iter = writers_.begin();
    ++iter;  // Advance past "first"
    for (; iter != writers_.end(); ++iter) {
        Writer* w = *iter;
        if (w->sync && !first->sync) {
            // Do not include a sync write into a batch handled by a non-sync write.
            break;
        }

        if (w->batch != NULL) {
            size += SaveBatchInternal::ByteSize(w->batch);
            if (size > max_size) {
                // Do not make batch too big
                break;
            }

            // Append to *result
            if (result == first->batch) {
                // Switch to temporary batch instead of disturbing caller's batch
                result = tmp_batch_;
                assert(SaveBatchInternal::Count(result) == 0);
                SaveBatchInternal::Append(result, first->batch);
            }
            SaveBatchInternal::Append(result, w->batch);
        }
        *last_writer = w;
    }
    return result;
}

// REQUIRES: mutex_ is held
// REQUIRES: this thread is currently at the front of the writer queue
Status KVDatabaseImpl::MakeRoomForWrite(bool force)
{
    mutex_.AssertHeld();
    assert(!writers_.empty());
    bool allow_delay = !force;
    Status s;
    while (true) {
        if (!bg_error_.ok()) {
            // Yield previous error
            s = bg_error_;
            break;
        } else if (allow_delay
                && versions_->NumLevelFiles(0)
                        >= config::kL0_SlowdownWritesTrigger) {
            // We are getting close to hitting a hard limit on the number of
            // L0 files.  Rather than delaying a single write by several
            // seconds when we hit the hard limit, start delaying each
            // individual write by 1ms to reduce latency variance.  Also,
            // this delay hands over some CPU to the compaction thread in
            // case it is sharing the same core as the writer.
            mutex_.Unlock();
            env_->SleepForMicroseconds(1000);
            allow_delay = false;  // Do not delay a single write more than once
            mutex_.Lock();
        } else if (!force
                && (mem_->ApproximateMemoryUsage() <= options_.write_buffer_size)) {
            // There is room in current memtable
            break;
        } else if (imm_ != NULL) {
            // We have filled up the current memtable, but the previous
            // one is still being compacted, so we wait.
            Log(options_.info_log, "Current memtable full; waiting...\n");
            bg_cv_.Wait();
        } else if (versions_->NumLevelFiles(0)
                >= config::kL0_StopWritesTrigger) {
            // There are too many level-0 files.
            Log(options_.info_log, "Too many L0 files; waiting...\n");
            bg_cv_.Wait();
        } else {
            // Attempt to switch to a new memtable and trigger compaction of old
            assert(versions_->PrevLogNumber() == 0);
            uint64_t new_log_number = versions_->NewFileNumber();
            ModDatabase* lfile = NULL;
            s = env_->NewModDatabase(LogFileName(dbname_, new_log_number),
                    &lfile);
            if (!s.ok()) {
                // Avoid chewing through file number space in a tight loop.
                versions_->ReuseFileNumber(new_log_number);
                break;
            }
            delete log_;
            delete logfile_;
            logfile_ = lfile;
            logfile_number_ = new_log_number;
            log_ = new KVSLogger::Writer(lfile);
            imm_ = mem_;
            has_imm_.Release_Store(imm_);
            mem_ = new MemKVStore(internal_comparator_);
            mem_->Ref();
            force = false;   // Do not force another compaction if have room
            MaybeScheduleCompression();
        }
    }
    return s;
}

bool KVDatabaseImpl::GetProperty(const Dissection& property,
        std::string* value)
{
    value->clear();

    MutexLock l(&mutex_);
    Dissection in = property;
    Dissection prefix("LzKVStore.");
    if (!in.starts_with(prefix))
        return false;
    in.remove_prefix(prefix.size());

    if (in.starts_with("num-files-at-level")) {
        in.remove_prefix(strlen("num-files-at-level"));
        uint64_t level;
        bool ok = ConsumeDecimalNumber(&in, &level) && in.empty();
        if (!ok || level >= config::kNumLevels) {
            return false;
        } else {
            char buf[100];
            snprintf(buf, sizeof(buf), "%d",
                    versions_->NumLevelFiles(static_cast<int>(level)));
            *value = buf;
            return true;
        }
    } else if (in == "stats") {
        char buf[200];
        snprintf(buf, sizeof(buf),
                "                               Compressions\n"
                        "Level  Files Size(MB) Time(sec) Read(MB) Write(MB)\n"
                        "--------------------------------------------------\n");
        value->append(buf);
        for (int level = 0; level < config::kNumLevels; level++) {
            int files = versions_->NumLevelFiles(level);
            if (stats_[level].micros > 0 || files > 0) {
                snprintf(buf, sizeof(buf), "%3d %8d %8.0f %9.0f %8.0f %9.0f\n",
                        level, files,
                        versions_->NumLevelBytes(level) / 1048576.0,
                        stats_[level].micros / 1e6,
                        stats_[level].bytes_read / 1048576.0,
                        stats_[level].bytes_written / 1048576.0);
                value->append(buf);
            }
        }
        return true;
    } else if (in == "mitables") {
        *value = versions_->current()->DebugString();
        return true;
    } else if (in == "approximate-memory-usage") {
        size_t total_usage = options_.block_cache->TotalCharge();
        if (mem_) {
            total_usage += mem_->ApproximateMemoryUsage();
        }
        if (imm_) {
            total_usage += imm_->ApproximateMemoryUsage();
        }
        char buf[50];
        snprintf(buf, sizeof(buf), "%llu",
                static_cast<unsigned long long>(total_usage));
        value->append(buf);
        return true;
    }

    return false;
}

void KVDatabaseImpl::GetApproximateSizes(const Range* range, int n,
        uint64_t* sizes)
{
    // TODO(opt): better implementation
    Version* v;
    {
        MutexLock l(&mutex_);
        versions_->current()->Ref();
        v = versions_->current();
    }

    for (int i = 0; i < n; i++) {
        // Convert user_key into a corresponding internal key.
        PrimaryKey k1(range[i].start, kMaxSequenceNumber, kValueTypeForSeek);
        PrimaryKey k2(range[i].limit, kMaxSequenceNumber, kValueTypeForSeek);
        uint64_t start = versions_->ApproximateOffsetOf(v, k1);
        uint64_t limit = versions_->ApproximateOffsetOf(v, k2);
        sizes[i] = (limit >= start ? limit - start : 0);
    }

    {
        MutexLock l(&mutex_);
        v->Unref();
    }
}

// Default implementations of convenience methods that subclasses of KVDatabase
// can call if they wish
Status KVDatabase::Put(const WriteOptions& opt, const Dissection& key,
        const Dissection& value)
{
    SaveBatch batch;
    batch.Put(key, value);
    return Write(opt, &batch);
}

Status KVDatabase::Delete(const WriteOptions& opt, const Dissection& key)
{
    SaveBatch batch;
    batch.Delete(key);
    return Write(opt, &batch);
}

KVDatabase::~KVDatabase()
{
}

Status KVDatabase::Open(const OpenOptions& options, const std::string& dbname,
        KVDatabase** dbptr)
{
    *dbptr = NULL;

    KVDatabaseImpl* impl = new KVDatabaseImpl(options, dbname);
    impl->mutex_.Lock();
    EditingVersion edit;
    // Recover handles create_if_missing, error_if_exists
    bool save_manifest = false;
    Status s = impl->Recover(&edit, &save_manifest);
    if (s.ok() && impl->mem_ == NULL) {
        // Create new log and a corresponding memtable.
        uint64_t new_log_number = impl->versions_->NewFileNumber();
        ModDatabase* lfile;
        s = options.env->NewModDatabase(LogFileName(dbname, new_log_number),
                &lfile);
        if (s.ok()) {
            edit.SetLogNumber(new_log_number);
            impl->logfile_ = lfile;
            impl->logfile_number_ = new_log_number;
            impl->log_ = new KVSLogger::Writer(lfile);
            impl->mem_ = new MemKVStore(impl->internal_comparator_);
            impl->mem_->Ref();
        }
    }
    if (s.ok() && save_manifest) {
        edit.SetPrevLogNumber(0);  // No older logs needed after recovery.
        edit.SetLogNumber(impl->logfile_number_);
        s = impl->versions_->LogAndApply(&edit, &impl->mutex_);
    }
    if (s.ok()) {
        impl->DeleteObsoleteFiles();
        impl->MaybeScheduleCompression();
    }
    impl->mutex_.Unlock();
    if (s.ok()) {
        assert(impl->mem_ != NULL);
        *dbptr = impl;
    } else {
        delete impl;
    }
    return s;
}

Image::~Image()
{
}

Status DestroyKVDatabase(const std::string& dbname, const OpenOptions& options)
{
    Configuration* env = options.env;
    std::vector < std::string > filenames;
    // Ignore error in case directory does not exist
    env->GetChildren(dbname, &filenames);
    if (filenames.empty()) {
        return Status::OK();
    }

    DatabaseLock* lock;
    const std::string lockname = LockFileName(dbname);
    Status result = env->LockFile(lockname, &lock);
    if (result.ok()) {
        uint64_t number;
        FileType type;
        for (size_t i = 0; i < filenames.size(); i++) {
            if (ParseFileName(filenames[i], &number, &type)
                    && type != kKVDatabaseLockFile) { // Lock file will be deleted at end
                Status del = env->DeleteFile(dbname + "/" + filenames[i]);
                if (result.ok() && !del.ok()) {
                    result = del;
                }
            }
        }
        env->UnlockFile(lock);  // Ignore error since state is already gone
        env->DeleteFile(lockname);
        env->DeleteDir(dbname); // Ignore error in case dir contains other files
    }
    return result;
}

}  // namespace LzKVStore
