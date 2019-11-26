#ifndef SOE_KVS_SRC_IMAGE_LIST_H_
#define SOE_KVS_SRC_IMAGE_LIST_H_

#include "kvconfig.hpp"
#include "image.hpp"

namespace LzKVStore {

class ImageList;

class ImageImpl: public Image
{
public:
    SequenceNumber number_;  // const after creation

private:
    friend class ImageList;

    // ImageImpl is kept in a doubly-linked circular list
    ImageImpl* prev_;
    ImageImpl* next_;

    ImageList* list_;                 // just for sanity checks
};

class ImageList
{
public:
    ImageList()
    {
        list_.prev_ = &list_;
        list_.next_ = &list_;
    }

    bool empty() const
    {
        return list_.next_ == &list_;
    }
    ImageImpl* oldest() const
    {
        assert(!empty());
        return list_.next_;
    }
    ImageImpl* newest() const
    {
        assert(!empty());
        return list_.prev_;
    }

    const ImageImpl* New(SequenceNumber seq)
    {
        ImageImpl* s = new ImageImpl;
        s->number_ = seq;
        s->list_ = this;
        s->next_ = &list_;
        s->prev_ = list_.prev_;
        s->prev_->next_ = s;
        s->next_->prev_ = s;
        return s;
    }

    void Delete(const ImageImpl* s)
    {
        assert(s->list_ == this);
        s->prev_->next_ = s->next_;
        s->next_->prev_ = s->prev_;
        delete s;
    }

private:
    // Dummy head of doubly-linked list of snapshots
    ImageImpl list_;
};

}  // namespace LzKVStore

#endif  // SOE_KVS_SRC_IMAGE_LIST_H_
