#ifndef SOE_KVS_SRC_KVSCONCAT_H_
#define SOE_KVS_SRC_KVSCONCAT_H_

namespace LzKVStore {

class Equalcomp;
class Iterator;

extern Iterator* NewMergingIterator(const Equalcomp* comparator,
        Iterator** children, int n);

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_KVSCONCAT_H_
