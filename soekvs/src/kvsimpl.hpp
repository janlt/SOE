#ifndef SOE_KVS_SRC_KVSIMPLH_
#define SOE_KVS_SRC_KVSIMPLH_

#include <deque>
#include <set>
#include "kvconfig.hpp"
#include "logfrontend.hpp"
#include "logbackend.hpp"
#include "image.hpp"
#include "imagelist.hpp"
#include "kvstore.hpp"
#include "configur.hpp"
#include "kvsarch.hpp"
#include "kvsthread.hpp"

namespace LzKVStore {

class MemKVStore;
class KVStoreAccell;
class Version;
class EditingVersion;
class VersionSet;

class KVDatabaseImpl: public KVDatabase
{
public:
    KVDatabaseImpl(const OpenOptions& options, const std::string& dbname);
    virtual ~KVDatabaseImpl();

    // Implementations of the KVDatabase interface
    virtual Status Put(const WriteOptions&, const Dissection& key,
            const Dissection& value);
    virtual Status Delete(const WriteOptions&, const Dissection& key);
    virtual Status Write(const WriteOptions& options, SaveBatch* updates);
    virtual Status Get(const ReadOptions& options, const Dissection& key,
            std::string* value);
    virtual Status Check(const ReadOptions& options, const Dissection& key);
    virtual Iterator* NewIterator(const ReadOptions&);
    virtual const Image* GetImage();
    virtual void ReleaseImage(const Image* snapshot);
    virtual bool GetProperty(const Dissection& property, std::string* value);
    virtual void GetApproximateSizes(const Range* range, int n,
            uint64_t* sizes);
    virtual void CompactRange(const Dissection* begin, const Dissection* end);

    // Extra methods (for testing) that are not in the public KVDatabase interface

    // Compact any files in the named level that overlap [*begin,*end]
    void TEST_CompactRange(int level, const Dissection* begin,
            const Dissection* end);

    // Force current memtable contents to be compacted.
    Status TEST_CompactMemKVStore();

    // Return an internal iterator over the current state of the database.
    // The keys of this iterator are internal keys (see format.h).
    // The returned iterator should be deleted when no longer needed.
    Iterator* TEST_NewInternalIterator();

    // Return the maximum overlapping data (in bytes) at next level for any
    // file at a level >= 1.
    int64_t TEST_MaxNextLevelOverlappingBytes();

    // Record a sample of bytes read at the specified internal key.
    // Samples are taken approximately once every config::kReadBytesPeriod
    // bytes.
    void RecordReadSample(Dissection key);

private:
    friend class KVDatabase;
    struct CompressionState;
    struct Writer;

    Iterator* NewInternalIterator(const ReadOptions&,
            SequenceNumber* latest_snapshot, uint32_t* seed);

    Status NewKVDatabase();

    // Recover the descriptor from persistent storage.  May do a significant
    // amount of work to recover recently logged updates.  Any changes to
    // be made to the descriptor are added to *edit.
    Status Recover(EditingVersion* edit, bool* save_manifest)
            EXCLUSIVE_LOCKS_REQUIRED(mutex_);

    void MaybeIgnoreError(Status* s) const;

    // Delete any unneeded files and stale in-memory entries.
    void DeleteObsoleteFiles();

    // Compact the in-memory write buffer to disk.  Switches to a new
    // log-file/memtable and writes a new descriptor iff successful.
    // Errors are recorded in bg_error_.
    void CompactMemKVStore() EXCLUSIVE_LOCKS_REQUIRED(mutex_);

    Status RecoverLogFile(uint64_t log_number, bool last_log,
            bool* save_manifest, EditingVersion* edit,
            SequenceNumber* max_sequence) EXCLUSIVE_LOCKS_REQUIRED(mutex_);

    Status WriteLevel0KVStore(MemKVStore* mem, EditingVersion* edit,
            Version* base) EXCLUSIVE_LOCKS_REQUIRED(mutex_);

    Status MakeRoomForWrite(bool force /* compact even if there is room? */)
            EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    SaveBatch* BuildBatchGroup(Writer** last_writer);

    void RecordBackgroundError(const Status& s);

    void MaybeScheduleCompression() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    static void BGWork(void* db);
    void BackgroundCall();
    void BackgroundCompression() EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    void CleanupCompression(CompressionState* compact)
            EXCLUSIVE_LOCKS_REQUIRED(mutex_);
    Status DoCompressionWork(CompressionState* compact)
            EXCLUSIVE_LOCKS_REQUIRED(mutex_);

    Status OpenCompressionOutputFile(CompressionState* compact);
    Status FinishCompressionOutputFile(CompressionState* compact,
            Iterator* input);
    Status InstallCompressionResults(CompressionState* compact)
            EXCLUSIVE_LOCKS_REQUIRED(mutex_);

    // Constant after construction
    Configuration* const env_;
    const PrimaryKeyEqualcomp internal_comparator_;
    const InternalFilteringPolicy internal_filter_policy_;
    const OpenOptions options_;  // options_.comparator == &internal_comparator_
    bool owns_info_log_;
    bool owns_cache_;
    const std::string dbname_;

    // table_cache_ provides its own synchronization
    KVStoreAccell* table_cache_;

    // Lock over the persistent KVDatabase state.  Non-NULL iff successfully acquired.
    DatabaseLock* db_lock_;

    // State below is protected by mutex_
    KVSArch::Mutex mutex_;
    KVSArch::AtomicPointer shutting_down_;
    KVSArch::CondVar bg_cv_;          // Signalled when background work finishes
    MemKVStore* mem_;
    MemKVStore* imm_;                // Memtable being compacted
    KVSArch::AtomicPointer has_imm_;  // So bg thread can detect non-NULL imm_
    ModDatabase* logfile_;
    uint64_t logfile_number_;
    KVSLogger::Writer* log_;
    uint32_t seed_;                // For sampling.

    // Queue of writers.
    std::deque<Writer*> writers_;
    SaveBatch* tmp_batch_;

    ImageList snapshots_;

    // Set of table files to protect from deletion because they are
    // part of ongoing compactions.
    std::set<uint64_t> pending_outputs_;

    // Has a background compaction been scheduled or is running?
    bool bg_compaction_scheduled_;

    // Information for a manual compaction
    struct ManualCompression {
        int level;
        bool done;
        const PrimaryKey* begin;   // NULL means beginning of key range
        const PrimaryKey* end;     // NULL means end of key range
        PrimaryKey tmp_storage;    // Used to keep track of compaction progress
    };
    ManualCompression* manual_compaction_;

    VersionSet* versions_;

    // Have we encountered a background error in paranoid mode?
    Status bg_error_;

    // Per level compaction stats.  stats_[level] stores the stats for
    // compactions that produced data for the specified "level".
    struct CompressionStats {
        int64_t micros;
        int64_t bytes_read;
        int64_t bytes_written;

        CompressionStats() :
                micros(0), bytes_read(0), bytes_written(0) {
        }

        void Add(const CompressionStats& c) {
            this->micros += c.micros;
            this->bytes_read += c.bytes_read;
            this->bytes_written += c.bytes_written;
        }
    };
    CompressionStats stats_[config::kNumLevels];

    // No copying allowed
    KVDatabaseImpl(const KVDatabaseImpl&);
    void operator=(const KVDatabaseImpl&);

    const Equalcomp* user_comparator() const {
        return internal_comparator_.user_comparator();
    }
};

// Sanitize db options.  The caller should delete result.info_log if
// it is not equal to src.info_log.
extern OpenOptions SanitizeOptions(const std::string& db,
        const PrimaryKeyEqualcomp* icmp, const InternalFilteringPolicy* ipolicy,
        const OpenOptions& src);

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_KVSIMPLH_
