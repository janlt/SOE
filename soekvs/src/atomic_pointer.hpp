#ifndef SOE_KVS_SRC_ATOMIC_POINTER_H_
#define SOE_KVS_SRC_ATOMIC_POINTER_H_

#include <cstdint>
#ifdef LZKVSTORE_ATOMIC_PRESENT
#include <atomic>
#endif

#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_CPU_X86_FAMILY 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
#define ARCH_CPU_X86_FAMILY 1
#endif

namespace LzKVStore {
namespace KVSArch {

// This is a special implementation of atomic pointer.
// It may later on be made compliant with LZ standard practices
// Gcc on x86
#if defined(ARCH_CPU_X86_FAMILY) && defined(__GNUC__)
inline void MemoryBarrier() {
  // http://gcc.gnu.org/ml/gcc/2003-04/msg01180.html info for information on
  // this idiom.
  __asm__ __volatile__("" : : : "memory");
}
#define LZKVSTORE_HAVE_MEMORY_BARRIER

// Sun Studio
#elif defined(ARCH_CPU_X86_FAMILY) && defined(__SUNPRO_CC)
inline void MemoryBarrier() {
  // http://gcc.gnu.org/ml/gcc/2003-04/msg01180.html info for information on
  // this idiom.
  asm volatile("" : : : "memory");
}
#define LZKVSTORE_HAVE_MEMORY_BARRIER

// ARM Linux
#elif defined(ARCH_CPU_ARM_FAMILY) && defined(__linux__)
typedef void (*LinuxKernelMemoryBarrierFunc)(void);
// The Linux ARM kernel provides a highly optimized device-specific memory
// barrier function at a fixed memory address that is mapped in every
// user-level process.
//
// This beats using CPU-specific instructions which are, on single-core
// devices, un-necessary and very costly (e.g. ARMv7-A "dmb" takes more
// than 180ns on a Cortex-A8 like the one on a Nexus One). Benchmarking
// shows that the extra function call cost is completely negligible on
// multi-core devices.
//
inline void MemoryBarrier() {
  (*(LinuxKernelMemoryBarrierFunc)0xffff0fa0)();
}
#define LZKVSTORE_HAVE_MEMORY_BARRIER

// ARM64
#elif defined(ARCH_CPU_ARM64_FAMILY)
inline void MemoryBarrier() {
  asm volatile("dmb sy" : : : "memory");
}
#define LZKVSTORE_HAVE_MEMORY_BARRIER

#endif

// AtomicPointer built using platform-specific MemoryBarrier()
#if defined(LZKVSTORE_HAVE_MEMORY_BARRIER)
class AtomicPointer {
 private:
  void* rep_;
 public:
  AtomicPointer() { }
  explicit AtomicPointer(void* p) : rep_(p) {}
  inline void* NoBarrier_Load() const { return rep_; }
  inline void NoBarrier_Store(void* v) { rep_ = v; }
  inline void* Acquire_Load() const {
    void* result = rep_;
    MemoryBarrier();
    return result;
  }
  inline void Release_Store(void* v) {
    MemoryBarrier();
    rep_ = v;
  }
};

// AtomicPointer based on <cstdatomic>
#elif defined(LZKVSTORE_ATOMIC_PRESENT)
class AtomicPointer {
 private:
  std::atomic<void*> rep_;
 public:
  AtomicPointer() { }
  explicit AtomicPointer(void* v) : rep_(v) { }
  inline void* Acquire_Load() const {
    return rep_.load(std::memory_order_acquire);
  }
  inline void Release_Store(void* v) {
    rep_.store(v, std::memory_order_release);
  }
  inline void* NoBarrier_Load() const {
    return rep_.load(std::memory_order_relaxed);
  }
  inline void NoBarrier_Store(void* v) {
    rep_.store(v, std::memory_order_relaxed);
  }
};

// We have neither MemoryBarrier(), nor <atomic>
#else
#error Please implement AtomicPointer for this platform.

#endif

#undef LZKVSTORE_HAVE_MEMORY_BARRIER
#undef ARCH_CPU_X86_FAMILY

}  // namespace KVSArch
}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_ATOMIC_POINTER_H_
