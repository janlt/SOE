#ifndef SOE_KVS_SRC_ITERATOR_CON_H_
#define SOE_KVS_SRC_ITERATOR_CON_H_

#include "iterator.hpp"
#include "dissect.hpp"

namespace LzKVStore {

class IteratorContainer
{
public:
    IteratorContainer() :
            iter_(NULL), valid_(false)
    {
    }
    explicit IteratorContainer(Iterator* iter) :
            iter_(NULL)
    {
        Set(iter);
    }
    ~IteratorContainer()
    {
        delete iter_;
    }
    Iterator* iter() const
    {
        return iter_;
    }

    // Takes ownership of "iter" and will delete it when destroyed, or
    // when Set() is invoked again.
    void Set(Iterator* iter)
    {
        delete iter_;
        iter_ = iter;
        if (iter_ == NULL) {
            valid_ = false;
        } else {
            Update();
        }
    }

    // Iterator interface methods
    bool Valid() const
    {
        return valid_;
    }
    Dissection key() const
    {
        assert(Valid());
        return key_;
    }
    Dissection value() const
    {
        assert(Valid());
        return iter_->value();
    }
    // Methods below require iter() != NULL
    Status status() const
    {
        assert(iter_);
        return iter_->status();
    }
    void Next()
    {
        assert(iter_);
        iter_->Next();
        Update();
    }
    void Prev()
    {
        assert(iter_);
        iter_->Prev();
        Update();
    }
    void Seek(const Dissection& k)
    {
        assert(iter_);
        iter_->Seek(k);
        Update();
    }
    void SeekToFirst()
    {
        assert(iter_);
        iter_->SeekToFirst();
        Update();
    }
    void SeekToLast()
    {
        assert(iter_);
        iter_->SeekToLast();
        Update();
    }

private:
    void Update()
    {
        valid_ = iter_->Valid();
        if (valid_) {
            key_ = iter_->key();
        }
    }

    Iterator* iter_;
    bool valid_;
    Dissection key_;
};

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_ITERATOR_CON_H_
