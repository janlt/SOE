#ifndef SOE_KVS_SRC_KVSARCH_LINUX_H_
#define SOE_KVS_SRC_KVSARCH_LINUX_H_

#include <endian.h>

#include <pthread.h>
#ifdef SNAPPY
#include <snappy.h>
#endif
#include <cstdint>
#include <string>
#include "atomic_pointer.hpp"

#ifndef PLATFORM_IS_LITTLE_ENDIAN
#define PLATFORM_IS_LITTLE_ENDIAN (__BYTE_ORDER == __LITTLE_ENDIAN)
#endif

#if defined(OS_MACOSX) || defined(OS_SOLARIS) || defined(OS_FREEBSD) ||\
    defined(OS_NETBSD) || defined(OS_OPENBSD) || defined(OS_DRAGONFLYBSD) ||\
    defined(OS_ANDROID) || defined(OS_HPUX) || defined(CYGWIN)
// Use fread/fwrite/fflush on platforms without _unlocked variants
#define fread_unlocked fread
#define fwrite_unlocked fwrite
#define fflush_unlocked fflush
#endif

#if defined(OS_MACOSX) || defined(OS_FREEBSD) ||\
    defined(OS_OPENBSD) || defined(OS_DRAGONFLYBSD)
// Use fsync() on platforms without fdatasync()
#define fdatasync fsync
#endif

#if defined(OS_ANDROID) && __ANDROID_API__ < 9
// fdatasync() was only introduced in API level 9 on Android. Use fsync()
// when targetting older platforms.
#define fdatasync fsync
#endif

namespace LzKVStore {
namespace KVSArch {

static const bool kLittleEndian = PLATFORM_IS_LITTLE_ENDIAN;
#undef PLATFORM_IS_LITTLE_ENDIAN

class CondVar;

class Mutex
{
public:
    Mutex();
    ~Mutex();

    void Lock();
    void Unlock();
    void AssertHeld()
    {
    }

private:
    friend class CondVar;
    pthread_mutex_t mu_;

    // No copying
    Mutex(const Mutex&);
    void operator=(const Mutex&);
};

class CondVar
{
public:
    explicit CondVar(Mutex* mu);
    ~CondVar();
    void Wait();
    void Signal();
    void SignalAll();
private:
    pthread_cond_t cv_;
    Mutex* mu_;
};

typedef pthread_once_t OnceType;
#define LZKVSTORE_ONCE_INIT PTHREAD_ONCE_INIT
extern void InitOnce(OnceType* once, void (*initializer)());

inline bool Snappy_Compress(const char* input, size_t length,
        ::std::string* output)
{
#ifdef SNAPPY
    output->resize(snappy::MaxCompressedLength(length));
    size_t outlen;
    snappy::RawCompress(input, length, &(*output)[0], &outlen);
    output->resize(outlen);
    return true;
#endif

    return false;
}

inline bool Snappy_GetUncompressedLength(const char* input, size_t length,
        size_t* result)
{
#ifdef SNAPPY
    return snappy::GetUncompressedLength(input, length, result);
#else
    return false;
#endif
}

inline bool Snappy_Uncompress(const char* input, size_t length, char* output)
{
#ifdef SNAPPY
    return snappy::RawUncompress(input, length, output);
#else
    return false;
#endif
}

inline bool GetHeapProfile(void (*func)(void*, const char*, int), void* arg)
{
    return false;
}

} // namespace KVSArch
} // namespace LzKVStore

#endif  // SOE_KVS_SRC_KVSARCH_LINUX_H_
