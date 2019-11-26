#ifndef SOE_KVS_SRC_CONFIGURE_H_
#define SOE_KVS_SRC_CONFIGURE_H_

#include <string>
#include <vector>
#include <stdarg.h>
#include <cstdint>
#include "status.hpp"

using namespace std;

namespace LzKVStore {

class DatabaseLock;
class Messanger;
class RADatabase;
class SeqDatabase;
class Dissection;
class ModDatabase;

class Configuration
{
public:
    Configuration()
    {
    }
    virtual ~Configuration();

    // Return a default environment suitable for the current operating
    // system.  Sophisticated users may wish to provide their own Configuration
    // implementation instead of relying on this default environment.
    //
    // The result of Default() belongs to LzKVStore and must never be deleted.
    static Configuration* Default();

    // Create a brand new sequentially-readable file with the specified name.
    // On success, stores a pointer to the new file in *result and returns OK.
    // On failure stores NULL in *result and returns non-OK.  If the file does
    // not exist, returns a non-OK status.
    //
    // The returned file will only be accessed by one thread at a time.
    virtual Status NewSeqDatabase(const std::string& fname,
            SeqDatabase** result) = 0;

    // Create a brand new random access read-only file with the
    // specified name.  On success, stores a pointer to the new file in
    // *result and returns OK.  On failure stores NULL in *result and
    // returns non-OK.  If the file does not exist, returns a non-OK
    // status.
    //
    // The returned file may be concurrently accessed by multiple threads.
    virtual Status NewRADatabase(const std::string& fname,
            RADatabase** result) = 0;

    // Create an object that writes to a new file with the specified
    // name.  Deletes any existing file with the same name and creates a
    // new file.  On success, stores a pointer to the new file in
    // *result and returns OK.  On failure stores NULL in *result and
    // returns non-OK.
    //
    // The returned file will only be accessed by one thread at a time.
    virtual Status NewModDatabase(const std::string& fname,
            ModDatabase** result) = 0;

    // Create an object that either appends to an existing file, or
    // writes to a new file (if the file does not exist to begin with).
    // On success, stores a pointer to the new file in *result and
    // returns OK.  On failure stores NULL in *result and returns
    // non-OK.
    //
    // The returned file will only be accessed by one thread at a time.
    //
    // May return an IsNotSupportedError error if this Configuration does
    // not allow appending to an existing file.  Users of Configuration (including
    // the LzKVStore implementation) must be prepared to deal with
    // an Configuration that does not support appending.
    virtual Status NewAppendableFile(const std::string& fname,
            ModDatabase** result);

    // Returns true iff the named file exists.
    virtual bool FileExists(const std::string& fname) = 0;

    // Store in *result the names of the children of the specified directory.
    // The names are relative to "dir".
    // Original contents of *results are dropped.
    virtual Status GetChildren(const std::string& dir,
            std::vector<std::string>* result) = 0;

    // Delete the named file.
    virtual Status DeleteFile(const std::string& fname) = 0;

    // Create the specified directory.
    virtual Status CreateDir(const std::string& dirname) = 0;

    // Delete the specified directory.
    virtual Status DeleteDir(const std::string& dirname) = 0;

    // Store the size of fname in *file_size.
    virtual Status GetFileSize(const std::string& fname,
            uint64_t* file_size) = 0;

    // Rename file src to target.
    virtual Status RenameFile(const std::string& src,
            const std::string& target) = 0;

    // Lock the specified file.  Used to prevent concurrent access to
    // the same db by multiple processes.  On failure, stores NULL in
    // *lock and returns non-OK.
    //
    // On success, stores a pointer to the object that represents the
    // acquired lock in *lock and returns OK.  The caller should call
    // UnlockFile(*lock) to release the lock.  If the process exits,
    // the lock will be automatically released.
    //
    // If somebody else already holds the lock, finishes immediately
    // with a failure.  I.e., this call does not wait for existing locks
    // to go away.
    //
    // May create the named file if it does not already exist.
    virtual Status LockFile(const std::string& fname, DatabaseLock** lock) = 0;

    // Release the lock acquired by a previous successful call to LockFile.
    // REQUIRES: lock was returned by a successful LockFile() call
    // REQUIRES: lock has not already been unlocked.
    virtual Status UnlockFile(DatabaseLock* lock) = 0;

    // Arrange to run "(*function)(arg)" once in a background thread.
    //
    // "function" may run in an unspecified thread.  Multiple functions
    // added to the same Configuration may run concurrently in different threads.
    // I.e., the caller may not assume that background work items are
    // serialized.
    virtual void Schedule(void (*function)(void* arg), void* arg) = 0;

    // Start a new thread, invoking "function(arg)" within the new thread.
    // When "function(arg)" returns, the thread will be destroyed.
    virtual void StartThread(void (*function)(void* arg), void* arg) = 0;

    // *path is set to a temporary directory that can be used for testing. It may
    // or many not have just been created. The directory may or may not differ
    // between runs of the same process, but subsequent calls will return the
    // same directory.
    virtual Status GetTestDirectory(std::string* path) = 0;

    // Create and return a log file for storing informational messages.
    virtual Status NewMessanger(const std::string& fname,
            Messanger** result) = 0;

    // Returns the number of micro-seconds since some fixed point in time. Only
    // useful for computing deltas of time.
    virtual uint64_t NowMicros() = 0;

    // Sleep/delay the thread for the prescribed number of micro-seconds.
    virtual void SleepForMicroseconds(int micros) = 0;

private:
    // No copying allowed
    Configuration(const Configuration&);
    void operator=(const Configuration&);
};

// A file abstraction for reading sequentially through a file
class SeqDatabase
{
public:
    SeqDatabase()
    {
    }
    virtual ~SeqDatabase();

    // Read up to "n" bytes from the file.  "scratch[0..n-1]" may be
    // written by this routine.  Sets "*result" to the data that was
    // read (including if fewer than "n" bytes were successfully read).
    // May set "*result" to point at data in "scratch[0..n-1]", so
    // "scratch[0..n-1]" must be live when "*result" is used.
    // If an error was encountered, returns a non-OK status.
    //
    // REQUIRES: External synchronization
    virtual Status Read(size_t n, Dissection* result, char* scratch) = 0;

    // Skip "n" bytes from the file. This is guaranteed to be no
    // slower that reading the same data, but may be faster.
    //
    // If end of file is reached, skipping will stop at the end of the
    // file, and Skip will return OK.
    //
    // REQUIRES: External synchronization
    virtual Status Skip(uint64_t n) = 0;

private:
    // No copying allowed
    SeqDatabase(const SeqDatabase&);
    void operator=(const SeqDatabase&);
};

// A file abstraction for randomly reading the contents of a file.
class RADatabase
{
public:
    RADatabase()
    {
    }
    virtual ~RADatabase();

    // Read up to "n" bytes from the file starting at "offset".
    // "scratch[0..n-1]" may be written by this routine.  Sets "*result"
    // to the data that was read (including if fewer than "n" bytes were
    // successfully read).  May set "*result" to point at data in
    // "scratch[0..n-1]", so "scratch[0..n-1]" must be live when
    // "*result" is used.  If an error was encountered, returns a non-OK
    // status.
    //
    // Safe for concurrent use by multiple threads.
    virtual Status Read(uint64_t offset, size_t n, Dissection* result,
            char* scratch) const = 0;

private:
    // No copying allowed
    RADatabase(const RADatabase&);
    void operator=(const RADatabase&);
};

// A file abstraction for sequential writing.  The implementation
// must provide buffering since callers may append small fragments
// at a time to the file.
class ModDatabase
{
public:
    ModDatabase()
    {
    }
    virtual ~ModDatabase();

    virtual Status Append(const Dissection& data) = 0;
    virtual Status Close() = 0;
    virtual Status Flush() = 0;
    virtual Status Sync() = 0;

private:
    // No copying allowed
    ModDatabase(const ModDatabase&);
    void operator=(const ModDatabase&);
};

// An interface for writing log messages.
class Messanger {
public:
    Messanger() {
    }
    virtual ~Messanger();

    // Write an entry to the log file with the specified format.
    virtual void Logv(const char* format, va_list ap) = 0;

private:
    // No copying allowed
    Messanger(const Messanger&);
    void operator=(const Messanger&);
};

// Identifies a locked file.
class DatabaseLock
{
public:
    DatabaseLock()
    {
    }
    virtual ~DatabaseLock();
private:
    // No copying allowed
    DatabaseLock(const DatabaseLock&);
    void operator=(const DatabaseLock&);
};

// Log the specified data to *info_log if info_log is non-NULL.
extern void Log(Messanger* info_log, const char* format, ...)
#   if defined(__GNUC__) || defined(__clang__)
        __attribute__((__format__ (__printf__, 2, 3)))
#   endif
        ;

// A utility routine: write "data" to the named file.
extern Status WriteStringToFile(Configuration* env, const Dissection& data,
        const std::string& fname);

// A utility routine: read contents of named file into *data
extern Status ReadFileToString(Configuration* env, const std::string& fname,
        std::string* data);

// An implementation of Configuration that forwards all calls to another Configuration.
// May be useful to clients who wish to override just part of the
// functionality of another Configuration.
class ConfigurationWrapper: public Configuration
{
public:
    // Initialize an ConfigurationWrapper that delegates all calls to *t
    explicit ConfigurationWrapper(Configuration* t) :
            target_(t)
    {
    }
    virtual ~ConfigurationWrapper();

    // Return the target to which this Configuration forwards all calls
    Configuration* target() const
    {
        return target_;
    }

    // The following text is boilerplate that forwards all methods to target()
    Status NewSeqDatabase(const std::string& f, SeqDatabase** r)
    {
        return target_->NewSeqDatabase(f, r);
    }
    Status NewRADatabase(const std::string& f, RADatabase** r)
    {
        return target_->NewRADatabase(f, r);
    }
    Status NewModDatabase(const std::string& f, ModDatabase** r)
    {
        return target_->NewModDatabase(f, r);
    }
    Status NewAppendableFile(const std::string& f, ModDatabase** r)
    {
        return target_->NewAppendableFile(f, r);
    }
    bool FileExists(const std::string& f) {
        return target_->FileExists(f);
    }
    Status GetChildren(const std::string& dir, std::vector<std::string>* r)
    {
        return target_->GetChildren(dir, r);
    }
    Status DeleteFile(const std::string& f)
    {
        return target_->DeleteFile(f);
    }
    Status CreateDir(const std::string& d)
    {
        return target_->CreateDir(d);
    }
    Status DeleteDir(const std::string& d)
    {
        return target_->DeleteDir(d);
    }
    Status GetFileSize(const std::string& f, uint64_t* s)
    {
        return target_->GetFileSize(f, s);
    }
    Status RenameFile(const std::string& s, const std::string& t)
    {
        return target_->RenameFile(s, t);
    }
    Status LockFile(const std::string& f, DatabaseLock** l)
    {
        return target_->LockFile(f, l);
    }
    Status UnlockFile(DatabaseLock* l)
    {
        return target_->UnlockFile(l);
    }
    void Schedule(void (*f)(void*), void* a)
    {
        return target_->Schedule(f, a);
    }
    void StartThread(void (*f)(void*), void* a)
    {
        return target_->StartThread(f, a);
    }
    virtual Status GetTestDirectory(std::string* path)
    {
        return target_->GetTestDirectory(path);
    }
    virtual Status NewMessanger(const std::string& fname, Messanger** result)
    {
        return target_->NewMessanger(fname, result);
    }
    uint64_t NowMicros()
    {
        return target_->NowMicros();
    }
    void SleepForMicroseconds(int micros)
    {
        target_->SleepForMicroseconds(micros);
    }
private:
    Configuration* target_;
};

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_CONFIGURE_H_
