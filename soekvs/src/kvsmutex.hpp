#ifndef SOE_KVS_SRC_KVS_MUTEX_H_
#define SOE_KVS_SRC_KVS_MUTEX_H_

#include "kvsarch.hpp"
#include "kvsthread.hpp"

namespace LzKVStore {

// Helper class that locks a mutex on construction and unlocks the mutex when
// the destructor of the MutexLock object is invoked.
//
// Typical usage:
//
//   void MyClass::MyMethod() {
//     MutexLock l(&mu_);       // mu_ is an instance variable
//     ... some complex code, possibly with multiple return paths ...
//   }

class SCOPED_LOCKABLE MutexLock
{
public:
    explicit MutexLock(KVSArch::Mutex *mu) EXCLUSIVE_LOCK_FUNCTION(mu)
    : mu_(mu)
    {
        this->mu_->Lock();
    }
    ~MutexLock() UNLOCK_FUNCTION() {this->mu_->Unlock();}

private:
    KVSArch::Mutex *const mu_;
    // No copying allowed
    MutexLock(const MutexLock&);
    void operator=(const MutexLock&);
};

}
  // namespace LzKVStore

#endif  // SOE_KVS_SRC_KVS_MUTEX_H_
