#ifndef SOE_KVS_SRC_ITERATOR2L_H_
#define SOE_KVS_SRC_ITERATOR2L_H_

#include "iterator.hpp"

namespace LzKVStore {

struct ReadOptions;

extern Iterator* NewTwoLevelIterator(Iterator* index_iter,
        Iterator* (*block_function)(void* arg, const ReadOptions& options,
                const Dissection& index_value), void* arg,
        const ReadOptions& options);

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_ITERATOR2L_H_
