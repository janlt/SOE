#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "kvsimpl.hpp"
#include "kvssetver.hpp"
#include "accell.hpp"
#include "kvstore.hpp"
#include "configur.hpp"
#include "kvsbatchops.hpp"
#include "kvsarch.hpp"
#include "crc32c.hpp"
#include "plots.hpp"
#include "kvsmutex.hpp"
#include "random.hpp"
#include "testutil.hpp"

// Comma-separated list of operations to run in the specified order
//   Actual perfs:
//      writeseq       -- write N values in sequential key order in async mode
//      writerandom    -- write N values in random key order in async mode
//      overwrite     -- overwrite N values in random key order in async mode
//      writesync      -- write N/100 values in random key order in sync mode
//      write100K      -- write N/1000 100K values in random order in async mode
//      deleteseq     -- delete N keys in sequential order
//      deleterandom  -- delete N keys in random order
//      readseq       -- read N times sequentially
//      readreverse   -- read N times in reverse order
//      readrandom    -- read N times in random order
//      readmissing   -- read N missing keys in random order
//      readhot       -- read N times in random order from 1% section of KVDatabase
//      seekrandom    -- N random seeks
//      open          -- cost of opening a KVDatabase
//      crc32c        -- repeated crc32c of 4K of data
//      acquireload   -- load N*1000 times
//   Meta operations:
//      compact     -- Compact the entire KVDatabase
//      stats       -- Print KVDatabase stats
//      mitables    -- Print mitable info
//      heapprofile -- Dump a heap profile (if supported by this port)
static const char* FLAGS_perfs = "writeseq,"
        "writesync,"
        "writerandom,"
        "overwrite,"
        "readrandom,"
        "readrandom,"  // Extra run to allow previous compactions to quiesce
        "readseq,"
        "readreverse,"
        "compact,"
        "readrandom,"
        "readseq,"
        "readreverse,"
        "write100K,"
        "crc32c,"
        "snappycomp,"
        "snappyuncomp,"
        "acquireload,";

// Number of key/values to place in database
static int FLAGS_num = 1000000;

// Number of read operations to do.  If negative, do FLAGS_num reads.
static int FLAGS_reads = -1;

// Number of concurrent threads to run.
static int FLAGS_threads = 1;

// Size of each value
static int FLAGS_value_size = 100;

// Arrange to generate values that shrink to this fraction of
// their original size after compression
static double FLAGS_compression_ratio = 0.5;

// Print histogram of operation timings
static bool FLAGS_histogram = false;

// Number of bytes to buffer in memtable before compacting
// (initialized to default value by "main")
static int FLAGS_write_buffer_size = 0;

// Number of bytes written to each file.
// (initialized to default value by "main")
static int FLAGS_max_file_size = 0;

// Approximate size of user data packed per block (before compression.
// (initialized to default value by "main")
static int FLAGS_block_size = 0;

// Number of bytes to use as a cache of uncompressed data.
// Negative means use default settings.
static int FLAGS_cache_size = -1;

// Maximum number of files to keep open at the same time (use default if == 0)
static int FLAGS_open_files = 0;

// Bloom filter bits per key.
// Negative means use default settings.
static int FLAGS_bloom_bits = 10;

// If true, do not destroy the existing database.  If you set this
// flag and also specify a perf that wants a fresh database, that
// perf will fail.
static bool FLAGS_use_existing_db = false;

// If true, reuse existing log/MANIFEST files when re-opening a database.
static bool FLAGS_reuse_logs = false;

// Use the db with the following name.
static const char* FLAGS_db = NULL;

namespace LzKVStore {

namespace {
LzKVStore::Configuration* g_env = NULL;

// Helper for quickly generating random data.
class RandomGenerator
{
private:
    std::string data_;
    int pos_;

public:
    RandomGenerator()
    {
        // We use a limited amount of data over and over again and ensure
        // that it is larger than the compression window (32KB), and also
        // large enough to serve all typical value sizes we want to write.
        Random rnd(301);
        std::string piece;
        while (data_.size() < 1048576) {
            // Add a short fragment that is as compressible as specified
            // by FLAGS_compression_ratio.
            KVSTest::CompressibleString(&rnd, FLAGS_compression_ratio, 100,
                    &piece);
            data_.append(piece);
        }
        pos_ = 0;
    }

    Dissection Generate(size_t len)
    {
        if (pos_ + len > data_.size()) {
            pos_ = 0;
            assert(len < data_.size());
        }
        pos_ += len;
        return Dissection(data_.data() + pos_ - len, len);
    }
};

#if defined(__linux)
static Dissection TrimSpace(Dissection s)
{
    size_t start = 0;
    while (start < s.size() && isspace(s[start])) {
        start++;
    }
    size_t limit = s.size();
    while (limit > start && isspace(s[limit-1])) {
        limit--;
    }
    return Dissection(s.data() + start, limit - start);
}
#endif

static void AppendWithSpace(std::string* str, Dissection msg)
{
    if (msg.empty())
        return;
    if (!str->empty()) {
        str->push_back(' ');
    }
    str->append(msg.data(), msg.size());
}

class Stats
{
private:
    double start_;
    double finish_;
    double seconds_;
    int done_;
    int next_report_;
    int64_t bytes_;
    double last_op_finish_;
    Plots hist_;
    std::string message_;

public:
    Stats()
    {
        Start();
    }

    void Start()
    {
        next_report_ = 100;
        last_op_finish_ = start_;
        hist_.Clear();
        done_ = 0;
        bytes_ = 0;
        seconds_ = 0;
        start_ = g_env->NowMicros();
        finish_ = start_;
        message_.clear();
    }

    void Merge(const Stats& other)
    {
        hist_.Merge(other.hist_);
        done_ += other.done_;
        bytes_ += other.bytes_;
        seconds_ += other.seconds_;
        if (other.start_ < start_)
            start_ = other.start_;
        if (other.finish_ > finish_)
            finish_ = other.finish_;

        // Just keep the messages from one thread
        if (message_.empty())
            message_ = other.message_;
    }

    void Stop()
    {
        finish_ = g_env->NowMicros();
        seconds_ = (finish_ - start_) * 1e-6;
    }

    void AddMessage(Dissection msg)
    {
        AppendWithSpace(&message_, msg);
    }

    void FinishedSingleOp()
    {
        if (FLAGS_histogram) {
            double now = g_env->NowMicros();
            double micros = now - last_op_finish_;
            hist_.Add(micros);
            if (micros > 20000) {
                fprintf(stderr, "long op: %.1f micros%30s\r", micros, "");
                fflush(stderr);
            }
            last_op_finish_ = now;
        }

        done_++;
        if (done_ >= next_report_) {
            if (next_report_ < 1000)
                next_report_ += 100;
            else if (next_report_ < 5000)
                next_report_ += 500;
            else if (next_report_ < 10000)
                next_report_ += 1000;
            else if (next_report_ < 50000)
                next_report_ += 5000;
            else if (next_report_ < 100000)
                next_report_ += 10000;
            else if (next_report_ < 500000)
                next_report_ += 50000;
            else
                next_report_ += 100000;
            fprintf(stderr, "... finished %d ops%30s\r", done_, "");
            fflush(stderr);
        }
    }

    void AddBytes(int64_t n)
    {
        bytes_ += n;
    }

    void Report(const Dissection& name)
    {
        // Pretend at least one op was done in case we are running a perf
        // that does not call FinishedSingleOp().
        if (done_ < 1)
            done_ = 1;

        std::string extra;
        if (bytes_ > 0) {
            // Rate is computed on actual elapsed time, not the sum of per-thread
            // elapsed times.
            double elapsed = (finish_ - start_) * 1e-6;
            char rate[100];
            snprintf(rate, sizeof(rate), "%6.1f MB/s",
                    (bytes_ / 1048576.0) / elapsed);
            extra = rate;
        }
        AppendWithSpace(&extra, message_);

        fprintf(stdout, "%-12s : %11.3f micros/op;%s%s\n",
                name.ToString().c_str(), seconds_ * 1e6 / done_,
                (extra.empty() ? "" : " "), extra.c_str());
        if (FLAGS_histogram) {
            fprintf(stdout, "Microseconds per op:\n%s\n",
                    hist_.ToString().c_str());
        }
        fflush(stdout);
    }
};

// State shared by all concurrent executions of the same perf.
struct SharedState {
    KVSArch::Mutex mu;
    KVSArch::CondVar cv;
    int total;

    // Each thread goes through the following states:
    //    (1) initializing
    //    (2) waiting for others to be initialized
    //    (3) running
    //    (4) done

    int num_initialized;
    int num_done;
    bool start;

    SharedState() :
            cv(&mu)
    {
    }
};

// Per-thread state for concurrent executions of the same perf.
struct ThreadState {
    int tid;             // 0..n-1 when running in n threads
    Random rand;         // Has different seeds for different threads
    Stats stats;
    SharedState* shared;

    ThreadState(int index) :
            tid(index), rand(1000 + index)
    {
    }
};

}  // namespace

class Perf
{
private:
    Accell* cache_;
    const FilteringPolicy* filter_policy_;
    KVDatabase* kvsd_;
    int num_;
    int value_size_;
    int entries_per_batch_;
    WriteOptions write_options_;
    int reads_;
    int heap_counter_;

    void PrintHeader()
    {
        const int kKeySize = 16;
        PrintConfigurationironment();
        fprintf(stdout, "Keys:       %d bytes each\n", kKeySize);
        fprintf(stdout,
                "Values:     %d bytes each (%d bytes after compression)\n",
                FLAGS_value_size,
                static_cast<int>(FLAGS_value_size * FLAGS_compression_ratio
                        + 0.5));
        fprintf(stdout, "Entries:    %d\n", num_);
        fprintf(stdout, "RawSize:    %.1f MB (estimated)\n",
                ((static_cast<int64_t>(kKeySize + FLAGS_value_size) * num_)
                        / 1048576.0));
        fprintf(stdout, "FileSize:   %.1f MB (estimated)\n",
                (((kKeySize + FLAGS_value_size * FLAGS_compression_ratio) * num_)
                        / 1048576.0));
        PrintWarnings();
        fprintf(stdout, "------------------------------------------------\n");
    }

    void PrintWarnings()
    {
#if defined(__GNUC__) && !defined(__OPTIMIZE__)
        fprintf(stdout,
                "WARNING: Optimization is disabled: perfs unnecessarily slow\n"
        );
#endif
#ifndef NDEBUG
        fprintf(stdout,
                "WARNING: Assertions are enabled; perfs unnecessarily slow\n");
#endif

        // See if snappy is working by attempting to compress a compressible string
        const char text[] = "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy";
        std::string compressed;
        if (!KVSArch::Snappy_Compress(text, sizeof(text), &compressed)) {
            fprintf(stdout, "WARNING: Snappy compression is not enabled\n");
        } else if (compressed.size() >= sizeof(text)) {
            fprintf(stdout, "WARNING: Snappy compression is not effective\n");
        }
    }

    void PrintConfigurationironment()
    {
        fprintf(stderr, "KVDatabase:    version %d.%d\n", kMajorVersion,
                kMinorVersion);

#if defined(__linux)
        time_t now = time(NULL);
        fprintf(stderr, "Date:       %s", ctime(&now));  // ctime() adds newline

        FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
        if (cpuinfo != NULL) {
            char line[1000];
            int num_cpus = 0;
            std::string cpu_type;
            std::string cache_size;
            while (fgets(line, sizeof(line), cpuinfo) != NULL) {
                const char* sep = strchr(line, ':');
                if (sep == NULL) {
                    continue;
                }
                Dissection key = TrimSpace(Dissection(line, sep - 1 - line));
                Dissection val = TrimSpace(Dissection(sep + 1));
                if (key == "model name") {
                    ++num_cpus;
                    cpu_type = val.ToString();
                } else if (key == "cache size") {
                    cache_size = val.ToString();
                }
            }
            fclose(cpuinfo);
            fprintf(stderr, "CPU:        %d * %s\n", num_cpus, cpu_type.c_str());
            fprintf(stderr, "CPUAccell:   %s\n", cache_size.c_str());
        }
#endif
    }

public:
    Perf() :
            cache_(
                    FLAGS_cache_size >= 0 ?
                            NewLRUAccell(FLAGS_cache_size) : NULL), filter_policy_(
                    FLAGS_bloom_bits >= 0 ?
                            NewBloomFilteringPolicy(FLAGS_bloom_bits) : NULL), kvsd_(
                    NULL), num_(FLAGS_num), value_size_(FLAGS_value_size), entries_per_batch_(
                    1), reads_(FLAGS_reads < 0 ? FLAGS_num : FLAGS_reads), heap_counter_(
                    0)
    {
        std::vector < std::string > files;
        g_env->GetChildren(FLAGS_db, &files);
        for (size_t i = 0; i < files.size(); i++) {
            if (Dissection(files[i]).starts_with("heap-")) {
                g_env->DeleteFile(std::string(FLAGS_db) + "/" + files[i]);
            }
        }
        if (!FLAGS_use_existing_db) {
            DestroyKVDatabase(FLAGS_db, OpenOptions());
        }
    }

    ~Perf()
    {
        delete kvsd_;
        delete cache_;
        delete filter_policy_;
    }

    void Run()
    {
        PrintHeader();
        Open();

        const char* perfs = FLAGS_perfs;
        while (perfs != NULL) {
            const char* sep = strchr(perfs, ',');
            Dissection name;
            if (sep == NULL) {
                name = perfs;
                perfs = NULL;
            } else {
                name = Dissection(perfs, sep - perfs);
                perfs = sep + 1;
            }

            // Reset parameters that may be overridden below
            num_ = FLAGS_num;
            reads_ = (FLAGS_reads < 0 ? FLAGS_num : FLAGS_reads);
            value_size_ = FLAGS_value_size;
            entries_per_batch_ = 1;
            write_options_ = WriteOptions();

            void (Perf::*method)(ThreadState*) = NULL;
            bool fresh_db = false;
            int num_threads = FLAGS_threads;

            if (name == Dissection("open")) {
                method = &Perf::OpenBench;
                num_ /= 10000;
                if (num_ < 1)
                    num_ = 1;
            } else if (name == Dissection("writeseq")) {
                fresh_db = true;
                method = &Perf::WriteSeq;
            } else if (name == Dissection("writebatch")) {
                fresh_db = true;
                entries_per_batch_ = 1000;
                method = &Perf::WriteSeq;
            } else if (name == Dissection("writerandom")) {
                fresh_db = true;
                method = &Perf::WriteRandom;
            } else if (name == Dissection("overwrite")) {
                fresh_db = false;
                method = &Perf::WriteRandom;
            } else if (name == Dissection("writesync")) {
                fresh_db = true;
                num_ /= 1000;
                write_options_.sync = true;
                method = &Perf::WriteRandom;
            } else if (name == Dissection("write100K")) {
                fresh_db = true;
                num_ /= 1000;
                value_size_ = 100 * 1000;
                method = &Perf::WriteRandom;
            } else if (name == Dissection("readseq")) {
                method = &Perf::ReadSequential;
            } else if (name == Dissection("readreverse")) {
                method = &Perf::ReadReverse;
            } else if (name == Dissection("readrandom")) {
                method = &Perf::ReadRandom;
            } else if (name == Dissection("readmissing")) {
                method = &Perf::ReadMissing;
            } else if (name == Dissection("seekrandom")) {
                method = &Perf::SeekRandom;
            } else if (name == Dissection("readhot")) {
                method = &Perf::ReadHot;
            } else if (name == Dissection("readrandomsmall")) {
                reads_ /= 1000;
                method = &Perf::ReadRandom;
            } else if (name == Dissection("deleteseq")) {
                method = &Perf::DeleteSeq;
            } else if (name == Dissection("deleterandom")) {
                method = &Perf::DeleteRandom;
            } else if (name == Dissection("readwhilewriting")) {
                num_threads++;  // Add extra thread for writing
                method = &Perf::ReadWhileWriting;
            } else if (name == Dissection("compact")) {
                method = &Perf::Compact;
            } else if (name == Dissection("crc32c")) {
                method = &Perf::Crc32c;
            } else if (name == Dissection("acquireload")) {
                method = &Perf::AcquireLoad;
            } else if (name == Dissection("snappycomp")) {
                method = &Perf::SnappyCompress;
            } else if (name == Dissection("snappyuncomp")) {
                method = &Perf::SnappyUncompress;
            } else if (name == Dissection("heapprofile")) {
                HeapProfile();
            } else if (name == Dissection("stats")) {
                PrintStats("LzKVStore.stats");
            } else if (name == Dissection("mitables")) {
                PrintStats("LzKVStore.mitables");
            } else {
                if (name != Dissection()) {  // No error message for empty name
                    fprintf(stderr, "unknown perf '%s'\n",
                            name.ToString().c_str());
                }
            }

            if (fresh_db) {
                if (FLAGS_use_existing_db) {
                    fprintf(stdout,
                            "%-12s : skipped (--use_existing_db is true)\n",
                            name.ToString().c_str());
                    method = NULL;
                } else {
                    delete kvsd_;
                    kvsd_ = NULL;
                    DestroyKVDatabase(FLAGS_db, OpenOptions());
                    Open();
                }
            }

            if (method != NULL) {
                RunPerf(num_threads, name, method);
            }
        }
    }

private:
    struct ThreadArg {
        Perf* bm;
        SharedState* shared;
        ThreadState* thread;
        void (Perf::*method)(ThreadState*);
    };

    static void ThreadBody(void* v)
    {
        ThreadArg* arg = reinterpret_cast<ThreadArg*>(v);
        SharedState* shared = arg->shared;
        ThreadState* thread = arg->thread;
        {
            MutexLock l(&shared->mu);
            shared->num_initialized++;
            if (shared->num_initialized >= shared->total) {
                shared->cv.SignalAll();
            }
            while (!shared->start) {
                shared->cv.Wait();
            }
        }

        thread->stats.Start();
        (arg->bm->*(arg->method))(thread);
        thread->stats.Stop();

        {
            MutexLock l(&shared->mu);
            shared->num_done++;
            if (shared->num_done >= shared->total) {
                shared->cv.SignalAll();
            }
        }
    }

    void RunPerf(int n, Dissection name, void (Perf::*method)(ThreadState*))
    {
        SharedState shared;
        shared.total = n;
        shared.num_initialized = 0;
        shared.num_done = 0;
        shared.start = false;

        ThreadArg* arg = new ThreadArg[n];
        for (int i = 0; i < n; i++) {
            arg[i].bm = this;
            arg[i].method = method;
            arg[i].shared = &shared;
            arg[i].thread = new ThreadState(i);
            arg[i].thread->shared = &shared;
            g_env->StartThread(ThreadBody, &arg[i]);
        }

        shared.mu.Lock();
        while (shared.num_initialized < n) {
            shared.cv.Wait();
        }

        shared.start = true;
        shared.cv.SignalAll();
        while (shared.num_done < n) {
            shared.cv.Wait();
        }
        shared.mu.Unlock();

        for (int i = 1; i < n; i++) {
            arg[0].thread->stats.Merge(arg[i].thread->stats);
        }
        arg[0].thread->stats.Report(name);

        for (int i = 0; i < n; i++) {
            delete arg[i].thread;
        }
        delete[] arg;
    }

    void Crc32c(ThreadState* thread)
    {
        // Checksum about 500MB of data total
        const int size = 4096;
        const char* label = "(4K per op)";
        std::string data(size, 'x');
        int64_t bytes = 0;
        uint32_t crc = 0;
        while (bytes < 500 * 1048576) {
            crc = crc32c::Value(data.data(), size);
            thread->stats.FinishedSingleOp();
            bytes += size;
        }
        // Print so result is not dead
        fprintf(stderr, "... crc=0x%x\r", static_cast<unsigned int>(crc));

        thread->stats.AddBytes(bytes);
        thread->stats.AddMessage(label);
    }

    void AcquireLoad(ThreadState* thread)
    {
        int dummy;
        KVSArch::AtomicPointer ap(&dummy);
        int count = 0;
        void *ptr = NULL;
        thread->stats.AddMessage("(each op is 1000 loads)");
        while (count < 100000) {
            for (int i = 0; i < 1000; i++) {
                ptr = ap.Acquire_Load();
            }
            count++;
            thread->stats.FinishedSingleOp();
        }
        if (ptr == NULL)
            exit(1); // Disable unused variable warning.
    }

    void SnappyCompress(ThreadState* thread)
    {
        RandomGenerator gen;
        Dissection input = gen.Generate(OpenOptions().block_size);
        int64_t bytes = 0;
        int64_t produced = 0;
        bool ok = true;
        std::string compressed;
        while (ok && bytes < 1024 * 1048576) {  // Compress 1G
            ok = KVSArch::Snappy_Compress(input.data(), input.size(),
                    &compressed);
            produced += compressed.size();
            bytes += input.size();
            thread->stats.FinishedSingleOp();
        }

        if (!ok) {
            thread->stats.AddMessage("(snappy failure)");
        } else {
            char buf[100];
            snprintf(buf, sizeof(buf), "(output: %.1f%%)",
                    (produced * 100.0) / bytes);
            thread->stats.AddMessage(buf);
            thread->stats.AddBytes(bytes);
        }
    }

    void SnappyUncompress(ThreadState* thread)
    {
        RandomGenerator gen;
        Dissection input = gen.Generate(OpenOptions().block_size);
        std::string compressed;
        bool ok = KVSArch::Snappy_Compress(input.data(), input.size(),
                &compressed);
        int64_t bytes = 0;
        char* uncompressed = new char[input.size()];
        while (ok && bytes < 1024 * 1048576) {  // Compress 1G
            ok = KVSArch::Snappy_Uncompress(compressed.data(),
                    compressed.size(), uncompressed);
            bytes += input.size();
            thread->stats.FinishedSingleOp();
        }
        delete[] uncompressed;

        if (!ok) {
            thread->stats.AddMessage("(snappy failure)");
        } else {
            thread->stats.AddBytes(bytes);
        }
    }

    void Open()
    {
        assert(kvsd_ == NULL);
        OpenOptions options;
        options.env = g_env;
        options.create_if_missing = !FLAGS_use_existing_db;
        options.block_cache = cache_;
        options.write_buffer_size = FLAGS_write_buffer_size;
        options.max_file_size = FLAGS_max_file_size;
        options.block_size = FLAGS_block_size;
        options.max_open_files = FLAGS_open_files;
        options.filter_policy = filter_policy_;
        options.reuse_logs = FLAGS_reuse_logs;
        //options.paranoid_checks = true;
        Status s = KVDatabase::Open(options, FLAGS_db, &kvsd_);
        if (!s.ok()) {
            fprintf(stderr, "open error: %s\n", s.ToString().c_str());
            exit(1);
        }
    }

    void OpenBench(ThreadState* thread)
    {
        for (int i = 0; i < num_; i++) {
            delete kvsd_;
            Open();
            thread->stats.FinishedSingleOp();
        }
    }

    void WriteSeq(ThreadState* thread)
    {
        DoWrite(thread, true);
    }

    void WriteRandom(ThreadState* thread)
    {
        DoWrite(thread, false);
    }

    void DoWrite(ThreadState* thread, bool seq)
    {
        if (num_ != FLAGS_num) {
            char msg[100];
            snprintf(msg, sizeof(msg), "(%d ops)", num_);
            thread->stats.AddMessage(msg);
        }

        RandomGenerator gen;
        SaveBatch batch;
        Status s;
        int64_t bytes = 0;
        for (int i = 0; i < num_; i += entries_per_batch_) {
            batch.Clear();
            for (int j = 0; j < entries_per_batch_; j++) {
                const int k = seq ? i + j : (thread->rand.Next() % FLAGS_num);
                char key[100];
                snprintf(key, sizeof(key), "%016d", k);
                //fprintf(stdout, "%d %d %s\n", i, j, key);
                batch.Put(key, gen.Generate(value_size_));
                bytes += value_size_ + strlen(key);
                thread->stats.FinishedSingleOp();
            }
            s = kvsd_->Write(write_options_, &batch);
            if (!s.ok()) {
                fprintf(stderr, "put error: %s\n", s.ToString().c_str());
                exit(1);
            }
        }
        thread->stats.AddBytes(bytes);
    }

    void ReadSequential(ThreadState* thread)
    {
        Iterator* iter = kvsd_->NewIterator(ReadOptions());
        int i = 0;
        int64_t bytes = 0;
        for (iter->SeekToFirst(); i < reads_ && iter->Valid(); iter->Next()) {
            bytes += iter->key().size() + iter->value().size();
            thread->stats.FinishedSingleOp();
            ++i;
        }
        delete iter;
        thread->stats.AddBytes(bytes);
    }

    void ReadReverse(ThreadState* thread)
    {
        Iterator* iter = kvsd_->NewIterator(ReadOptions());
        int i = 0;
        int64_t bytes = 0;
        for (iter->SeekToLast(); i < reads_ && iter->Valid(); iter->Prev()) {
            bytes += iter->key().size() + iter->value().size();
            thread->stats.FinishedSingleOp();
            ++i;
        }
        delete iter;
        thread->stats.AddBytes(bytes);
    }

    void ReadRandom(ThreadState* thread)
    {
        ReadOptions options;
        std::string value;
        int found = 0;
        for (int i = 0; i < reads_; i++) {
            char key[100];
            const int k = thread->rand.Next() % FLAGS_num;
            snprintf(key, sizeof(key), "%016d", k);
            if (kvsd_->Get(options, key, &value).ok()) {
                found++;
            }
            thread->stats.FinishedSingleOp();
        }
        char msg[100];
        snprintf(msg, sizeof(msg), "(%d of %d found)", found, num_);
        thread->stats.AddMessage(msg);
    }

    void ReadMissing(ThreadState* thread)
    {
        ReadOptions options;
        std::string value;
        for (int i = 0; i < reads_; i++) {
            char key[100];
            const int k = thread->rand.Next() % FLAGS_num;
            snprintf(key, sizeof(key), "%016d.", k);
            kvsd_->Get(options, key, &value);
            thread->stats.FinishedSingleOp();
        }
    }

    void ReadHot(ThreadState* thread)
    {
        ReadOptions options;
        std::string value;
        const int range = (FLAGS_num + 99) / 100;
        for (int i = 0; i < reads_; i++) {
            char key[100];
            const int k = thread->rand.Next() % range;
            snprintf(key, sizeof(key), "%016d", k);
            kvsd_->Get(options, key, &value);
            thread->stats.FinishedSingleOp();
        }
    }

    void SeekRandom(ThreadState* thread)
    {
        ReadOptions options;
        int found = 0;
        for (int i = 0; i < reads_; i++) {
            Iterator* iter = kvsd_->NewIterator(options);
            char key[100];
            const int k = thread->rand.Next() % FLAGS_num;
            snprintf(key, sizeof(key), "%016d", k);
            iter->Seek(key);
            if (iter->Valid() && iter->key() == key)
                found++;
            delete iter;
            thread->stats.FinishedSingleOp();
        }
        char msg[100];
        snprintf(msg, sizeof(msg), "(%d of %d found)", found, num_);
        thread->stats.AddMessage(msg);
    }

    void DoDelete(ThreadState* thread, bool seq)
    {
        RandomGenerator gen;
        SaveBatch batch;
        Status s;
        for (int i = 0; i < num_; i += entries_per_batch_) {
            batch.Clear();
            for (int j = 0; j < entries_per_batch_; j++) {
                const int k = seq ? i + j : (thread->rand.Next() % FLAGS_num);
                char key[100];
                snprintf(key, sizeof(key), "%016d", k);
                batch.Delete(key);
                thread->stats.FinishedSingleOp();
            }
            s = kvsd_->Write(write_options_, &batch);
            if (!s.ok()) {
                fprintf(stderr, "del error: %s\n", s.ToString().c_str());
                exit(1);
            }
        }
    }

    void DeleteSeq(ThreadState* thread)
    {
        DoDelete(thread, true);
    }

    void DeleteRandom(ThreadState* thread)
    {
        DoDelete(thread, false);
    }

    void ReadWhileWriting(ThreadState* thread)
    {
        if (thread->tid > 0) {
            ReadRandom(thread);
        } else {
            // Special thread that keeps writing until other threads are done.
            RandomGenerator gen;
            while (true) {
                {
                    MutexLock l(&thread->shared->mu);
                    if (thread->shared->num_done + 1
                            >= thread->shared->num_initialized) {
                        // Other threads have finished
                        break;
                    }
                }

                const int k = thread->rand.Next() % FLAGS_num;
                char key[100];
                snprintf(key, sizeof(key), "%016d", k);
                Status s = kvsd_->Put(write_options_, key,
                        gen.Generate(value_size_));
                if (!s.ok()) {
                    fprintf(stderr, "put error: %s\n", s.ToString().c_str());
                    exit(1);
                }
            }

            // Do not count any of the preceding work/delay in stats.
            thread->stats.Start();
        }
    }

    void Compact(ThreadState* thread)
    {
        kvsd_->CompactRange(NULL, NULL);
    }

    void PrintStats(const char* key)
    {
        std::string stats;
        if (!kvsd_->GetProperty(key, &stats)) {
            stats = "(failed)";
        }
        fprintf(stdout, "\n%s\n", stats.c_str());
    }

    static void WriteToFile(void* arg, const char* buf, int n)
    {
        reinterpret_cast<ModDatabase*>(arg)->Append(Dissection(buf, n));
    }

    void HeapProfile()
    {
        char fname[100];
        snprintf(fname, sizeof(fname), "%s/heap-%04d", FLAGS_db,
                ++heap_counter_);
        ModDatabase* file;
        Status s = g_env->NewModDatabase(fname, &file);
        if (!s.ok()) {
            fprintf(stderr, "%s\n", s.ToString().c_str());
            return;
        }
        bool ok = KVSArch::GetHeapProfile(WriteToFile, file);
        delete file;
        if (!ok) {
            fprintf(stderr, "heap profiling not supported\n");
            g_env->DeleteFile(fname);
        }
    }
};

}  // namespace LzKVStore

int main(int argc, char** argv)
{
    FLAGS_write_buffer_size = LzKVStore::OpenOptions().write_buffer_size;
    FLAGS_max_file_size = LzKVStore::OpenOptions().max_file_size;
    FLAGS_block_size = LzKVStore::OpenOptions().block_size;
    FLAGS_open_files = LzKVStore::OpenOptions().max_open_files;
    std::string default_kvsd_path;

    for (int i = 1; i < argc; i++) {
        double d;
        int n;
        char junk;
        if (LzKVStore::Dissection(argv[i]).starts_with("--perfs=")) {
            FLAGS_perfs = argv[i] + strlen("--perfs=");
        } else if (sscanf(argv[i], "--compression_ratio=%lf%c", &d, &junk)
                == 1) {
            FLAGS_compression_ratio = d;
        } else if (sscanf(argv[i], "--histogram=%d%c", &n, &junk) == 1
                && (n == 0 || n == 1)) {
            FLAGS_histogram = n;
        } else if (sscanf(argv[i], "--use_existing_db=%d%c", &n, &junk) == 1
                && (n == 0 || n == 1)) {
            FLAGS_use_existing_db = n;
        } else if (sscanf(argv[i], "--reuse_logs=%d%c", &n, &junk) == 1
                && (n == 0 || n == 1)) {
            FLAGS_reuse_logs = n;
        } else if (sscanf(argv[i], "--num=%d%c", &n, &junk) == 1) {
            FLAGS_num = n;
        } else if (sscanf(argv[i], "--reads=%d%c", &n, &junk) == 1) {
            FLAGS_reads = n;
        } else if (sscanf(argv[i], "--threads=%d%c", &n, &junk) == 1) {
            FLAGS_threads = n;
        } else if (sscanf(argv[i], "--value_size=%d%c", &n, &junk) == 1) {
            FLAGS_value_size = n;
        } else if (sscanf(argv[i], "--write_buffer_size=%d%c", &n, &junk)
                == 1) {
            FLAGS_write_buffer_size = n;
        } else if (sscanf(argv[i], "--max_file_size=%d%c", &n, &junk) == 1) {
            FLAGS_max_file_size = n;
        } else if (sscanf(argv[i], "--block_size=%d%c", &n, &junk) == 1) {
            FLAGS_block_size = n;
        } else if (sscanf(argv[i], "--cache_size=%d%c", &n, &junk) == 1) {
            FLAGS_cache_size = n;
        } else if (sscanf(argv[i], "--bloom_bits=%d%c", &n, &junk) == 1) {
            FLAGS_bloom_bits = n;
        } else if (sscanf(argv[i], "--open_files=%d%c", &n, &junk) == 1) {
            FLAGS_open_files = n;
        } else if (strncmp(argv[i], "--db=", 5) == 0) {
            FLAGS_db = argv[i] + 5;
        } else {
            fprintf(stderr, "Invalid flag '%s'\n", argv[i]);
            exit(1);
        }
    }

    LzKVStore::g_env = LzKVStore::Configuration::Default();

    // Choose a location for the test database if none given with --db=<path>
    if (FLAGS_db == NULL) {
        LzKVStore::g_env->GetTestDirectory(&default_kvsd_path);
        default_kvsd_path += "/kvsperf";
        FLAGS_db = default_kvsd_path.c_str();
    }

    LzKVStore::Perf perf;
    perf.Run();
    return 0;
}
